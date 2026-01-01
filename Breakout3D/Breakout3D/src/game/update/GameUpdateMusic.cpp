// GameUpdateMusic.cpp
/**
 * @file GameUpdateMusic.cpp
 * @brief Comutador simples de “grupo de música”.
 *
 * Ideia:
 *  - Evitar reiniciar a mesma música repetidamente (comparando com m_currentMusicGroup).
 *  - Quando muda, pede ao sistema de áudio para tocar o novo grupo com fade.
 */
 #include "game/Game.hpp"

 namespace game {
 
 void Game::setMusic(const std::string& group, float fadeSeconds) {
     // Se for a mesma música/grupo, não faz nada (evita re-trigger).
     if (group == m_currentMusicGroup) return;
 
     m_currentMusicGroup = group;
     m_audio.playMusic(group, fadeSeconds);
 }
 
 } // namespace game
 