// GameUpdateEndOverlay.cpp
/**
 * @file GameUpdateEndOverlay.cpp
 * @brief Lógica de input/UI do overlay de fim de jogo (GAME_OVER / WIN).
 *
 * Responsabilidades:
 *  - Só corre quando o jogo está em GAME_OVER ou WIN.
 *  - Calcula layout do overlay (endOverlay).
 *  - Atualiza hover do rato (hoveredOverlayButton).
 *  - Processa cliques:
 *      - LeftBtn  -> Restart (reinicia jogo / nível)
 *      - RightBtn -> Voltar ao Menu (ecrã certo consoante o modo)
 *  - Dispara música/stingers quando entra no estado de fim.
 *
 * Notas:
 *  - Coordenadas UI são y-up, por isso convertemos py_raw (top-left) para py (y-up).
 *  - m_prevMode é usado para detetar transição e tocar música/stingers apenas na entrada.
 */
 #include "game/Game.hpp"

 #include "game/ui/OverlayLayout.hpp"
 
 namespace game {
 
 bool Game::updateEndOverlay(const engine::Input& input) {
     // Só processa lógica de UI se estivermos em GAME_OVER ou WIN.
     if (!(m_state.mode == GameMode::GAME_OVER || m_state.mode == GameMode::WIN)) return false;
 
     auto [fbW, fbH] = m_window.getFramebufferSize();
     auto [px, py_raw] = input.mousePosFbPx();
 
     // Converter coords do rato para o espaço y-up usado pelo UI
     float py = (float)fbH - py_raw;
 
     bool click = input.mousePressed(engine::MouseButton::Left);
 
     // Layout do overlay de fim (botões, painel, etc).
     const auto L = game::ui::endOverlay(fbW, fbH);
 
     // ------------------------------------------------------------------
     // Entrada no overlay: música + stingers
     // ------------------------------------------------------------------
     // Só dispara quando há transição de modo (evita repetir o stinger em loop).
     if (m_prevMode != m_state.mode) {
         setMusic("music_results_loop", 0.25f);
 
         if (m_state.mode == GameMode::GAME_OVER) {
             // GAME OVER: SFX stinger + stinger musical.
             // (assumindo que ambos existem nos assets)
             m_audio.playSfx("stingers/stinger_game_over", -1.0f);
             m_audio.playStinger("stinger_game_over", +3.0f);
         } else {
             // WIN: stinger diferente para Rogue vs outros modos.
             m_audio.playStinger(
                 (m_state.gameType == GameType::ROGUE) ? "stinger_run_complete" : "stinger_level_clear",
                 +2.5f
             );
         }
     }
 
     // ------------------------------------------------------------------
     // Hover state
     // ------------------------------------------------------------------
     m_state.hoveredOverlayButton = -1;
     if (L.leftBtn.contains(px, py)) m_state.hoveredOverlayButton = 0;
     else if (L.rightBtn.contains(px, py)) m_state.hoveredOverlayButton = 1;
 
     // ------------------------------------------------------------------
     // Click actions
     // ------------------------------------------------------------------
     if (click) {
         // Left button: Restart
         // Em LEVELS, a intenção é “reiniciar o mesmo level”, mas init() já respeita currentLevel.
         if (L.leftBtn.contains(px, py)) {
             m_audio.playSfx("ui/ui_confirm");
             init();
             return true;
         }
 
         // Right button: Back to Menu
         if (L.rightBtn.contains(px, py)) {
             m_audio.playSfx("ui/ui_back");
             m_state.mode = GameMode::MENU;
 
             // LEVELS: volta ao seletor de níveis
             if (m_state.gameType == GameType::LEVELS) {
                 m_state.currentMenuScreen = MenuScreen::LEVEL_SELECT;
             } else {
                 m_state.currentMenuScreen = MenuScreen::MAIN;
             }
 
             m_state.showInstructions = false;
             return true;
         }
     }
 
     // Update prev trackers (early return)
     m_prevMode = m_state.mode;
     m_prevGameType = m_state.gameType;
     return true;
 }
 
 } // namespace game
 