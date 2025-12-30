#include "game/Game.hpp"

#include "game/systems/InputSystem.hpp"
#include "game/ui/OverlayLayout.hpp"

namespace game {

bool Game::updateMenu(const engine::Input& input) {
    if (m_state.mode != GameMode::MENU) return false;

    // Keep menu layout synced with the current framebuffer size and real font metrics.
    {
        auto [fbW, fbH] = m_window.getFramebufferSize();
        m_state.menuLayout = ui::calculateMenuLayout(m_renderer, fbW, fbH);
    }
    if (InputSystem::handleMenuInput(m_state, input, m_window)) {
        // If menu handled input and signaled init needed
        if (m_state.mode == GameMode::PLAYING) {
            init();
        }
    }

    // --- Menu audio director ---
    if (m_prevMode != GameMode::MENU) {
        setMusic("music_menu_loop", 0.25f);
        // Safety: kill gameplay loops while in menu.
        m_audio.setSfxLoopEnabled("endless/endless_warning_loop", false, 0.12f);
    }

    if (m_state.currentMenuScreen != m_prevMenuScreen) {
        m_audio.playSfx("ui/ui_select");
    }
    if (m_state.showInstructions != m_prevShowInstructions) {
        m_audio.playSfx(m_state.showInstructions ? "ui/ui_confirm" : "ui/ui_back");
    }
    if (m_state.showInstructions && m_state.instructionsTab != m_prevInstructionsTab) {
        m_audio.playSfx("ui/ui_toggle", -2.0f);
    }

    // More instruction UX SFX: powerup inspector arrows and rogue card modal open/close.
    if (m_state.showInstructions) {
        if (m_state.instructionsTab == 1 && m_state.powerupInspectIndex != m_prevPowerupInspectIndex) {
            m_audio.playSfx("ui/ui_level_scroll", -2.5f);
        }
        if (m_state.instructionsTab == 2 && m_state.rogueCardsInspectOpen != m_prevRogueCardsInspectOpen) {
            m_audio.playSfx(m_state.rogueCardsInspectOpen ? "ui/ui_card_flip" : "ui/ui_card_cancel", -2.0f);
        }
    }

    bool hoverChanged =
        (m_state.hoveredMenuButton != m_prevHoveredMenuButton) ||
        (m_state.hoveredPlayModeButton != m_prevHoveredPlayModeButton) ||
        (m_state.hoveredCloseButton != (m_prevHoveredCloseButton != 0)) ||
        (m_state.hoveredPowerupNav != m_prevHoveredPowerupNav);
    if (hoverChanged) {
        if (m_state.hoveredMenuButton != -1 || m_prevHoveredMenuButton != -1 ||
            m_state.hoveredPlayModeButton != -1 || m_prevHoveredPlayModeButton != -1 ||
            m_state.hoveredCloseButton || (m_prevHoveredCloseButton != 0) ||
            m_state.hoveredPowerupNav != -1 || m_prevHoveredPowerupNav != -1) {
            m_audio.playSfx("ui/ui_move", -6.0f);
        }
    }

    // Update prev trackers before menu early-return
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

    // IMPORTANT: if we just left MENU (e.g. clicked PLAY), don't early-return.
    return (m_state.mode == GameMode::MENU);
}

} // namespace game


