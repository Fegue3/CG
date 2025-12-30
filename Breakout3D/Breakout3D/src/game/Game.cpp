#include "game/Game.hpp"
#include "game/systems/InitSystem.hpp"

namespace game {

Game::Game(engine::Window& window, engine::Time& time, engine::Renderer& renderer, GameAssets& assets)
    : m_window(window), m_time(time), m_renderer(renderer), m_assets(assets) {}

void Game::init() {
    InitSystem::initGame(m_state, m_cfg);
}

} // namespace game


