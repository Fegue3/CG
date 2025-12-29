#include "game/systems/InitSystem.hpp"
#include "game/entities/Brick.hpp"
#include "game/entities/Ball.hpp"
#include "game/systems/PhysicsSystem.hpp"
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <fstream>
#include <string>

namespace game {

// Weighted base HP selection for "random" bricks:
// 1=green (most common) -> 2=yellow -> 3=blue -> 4=purple (rarest)
static int pickWeightedBaseHp_1to4() {
    int r = rand() % 100;
    if (r < 30) return 1;      // 30%  (green)
    if (r < 55) return 2;      // 25%  (yellow)
    if (r < 80) return 3;      // 25%  (blue)
    return 4;                  // 20%  (purple)
}

static std::string endlessBestScorePath() {
    const char* home = std::getenv("HOME");
    if (home && *home) {
        return std::string(home) + "/.breakout3d_endless_best.txt";
    }
    return "breakout3d_endless_best.txt";
}

static int loadEndlessBestScore() {
    std::ifstream f(endlessBestScorePath());
    int best = 0;
    if (f >> best) return std::max(0, best);
    return 0;
}

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
    state.endlessBestScore = loadEndlessBestScore();
    state.endlessStreakPoints = 0;
    state.endlessStreakPosPoints = 0;
    state.endlessStreakNegPoints = 0;
    state.endlessStreakIdleTimer = 0.0f;
    state.endlessStreakBanking = false;
    state.endlessStreakBankTimer = 0.0f;
    state.endlessDangerActive = false;
    state.endlessDangerTimer = 0.0f;
    state.endlessElapsedTime = 0.0f;

    // Clear transient finisher / debug state.
    state.winFinisherActive = false;
    state.winFinisherTimer = 0.0f;
    state.winFinisherRealTimer = 0.0f;
    state.lastBrickDestroyedValid = false;
    state.winFinisherAnchorValid = false;
    state.winFinisherHoldBrickValid = false;
    
    // For endless mode, wave is already set before calling init()
    // For normal mode, reset wave to 1
    if (state.gameType == GameType::NORMAL) {
        state.wave = 1;
    }

    resetBallAndPaddle(state, cfg);

    state.powerups.clear();
    state.expandTimer = 0.0f;
    state.slowTimer = 0.0f;
    state.fireballTimer = 0.0f;
    state.shieldTimer = 0.0f;
    state.reverseTimer = 0.0f;
    state.tinyTimer = 0.0f;
    state.fireballExplosions.clear();
    state.fireballShards.clear();
    state.fireballShakeTimer = 0.0f;
    state.fireballShakeAnchorPos = glm::vec3(0.0f);
    state.scorePopups.clear();
    state.pendingRespawnAfterFireball = false;
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
    int rows = 9;  // Default for normal mode (may be overridden below)

    // Adjust difficulty for endless mode
    if (waveNumber > 0) {
        rows = 9 + (waveNumber / 2); // Add rows every 2 waves
    }
    // Normal mode preset layout:
    // - 3 rows green (1 HP)
    // - 1 row yellow (2 HP)
    // - 1 row blue (3 HP)
    // - 1 row purple (4 HP)
    if (waveNumber == 0) {
        rows = 6;
    }

    glm::vec3 brickSize(2.95f, 0.7f, 1.30f); 
    float gapX = 0.04f;
    float gapZ = 0.03f;

    float startZ = cfg.arenaMinZ + 0.85f;
    float totalW = cols * brickSize.x + (cols - 1) * gapX;
    float leftX  = -totalW * 0.5f + brickSize.x * 0.5f;

    // Test feature: spawn just one brick (useful for debugging / finisher testing).
    if (state.testOneBrick) {
        Brick b;
        b.size = brickSize;
        b.alive = true;
        b.pos.x = 0.0f;
        b.pos.y = 0.0f;
        b.pos.z = startZ + 4.0f * (brickSize.z + gapZ);
        b.maxHp = b.hp = 1;
        state.bricks.push_back(b);
        return;
    }

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            Brick b;
            b.size = brickSize;
            b.alive = true;

            b.pos.x = leftX + c * (brickSize.x + gapX);
            b.pos.y = 0.0f;
            b.pos.z = startZ + r * (brickSize.z + gapZ);

            // Normal mode: single preset level (from closest-to-paddle -> farthest)
            // 3 rows green (1 HP), then 1 row yellow (2 HP),
            // then 1 row blue (3 HP), then 1 row purple (4 HP).
            if (waveNumber == 0) {
                int hp = 1;
                // NOTE: r grows towards the paddle (increasing Z).
                // So the "first" rows the player sees (near the paddle) are the highest r.
                if (r >= 3) hp = 1;         // green (3 rows closest): r = 3,4,5
                else if (r == 2) hp = 2;    // yellow (next 1 row)
                else if (r == 1) hp = 3;    // blue (next 1 row)
                else hp = 4;                // purple (1 row farthest): r = 0

                b.maxHp = b.hp = hp;
                state.bricks.push_back(b);
                continue;
            }

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
                    b.maxHp = b.hp = pickWeightedBaseHp_1to4();
                } else {
                    int hpBonus = waveNumber / 5;
                    int baseHp = pickWeightedBaseHp_1to4();
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

        int baseHp = pickWeightedBaseHp_1to4();
        int difficultyBonus = (rowGlobal / 20);
        b.maxHp = b.hp = std::min(6, baseHp + difficultyBonus);

        state.bricks.push_back(b);
    }

    state.endlessRowsSpawned = totalRowsBefore + rowsToInsert;
}

} // namespace game


