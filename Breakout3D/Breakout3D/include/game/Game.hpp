#pragma once
#include "engine/Window.hpp"
#include "engine/Time.hpp"
#include "engine/Renderer.hpp"
#include "engine/Input.hpp"
#include "game/GameConfig.hpp"
#include "game/GameState.hpp"

// forward declare (não precisa incluir o header aqui)
namespace game { struct GameAssets; }

namespace game {

class Game {
public:
    Game(engine::Window& window, engine::Time& time, engine::Renderer& renderer, GameAssets& assets);

    void init();
    void update(const engine::Input& input);
    void render();

private:
    engine::Window& m_window;
    engine::Time& m_time;
    engine::Renderer& m_renderer;
    GameAssets& m_assets;

    GameConfig m_cfg;
    GameState m_state;
};

} // namespace game
