#include "game/systems/CollisionSystem.hpp"
#include "game/entities/Brick.hpp"
#include "game/systems/PowerUpSystem.hpp"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

static float clampf(float v, float a, float b) { 
    return std::max(a, std::min(v, b)); 
}

static bool sphereAabbXZ(const glm::vec3& c, float r, const glm::vec3& bpos, const glm::vec3& bsize) {
    float hx = bsize.x * 0.5f;
    float hz = bsize.z * 0.5f;

    float minX = bpos.x - hx, maxX = bpos.x + hx;
    float minZ = bpos.z - hz, maxZ = bpos.z + hz;

    float cx = clampf(c.x, minX, maxX);
    float cz = clampf(c.z, minZ, maxZ);

    float dx = c.x - cx;
    float dz = c.z - cz;
    return (dx * dx + dz * dz) <= (r * r);
}

namespace game {

void CollisionSystem::handleWorldCollisions(Ball& ball, const GameConfig& cfg) {
    // Left/right walls
    if (ball.pos.x - cfg.ballRadius < cfg.arenaMinX) {
        ball.pos.x = cfg.arenaMinX + cfg.ballRadius;
        ball.vel.x = std::abs(ball.vel.x);
    }
    if (ball.pos.x + cfg.ballRadius > cfg.arenaMaxX) {
        ball.pos.x = cfg.arenaMaxX - cfg.ballRadius;
        ball.vel.x = -std::abs(ball.vel.x);
    }
    // Top wall (front wall in game terms)
    if (ball.pos.z - cfg.ballRadius < cfg.arenaMinZ) {
        ball.pos.z = cfg.arenaMinZ + cfg.ballRadius;
        ball.vel.z = std::abs(ball.vel.z);
    }
}

void CollisionSystem::handlePaddleCollision(Ball& ball, const glm::vec3& paddlePos,
                                            const glm::vec3& paddleSize, const GameConfig& cfg) {
    float halfZ = paddleSize.z * 0.5f;
    float paddleHalfX = paddleSize.x * 0.5f;
    float minX = paddlePos.x - paddleHalfX;
    float maxX = paddlePos.x + paddleHalfX;
    float minZ = paddlePos.z - halfZ;
    float maxZ = paddlePos.z + halfZ;

    float cx = clampf(ball.pos.x, minX, maxX);
    float cz = clampf(ball.pos.z, minZ, maxZ);

    float dx = ball.pos.x - cx;
    float dz = ball.pos.z - cz;

    if (dx*dx + dz*dz <= cfg.ballRadius*cfg.ballRadius) {
        // Hitting the front face
        if (cz == minZ && ball.vel.z > 0.0f) {
            ball.pos.z = minZ - cfg.ballRadius - 0.002f;
            float t = (ball.pos.x - paddlePos.x) / paddleHalfX;
            t = clampf(t, -1.0f, 1.0f);
            float ang = t * glm::radians(60.0f);
            ball.vel.x = std::sin(ang) * cfg.ballSpeed;
            ball.vel.z = -std::cos(ang) * cfg.ballSpeed;
        }
        // Hitting the sides
        else if (cx == minX || cx == maxX) {
            ball.vel.x = -ball.vel.x;
            float sideSign = (ball.pos.x < cx) ? -1.0f : 1.0f;
            ball.pos.x = cx + sideSign * (cfg.ballRadius + 0.002f);
        }
    }
}

bool CollisionSystem::handleBrickCollisions(Ball& ball, GameState& state, const GameConfig& cfg) {
    if (state.brickHitCooldown > 0.0f) {
        return false;
    }

    for (auto& br : state.bricks) {
        if (!br.alive) continue;
        if (sphereAabbXZ(ball.pos, cfg.ballRadius, br.pos, br.size)) {
            br.hp--;
            if (br.hp <= 0) {
                br.alive = false;
                PowerUpSystem::spawnPowerUp(state, br.pos, cfg.powerUpChance);
                
                // Points:
                // - Bricks: higher HP (color) = more points
                // - Endless: small wave bonus so later waves score higher
                auto brickPoints = [&](int hp) -> int {
                    // hp 1..6 (endless can go up to 6)
                    switch (hp) {
                        case 1: return 50;   // green
                        case 2: return 120;  // yellow
                        case 3: return 220;  // blue
                        case 4: return 350;  // purple
                        case 5: return 500;
                        default: return 700; // 6+
                    }
                };

                int baseScore = brickPoints(br.maxHp);
                int waveBonus = (state.gameType == GameType::ENDLESS) ? (state.wave * 25) : 0;
                int pts = baseScore + waveBonus;
                if (state.gameType == GameType::ENDLESS) {
                    // Endless: accumulate into a streak bank (committed later by Game::update)
                    state.endlessStreakPoints += pts;
                    state.endlessStreakIdleTimer = 0.0f;
                    // If we were in the middle of "banking" animation, cancel it (fresh points came in)
                    state.endlessStreakBanking = false;
                    state.endlessStreakBankTimer = 0.0f;
                } else {
                    state.score += pts;
                }
                
                // Count destroyed bricks for endless mode spawning
                if (state.gameType == GameType::ENDLESS) {
                    state.bricksDestroyedThisWave++;
                    if (state.bricksDestroyedThisWave >= 15) {
                        state.pendingSpawnBricks += 12;
                        state.bricksDestroyedThisWave -= 15;
                    }
                }
            }

            glm::vec3 diff = ball.pos - br.pos;
            float ax = std::abs(diff.x) / (br.size.x * 0.5f);
            float az = std::abs(diff.z) / (br.size.z * 0.5f);

            if (ax > az) {
                ball.vel.x = -ball.vel.x;
                float sign = (diff.x >= 0.0f) ? 1.0f : -1.0f;
                ball.pos.x = br.pos.x + sign * (br.size.x * 0.5f + cfg.ballRadius + 0.002f);
            } else {
                ball.vel.z = -ball.vel.z;
                float sign = (diff.z >= 0.0f) ? 1.0f : -1.0f;
                ball.pos.z = br.pos.z + sign * (br.size.z * 0.5f + cfg.ballRadius + 0.002f);
            }
            state.brickHitCooldown = 0.045f;
            return true;
        }
    }
    return false;
}

} // namespace game


