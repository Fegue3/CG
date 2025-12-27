#pragma once
#include "game/GameState.hpp"
#include "game/GameConfig.hpp"

namespace game {

class PhysicsSystem {
public:
    // Update paddle position based on input
    static void updatePaddle(GameState& state, const GameConfig& cfg, float dir, float dt);
    
    // Update ball positions and velocities
    static void updateBalls(GameState& state, const GameConfig& cfg, float dt);
    
    // Reset ball to paddle position
    static void resetBallToPaddle(Ball& ball, const glm::vec3& paddlePos, const GameConfig& cfg);
};

} // namespace game


