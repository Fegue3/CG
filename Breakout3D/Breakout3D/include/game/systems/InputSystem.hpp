#pragma once
#include "game/GameState.hpp"
#include "game/GameConfig.hpp"
#include "engine/Input.hpp"
#include "engine/Window.hpp"
#include <glm/glm.hpp>

namespace game {

class InputSystem {
public:
    // Handle menu input (returns true if menu action was taken)
    static bool handleMenuInput(GameState& state, const engine::Input& input, engine::Window& window);
    
    // Handle game input (paddle movement, ball launch, pause, etc.)
    static void handleGameInput(GameState& state, const engine::Input& input, const GameConfig& cfg, engine::Window& window, float dt);
    
private:
    static bool pointInRectPx(float px, float py, float x, float y, float w, float h);
};

} // namespace game

