#include "game/systems/InputSystem.hpp"
#include "game/GameConfig.hpp"
#include "game/GameState.hpp"
#include "game/entities/Ball.hpp"
#include "game/systems/PhysicsSystem.hpp"
#include "game/rogue/RogueCards.hpp"
#include "engine/Window.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

namespace game {

bool InputSystem::pointInRectPx(float px, float py, float x, float y, float w, float h) {
    return (px >= x && px <= x + w && py >= y && py <= y + h);
}

static glm::vec3 computeLaunchDirFromMouse(
    const GameState& state,
    const engine::Input& input,
    const GameConfig& cfg,
    engine::Window& window,
    const Ball& ball
) {
    auto [fbW, fbH] = window.getFramebufferSize();
    fbW = std::max(1, fbW);
    fbH = std::max(1, fbH);

    // Mirror the camera setup used in Game::render() (so aiming matches what the player sees).
    float arenaW = (cfg.arenaMaxX - cfg.arenaMinX);
    float arenaD = (cfg.arenaMaxZ - cfg.arenaMinZ);
    float base   = std::max(arenaW, arenaD);
    float zMid   = (cfg.arenaMinZ + cfg.arenaMaxZ) * 0.5f;

    glm::vec3 camPos, camTarget;
    float fovDeg = 45.0f;
    if (state.cameraMode == 1) {
        camPos    = glm::vec3(0.0f, base * 1.03f, zMid + 0.5f);
        camTarget = glm::vec3(0.0f, 0.0f, zMid);
    } else {
        camPos    = glm::vec3(0.0f, base * 0.62f, base * 0.82f);
        camTarget = glm::vec3(0.0f, 0.0f, 0.8f);
    }

    glm::mat4 V = glm::lookAt(camPos, camTarget, glm::vec3(0, 1, 0));
    glm::mat4 P = glm::perspective(glm::radians(fovDeg), (float)fbW / (float)fbH, 0.1f, 300.0f);
    glm::mat4 invPV = glm::inverse(P * V);

    auto [mx, myRaw] = input.mousePosFbPx(); // myRaw: origin at top-left (GLFW-style)
    float xNdc = (mx / (float)fbW) * 2.0f - 1.0f;
    float yNdc = 1.0f - (myRaw / (float)fbH) * 2.0f;

    auto unproject = [&](float zNdc) -> glm::vec3 {
        glm::vec4 p = invPV * glm::vec4(xNdc, yNdc, zNdc, 1.0f);
        if (std::abs(p.w) > 1e-6f) p /= p.w;
        return glm::vec3(p);
    };

    glm::vec3 farPt = unproject(1.0f);
    glm::vec3 rayDir = farPt - camPos;
    float len2 = glm::dot(rayDir, rayDir);
    if (len2 < 1e-10f) {
        return glm::vec3(0.0f, 0.0f, -1.0f);
    }
    rayDir = glm::normalize(rayDir);

    // Intersect the mouse ray with the gameplay plane (XZ plane at the ball's height).
    float denom = rayDir.y;
    if (std::abs(denom) < 1e-6f) {
        return glm::vec3(0.0f, 0.0f, -1.0f);
    }

    float t = (ball.pos.y - camPos.y) / denom;
    if (!(t > 0.0f)) {
        return glm::vec3(0.0f, 0.0f, -1.0f);
    }

    glm::vec3 hit = camPos + rayDir * t;

    // Direction in the game plane.
    glm::vec3 dir(hit.x - ball.pos.x, 0.0f, hit.z - ball.pos.z);
    float d2 = glm::dot(dir, dir);
    if (d2 < 1e-10f) {
        dir = glm::vec3(0.0f, 0.0f, -1.0f);
    } else {
        // Ensure the ball always goes toward the bricks (negative Z).
        if (dir.z > -1e-6f) dir.z = -std::abs(dir.z) - 0.25f;

        // Clamp horizontal angle to avoid near-horizontal shots.
        float zabs = std::max(1e-4f, std::abs(dir.z));
        float horizontalRatio = dir.x / zabs;
        horizontalRatio = std::clamp(horizontalRatio, -0.8f, 0.8f);
        dir.x = horizontalRatio * zabs;
    }

    return glm::normalize(dir);
}

void InputSystem::handleGameInput(GameState& state, const engine::Input& input, const GameConfig& cfg, engine::Window& window, float dt) {
    // Only PLAYING/PAUSED participate in the pause toggle.
    // Special modes like ROGUE_CARDS should fully freeze gameplay input.
    if (input.keyPressed(engine::Key::Escape)) {
        if (state.mode == GameMode::PLAYING) state.mode = GameMode::PAUSED;
        else if (state.mode == GameMode::PAUSED) state.mode = GameMode::PLAYING;
    }

    if (state.mode != GameMode::PLAYING) return;

    auto [fbW, fbH] = window.getFramebufferSize();

    // BG Selector HUD Logic
    {
        float boxSize = 30.0f;
        float gap = 10.0f;
        float startX = fbW - (boxSize + gap) * 5.0f - 20.0f;
        float startY = 20.0f;

        auto [mx, my_raw] = input.mousePosFbPx();
        float my = (float)fbH - my_raw;

        if (input.mousePressed(engine::MouseButton::Left)) {
            for (int i = -1; i < 4; i++) {
                float bx = startX + (i + 1) * (boxSize + gap);
                float by = fbH - startY - boxSize;
                if (mx >= bx && mx <= bx + boxSize && my >= by && my <= by + boxSize) {
                    state.currentBg = i;
                }
            }
        }
    }

    if (input.keyPressed(engine::Key::K1)) state.cameraMode = 1;
    if (input.keyPressed(engine::Key::K2)) state.cameraMode = 2;

    // Paddle movement
    float dir = 0.0f;
    if (input.keyDown(engine::Key::A) || input.keyDown(engine::Key::Left))  dir -= 1.0f;
    if (input.keyDown(engine::Key::D) || input.keyDown(engine::Key::Right)) dir += 1.0f;

    // Curse: reverse controls
    if (state.reverseTimer > 0.0f) {
        dir = -dir;
    }

    PhysicsSystem::updatePaddle(state, cfg, dir, dt);

    // Ball launching
    for (auto& ball : state.balls) {
        // Rogue fairness: while queued rows are spawning, keep the ball attached.
        if (state.gameType == GameType::ROGUE && state.roguePendingRowsToSpawn > 0) {
            continue;
        }
        if (ball.attached && input.keyDown(engine::Key::Space)) {
            ball.attached = false;

            // Accurate aim: unproject mouse through the current camera and intersect with the arena plane.
            glm::vec3 d = computeLaunchDirFromMouse(state, input, cfg, window, ball);
            float sp = cfg.ballSpeed;
            if (state.gameType == GameType::ROGUE) sp *= game::rogue::ballSpeedMult(state);
            ball.vel = d * sp;
        }
    }
}

} // namespace game


