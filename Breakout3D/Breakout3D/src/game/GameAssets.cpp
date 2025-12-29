#include "game/GameAssets.hpp"
#include "engine/Texture.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace game {

// stb_image implementation is provided by engine/Texture.cpp.
#include "external/stb_image.h"

static void joinIfRunning(std::thread& t) {
    if (t.joinable()) t.join();
}

static void startDecodeThread(const game::GameAssets* self, int idx, const std::string& path) {
    self->powerupVideoDecoding[idx] = true;
    joinIfRunning(self->powerupVideoThreads[idx]);
    self->powerupVideoThreads[idx] = std::thread([self, idx, path]() {
        // Read file bytes
        std::ifstream f(path, std::ios::binary);
        if (!f) { self->powerupVideoDecoding[idx] = false; return; }
        f.seekg(0, std::ios::end);
        std::streamoff len = f.tellg();
        f.seekg(0, std::ios::beg);
        if (len <= 0) { self->powerupVideoDecoding[idx] = false; return; }
        std::vector<unsigned char> bytes((size_t)len);
        f.read((char*)bytes.data(), len);
        if (!f) { self->powerupVideoDecoding[idx] = false; return; }

        int* delays = nullptr;
        int w = 0, h = 0, framesN = 0, comp = 0;
        stbi_uc* data = stbi_load_gif_from_memory(bytes.data(), (int)bytes.size(), &delays, &w, &h, &framesN, &comp, 4);
        if (!data || w <= 0 || h <= 0 || framesN <= 0) {
            if (data) stbi_image_free(data);
            if (delays) stbi_image_free(delays);
            self->powerupVideoDecoding[idx] = false;
            return;
        }

        // Keep it light: cap frames and skip frames.
        const int frameStep = 2;
        const int maxFrames = 120;

        std::vector<std::vector<unsigned char>> frames;
        std::vector<int> dly;
        frames.reserve((size_t)std::min(framesN, maxFrames));
        dly.reserve((size_t)std::min(framesN, maxFrames));

        const size_t frameBytes = (size_t)w * (size_t)h * 4;
        int kept = 0;
        for (int i = 0; i < framesN && kept < maxFrames; i += frameStep) {
            const unsigned char* src = data + (size_t)i * frameBytes;
            std::vector<unsigned char> buf(src, src + frameBytes);
            frames.push_back(std::move(buf));
            dly.push_back(delays ? delays[i] : 100);
            kept++;
        }

        stbi_image_free(data);
        if (delays) stbi_image_free(delays);

        {
            std::lock_guard<std::mutex> lk(self->powerupVideoMutex[idx]);
            auto& out = self->powerupVideoDecodedData[idx];
            out.w = w;
            out.h = h;
            out.framesRGBA = std::move(frames);
            out.delaysMs = std::move(dly);
            self->powerupVideoUploadCursor[idx] = 0;
            self->powerupVideoDecoded[idx] = true;
        }
        self->powerupVideoDecoding[idx] = false;
    });
}

void GameAssets::startPowerupVideoPreload() const {
    bool expected = false;
    if (!powerupVideoPreloadStarted.compare_exchange_strong(expected, true)) return;

    for (int i = 0; i < 8; ++i) {
        if (powerupVideoLoaded[i]) continue;
        if (powerupVideoTried[i]) continue;
        powerupVideoTried[i] = true;
        const std::string path = powerupVideoPaths[i];
        if (path.empty() || !std::filesystem::exists(path)) continue;
        startDecodeThread(this, i, path);
    }
}

