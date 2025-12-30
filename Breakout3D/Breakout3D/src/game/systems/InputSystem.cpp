#include "game/systems/InputSystem.hpp"
#include "game/GameConfig.hpp"
#include "game/GameState.hpp"
#include "game/entities/Ball.hpp"
#include "game/systems/PhysicsSystem.hpp"
#include "game/ui/OverlayLayout.hpp"
#include "game/ui/InstructionsOverlayLayout.hpp"
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

bool InputSystem::handleMenuInput(GameState& state, const engine::Input& input, engine::Window& window) {
    auto [fbW, fbH] = window.getFramebufferSize();
    auto [px, py_raw] = input.mousePosFbPx();
    float py = (float)fbH - py_raw;
    bool click = input.mousePressed(engine::MouseButton::Left);

    // Get cached menu layout (computed using real font metrics; must match UIRender.cpp)
    const ui::MenuLayout& menu = state.menuLayout;

    // Secret: Press L to unlock all levels
#ifdef BREAKOUT3D_DEBUG
    if (input.keyPressed(engine::Key::L)) {
        state.levelsBestLevel = 20;
    }
#endif

    // If instructions overlay is shown, handle BACK + (Powerups tab) inspector interactions.
    if (state.showInstructions) {
        state.hoveredMenuButton = -1; // No button hovered when instructions are shown
        const auto OL = ui::instructionsOverlayLayout(menu, fbW, fbH, state.instructionsTab);

        // Update hover state for BACK button
        state.hoveredCloseButton = OL.backBtn.contains(px, py);
        state.hoveredPowerupNav = -1;

        // Powerups inspector interactions (only on tab 1)
        if (state.instructionsTab == 1) {
            // Hover arrows
            if (OL.navLeft.contains(px, py)) state.hoveredPowerupNav = 0;
            else if (OL.navRight.contains(px, py)) state.hoveredPowerupNav = 1;

            bool down = input.mouseDown(engine::MouseButton::Left);

            // Drag rotate the model in the left viewport
            if (down && OL.modelRect.contains(px, py)) {
                if (!state.powerupInspectDragging) {
                    state.powerupInspectDragging = true;
                    state.powerupInspectLastMouse = glm::vec2(px, py);
                } else {
                    glm::vec2 cur(px, py);
                    glm::vec2 d = cur - state.powerupInspectLastMouse;
                    state.powerupInspectLastMouse = cur;

                    const float k = 0.0105f;
                    state.powerupInspectYaw += d.x * k;
                    state.powerupInspectPitch += d.y * k;
                    // keep pitch in a nice range (avoid flip chaos)
                    const float lim = 1.45f;
                    if (state.powerupInspectPitch > lim) state.powerupInspectPitch = lim;
                    if (state.powerupInspectPitch < -lim) state.powerupInspectPitch = -lim;
                }
            } else {
                state.powerupInspectDragging = false;
            }

            // Click arrows to switch powerups (wrap)
            if (click) {
                const int count = 8; // must match render list in MenuRender.cpp
                if (state.hoveredPowerupNav == 0) {
                    state.powerupInspectIndex = (state.powerupInspectIndex - 1 + count) % count;
                    state.powerupInspectYaw = 0.0f;
                    state.powerupInspectPitch = 0.0f;
                    return true;
                }
                if (state.hoveredPowerupNav == 1) {
                    state.powerupInspectIndex = (state.powerupInspectIndex + 1) % count;
                    state.powerupInspectYaw = 0.0f;
                    state.powerupInspectPitch = 0.0f;
                    return true;
                }
            }
        } else {
            state.powerupInspectDragging = false;
        }

        // Rogue cards browser interactions (tab 2)
        if (state.instructionsTab == 2) {
            // Rebuild the same grouped list used by MenuRender, so hitboxes match visuals.
            std::vector<game::rogue::RogueCardId> powerups;
            std::vector<game::rogue::RogueCardId> modifiers;
            std::vector<game::rogue::RogueCardId> ops;
            for (auto id : game::rogue::allCardIds()) {
                const auto& def = game::rogue::cardDef(id);
                if (def.isOp) ops.push_back(id);
                else if (game::rogue::isPowerupCard(id)) powerups.push_back(id);
                else modifiers.push_back(id);
            }

            float contentX = OL.modelRect.x;
            float contentY = OL.modelRect.y;
            float contentW = (OL.infoRect.x + OL.infoRect.w) - contentX;
            float contentH = OL.modelRect.h;
            float gap = 22.0f * menu.uiScale;
            float colW = (contentW - 2.0f * gap) / 3.0f;
            float colX0 = contentX;
            float colX1 = contentX + colW + gap;
            float colX2 = contentX + (colW + gap) * 2.0f;

            float itemH = 54.0f * menu.uiScale;
            float itemGap = 10.0f * menu.uiScale;
            float step = itemH + itemGap;
            float viewTopY = contentY + contentH - 80.0f * menu.uiScale;
            float viewBottomY = contentY + 30.0f * menu.uiScale;
            float viewH = std::max(1.0f, viewTopY - viewBottomY);

            auto maxScrollFor = [&](const std::vector<game::rogue::RogueCardId>& ids) -> float {
                float totalH = std::max(0.0f, (float)ids.size() * step - itemGap);
                return std::max(0.0f, totalH - viewH);
            };

            auto hitList = [&](float x, float scrollPx, const std::vector<game::rogue::RogueCardId>& ids, int baseIndex) {
                float maxScroll = maxScrollFor(ids);
                scrollPx = std::clamp(scrollPx, 0.0f, maxScroll);

                float y = viewTopY + scrollPx;
                for (size_t i = 0; i < ids.size(); ++i) {
                    if (y < viewBottomY) break;
                    game::ui::Rect R{x, y - itemH, colW, itemH};
                    if (R.contains(px, py)) {
                        state.hoveredRogueCardsItem = baseIndex + (int)i;
                        return true;
                    }
                    y -= step;
                }
                return false;
            };

            // Default: no hover unless over an item.
            state.hoveredRogueCardsItem = -1;

            // Modal: if open, clicking outside closes it.
            if (state.rogueCardsInspectOpen) {
                float cardWBase = 420.0f;
                float cardHBase = 900.0f;
                float s = std::min((float)fbW / (cardWBase + 120.0f), (float)fbH / (cardHBase + 160.0f));
                s = std::max(0.65f, std::min(1.45f, s));
                float cardW = cardWBase * s;
                float cardH = cardHBase * s;
                float cx = ((float)fbW - cardW) * 0.5f;
                float cy = ((float)fbH - cardH) * 0.5f;
                game::ui::Rect cardRect{cx, cy, cardW, cardH};

                if (click && !cardRect.contains(px, py)) {
                    state.rogueCardsInspectOpen = false;
                    return true;
                }
                // While modal is open, don't interact with the list.
            } else {
                int base0 = 0;
                int base1 = base0 + (int)powerups.size();
                int base2 = base1 + (int)modifiers.size();

                bool hit = false;
                hit = hitList(colX0, state.rogueCardsScrollPowerups, powerups, base0) || hit;
                hit = hitList(colX1, state.rogueCardsScrollModifiers, modifiers, base1) || hit;
                hit = hitList(colX2, state.rogueCardsScrollOp, ops, base2) || hit;

                // Wheel scrolling: each column scrolls independently when hovered.
                float wheel = input.mouseScrollY();
                if (std::abs(wheel) > 1e-4f) {
                    auto applyScroll = [&](float& scrollPx, const std::vector<game::rogue::RogueCardId>& ids) {
                        float maxScroll = maxScrollFor(ids);
                        const float speed = 42.0f * menu.uiScale; // px per wheel notch-ish
                        scrollPx -= wheel * speed; // GLFW: +wheel = scroll up
                        scrollPx = std::clamp(scrollPx, 0.0f, maxScroll);
                    };

                    bool inY = (py >= viewBottomY && py <= viewTopY);
                    if (inY && px >= colX0 && px <= colX0 + colW) applyScroll(state.rogueCardsScrollPowerups, powerups);
                    else if (inY && px >= colX1 && px <= colX1 + colW) applyScroll(state.rogueCardsScrollModifiers, modifiers);
                    else if (inY && px >= colX2 && px <= colX2 + colW) applyScroll(state.rogueCardsScrollOp, ops);
                }

                if (click && hit && state.hoveredRogueCardsItem >= 0) {
                    int idx = state.hoveredRogueCardsItem;
                    if (idx < base1) {
                        state.rogueCardsSelected = powerups[(size_t)idx];
                    } else if (idx < base2) {
                        state.rogueCardsSelected = modifiers[(size_t)(idx - base1)];
                    } else {
                        state.rogueCardsSelected = ops[(size_t)(idx - base2)];
                    }
                    state.rogueCardsInspectOpen = true;
                    return true;
                }
            }
        } else {
            state.hoveredRogueCardsItem = -1;
        }

        if (click) {
            // Click on BACK = close overlay
            if (state.hoveredCloseButton) {
                state.showInstructions = false;
                state.rogueCardsInspectOpen = false;
            }
        }
        return true; // Menu is handling input
    }

    state.hoveredCloseButton = false;
    state.hoveredPowerupNav = -1;
    state.powerupInspectDragging = false;

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
        // Hover should activate on the whole card (not only the PLAY button) for card scaling.
        if (menu.normal.card.contains(px, py) || menu.normal.playBtn.contains(px, py)) state.hoveredMenuButton = 0;
        else if (menu.endless.card.contains(px, py) || menu.endless.playBtn.contains(px, py)) state.hoveredMenuButton = 1;
        else if (menu.rogue.card.contains(px, py) || menu.rogue.playBtn.contains(px, py)) state.hoveredMenuButton = 2;
        else if (menu.levels.card.contains(px, py) || menu.levels.playBtn.contains(px, py)) state.hoveredMenuButton = 3;
        else if (menu.backBtn.contains(px, py)) state.hoveredMenuButton = 4;         // BACK

        // Specific hover for PLAY buttons (used to apply neon border like Rogue pick buttons)
        state.hoveredPlayModeButton = -1;
        if (menu.normal.playBtn.contains(px, py)) state.hoveredPlayModeButton = 0;
        else if (menu.endless.playBtn.contains(px, py)) state.hoveredPlayModeButton = 1;
        else if (menu.rogue.playBtn.contains(px, py)) state.hoveredPlayModeButton = 2;
        else if (menu.levels.playBtn.contains(px, py)) state.hoveredPlayModeButton = 3;
    } else if (state.currentMenuScreen == MenuScreen::OPTIONS) {
        float offsetY = -50.0f; // Options buttons start lower
        ui::Rect soundBtn = {menu.btn1.x, menu.btn1.y + offsetY, menu.btn1.w, menu.btn1.h};
        
        if (soundBtn.contains(px, py)) state.hoveredMenuButton = 0;    // SOUND
        // BACK button
        else {
            float s = menu.uiScale;
            float backW = 120.0f * s;
            float backH = 50.0f * s;
            float backX = menu.panelX + 20.0f * s;
            float backY = menu.panelY + 15.0f * s;
            if (pointInRectPx(px, py, backX, backY, backW, backH)) {
                state.hoveredMenuButton = 1; // BACK
            }
        }
    } else if (state.currentMenuScreen == MenuScreen::SOUND) {
        // Sound settings: sliders + back
        const auto SL = game::ui::soundSettingsLayout(state.menuLayout, fbW, fbH);

        state.hoveredSoundBack = SL.backBtn.contains(px, py);
        state.hoveredSoundSlider = -1;

        auto hitSlider = [&](int idx, const game::ui::Rect& track, const game::ui::Rect& knob) {
            // generous hitbox: track + knob height
            game::ui::Rect hit{track.x, knob.y, track.w, knob.h};
            if (hit.contains(px, py)) state.hoveredSoundSlider = idx;
        };
        hitSlider(0, SL.master.track, SL.master.knob);
        hitSlider(1, SL.sfx.track, SL.sfx.knob);
        hitSlider(2, SL.music.track, SL.music.knob);
        hitSlider(3, SL.stinger.track, SL.stinger.knob);

        // For consistent button hover rendering in drawButton(), reuse hoveredMenuButton index 2 as BACK.
        if (state.hoveredSoundBack) state.hoveredMenuButton = 2;

        // Dragging logic
        bool down = input.mouseDown(engine::MouseButton::Left);
        if (!down) {
            state.draggingSound = false;
            state.draggingSoundSlider = -1;
        }

        auto applySlider = [&](int idx, float norm) {
            auto clamp01 = [](float v) { return std::max(0.0f, std::min(1.0f, v)); };
            norm = clamp01(norm);

            // Sliders are volume (0..1), not dB.
            if (idx == 0) state.audioMasterVol = norm;
            else if (idx == 1) state.audioSfxVol = norm;
            else if (idx == 2) state.audioMusicVol = norm;
            else if (idx == 3) state.audioStingerVol = norm;
        };

        if (click) {
            if (state.hoveredSoundBack) {
                state.currentMenuScreen = MenuScreen::OPTIONS;
                state.hoveredSoundSlider = -1;
                state.draggingSound = false;
                state.draggingSoundSlider = -1;
                return true;
            }

            if (state.hoveredSoundSlider != -1) {
                state.draggingSound = true;
                state.draggingSoundSlider = state.hoveredSoundSlider;
                // Set immediately on click.
                const game::ui::Rect* track = nullptr;
                if (state.draggingSoundSlider == 0) track = &SL.master.track;
                else if (state.draggingSoundSlider == 1) track = &SL.sfx.track;
                else if (state.draggingSoundSlider == 2) track = &SL.music.track;
                else if (state.draggingSoundSlider == 3) track = &SL.stinger.track;
                if (track) {
                    float norm = (px - track->x) / std::max(1.0f, track->w);
                    applySlider(state.draggingSoundSlider, norm);
                }
                return true;
            }
        }

        if (state.draggingSound && state.draggingSoundSlider != -1 && down) {
            const game::ui::Rect* track = nullptr;
            if (state.draggingSoundSlider == 0) track = &SL.master.track;
            else if (state.draggingSoundSlider == 1) track = &SL.sfx.track;
            else if (state.draggingSoundSlider == 2) track = &SL.music.track;
            else if (state.draggingSoundSlider == 3) track = &SL.stinger.track;
            if (track) {
                float norm = (px - track->x) / std::max(1.0f, track->w);
                applySlider(state.draggingSoundSlider, norm);
            }
        }
    } else if (state.currentMenuScreen == MenuScreen::INSTRUCTIONS) {
        float offsetY = -50.0f; // Instructions buttons start lower (matches OPTIONS layout)
        ui::Rect controlsBtn = {menu.btn1.x, menu.btn1.y + offsetY, menu.btn1.w, menu.btn1.h};
        ui::Rect powerupsBtn = {menu.btn2.x, menu.btn2.y + offsetY, menu.btn2.w, menu.btn2.h};
        ui::Rect rogueCardsBtn = {menu.btn3.x, menu.btn3.y + offsetY, menu.btn3.w, menu.btn3.h};

        if (controlsBtn.contains(px, py)) state.hoveredMenuButton = 0;       // CONTROLS
        else if (powerupsBtn.contains(px, py)) state.hoveredMenuButton = 1;  // POWERUPS
        else if (rogueCardsBtn.contains(px, py)) state.hoveredMenuButton = 2;  // ROGUE CARDS
        else {
            float s = menu.uiScale;
            float backW = 120.0f * s;
            float backH = 50.0f * s;
            float backX = menu.panelX + 20.0f * s;
            float backY = menu.panelY + 15.0f * s;
            if (pointInRectPx(px, py, backX, backY, backW, backH)) {
                state.hoveredMenuButton = 3; // BACK
            }
        }
    } else if (state.currentMenuScreen == MenuScreen::LEVEL_SELECT) {
        // Level select: hover detection for level buttons
        // Use custom panel dimensions (same as MenuRender.cpp)
        float s = menu.uiScale;
        float levelPanelW = 800.0f * s;
        float levelPanelH = 650.0f * s;
        float levelPanelX = ((float)fbW - levelPanelW) * 0.5f;
        float levelPanelY = ((float)fbH * 0.40f) - levelPanelH * 0.5f;
        
        const int totalLevels = 20;
        const int cols = 5;
        const int rows = 4;
        
        float btnSize = 110.0f * s;
        float gapX = 25.0f * s;
        float gapY = 25.0f * s;
        float gridW = cols * btnSize + (cols - 1) * gapX;
        float startX = levelPanelX + (levelPanelW - gridW) * 0.5f;
        float startY = levelPanelY + 80.0f * s;
        
        state.hoveredLevelButton = -1;
        for (int i = 0; i < totalLevels; i++) {
            int row = i / cols;
            int col = i % cols;
            float x = startX + col * (btnSize + gapX);
            float y = startY + (rows - 1 - row) * (btnSize + gapY);
            
            // Convert OpenGL Y to screen Y (Y=0 at top, grows downward)
            float screenYBottom = fbH - y;  // Top edge in pixel coordinates
            float screenYTop = screenYBottom - btnSize;  // Bottom edge in pixel coordinates
            
            if (pointInRectPx(px, py_raw, x, screenYTop, btnSize, btnSize)) {
                state.hoveredLevelButton = i;
                break;
            }
        }
        
        // BACK button
        float backW = 120.0f * s;
        float backH = 50.0f * s;
        float backX = levelPanelX + 20.0f * s;
        float backY = levelPanelY + 15.0f * s;
        if (pointInRectPx(px, py, backX, backY, backW, backH)) {
            state.hoveredMenuButton = 10; // BACK
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
            // NORMAL Mode (click anywhere on the card or PLAY button)
            if (menu.normal.playBtn.contains(px, py) || menu.normal.card.contains(px, py)) {
                state.showInstructions = false;
                state.gameType = GameType::NORMAL;
                state.testOneBrick = false;
                state.mode = GameMode::PLAYING;
                return true;
            }
            // ENDLESS Mode (click anywhere on the card or PLAY button)
            if (menu.endless.playBtn.contains(px, py) || menu.endless.card.contains(px, py)) {
                state.showInstructions = false;
                state.gameType = GameType::ENDLESS;
                state.wave = 1;
                state.testOneBrick = false;
                state.mode = GameMode::PLAYING;
                return true;
            }
            // ROGUE Mode (click anywhere on the card or PLAY button)
            if (menu.rogue.playBtn.contains(px, py) || menu.rogue.card.contains(px, py)) {
                state.showInstructions = false;
                state.gameType = GameType::ROGUE;
                state.wave = 1;
                state.testOneBrick = false;
                state.mode = GameMode::PLAYING;
                return true;
            }
            // LEVELS Mode - go to level select screen (click anywhere on the card or PLAY button)
            if (menu.levels.playBtn.contains(px, py) || menu.levels.card.contains(px, py)) {
                state.currentMenuScreen = MenuScreen::LEVEL_SELECT;
                state.hoveredLevelButton = -1;
                return true;
            }

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
                state.currentMenuScreen = MenuScreen::SOUND;
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
        } else if (state.currentMenuScreen == MenuScreen::LEVEL_SELECT) {
            // Level select screen: grid of 20 level buttons (4 rows x 5 cols)
            // Use custom panel dimensions (same as MenuRender.cpp)
            float s = menu.uiScale;
            float levelPanelW = 800.0f * s;
            float levelPanelH = 650.0f * s;
            float levelPanelX = ((float)fbW - levelPanelW) * 0.5f;
            float levelPanelY = ((float)fbH * 0.40f) - levelPanelH * 0.5f;
            
            const int totalLevels = 20;
            const int cols = 5;
            const int rows = 4;
            
            float btnSize = 110.0f * s;
            float gapX = 25.0f * s;
            float gapY = 25.0f * s;
            float gridW = cols * btnSize + (cols - 1) * gapX;
            float startX = levelPanelX + (levelPanelW - gridW) * 0.5f;
            float startY = levelPanelY + 80.0f * s;
            
            // Check level button clicks
            for (int i = 0; i < totalLevels; i++) {
                int row = i / cols;
                int col = i % cols;
                float x = startX + col * (btnSize + gapX);
                float y = startY + (rows - 1 - row) * (btnSize + gapY);
                
                // Convert OpenGL Y to screen Y (Y=0 at top, grows downward)
                float screenYBottom = fbH - y;  // Top edge in pixel coordinates
                float screenYTop = screenYBottom - btnSize;  // Bottom edge in pixel coordinates
                
                if (pointInRectPx(px, py_raw, x, screenYTop, btnSize, btnSize)) {
                    // Check if level is unlocked
                    if (i < state.levelsBestLevel) {
                        // Start this level
                        state.showInstructions = false;
                        state.gameType = GameType::LEVELS;
                        state.currentLevel = i + 1;
                        state.testOneBrick = false;
                        state.mode = GameMode::PLAYING;
                        return true;
                    }
                    return true; // Clicked but locked
                }
            }
            
            // BACK button
            float backW = 120.0f * s;
            float backH = 50.0f * s;
            float backX = levelPanelX + 20.0f * s;
            float backY = levelPanelY + 15.0f * s;
            if (pointInRectPx(px, py, backX, backY, backW, backH)) {
                state.currentMenuScreen = MenuScreen::PLAY_MODES;
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

            ui::Rect rogueCardsBtn = {menu.btn3.x, menu.btn3.y + offsetY, menu.btn3.w, menu.btn3.h};
            if (rogueCardsBtn.contains(px, py)) {
                state.instructionsTab = 2;
                state.showInstructions = true;
                state.rogueCardsInspectOpen = false;
                state.hoveredRogueCardsItem = -1;
                state.rogueCardsScrollPowerups = 0.0f;
                state.rogueCardsScrollModifiers = 0.0f;
                state.rogueCardsScrollOp = 0.0f;
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

