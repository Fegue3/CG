#include "game/Game.hpp"

#include "game/ui/OverlayLayout.hpp"

namespace game {

bool Game::updatePausedOverlay(const engine::Input& input) {
    // PAUSED: Click UI logic for buttons
    if (m_state.mode != GameMode::PAUSED) return false;

    auto [fbW, fbH] = m_window.getFramebufferSize();
    auto [px, py_raw] = input.mousePosFbPx();
    float py = (float)fbH - py_raw;
    bool click = input.mousePressed(engine::MouseButton::Left);

    // Use larger panel for ROGUE mode
    const auto L = (m_state.gameType == GameType::ROGUE && !m_state.rogueChosen.empty())
        ? game::ui::pauseOverlayRogue(fbW, fbH)
        : game::ui::pauseOverlay(fbW, fbH);

    // Update hover state for buttons
    m_state.hoveredOverlayButton = -1;
    if (L.leftBtn.contains(px, py)) m_state.hoveredOverlayButton = 0;
    else if (L.rightBtn.contains(px, py)) m_state.hoveredOverlayButton = 1;

    // Update hover state for Rogue cards (if in ROGUE mode)
    m_state.hoveredPauseRogueCard = -1;
    if (m_state.gameType == GameType::ROGUE && !m_state.rogueChosen.empty()) {
        float panelX = L.panel.x;
        float panelY = L.panel.y;
        float panelH = L.panel.h;

        // Badge grid positioning (must match rendering in PauseEndOverlays.cpp)
        float titleH = 22.0f;
        float titleY = panelY + panelH - titleH - 165.0f;

        float badgeStartY = titleY - 80.0f;  // More space below title
        float badgeBaseW = 50.0f;
        float badgeBaseH = 50.0f;
        float badgeGap = 10.0f;  // 10px spacing between columns
        float badgeRowGap = 80.0f;  // Increased spacing between rows
        float badgesPerRow = 10;  // 10 columns for 2 rows layout

        // Calculate starting X
        float totalWidth = badgesPerRow * badgeBaseW + (badgesPerRow - 1) * badgeGap;
        float badgeStartX = panelX + (L.panel.w - totalWidth) * 0.5f;

        int rowIdx = 0;
        int colIdx = 0;
        int cardCount = 0;

        for (size_t i = 0; i < m_state.rogueChosen.size() && cardCount < 20; ++i) {
            // Badge position (fixed, doesn't move on hover)
            float badgeX = badgeStartX + colIdx * (badgeBaseW + badgeGap);
            float badgeY = badgeStartY - rowIdx * badgeRowGap;  // Use badgeRowGap for more spacing
            float badgeW = badgeBaseW;
            float badgeH = badgeBaseH;

            // Check hit
            if (px >= badgeX && px <= badgeX + badgeW &&
                py >= badgeY && py <= badgeY + badgeH) {
                m_state.hoveredPauseRogueCard = (int)i;
                break;
            }

            colIdx++;
            if (colIdx >= (int)badgesPerRow) {
                colIdx = 0;
                rowIdx++;
            }
            cardCount++;
        }
    }

    if (click) {
        // Left button: Restart
        if (L.leftBtn.contains(px, py)) {
            m_audio.playSfx("ui/ui_confirm");
            init();
            return true;
        }
        // Right button: Back to Menu
        if (L.rightBtn.contains(px, py)) {
            m_audio.playSfx("ui/ui_back");
            m_state.mode = GameMode::MENU;
            m_state.currentMenuScreen = MenuScreen::MAIN;
            m_state.showInstructions = false;
            return true;
        }
    }

    // Update prev trackers (early return)
    m_prevMode = m_state.mode;
    m_prevGameType = m_state.gameType;

    // Avoid long-running loops while paused.
    m_audio.setSfxLoopEnabled("endless/endless_warning_loop", false, 0.12f);
    return true;
}

} // namespace game


