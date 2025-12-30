#pragma once
#include "game/GameState.hpp"
#include "game/GameConfig.hpp"
#include "game/entities/Ball.hpp"

namespace game {

class CollisionSystem {
public:
    // Handle all world collisions (walls, boundaries)
    static bool handleWorldCollisions(Ball& ball, const GameConfig& cfg);
    
    // Handle paddle collision
    static bool handlePaddleCollision(Ball& ball, const GameState& state, const glm::vec3& paddlePos,
                                      const glm::vec3& paddleSize, const GameConfig& cfg);
    
    struct BrickCollisionInfo {
        bool hit = false;
        bool fireball = false;
        bool damaged = false; // hit but not destroyed
        bool cracked = false; // transitioned into "low hp" feel
        bool broke = false;   // destroyed at least one brick
        int bricksKilled = 0; // fireball can kill multiple
        int hpBefore = 0;
        int hpAfter = 0;
        int maxHp = 0;
    };

    // Handle brick collisions - returns true if a collision occurred
    static BrickCollisionInfo handleBrickCollisions(Ball& ball, GameState& state, const GameConfig& cfg);
};

} // namespace game