void GameAssets::pumpPowerupVideoPreload(int uploadBudgetFrames) const {
    uploadBudgetFrames = std::max(0, uploadBudgetFrames);
    if (uploadBudgetFrames == 0) return;

    // Round-robin upload across videos so we don't stall.
    static int rr = 0;
    for (int step = 0; step < 8 && uploadBudgetFrames > 0; ++step) {
        int idx = (rr + step) % 8;
        if (powerupVideoLoaded[idx]) continue;
        if (!powerupVideoDecoded[idx]) continue;

        std::lock_guard<std::mutex> lk(powerupVideoMutex[idx]);
        auto& d = powerupVideoDecodedData[idx];
        if (d.framesRGBA.empty() || d.w <= 0 || d.h <= 0) {
            powerupVideoDecoded[idx] = false;
            continue;
        }

        auto& anim = powerupVideos[idx];
        int& cursor = powerupVideoUploadCursor[idx];
        if (cursor == 0) {
            anim.frames.clear();
            anim.delaysMs.clear();
            anim.frames.reserve(d.framesRGBA.size());
            anim.delaysMs.reserve(d.delaysMs.size());
        }

        // Upload one frame per iteration for smoothness.
        anim.frames.push_back(engine::Texture2D::loadFromRGBA(d.framesRGBA[cursor].data(), d.w, d.h, false));
        anim.delaysMs.push_back((cursor < (int)d.delaysMs.size()) ? d.delaysMs[cursor] : 100);
        cursor++;
        uploadBudgetFrames--;

        if (cursor >= (int)d.framesRGBA.size()) {
            d.framesRGBA.clear();
            d.delaysMs.clear();
            d.w = d.h = 0;
            powerupVideoDecoded[idx] = false;
            powerupVideoLoaded[idx] = !anim.frames.empty();
        }
    }
    rr = (rr + 1) % 8;
}

const engine::AnimatedTexture2D& GameAssets::powerupVideo(int idx) const {
    static engine::AnimatedTexture2D empty;
    if (idx < 0 || idx >= 8) return empty;
    if (powerupVideoLoaded[idx]) return powerupVideos[idx];

    // If we have decoded frames waiting, upload a few per frame (so UI never stalls).
    if (powerupVideoDecoded[idx]) {
        std::lock_guard<std::mutex> lk(powerupVideoMutex[idx]);
        auto& d = powerupVideoDecodedData[idx];
        auto& anim = powerupVideos[idx];

        const int uploadPerCall = 3;
        int& cursor = powerupVideoUploadCursor[idx];
        if (cursor == 0) {
            anim.frames.clear();
            anim.delaysMs.clear();
            anim.frames.reserve(d.framesRGBA.size());
            anim.delaysMs.reserve(d.delaysMs.size());
        }

        int end = std::min((int)d.framesRGBA.size(), cursor + uploadPerCall);
        for (int i = cursor; i < end; ++i) {
            anim.frames.push_back(engine::Texture2D::loadFromRGBA(d.framesRGBA[i].data(), d.w, d.h, false));
            anim.delaysMs.push_back((i < (int)d.delaysMs.size()) ? d.delaysMs[i] : 100);
        }
        cursor = end;

        if (cursor >= (int)d.framesRGBA.size()) {
            // Done uploading; free CPU buffers.
            d.framesRGBA.clear();
            d.delaysMs.clear();
            d.w = d.h = 0;
            powerupVideoDecoded[idx] = false;
            powerupVideoLoaded[idx] = !anim.frames.empty();
        }
        return powerupVideos[idx];
    }

    // Kick off decoding in the background (only once).
    if (!powerupVideoTried[idx]) {
        powerupVideoTried[idx] = true;

        const std::string path = powerupVideoPaths[idx];
        if (!path.empty() && std::filesystem::exists(path)) {
            startDecodeThread(this, idx, path);
        }
    }

    return powerupVideos[idx];
}

