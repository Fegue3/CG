// GameUpdatePausedOverlay.cpp
/**
 * @file GameUpdatePausedOverlay.cpp
 * @brief Lógica de input/UI do overlay de PAUSA.
 *
 * Comportamento:
 *  - Só corre quando mode == PAUSED.
 *  - Calcula layout:
 *      - pauseOverlayRogue (se ROGUE e houver cartas escolhidas) -> painel maior
 *      - pauseOverlay normal caso contrário
 *  - Atualiza hover dos botões principais (left/right)
 *  - Em Rogue, também faz hit-test dos badges das cartas (grid 10x2, até 20 cartas)
 *  - Clicks:
 *      - LeftBtn  -> Restart (init)
 *      - RightBtn -> Back to Menu (MAIN)
 *
 * Nota:
 *  - A posição do grid e sizing tem de bater certo com o render em PauseEndOverlays.cpp.
 */
 #include "game/Game.hpp"

 #include "game/ui/OverlayLayout.hpp"
 
 namespace game {
 
 bool Game::updatePausedOverlay(const engine::Input& input) {
     if (m_state.mode != GameMode::PAUSED) return false;
 
     auto [fbW, fbH] = m_window.getFramebufferSize();
     auto [px, py_raw] = input.mousePosFbPx();
     float py = (float)fbH - py_raw;
     bool click = input.mousePressed(engine::MouseButton::Left);
 
     // Layout: Rogue tem painel maior se houver cartas escolhidas (para mostrar o deck).
     const auto L = (m_state.gameType == GameType::ROGUE && !m_state.rogueChosen.empty())
         ? game::ui::pauseOverlayRogue(fbW, fbH)
         : game::ui::pauseOverlay(fbW, fbH);
 
     // ------------------------------------------------------------------
     // Hover dos botões principais
     // ------------------------------------------------------------------
     m_state.hoveredOverlayButton = -1;
     if (L.leftBtn.contains(px, py)) m_state.hoveredOverlayButton = 0;
     else if (L.rightBtn.contains(px, py)) m_state.hoveredOverlayButton = 1;
 
     // ------------------------------------------------------------------
     // Hover das cartas Rogue (badges)
     // ------------------------------------------------------------------
     m_state.hoveredPauseRogueCard = -1;
 
     if (m_state.gameType == GameType::ROGUE && !m_state.rogueChosen.empty()) {
         float panelX = L.panel.x;
         float panelY = L.panel.y;
         float panelH = L.panel.h;
 
         // “Âncoras” de posicionamento — devem bater certo com o render.
         float titleH = 22.0f;
         float titleY = panelY + panelH - titleH - 165.0f;
 
         float badgeStartY = titleY - 80.0f;  // espaço abaixo do título
         float badgeBaseW  = 50.0f;
         float badgeBaseH  = 50.0f;
         float badgeGap    = 10.0f;           // spacing entre colunas
         float badgeRowGap = 80.0f;           // spacing entre linhas (mais largo)
         float badgesPerRow = 10;             // 10 colunas -> 2 linhas -> até 20 cartas
 
         // Centrar grid horizontalmente dentro do painel
         float totalWidth = badgesPerRow * badgeBaseW + (badgesPerRow - 1) * badgeGap;
         float badgeStartX = panelX + (L.panel.w - totalWidth) * 0.5f;
 
         int rowIdx = 0;
         int colIdx = 0;
         int cardCount = 0;
 
         // Até 20 cartas, em ordem.
         for (size_t i = 0; i < m_state.rogueChosen.size() && cardCount < 20; ++i) {
             float badgeX = badgeStartX + colIdx * (badgeBaseW + badgeGap);
             float badgeY = badgeStartY - rowIdx * badgeRowGap;
             float badgeW = badgeBaseW;
             float badgeH = badgeBaseH;
 
             // Hit test simples (px/py em y-up)
             if (px >= badgeX && px <= badgeX + badgeW &&
                 py >= badgeY && py <= badgeY + badgeH) {
                 m_state.hoveredPauseRogueCard = (int)i;
                 break;
             }
 
             // Avança para o próximo slot no grid
             colIdx++;
             if (colIdx >= (int)badgesPerRow) {
                 colIdx = 0;
                 rowIdx++;
             }
             cardCount++;
         }
     }
 
     // ------------------------------------------------------------------
     // Click actions
     // ------------------------------------------------------------------
     if (click) {
         // Left: Restart
         if (L.leftBtn.contains(px, py)) {
             m_audio.playSfx("ui/ui_confirm");
             init();
             return true;
         }
 
         // Right: Back to Menu
         if (L.rightBtn.contains(px, py)) {
             m_audio.playSfx("ui/ui_back");
             m_state.mode = GameMode::MENU;
             m_state.currentMenuScreen = MenuScreen::MAIN;
             m_state.showInstructions = false;
             return true;
         }
     }
 
     // Prev trackers (early return)
     m_prevMode = m_state.mode;
     m_prevGameType = m_state.gameType;
 
     // Segurança: enquanto pausado, não deixar loops de gameplay a tocar.
     m_audio.setSfxLoopEnabled("endless/endless_warning_loop", false, 0.12f);
 
     return true;
 }
 
 } // namespace game
 