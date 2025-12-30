#include "game/Game.hpp"
#include "game/systems/InitSystem.hpp"

namespace game {

Game::Game(engine::Window& window, engine::Time& time, engine::Renderer& renderer, GameAssets& assets)
    : m_window(window), m_time(time), m_renderer(renderer), m_assets(assets) {
    // Audio is optional: if device init fails, gameplay continues silently.
    m_audio.init("assets/audio");
    if (m_audio.isEnabled()) {
        // Start at menu music immediately (Game starts in MENU state).
        m_audio.playMusic("music_menu_loop", 0.0f);
        m_currentMusicGroup = "music_menu_loop";
    }
}

void Game::init() {
    InitSystem::initGame(m_state, m_cfg);

    // Always switch to the correct music when a run starts/restarts.
    // (Rogue starts in the card overlay, but should still use act music, not menu music.)
    if (m_audio.isEnabled()) {
        auto setMusicNow = [&](const std::string& group, float fadeSeconds) {
            if (group == m_currentMusicGroup) return;
            m_currentMusicGroup = group;
            m_audio.playMusic(group, fadeSeconds);
        };

        if (m_state.gameType == GameType::ENDLESS) {
            m_endlessMusicTier = 0;
            setMusicNow("music_endless_loop", 0.25f);
        } else if (m_state.gameType == GameType::ROGUE) {
            setMusicNow("music_rogue_act1_loop", 0.25f);
        } else {
            setMusicNow("music_gameplay_loop", 0.25f);
        }

        // Start-of-run punctuation.
        m_audio.playStinger("stinger_level_start", +2.0f);
    }
}

} // namespace game


