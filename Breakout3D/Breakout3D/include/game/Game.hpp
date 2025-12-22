#pragma once
#include "engine/Window.hpp"
#include "engine/Time.hpp"
#include "engine/Renderer.hpp"
#include "game/GameConfig.hpp"
#include "game/GameState.hpp"

namespace game {

class Game {
public:
    Game(engine::Window& window, engine::Time& time, engine::Renderer& renderer);

    void init();
    void update();
    void render();

private:
    engine::Window& m_window;
    engine::Time& m_time;
    engine::Renderer& m_renderer;

    GameConfig m_cfg;
    GameState m_state;

    int m_lastF11 = 0;
};

} // namespace game
