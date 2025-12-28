#include "game/systems/InputSystem.hpp"
#include "game/GameConfig.hpp"
#include "game/GameState.hpp"
#include "game/entities/Ball.hpp"
#include "game/systems/PhysicsSystem.hpp"
#include "game/ui/OverlayLayout.hpp"
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

    // Get menu layout (must match UIRender.cpp)
    ui::MenuLayout menu = ui::calculateMenuLayout(fbW, fbH);

    // If instructions panel is shown, only allow clicking on the close button (X)
    if (state.showInstructions) {
        state.hoveredMenuButton = -1; // No button hovered when instructions are shown
        float instrW = 820.0f;
        float instrH = 620.0f;
        float instrX = (fbW - instrW) * 0.5f;
        float instrY = (fbH - instrH) * 0.5f;

        // Close button position (X in top-right corner)
        float closeSize = 44.0f;
        float closeX = instrX + instrW - closeSize - 10.0f;
        float closeY = instrY + instrH - closeSize - 6.0f;

        // Update hover state for close button
        state.hoveredCloseButton = pointInRectPx(px, py, closeX, closeY, closeSize, closeSize);

        if (click) {
            // Click on close button (X) = close it
            if (state.hoveredCloseButton) {
                state.showInstructions = false;
            }
        }
        return true; // Menu is handling input
    }

    state.hoveredCloseButton = false;

    // Update hover state
    state.hoveredMenuButton = -1;
    if (menu.btn1.contains(px, py)) state.hoveredMenuButton = 0;
    else if (menu.btn2.contains(px, py)) state.hoveredMenuButton = 1;
    else if (menu.btn3.contains(px, py)) state.hoveredMenuButton = 2;
    else if (menu.btn4.contains(px, py)) state.hoveredMenuButton = 3;

    if (click) {
        // Normal Mode button
        if (menu.btn1.contains(px, py)) {
            state.showInstructions = false;
            state.gameType = GameType::NORMAL;
            state.testOneBrick = false;
            state.mode = GameMode::PLAYING; // Switch to playing mode
            return true; // Signal that game should init
        }
        // Endless Mode button
        if (menu.btn2.contains(px, py)) {
            state.showInstructions = false;
            state.gameType = GameType::ENDLESS;
            state.wave = 1;
            state.testOneBrick = false;
            state.mode = GameMode::PLAYING; // Switch to playing mode
            return true; // Signal that game should init
        }
        // Instructions button
        if (menu.btn3.contains(px, py)) {
            state.showInstructions = true;
            return true;
        }
        // Test feature (click the "4" badge): Start a one-brick test level.
        if (menu.testBadge.contains(px, py)) {
            state.showInstructions = false;
            state.gameType = GameType::NORMAL;
            state.testOneBrick = true;
            state.mode = GameMode::PLAYING;
            return true;
        }
        // Exit button
        if (menu.btn4.contains(px, py)) {
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

    // Curse: reverse controls
    if (state.reverseTimer > 0.0f) {
        dir = -dir;
    }
    
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

