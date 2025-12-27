#pragma once
#include "game/GameState.hpp"
#include "game/GameConfig.hpp"
#include <glm/glm.hpp>

namespace game {

class PowerUpSystem {
public:
    // Spawn a power-up at the given position with a chance
    static void spawnPowerUp(GameState& state, const glm::vec3& pos, float chance);
    
    // Update power-ups (falling, collection, effects)
    static void updatePowerUps(GameState& state, const GameConfig& cfg, float dt);
    
    // Apply power-up effect when collected
    static void applyPowerUpEffect(GameState& state, const GameConfig& cfg, PowerUpType type);
};

} // namespace game


