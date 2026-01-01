/**
 * @file InputSystemMenu.cpp
 * @brief Input do menu (hover/click) + overlays (Instructions) + SOUND sliders + Level Select.
 *
 * Fluxo geral:
 *  - Lê mouse em framebuffer pixels, converte Y (GLFW->UI).
 *  - Se instructions overlay está aberto:
 *      - BACK fecha overlay
 *      - Tab Powerups: drag rotate (viewport) + setas navegam
 *      - Tab Rogue Cards: 3 colunas scroll independentes + modal inspector (click fora fecha)
 *  - Caso contrário:
 *      - MAIN: PLAY / INSTRUCTIONS / OPTIONS / EXIT + test badge
 *      - PLAY_MODES: 4 cards + BACK (hover no card inteiro)
 *      - OPTIONS: SOUND + BACK
 *      - SOUND: sliders (click+drag) + BACK
 *      - INSTRUCTIONS screen: botões para abrir overlay nas tabs
 *      - LEVEL_SELECT: grid 20 níveis + lock + BACK
 */
 #include "game/systems/InputSystem.hpp"
 #include "game/GameConfig.hpp"
 #include "game/GameState.hpp"
 #include "game/ui/OverlayLayout.hpp"
 #include "game/ui/InstructionsOverlayLayout.hpp"
 #include "game/rogue/RogueCards.hpp"
 #include "engine/Window.hpp"
 #include "engine/Input.hpp"
 
 #include <algorithm>
 #include <cmath>
 #include <vector>
 #include <glm/glm.hpp>
 
 namespace game {
 
 bool InputSystem::handleMenuInput(GameState& state, const engine::Input& input, engine::Window& window) {
     auto [fbW, fbH] = window.getFramebufferSize();
     auto [px, py_raw] = input.mousePosFbPx();
 
     // UI usa origem bottom-left; input vem com origem top-left
     float py = (float)fbH - py_raw;
 
     bool click = input.mousePressed(engine::MouseButton::Left);
 
     // Layout já calculado e cached no state (tem de bater com o render)
     const ui::MenuLayout& menu = state.menuLayout;
 
 #ifdef BREAKOUT3D_DEBUG
     // Secret: unlock levels
     if (input.keyPressed(engine::Key::L)) {
         state.levelsBestLevel = 20;
     }
 #endif
 
     // ---------------- Instructions Overlay (modal-ish) ----------------
     if (state.showInstructions) {
         state.hoveredMenuButton = -1;
         const auto OL = ui::instructionsOverlayLayout(menu, fbW, fbH, state.instructionsTab);
 
         state.hoveredCloseButton = OL.backBtn.contains(px, py);
         state.hoveredPowerupNav = -1;
 
         // ---------- Tab 1: Powerups inspector ----------
         if (state.instructionsTab == 1) {
             if (OL.navLeft.contains(px, py)) state.hoveredPowerupNav = 0;
             else if (OL.navRight.contains(px, py)) state.hoveredPowerupNav = 1;
 
             bool down = input.mouseDown(engine::MouseButton::Left);
 
             // Drag rotate (viewport do modelo)
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
 
                     const float lim = 1.45f;
                     if (state.powerupInspectPitch > lim) state.powerupInspectPitch = lim;
                     if (state.powerupInspectPitch < -lim) state.powerupInspectPitch = -lim;
                 }
             } else {
                 state.powerupInspectDragging = false;
             }
 
             // Click arrows para navegar (wrap)
             if (click) {
                 const int count = 8; // tem de bater com o render
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
 
         // ---------- Tab 2: Rogue cards browser ----------
         if (state.instructionsTab == 2) {
             // Recria listas (powerups/modifiers/ops) para hitboxes baterem com o render.
             std::vector<game::rogue::RogueCardId> powerups;
             std::vector<game::rogue::RogueCardId> modifiers;
             std::vector<game::rogue::RogueCardId> ops;
 
             for (auto id : game::rogue::allCardIds()) {
                 const auto& def = game::rogue::cardDef(id);
                 if (def.isOp) ops.push_back(id);
                 else if (game::rogue::isPowerupCard(id)) powerups.push_back(id);
                 else modifiers.push_back(id);
             }
 
             // Geometria: 3 colunas dentro do content rect
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
 
             // Hit test de uma lista numa coluna
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
 
             state.hoveredRogueCardsItem = -1;
 
             // Modal inspector: click fora fecha, e bloqueia list interactions
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
             } else {
                 int base0 = 0;
                 int base1 = base0 + (int)powerups.size();
                 int base2 = base1 + (int)modifiers.size();
 
                 bool hit = false;
                 hit = hitList(colX0, state.rogueCardsScrollPowerups, powerups, base0) || hit;
                 hit = hitList(colX1, state.rogueCardsScrollModifiers, modifiers, base1) || hit;
                 hit = hitList(colX2, state.rogueCardsScrollOp, ops, base2) || hit;
 
                 // Wheel scroll: coluna sob o rato
                 float wheel = input.mouseScrollY();
                 if (std::abs(wheel) > 1e-4f) {
                     auto applyScroll = [&](float& scrollPx, const std::vector<game::rogue::RogueCardId>& ids) {
                         float maxScroll = maxScrollFor(ids);
                         const float speed = 42.0f * menu.uiScale;
                         scrollPx -= wheel * speed;
                         scrollPx = std::clamp(scrollPx, 0.0f, maxScroll);
                     };
 
                     bool inY = (py >= viewBottomY && py <= viewTopY);
                     if (inY && px >= colX0 && px <= colX0 + colW) applyScroll(state.rogueCardsScrollPowerups, powerups);
                     else if (inY && px >= colX1 && px <= colX1 + colW) applyScroll(state.rogueCardsScrollModifiers, modifiers);
                     else if (inY && px >= colX2 && px <= colX2 + colW) applyScroll(state.rogueCardsScrollOp, ops);
                 }
 
                 // Click num item abre modal
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
 
         // BACK fecha overlay
         if (click) {
             if (state.hoveredCloseButton) {
                 state.showInstructions = false;
                 state.rogueCardsInspectOpen = false;
             }
         }
 
         return true;
     }
 
     // ---------------- reset hover de overlay quando não está aberto ----------------
     state.hoveredCloseButton = false;
     state.hoveredPowerupNav = -1;
     state.powerupInspectDragging = false;
 
     // Test badge hover (MAIN only)
     state.hoveredTestBadge = false;
     if (state.currentMenuScreen == MenuScreen::MAIN) {
         state.hoveredTestBadge = menu.testBadge.contains(px, py);
     }
 
     // ---------------- Hover por screen ----------------
     state.hoveredMenuButton = -1;
 
     if (state.currentMenuScreen == MenuScreen::MAIN) {
         if (menu.btn1.contains(px, py)) state.hoveredMenuButton = 0;      // PLAY
         else if (menu.btn2.contains(px, py)) state.hoveredMenuButton = 1; // INSTRUCTIONS
         else if (menu.btn3.contains(px, py)) state.hoveredMenuButton = 2; // OPTIONS
         else if (menu.btn4.contains(px, py)) state.hoveredMenuButton = 3; // EXIT
     } else if (state.currentMenuScreen == MenuScreen::PLAY_MODES) {
         if (menu.normal.card.contains(px, py) || menu.normal.playBtn.contains(px, py)) state.hoveredMenuButton = 0;
         else if (menu.endless.card.contains(px, py) || menu.endless.playBtn.contains(px, py)) state.hoveredMenuButton = 1;
         else if (menu.rogue.card.contains(px, py) || menu.rogue.playBtn.contains(px, py)) state.hoveredMenuButton = 2;
         else if (menu.levels.card.contains(px, py) || menu.levels.playBtn.contains(px, py)) state.hoveredMenuButton = 3;
         else if (menu.backBtn.contains(px, py)) state.hoveredMenuButton = 4;
 
         state.hoveredPlayModeButton = -1;
         if (menu.normal.playBtn.contains(px, py)) state.hoveredPlayModeButton = 0;
         else if (menu.endless.playBtn.contains(px, py)) state.hoveredPlayModeButton = 1;
         else if (menu.rogue.playBtn.contains(px, py)) state.hoveredPlayModeButton = 2;
         else if (menu.levels.playBtn.contains(px, py)) state.hoveredPlayModeButton = 3;
     } else if (state.currentMenuScreen == MenuScreen::OPTIONS) {
         float offsetY = -50.0f;
         ui::Rect soundBtn = {menu.btn1.x, menu.btn1.y + offsetY, menu.btn1.w, menu.btn1.h};
 
         if (soundBtn.contains(px, py)) state.hoveredMenuButton = 0; // SOUND
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
         // Sound screen: sliders + back
         const auto SL = game::ui::soundSettingsLayout(state.menuLayout, fbW, fbH);
 
         state.hoveredSoundBack = SL.backBtn.contains(px, py);
         state.hoveredSoundSlider = -1;
 
         auto hitSlider = [&](int idx, const game::ui::Rect& track, const game::ui::Rect& knob) {
             // hitbox generosa (track + knob height)
             game::ui::Rect hit{track.x, knob.y, track.w, knob.h};
             if (hit.contains(px, py)) state.hoveredSoundSlider = idx;
         };
         hitSlider(0, SL.master.track, SL.master.knob);
         hitSlider(1, SL.sfx.track, SL.sfx.knob);
         hitSlider(2, SL.music.track, SL.music.knob);
         hitSlider(3, SL.stinger.track, SL.stinger.knob);
 
         // drawButton() usa hoveredMenuButton index 2 como BACK
         if (state.hoveredSoundBack) state.hoveredMenuButton = 2;
 
         // Drag logic
         bool down = input.mouseDown(engine::MouseButton::Left);
         if (!down) {
             state.draggingSound = false;
             state.draggingSoundSlider = -1;
         }
 
         auto applySlider = [&](int idx, float norm) {
             auto clamp01 = [](float v) { return std::max(0.0f, std::min(1.0f, v)); };
             norm = clamp01(norm);
 
             if (idx == 0) state.audioMasterVol  = norm;
             else if (idx == 1) state.audioSfxVol = norm;
             else if (idx == 2) state.audioMusicVol = norm;
             else if (idx == 3) state.audioStingerVol = norm;
         };
 
         // Click: BACK ou inicia drag
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
 
         // Drag contínuo
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
         float offsetY = -50.0f;
         ui::Rect controlsBtn   = {menu.btn1.x, menu.btn1.y + offsetY, menu.btn1.w, menu.btn1.h};
         ui::Rect powerupsBtn   = {menu.btn2.x, menu.btn2.y + offsetY, menu.btn2.w, menu.btn2.h};
         ui::Rect rogueCardsBtn = {menu.btn3.x, menu.btn3.y + offsetY, menu.btn3.w, menu.btn3.h};
 
         if (controlsBtn.contains(px, py)) state.hoveredMenuButton = 0;
         else if (powerupsBtn.contains(px, py)) state.hoveredMenuButton = 1;
         else if (rogueCardsBtn.contains(px, py)) state.hoveredMenuButton = 2;
         else {
             float s = menu.uiScale;
             float backW = 120.0f * s;
             float backH = 50.0f * s;
             float backX = menu.panelX + 20.0f * s;
             float backY = menu.panelY + 15.0f * s;
             if (pointInRectPx(px, py, backX, backY, backW, backH)) {
                 state.hoveredMenuButton = 3;
             }
         }
     } else if (state.currentMenuScreen == MenuScreen::LEVEL_SELECT) {
         // hover grid 20 levels
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
 
             float screenYBottom = fbH - y;
             float screenYTop = screenYBottom - btnSize;
 
             if (pointInRectPx(px, py_raw, x, screenYTop, btnSize, btnSize)) {
                 state.hoveredLevelButton = i;
                 break;
             }
         }
 
         // BACK hover
         float backW = 120.0f * s;
         float backH = 50.0f * s;
         float backX = levelPanelX + 20.0f * s;
         float backY = levelPanelY + 15.0f * s;
         if (pointInRectPx(px, py, backX, backY, backW, backH)) {
             state.hoveredMenuButton = 10;
         }
     }
 
     // ---------------- Click actions ----------------
     if (click) {
         if (state.currentMenuScreen == MenuScreen::MAIN) {
             if (menu.testBadge.contains(px, py)) {
                 state.showInstructions = false;
                 state.gameType = GameType::NORMAL;
                 state.testOneBrick = true;
                 state.mode = GameMode::PLAYING;
                 return true;
             }
 
             if (menu.btn1.contains(px, py)) {
                 state.currentMenuScreen = MenuScreen::PLAY_MODES;
                 return true;
             }
             if (menu.btn2.contains(px, py)) {
                 state.currentMenuScreen = MenuScreen::INSTRUCTIONS;
                 return true;
             }
             if (menu.btn3.contains(px, py)) {
                 state.currentMenuScreen = MenuScreen::OPTIONS;
                 return true;
             }
             if (menu.btn4.contains(px, py)) {
                 window.requestClose();
                 return true;
             }
         } else if (state.currentMenuScreen == MenuScreen::PLAY_MODES) {
             if (menu.normal.playBtn.contains(px, py) || menu.normal.card.contains(px, py)) {
                 state.showInstructions = false;
                 state.gameType = GameType::NORMAL;
                 state.testOneBrick = false;
                 state.mode = GameMode::PLAYING;
                 return true;
             }
             if (menu.endless.playBtn.contains(px, py) || menu.endless.card.contains(px, py)) {
                 state.showInstructions = false;
                 state.gameType = GameType::ENDLESS;
                 state.wave = 1;
                 state.testOneBrick = false;
                 state.mode = GameMode::PLAYING;
                 return true;
             }
             if (menu.rogue.playBtn.contains(px, py) || menu.rogue.card.contains(px, py)) {
                 state.showInstructions = false;
                 state.gameType = GameType::ROGUE;
                 state.wave = 1;
                 state.testOneBrick = false;
                 state.mode = GameMode::PLAYING;
                 return true;
             }
             if (menu.levels.playBtn.contains(px, py) || menu.levels.card.contains(px, py)) {
                 state.currentMenuScreen = MenuScreen::LEVEL_SELECT;
                 state.hoveredLevelButton = -1;
                 return true;
             }
 
             if (menu.backBtn.contains(px, py)) {
                 state.currentMenuScreen = MenuScreen::MAIN;
                 return true;
             }
         } else if (state.currentMenuScreen == MenuScreen::OPTIONS) {
             float offsetY = -50.0f;
             ui::Rect soundBtn = {menu.btn1.x, menu.btn1.y + offsetY, menu.btn1.w, menu.btn1.h};
             if (soundBtn.contains(px, py)) {
                 state.currentMenuScreen = MenuScreen::SOUND;
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
         } else if (state.currentMenuScreen == MenuScreen::LEVEL_SELECT) {
             // click levels
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
 
             for (int i = 0; i < totalLevels; i++) {
                 int row = i / cols;
                 int col = i % cols;
                 float x = startX + col * (btnSize + gapX);
                 float y = startY + (rows - 1 - row) * (btnSize + gapY);
 
                 float screenYBottom = fbH - y;
                 float screenYTop = screenYBottom - btnSize;
 
                 if (pointInRectPx(px, py_raw, x, screenYTop, btnSize, btnSize)) {
                     if (i < state.levelsBestLevel) {
                         state.showInstructions = false;
                         state.gameType = GameType::LEVELS;
                         state.currentLevel = i + 1;
                         state.testOneBrick = false;
                         state.mode = GameMode::PLAYING;
                         return true;
                     }
                     return true; // clicked locked
                 }
             }
 
             // BACK
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
 
     return true;
 }
 
 } // namespace game
 