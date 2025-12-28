#include "game/systems/InputSystem.hpp"
#include "game/GameConfig.hpp"
#include "game/GameState.hpp"
#include "game/entities/Ball.hpp"
#include "game/systems/PhysicsSystem.hpp"
#include "game/ui/OverlayLayout.hpp"
#include "engine/Window.hpp"
#include <glm/glm.hpp>
#include <algorithm>

namespace game {

bool InputSystem::pointInRectPx(float px, float py, float x, float y, float w, float h) {
    return (px >= x && px <= x + w && py >= y && py <= y + h);
}

bool InputSystem::handleMenuInput(GameState& state, const engine::Input& input, engine::Window& window) {
    auto [fbW, fbH] = window.getFramebufferSize();
    auto [px, py_raw] = input.mousePosFbPx();
    float py = (float)fbH - py_raw;
    bool click = input.mousePressed(engine::MouseButton::Left);

    // Get cached menu layout (computed using real font metrics; must match UIRender.cpp)
    const ui::MenuLayout& menu = state.menuLayout;

    // If instructions overlay is shown, only allow clicking on the BACK button (bottom-left)
    if (state.showInstructions) {
        state.hoveredMenuButton = -1; // No button hovered when instructions are shown
        // Must match UIRender.cpp exactly
        float instrW = std::min(980.0f, (float)fbW * 0.75f);
        float instrH = std::min(690.0f, (float)fbH * 0.72f);
        float instrX = ((float)fbW - instrW) * 0.5f;
        float instrY = std::max(40.0f, ((float)fbH - instrH) * 0.5f - 60.0f);

        // Keep the overlay BELOW the big title (match OverlayLayout.cpp spacing).
        const float titlePanelGap = 26.0f;
        float maxTopY = menu.titleY - titlePanelGap; // y-up
        if (instrY + instrH > maxTopY) {
            instrY = maxTopY - instrH;
            instrY = std::max(40.0f, instrY);
        }

        // BACK button position (bottom-left corner of panel)
        float backW = 150.0f;
        float backH = 56.0f;
        float backX = instrX + 22.0f;
        float backY = instrY + 16.0f;

        // Update hover state for BACK button
        state.hoveredCloseButton = pointInRectPx(px, py, backX, backY, backW, backH);

        if (click) {
            // Click on BACK = close overlay
            if (state.hoveredCloseButton) {
                state.showInstructions = false;
            }
        }
        return true; // Menu is handling input
    }

    state.hoveredCloseButton = false;

    // Check hover on test badge (only on MAIN menu)
    state.hoveredTestBadge = false;
    if (state.currentMenuScreen == MenuScreen::MAIN) {
        state.hoveredTestBadge = menu.testBadge.contains(px, py);
    }

    // Update hover state based on current menu screen
    state.hoveredMenuButton = -1;
    if (state.currentMenuScreen == MenuScreen::MAIN) {
        if (menu.btn1.contains(px, py)) state.hoveredMenuButton = 0;      // PLAY
        else if (menu.btn2.contains(px, py)) state.hoveredMenuButton = 1; // INSTRUCTIONS
        else if (menu.btn3.contains(px, py)) state.hoveredMenuButton = 2; // OPTIONS
        else if (menu.btn4.contains(px, py)) state.hoveredMenuButton = 3; // EXIT
    } else if (state.currentMenuScreen == MenuScreen::PLAY_MODES) {
        // Hover should activate on the whole card (not only the PLAY button).
        if (menu.normal.card.contains(px, py) || menu.normal.playBtn.contains(px, py)) state.hoveredMenuButton = 0;
        else if (menu.endless.card.contains(px, py) || menu.endless.playBtn.contains(px, py)) state.hoveredMenuButton = 1;
        else if (menu.rogue.card.contains(px, py) || menu.rogue.playBtn.contains(px, py)) state.hoveredMenuButton = 2;
        else if (menu.levels.card.contains(px, py) || menu.levels.playBtn.contains(px, py)) state.hoveredMenuButton = 3;
        else if (menu.backBtn.contains(px, py)) state.hoveredMenuButton = 4;         // BACK
    } else if (state.currentMenuScreen == MenuScreen::OPTIONS) {
        float offsetY = -50.0f; // Options buttons start lower
        ui::Rect soundBtn = {menu.btn1.x, menu.btn1.y + offsetY, menu.btn1.w, menu.btn1.h};
        ui::Rect graphicsBtn = {menu.btn2.x, menu.btn2.y + offsetY, menu.btn2.w, menu.btn2.h};
        
        if (soundBtn.contains(px, py)) state.hoveredMenuButton = 0;    // SOUND
        else if (graphicsBtn.contains(px, py)) state.hoveredMenuButton = 1; // GRAPHICS
        // BACK button
        else {
            float s = menu.uiScale;
            float backW = 120.0f * s;
            float backH = 50.0f * s;
            float backX = menu.panelX + 20.0f * s;
            float backY = menu.panelY + 15.0f * s;
            if (pointInRectPx(px, py, backX, backY, backW, backH)) {
                state.hoveredMenuButton = 2; // BACK
            }
        }
    } else if (state.currentMenuScreen == MenuScreen::INSTRUCTIONS) {
        float offsetY = -50.0f; // Instructions buttons start lower (matches OPTIONS layout)
        ui::Rect controlsBtn = {menu.btn1.x, menu.btn1.y + offsetY, menu.btn1.w, menu.btn1.h};
        ui::Rect powerupsBtn = {menu.btn2.x, menu.btn2.y + offsetY, menu.btn2.w, menu.btn2.h};

        if (controlsBtn.contains(px, py)) state.hoveredMenuButton = 0;       // CONTROLS
        else if (powerupsBtn.contains(px, py)) state.hoveredMenuButton = 1;  // POWERUPS
        else {
            float s = menu.uiScale;
            float backW = 120.0f * s;
            float backH = 50.0f * s;
            float backX = menu.panelX + 20.0f * s;
            float backY = menu.panelY + 15.0f * s;
            if (pointInRectPx(px, py, backX, backY, backW, backH)) {
                state.hoveredMenuButton = 2; // BACK
            }
        }
    }

    if (click) {
        if (state.currentMenuScreen == MenuScreen::MAIN) {
            // Test feature (click the "4" badge): Start a one-brick test level.
            if (menu.testBadge.contains(px, py)) {
                state.showInstructions = false;
                state.gameType = GameType::NORMAL;
                state.testOneBrick = true;
                state.mode = GameMode::PLAYING;
                return true;
            }
            
            // PLAY button - go to play modes submenu
            if (menu.btn1.contains(px, py)) {
                state.currentMenuScreen = MenuScreen::PLAY_MODES;
                return true;
            }
            // INSTRUCTIONS button
            if (menu.btn2.contains(px, py)) {
                state.currentMenuScreen = MenuScreen::INSTRUCTIONS;
                return true;
            }
            // OPTIONS button - go to options submenu
            if (menu.btn3.contains(px, py)) {
                state.currentMenuScreen = MenuScreen::OPTIONS;
                return true;
            }
            // EXIT button
            if (menu.btn4.contains(px, py)) {
                window.requestClose();
                return true;
            }
        } else if (state.currentMenuScreen == MenuScreen::PLAY_MODES) {
            // NORMAL Mode
            if (menu.normal.playBtn.contains(px, py)) {
                state.showInstructions = false;
                state.gameType = GameType::NORMAL;
                state.testOneBrick = false;
                state.mode = GameMode::PLAYING;
                return true;
            }
            // ENDLESS Mode
            if (menu.endless.playBtn.contains(px, py)) {
                state.showInstructions = false;
                state.gameType = GameType::ENDLESS;
                state.wave = 1;
                state.testOneBrick = false;
                state.mode = GameMode::PLAYING;
                return true;
            }
            // ROGUE/LEVELS: coming soon (buttons are drawn as disabled)
            if (menu.rogue.playBtn.contains(px, py)) return true;
            if (menu.levels.playBtn.contains(px, py)) return true;

            // BACK
            if (menu.backBtn.contains(px, py)) {
                state.currentMenuScreen = MenuScreen::MAIN;
                return true;
            }
        } else if (state.currentMenuScreen == MenuScreen::OPTIONS) {
            // SOUND button (placeholder)
            float offsetY = -50.0f;
            ui::Rect soundBtn = {menu.btn1.x, menu.btn1.y + offsetY, menu.btn1.w, menu.btn1.h};
            if (soundBtn.contains(px, py)) {
                // TODO: Implement sound settings
                return true;
            }
            // GRAPHICS button (placeholder)
            ui::Rect graphicsBtn = {menu.btn2.x, menu.btn2.y + offsetY, menu.btn2.w, menu.btn2.h};
            if (graphicsBtn.contains(px, py)) {
                // TODO: Implement graphics settings
                return true;
            }
            // BACK button
            float s = menu.uiScale;
            float backW = 120.0f * s;
            float backH = 50.0f * s;
            float backX = menu.panelX + 20.0f * s;
            float backY = menu.panelY + 15.0f * s;
            if (pointInRectPx(px, py, backX, backY, backW, backH)) {
                state.currentMenuScreen = MenuScreen::MAIN;
                return true;
            }
        } else if (state.currentMenuScreen == MenuScreen::INSTRUCTIONS) {
            float offsetY = -50.0f;
            ui::Rect controlsBtn = {menu.btn1.x, menu.btn1.y + offsetY, menu.btn1.w, menu.btn1.h};
            if (controlsBtn.contains(px, py)) {
                state.instructionsTab = 0;
                state.showInstructions = true;
                return true;
            }

            ui::Rect powerupsBtn = {menu.btn2.x, menu.btn2.y + offsetY, menu.btn2.w, menu.btn2.h};
            if (powerupsBtn.contains(px, py)) {
                state.instructionsTab = 1;
                state.showInstructions = true;
                return true;
            }

            float s = menu.uiScale;
            float backW = 120.0f * s;
            float backH = 50.0f * s;
            float backX = menu.panelX + 20.0f * s;
            float backY = menu.panelY + 15.0f * s;
            if (pointInRectPx(px, py, backX, backY, backW, backH)) {
                state.currentMenuScreen = MenuScreen::MAIN;
                return true;
            }
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

