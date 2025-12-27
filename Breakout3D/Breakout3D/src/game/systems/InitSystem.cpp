#include "game/systems/InitSystem.hpp"
#include "game/entities/Brick.hpp"
#include "game/entities/Ball.hpp"
#include "game/systems/PhysicsSystem.hpp"
#include <cstdlib>
#include <ctime>
#include <algorithm>

namespace game {

bool InitSystem::anyBricksAlive(const GameState& state) {
    for (const auto& b : state.bricks) {
        if (b.alive) return true;
    }
    return false;
}

void InitSystem::resetBallAndPaddle(GameState& state, const GameConfig& cfg) {
    state.paddlePos = glm::vec3(
        0.0f, 0.0f,
        cfg.arenaMaxZ - (cfg.paddleSize.z * 0.5f) - 0.25f
    );
    
    state.balls.clear();
    Ball firstBall;
    PhysicsSystem::resetBallToPaddle(firstBall, state.paddlePos, cfg);
    state.balls.push_back(firstBall);
}

void InitSystem::initGame(GameState& state, const GameConfig& cfg) {
    srand(static_cast<unsigned int>(time(nullptr)));
    state.mode = GameMode::PLAYING;
    state.lives = 3;
    state.bricksDestroyedThisWave = 0;
    state.endlessRowsSpawned = 0;
    state.score = 0;
    state.endlessDangerActive = false;
    state.endlessDangerTimer = 0.0f;
    
    // For endless mode, wave is already set before calling init()
    // For normal mode, reset wave to 1
    if (state.gameType == GameType::NORMAL) {
        state.wave = 1;
    }

    resetBallAndPaddle(state, cfg);

    state.powerups.clear();
    state.expandTimer = 0.0f;
    state.slowTimer = 0.0f;
    state.brickHitCooldown = 0.0f;

    // Generate bricks (0 for normal, waveNumber for endless)
    int waveToGenerate = (state.gameType == GameType::ENDLESS) ? state.wave : 0;
    int rowsInitial = 9;
    if (state.gameType == GameType::ENDLESS) {
        rowsInitial = 9 + (state.wave / 2);
    }
    generateBricks(state, cfg, waveToGenerate);
    if (state.gameType == GameType::ENDLESS) {
        state.endlessRowsSpawned = rowsInitial;
    }
}

void InitSystem::generateBricks(GameState& state, const GameConfig& cfg, int waveNumber) {
    state.bricks.clear();
    
    const int cols = 12;
    int rows = 9;  // Default for normal mode

    // Adjust difficulty for endless mode
    if (waveNumber > 0) {
        rows = 9 + (waveNumber / 2); // Add rows every 2 waves
    }

    glm::vec3 brickSize(2.95f, 0.7f, 1.30f); 
    float gapX = 0.04f;
    float gapZ = 0.03f;

    float startZ = cfg.arenaMinZ + 0.85f;
    float totalW = cols * brickSize.x + (cols - 1) * gapX;
    float leftX  = -totalW * 0.5f + brickSize.x * 0.5f;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            Brick b;
            b.size = brickSize;
            b.alive = true;

            b.pos.x = leftX + c * (brickSize.x + gapX);
            b.pos.y = 0.0f;
            b.pos.z = startZ + r * (brickSize.z + gapZ);

            // Split wall into Front (closer to paddle) and Back (further away)
            bool isFrontHalf = (r >= rows / 2);

            if (isFrontHalf) {
                // Front half: Easier bricks
                int r2 = rand() % 100;
                if (r2 < 45)      b.maxHp = b.hp = 1;
                else if (r2 < 85) b.maxHp = b.hp = 2;
                else              b.maxHp = b.hp = 3;
            } else {
                // Back half: Random bricks
                if (waveNumber == 0) {
                    b.maxHp = b.hp = 1 + (rand() % 4);
                } else {
                    int hpBonus = waveNumber / 5;
                    int baseHp = 1 + (rand() % 4);
                    b.maxHp = b.hp = std::min(6, baseHp + hpBonus);
                }
            }

            state.bricks.push_back(b);
        }
    }
}

void InitSystem::spawnIncrementalBricks(GameState& state, const GameConfig& cfg, int count, int waveNumber) {
    glm::vec3 brickSize(2.95f, 0.7f, 1.30f);
    float gapX = 0.04f;
    float gapZ = 0.03f;

    const int cols = 12;
    float totalW = cols * brickSize.x + (cols - 1) * gapX;
    float leftX = -totalW * 0.5f + brickSize.x * 0.5f;
    float stepZ = brickSize.z + gapZ;
    float topZ = cfg.arenaMinZ + 0.85f;

    int rowsToInsert = (count + cols - 1) / cols;
    float push = rowsToInsert * stepZ;
    
    for (auto& br : state.bricks) {
        if (!br.alive) continue;
        br.pos.z += push;
    }

    int totalRowsBefore = state.endlessRowsSpawned;

    for (int i = 0; i < count; ++i) {
        Brick b;
        b.size = brickSize;
        b.alive = true;

        int col = i % cols;
        int rowLocal = (i / cols);
        int rowGlobal = totalRowsBefore + rowLocal;

        b.pos.x = leftX + col * (brickSize.x + gapX);
        b.pos.y = 0.0f;
        b.pos.z = topZ + rowLocal * stepZ;

        int baseHp = 1 + (rand() % 4);
        int difficultyBonus = (rowGlobal / 20);
        b.maxHp = b.hp = std::min(6, baseHp + difficultyBonus);

        state.bricks.push_back(b);
    }

    state.endlessRowsSpawned = totalRowsBefore + rowsToInsert;
}

} // namespace game


