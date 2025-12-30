#include "game/Game.hpp"

namespace game {

void Game::setMusic(const std::string& group, float fadeSeconds) {
    if (group == m_currentMusicGroup) return;
    m_currentMusicGroup = group;
    m_audio.playMusic(group, fadeSeconds);
}

} // namespace game


