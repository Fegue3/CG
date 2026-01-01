// Game.cpp (excerto relevante à inicialização de áudio)
/**
 * @file Game.cpp (excerto)
 * @brief Ponto de ligação do Game ao AudioSystem: init opcional + música inicial e música ao começar runs.
 *
 * No construtor:
 *  - Tenta init("assets/audio")
 *  - Se tiver áudio, começa logo com música do menu.
 *
 * Em Game::init():
 *  - Após reset do estado (InitSystem::initGame), troca para a música do modo (ENDLESS/ROGUE/NORMAL)
 *  - Toca stinger de “start-of-run”
 */
 #include "game/Game.hpp"
 #include "game/systems/InitSystem.hpp"
 
 namespace game {
 
 Game::Game(engine::Window& window, engine::Time& time, engine::Renderer& renderer, GameAssets& assets)
     : m_window(window), m_time(time), m_renderer(renderer), m_assets(assets) {
 
     // Áudio é opcional: se falhar init, o jogo continua em silêncio.
     m_audio.init("assets/audio");
 
     if (m_audio.isEnabled()) {
         // Jogo arranca em MENU -> música do menu imediatamente.
         m_audio.playMusic("music_menu_loop", 0.0f);
         m_currentMusicGroup = "music_menu_loop";
     }
 }
 
 void Game::init() {
     InitSystem::initGame(m_state, m_cfg);
 
     // Sempre que começa/recomeça uma run, garantir música correta.
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
             // Rogue começa no overlay de cartas, mas deve tocar act music e não menu.
             setMusicNow("music_rogue_act1_loop", 0.25f);
         } else {
             setMusicNow("music_gameplay_loop", 0.25f);
         }
 
         // Punctuation do início de run.
         m_audio.playStinger("stinger_level_start", +2.0f);
     }
 }
 
 } // namespace game
 