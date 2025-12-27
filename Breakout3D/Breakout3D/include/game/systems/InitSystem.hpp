#pragma once
#include "game/GameState.hpp"
#include "game/GameConfig.hpp"

namespace game {

class InitSystem {
public:
    // Initialize the game state
    static void initGame(GameState& state, const GameConfig& cfg);
    
    // Generate bricks for a level/wave
    static void generateBricks(GameState& state, const GameConfig& cfg, int waveNumber = 0);
    
    // Spawn incremental bricks (for endless mode)
    static void spawnIncrementalBricks(GameState& state, const GameConfig& cfg, int count, int waveNumber);
    
    // Reset ball and paddle to starting position
    static void resetBallAndPaddle(GameState& state, const GameConfig& cfg);
    
    // Check if any bricks are alive
    static bool anyBricksAlive(const GameState& state);
};

} // namespace game


