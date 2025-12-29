#pragma once
#include "engine/Mesh.hpp"
#include "engine/Texture.hpp"
#include "engine/Shader.hpp"
#include "engine/AnimatedTexture.hpp"
#include <array>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

namespace game {

struct GameAssets {
    engine::Mesh paddle;
    engine::Mesh ball;
    engine::Mesh heart;
    engine::Mesh wall;

    engine::Mesh brick01;

    engine::Mesh brick02;
    engine::Mesh brick02_1hit;

    engine::Mesh brick03;
    engine::Mesh brick03_1hit;
    engine::Mesh brick03_2hit;

    engine::Mesh brick04;
    engine::Mesh brick04_1hit;
    engine::Mesh brick04_2hit;
    engine::Mesh brick04_3hit;

    engine::Mesh expand;
    engine::Mesh extraBall;
    engine::Mesh slow;
    engine::Mesh extraLife;

    // New powerups
    engine::Mesh fireball;
    engine::Mesh shield;
    engine::Mesh skull; // reverse controls curse
    engine::Mesh minus; // tiny paddle curse
    engine::Texture2D backgroundTexs[4];

    // Optional UI "video" previews (animated GIFs), lazy-loaded to keep startup fast.
    // Put files in `assets/video/*.gif`.
    // Order matches the Powerups inspector list in `MenuRender.cpp`:
    // 0 EXPAND, 1 EXTRA_BALL, 2 EXTRA_LIFE, 3 FIREBALL, 4 SLOW, 5 SHIELD, 6 REVERSE, 7 TINY
    std::string powerupVideoPaths[8];
    mutable engine::AnimatedTexture2D powerupVideos[8];
    mutable bool powerupVideoLoaded[8]{};
    // Async decode/upload state (to avoid stalling when opening the menu).
    struct DecodedGif {
        int w = 0;
        int h = 0;
        std::vector<std::vector<unsigned char>> framesRGBA;
        std::vector<int> delaysMs;
    };
    mutable std::array<std::thread, 8> powerupVideoThreads{};
    mutable std::array<std::atomic<bool>, 8> powerupVideoDecoding{};
    mutable std::array<std::atomic<bool>, 8> powerupVideoDecoded{};
    mutable std::array<std::atomic<bool>, 8> powerupVideoTried{};
    mutable std::array<std::mutex, 8> powerupVideoMutex{};
    mutable std::array<DecodedGif, 8> powerupVideoDecodedData{};
    mutable std::array<int, 8> powerupVideoUploadCursor{};
    mutable std::atomic<bool> powerupVideoPreloadStarted{false};

    const engine::AnimatedTexture2D& powerupVideo(int idx) const;
    void startPowerupVideoPreload() const;            // kicks off decode threads for all
    void pumpPowerupVideoPreload(int uploadBudgetFrames = 6) const; // uploads a few frames per tick
    
    // Scrolling background shader and mesh
    engine::Shader scrollingBgShader;
    engine::Mesh backgroundMesh;

    bool loadAll();
    void destroy();
};

} // namespace game
