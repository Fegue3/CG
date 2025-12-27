#include "game/systems/InputSystem.hpp"
#include "game/GameConfig.hpp"
#include "game/GameState.hpp"
#include "game/entities/Ball.hpp"
#include "game/systems/PhysicsSystem.hpp"
#include "engine/Window.hpp"
#include <glm/glm.hpp>

namespace game {

bool InputSystem::pointInRectPx(float px, float py, float x, float y, float w, float h) {
    return (px >= x && px <= x + w && py >= y && py <= y + h);
}

bool InputSystem::handleMenuInput(GameState& state, const engine::Input& input, engine::Window& window) {
    auto [fbW, fbH] = window.getFramebufferSize();
    auto [px, py_raw] = input.mousePosFbPx();
    float py = (float)fbH - py_raw;
    bool click = input.mousePressed(engine::MouseButton::Left);

    // Menu button positions
    float panelW = 500.0f;
    float panelH = 480.0f;
    float panelX = (fbW - panelW) * 0.5f;
    float panelY = (fbH - panelH) * 0.5f;

    float btnW = 200.0f;
    float btnH = 70.0f;
    float btnX = panelX + (panelW - btnW) * 0.5f;
    
    // Button positions (from top to bottom)
    float btn1Y = panelY + 360.0f; // Normal Mode (top)
    float btn2Y = panelY + 250.0f; // Endless Mode
    float btn3Y = panelY + 140.0f; // Instructions
    float btn4Y = panelY + 30.0f;  // Exit (bottom)

    // Small "4" test badge inside the bottom button (same coords as in Game.cpp render).
    float testW = 48.0f;
    float testH = btnH - 16.0f;
    float testX = btnX + btnW - 8.0f - testW;
    float testY = btn4Y + 8.0f;

    // If instructions panel is shown, only allow clicking on the panel or outside to close
    if (state.showInstructions) {
        float instrW = 600.0f;
        float instrH = 320.0f;
        float instrX = (fbW - instrW) * 0.5f;
        float instrY = (fbH - instrH) * 0.5f;

        if (click) {
            // Click outside the instructions panel = close it
            if (!pointInRectPx(px, py, instrX, instrY, instrW, instrH)) {
                state.showInstructions = false;
            }
        }
        return true; // Menu is handling input
    }

    if (click) {
        // Normal Mode button
        if (pointInRectPx(px, py, btnX, btn1Y, btnW, btnH)) {
            state.showInstructions = false;
            state.gameType = GameType::NORMAL;
            state.testOneBrick = false;
            state.mode = GameMode::PLAYING; // Switch to playing mode
            return true; // Signal that game should init
        }
        // Endless Mode button
        if (pointInRectPx(px, py, btnX, btn2Y, btnW, btnH)) {
            state.showInstructions = false;
            state.gameType = GameType::ENDLESS;
            state.wave = 1;
            state.testOneBrick = false;
            state.mode = GameMode::PLAYING; // Switch to playing mode
            return true; // Signal that game should init
        }
        // Instructions button
        if (pointInRectPx(px, py, btnX, btn3Y, btnW, btnH)) {
            state.showInstructions = true;
            return true;
        }
        // Test feature (click the "4" badge): Start a one-brick test level.
        if (pointInRectPx(px, py, testX, testY, testW, testH)) {
            state.showInstructions = false;
            state.gameType = GameType::NORMAL;
            state.testOneBrick = true;
            state.mode = GameMode::PLAYING;
            return true;
        }
        // Exit button
        if (pointInRectPx(px, py, btnX, btn4Y, btnW, btnH)) {
            window.requestClose();
            return true;
        }
    }

    return true; // Menu is handling input
}

void InputSystem::handleGameInput(GameState& state, const engine::Input& input, const GameConfig& cfg, engine::Window& window, float dt) {
    // Toggle Pause with Escape
    if (input.keyPressed(engine::Key::Escape)) {
        if (state.mode == GameMode::PLAYING) {
            state.mode = GameMode::PAUSED;
        } else if (state.mode == GameMode::PAUSED) {
            state.mode = GameMode::PLAYING;
        }
    }

    if (state.mode == GameMode::PAUSED) {
        return;
    }

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
    
    PhysicsSystem::updatePaddle(state, cfg, dir, dt);

    // Ball launching
    for (auto& ball : state.balls) {
        if (ball.attached && input.keyDown(engine::Key::Space)) {
            ball.attached = false;
            glm::vec3 d = glm::normalize(glm::vec3(0.30f, 0.0f, -1.0f));
            ball.vel = d * cfg.ballSpeed;
        }
    }
}

} // namespace game

