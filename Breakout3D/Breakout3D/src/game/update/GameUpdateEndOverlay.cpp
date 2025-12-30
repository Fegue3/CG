#include "game/Game.hpp"

#include "game/ui/OverlayLayout.hpp"

namespace game {

bool Game::updateEndOverlay(const engine::Input& input) {
    // GAME OVER / WIN: Click UI logic
    if (!(m_state.mode == GameMode::GAME_OVER || m_state.mode == GameMode::WIN)) return false;

    auto [fbW, fbH] = m_window.getFramebufferSize();
    auto [px, py_raw] = input.mousePosFbPx();
    float py = (float)fbH - py_raw;
    bool click = input.mousePressed(engine::MouseButton::Left);

    const auto L = game::ui::endOverlay(fbW, fbH);

    // Entry music/stingers
    if (m_prevMode != m_state.mode) {
        setMusic("music_results_loop", 0.25f);
        if (m_state.mode == GameMode::GAME_OVER) {
            // SFX stinger + musical stinger (both exist in assets)
            m_audio.playSfx("stingers/stinger_game_over", -1.0f);
            m_audio.playStinger("stinger_game_over", +3.0f);
        } else {
            // WIN
            m_audio.playStinger((m_state.gameType == GameType::ROGUE) ? "stinger_run_complete" : "stinger_level_clear", +2.5f);
        }
    }

    // Update hover state
    m_state.hoveredOverlayButton = -1;
    if (L.leftBtn.contains(px, py)) m_state.hoveredOverlayButton = 0;
    else if (L.rightBtn.contains(px, py)) m_state.hoveredOverlayButton = 1;

    if (click) {
        // Left button: Restart (for LEVELS mode, restart same level)
        if (L.leftBtn.contains(px, py)) {
            m_audio.playSfx("ui/ui_confirm");
            if (m_state.gameType == GameType::LEVELS) {
                // Restart the same level (keep currentLevel)
                init();
            } else {
                init();
            }
            return true;
        }
        // Right button: Back to Menu
        if (L.rightBtn.contains(px, py)) {
            m_audio.playSfx("ui/ui_back");
            m_state.mode = GameMode::MENU;
            // LEVELS mode: return to level select
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


