// GameUpdateMenu.cpp
/**
 * @file GameUpdateMenu.cpp
 * @brief Update do estado MENU: input, layout, e “audio director” do menu.
 *
 * Responsabilidades:
 *  - Só corre quando mode == MENU.
 *  - Recalcula menuLayout sempre com métricas reais e framebuffer atual (resizing safe).
 *  - Encaminha input para InputSystem::handleMenuInput().
 *  - Se o input mudar o modo para PLAYING, chama init().
 *  - Gere música do menu e SFX de navegação:
 *      - mudar ecrã (MAIN/OPTIONS/PLAY_MODES/...)
 *      - abrir/fechar instructions overlay
 *      - mudar tab
 *      - hover changes (ui_move)
 */
 #include "game/Game.hpp"

 #include "game/systems/InputSystem.hpp"
 #include "game/ui/OverlayLayout.hpp"
 
 namespace game {
 
 bool Game::updateMenu(const engine::Input& input) {
     if (m_state.mode != GameMode::MENU) return false;
 
     // ------------------------------------------------------------------
     // Layout sync: necessário para que input hitboxes batam certo com render
     // ------------------------------------------------------------------
     {
         auto [fbW, fbH] = m_window.getFramebufferSize();
         m_state.menuLayout = ui::calculateMenuLayout(m_renderer, fbW, fbH);
     }
 
     // Encaminha input do menu para o InputSystem.
     if (InputSystem::handleMenuInput(m_state, input, m_window)) {
         // Se o menu trocou o modo para PLAYING (ex: clicou PLAY), inicializa o jogo.
         if (m_state.mode == GameMode::PLAYING) {
             init();
         }
     }
 
     // ------------------------------------------------------------------
     // Menu audio director
     // ------------------------------------------------------------------
     // Ao entrar no MENU:
     //  - tocar música de menu
     //  - matar loops de gameplay (segurança)
     if (m_prevMode != GameMode::MENU) {
         setMusic("music_menu_loop", 0.25f);
         m_audio.setSfxLoopEnabled("endless/endless_warning_loop", false, 0.12f);
     }
 
     // Mudança de ecrã no menu -> SFX select
     if (m_state.currentMenuScreen != m_prevMenuScreen) {
         m_audio.playSfx("ui/ui_select");
     }
 
     // Abrir/fechar overlay de instruções -> confirm/back
     if (m_state.showInstructions != m_prevShowInstructions) {
         m_audio.playSfx(m_state.showInstructions ? "ui/ui_confirm" : "ui/ui_back");
     }
 
     // Trocar tab de instruções -> toggle
     if (m_state.showInstructions && m_state.instructionsTab != m_prevInstructionsTab) {
         m_audio.playSfx("ui/ui_toggle", -2.0f);
     }
 
     // UX extra no overlay:
     //  - tab powerups: trocar item do inspector
     //  - tab rogue cards: abrir/fechar modal do cartão
     if (m_state.showInstructions) {
         if (m_state.instructionsTab == 1 && m_state.powerupInspectIndex != m_prevPowerupInspectIndex) {
             m_audio.playSfx("ui/ui_level_scroll", -2.5f);
         }
         if (m_state.instructionsTab == 2 && m_state.rogueCardsInspectOpen != m_prevRogueCardsInspectOpen) {
             m_audio.playSfx(m_state.rogueCardsInspectOpen ? "ui/ui_card_flip" : "ui/ui_card_cancel", -2.0f);
         }
     }
 
     // ------------------------------------------------------------------
     // Hover SFX: toca um “ui_move” quando a seleção hover muda (entrar/sair)
     // ------------------------------------------------------------------
     bool hoverChanged =
         (m_state.hoveredMenuButton != m_prevHoveredMenuButton) ||
         (m_state.hoveredPlayModeButton != m_prevHoveredPlayModeButton) ||
         (m_state.hoveredCloseButton != (m_prevHoveredCloseButton != 0)) ||
         (m_state.hoveredPowerupNav != m_prevHoveredPowerupNav);
 
     if (hoverChanged) {
         // Só toca se a mudança envolver “estar sobre algo clicável”
         if (m_state.hoveredMenuButton != -1 || m_prevHoveredMenuButton != -1 ||
             m_state.hoveredPlayModeButton != -1 || m_prevHoveredPlayModeButton != -1 ||
             m_state.hoveredCloseButton || (m_prevHoveredCloseButton != 0) ||
             m_state.hoveredPowerupNav != -1 || m_prevHoveredPowerupNav != -1) {
             m_audio.playSfx("ui/ui_move", -6.0f);
         }
     }
 
     // ------------------------------------------------------------------
     // Prev trackers (antes do early return)
     // ------------------------------------------------------------------
     m_prevMode = m_state.mode;
     m_prevGameType = m_state.gameType;
     m_prevMenuScreen = m_state.currentMenuScreen;
     m_prevHoveredMenuButton = m_state.hoveredMenuButton;
     m_prevHoveredPlayModeButton = m_state.hoveredPlayModeButton;
     m_prevShowInstructions = m_state.showInstructions;
     m_prevInstructionsTab = m_state.instructionsTab;
     m_prevHoveredCloseButton = m_state.hoveredCloseButton ? 1 : 0;
     m_prevHoveredPowerupNav = m_state.hoveredPowerupNav;
     m_prevPowerupInspectIndex = m_state.powerupInspectIndex;
     m_prevRogueCardsInspectOpen = m_state.rogueCardsInspectOpen;
 
     // IMPORTANTE:
     // Se o menu acabou de sair (m_state.mode != MENU), devolve false para permitir update do gameplay.
     return (m_state.mode == GameMode::MENU);
 }
 
 } // namespace game
 