bool GameAssets::loadAll() {
    try {
        // baseDirPath: aqui é o único sítio onde defines isto
        engine::Mesh::setBaseDirPath("assets/models");

        ball   = engine::Mesh::loadOBJ("Ball.obj");
        paddle = engine::Mesh::loadOBJ("Paddle.obj");
        heart  = engine::Mesh::loadOBJ("heart.obj");

        brick01 = engine::Mesh::loadOBJ("Brick_01.obj");

        brick02       = engine::Mesh::loadOBJ("Brick_02.obj");
        brick02_1hit  = engine::Mesh::loadOBJ("Brick_02_1hit.obj");

        brick03       = engine::Mesh::loadOBJ("Brick_03.obj");
        brick03_1hit  = engine::Mesh::loadOBJ("Brick_03_1hit.obj");
        brick03_2hit  = engine::Mesh::loadOBJ("Brick_03_2hit.obj");

        brick04       = engine::Mesh::loadOBJ("Brick_04.obj");
        brick04_1hit  = engine::Mesh::loadOBJ("Brick_04_1hit.obj");
        brick04_2hit  = engine::Mesh::loadOBJ("Brick_04_2hit.obj");
        brick04_3hit  = engine::Mesh::loadOBJ("Brick_04_3hit.obj");

        expand        = engine::Mesh::loadOBJ("Expand.obj");
        extraBall     = engine::Mesh::loadOBJ("Extra_Ball.obj");
        slow          = engine::Mesh::loadOBJ("Slow.obj");
        extraLife     = engine::Mesh::loadOBJ("extralife.obj");
        fireball      = engine::Mesh::loadOBJ("Fireball.obj");
        shield        = engine::Mesh::loadOBJ("Shield.obj");
        skull         = engine::Mesh::loadOBJ("Skull.obj");
        minus         = engine::Mesh::loadOBJ("Minus.obj");

        // Se quiseres walls com o mesmo mesh do brick:
        wall = brick01;

        backgroundTexs[0] = engine::Texture2D::loadFromFile("assets/textures/Background.png", true);
        backgroundTexs[1] = engine::Texture2D::loadFromFile("assets/textures/Background2.png", true);
        backgroundTexs[2] = engine::Texture2D::loadFromFile("assets/textures/Background3.png", true);
        backgroundTexs[3] = engine::Texture2D::loadFromFile("assets/textures/Background4.png", true);

        // Optional powerup "video" previews (animated GIFs): set paths only (lazy-loaded on first display).
        powerupVideoPaths[0] = "assets/video/Expand_powerup.gif";
        powerupVideoPaths[1] = "assets/video/Extra-Ball_powerup.gif";
        powerupVideoPaths[2] = "assets/video/Extra-life_powerup.gif";
        powerupVideoPaths[3] = "assets/video/Fireball_powerup.gif";
        powerupVideoPaths[4] = "assets/video/Slow_powerup.gif";
        powerupVideoPaths[5] = "assets/video/Shield_powerup.gif";
        powerupVideoPaths[6] = "assets/video/Reserve_powerup.gif"; // REVERSE
        powerupVideoPaths[7] = "assets/video/Tiny_powerup.gif";
        for (int i = 0; i < 8; ++i) {
            powerupVideoLoaded[i] = false;
            powerupVideoDecoding[i] = false;
            powerupVideoDecoded[i] = false;
            powerupVideoTried[i] = false;
            powerupVideoUploadCursor[i] = 0;
        }
        powerupVideoPreloadStarted = false;

        return true;
    } catch (const std::exception& e) {
        std::cerr << "[GameAssets] loadAll failed: " << e.what() << "\n";
        return false;
    }
}

void GameAssets::destroy() {
    // cuidado: wall = brick01 é cópia de handles OpenGL.
    // Se fizeres isso, NÃO destruas wall separado. Melhor: remove wall e usa brick01.
    // Vou destruir só os "donos reais":
    ball.destroy();
    paddle.destroy();
    heart.destroy();

    brick01.destroy();

    brick02.destroy();
    brick02_1hit.destroy();

    brick03.destroy();
    brick03_1hit.destroy();
    brick03_2hit.destroy();

    brick04.destroy();
    brick04_1hit.destroy();
    brick04_2hit.destroy();
    brick04_3hit.destroy();

    expand.destroy();
    extraBall.destroy();
    slow.destroy();
    extraLife.destroy();
    fireball.destroy();
    shield.destroy();
    skull.destroy();
    minus.destroy();
    for (int i = 0; i < 4; i++) backgroundTexs[i].destroy();

    for (int i = 0; i < 8; ++i) {
        powerupVideos[i].destroy();
        powerupVideoLoaded[i] = false;
        powerupVideoDecoding[i] = false;
        powerupVideoDecoded[i] = false;
        powerupVideoTried[i] = false;
        powerupVideoPaths[i].clear();
        powerupVideoUploadCursor[i] = 0;
        {
            std::lock_guard<std::mutex> lk(powerupVideoMutex[i]);
            powerupVideoDecodedData[i] = DecodedGif{};
        }
        joinIfRunning(powerupVideoThreads[i]);
    }
    powerupVideoPreloadStarted = false;
}

} // namespace game
