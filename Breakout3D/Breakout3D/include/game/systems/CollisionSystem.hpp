#pragma once
#include "game/GameState.hpp"
#include "game/GameConfig.hpp"
#include "game/entities/Ball.hpp"

namespace game {

class CollisionSystem {
public:
    // Handle all world collisions (walls, boundaries)
    static void handleWorldCollisions(Ball& ball, const GameConfig& cfg);
    
    // Handle paddle collision
    static void handlePaddleCollision(Ball& ball, const glm::vec3& paddlePos, 
                                      const glm::vec3& paddleSize, const GameConfig& cfg);
    
    // Handle brick collisions - returns true if a collision occurred
    static bool handleBrickCollisions(Ball& ball, GameState& state, const GameConfig& cfg);
};

} // namespace game


