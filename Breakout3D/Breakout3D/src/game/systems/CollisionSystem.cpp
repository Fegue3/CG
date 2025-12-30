#include "game/systems/CollisionSystem.hpp"
#include "game/entities/Brick.hpp"
#include "game/systems/PowerUpSystem.hpp"
#include "game/rogue/RogueCards.hpp"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

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

bool CollisionSystem::handleWorldCollisions(Ball& ball, const GameConfig& cfg) {
    bool hit = false;
    // Left/right walls
    if (ball.pos.x - cfg.ballRadius < cfg.arenaMinX) {
        ball.pos.x = cfg.arenaMinX + cfg.ballRadius;
        ball.vel.x = std::abs(ball.vel.x);
        hit = true;
    }
    if (ball.pos.x + cfg.ballRadius > cfg.arenaMaxX) {
        ball.pos.x = cfg.arenaMaxX - cfg.ballRadius;
        ball.vel.x = -std::abs(ball.vel.x);
        hit = true;
    }
    // Top wall (front wall in game terms)
    if (ball.pos.z - cfg.ballRadius < cfg.arenaMinZ) {
        ball.pos.z = cfg.arenaMinZ + cfg.ballRadius;
        ball.vel.z = std::abs(ball.vel.z);
        hit = true;
    }
    return hit;
}

bool CollisionSystem::handlePaddleCollision(Ball& ball, const GameState& state, const glm::vec3& paddlePos,
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
        // Robust resolution: pick the dominant penetration axis (XZ only).
        float ax = std::abs(dx);
        float az = std::abs(dz);
        float speed = glm::length(glm::vec2(ball.vel.x, ball.vel.z));
        if (speed < 1e-4f) speed = cfg.ballSpeed;

        // Prefer Z resolution unless X penetration is clearly larger.
        bool resolveX = (ax > az * 1.05f);

        if (resolveX) {
            // Side faces (left/right) — bounce X.
            ball.vel.x = -ball.vel.x;
            float sideSign = (ball.pos.x < cx) ? -1.0f : 1.0f;
            ball.pos.x = cx + sideSign * (cfg.ballRadius + 0.002f);
        } else {
            // Front/back faces — bounce Z and add angle based on hit position.
            bool frontSide = (ball.pos.z < paddlePos.z);

            // Rogue environment: sticky paddle only when hitting the front face while moving toward it.
            if (frontSide && ball.vel.z > 0.0f &&
                state.gameType == GameType::ROGUE && state.rogueStickyPaddle && !ball.isFireball) {
                ball.attached = true;
                ball.vel = glm::vec3(0.0f);
                ball.pos.z = minZ - cfg.ballRadius - 0.002f;
                return true;
            }

            float t = (ball.pos.x - paddlePos.x) / std::max(0.001f, paddleHalfX);
            t = clampf(t, -1.0f, 1.0f);
            float ang = t * glm::radians(60.0f);
            ball.vel.x = std::sin(ang) * speed;
            ball.vel.z = (frontSide ? -std::cos(ang) : std::cos(ang)) * speed;

            ball.pos.z = (frontSide ? (minZ - cfg.ballRadius - 0.002f) : (maxZ + cfg.ballRadius + 0.002f));
        }
        return true;
    }
    return false;
}

CollisionSystem::BrickCollisionInfo CollisionSystem::handleBrickCollisions(Ball& ball, GameState& state, const GameConfig& cfg) {
    BrickCollisionInfo info;
    if (state.brickHitCooldown > 0.0f) {
        return info;
    }

    for (auto& br : state.bricks) {
        if (!br.alive) continue;
        if (sphereAabbXZ(ball.pos, cfg.ballRadius, br.pos, br.size)) {
            info.hit = true;
            const bool fireballActive = ball.isFireball;
            info.fireball = fireballActive;
            info.hpBefore = br.hp;
            info.maxHp = br.maxHp;

            // Scoring helper
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

            auto awardBrickPoints = [&](int maxHp, bool immediateScore) -> int {
                int baseScore = brickPoints(maxHp);
                int waveBonus = (state.gameType == GameType::ENDLESS || state.gameType == GameType::ROGUE) ? (state.wave * 25) : 0;
                int pts = baseScore + waveBonus;
                if (state.gameType == GameType::ROGUE) {
                    pts = (int)std::round((float)pts * state.rogueBrickPointsMult);
                }

                if (state.gameType == GameType::ENDLESS || state.gameType == GameType::ROGUE) {
                    if (immediateScore) {
                        // Meteor/fireball: push points directly into the visible score.
                        state.score += pts;
                        return pts;
                    }
                    // Endless/Rogue: accumulate into a streak bank (committed later by Game::update)
                    state.endlessStreakPoints += pts;
                    state.endlessStreakPosPoints += pts;
                    state.endlessStreakIdleTimer = 0.0f;
                    // If we were in the middle of "banking" animation, cancel it (fresh points came in)
                    state.endlessStreakBanking = false;
                    state.endlessStreakBankTimer = 0.0f;

                    if (state.gameType == GameType::ENDLESS) {
                        // Count destroyed bricks for endless mode spawning
                        state.bricksDestroyedThisWave++;
                        // Softer ramp: early game requires MORE bricks (easier),
                        // then gradually returns to the classic 15 as time goes on.
                        const float t = state.endlessElapsedTime;
                        float u = (t <= 120.0f) ? 0.0f : std::min(1.0f, (t - 120.0f) / 480.0f); // 0..1 from 2min to 10min
                        int required = (int)std::round(22.0f - (7.0f * u)); // 22 -> 15
                        if (required < 15) required = 15;
                        if (required > 22) required = 22;

                        if (state.bricksDestroyedThisWave >= required) {
                            state.pendingSpawnBricks += 12;
                            state.bricksDestroyedThisWave -= required;
                        }
                    }
                } else {
                    state.score += pts;
                }
                return pts;
            };

            auto killBrick = [&](Brick& b, bool allowPowerupDrop) -> int {
                if (!b.alive) return 0;
                b.alive = false;
                info.bricksKilled += 1;

                // Track where the last destroyed brick was (for anchoring VFX).
                state.lastBrickDestroyedValid = true;
                state.lastBrickDestroyedPos = b.pos;

                // Rogue wave progression counts destroyed bricks (quota-based waves).
                if (state.gameType == GameType::ROGUE) {
                    state.rogueBricksBrokenThisWave++;
                }

                if (allowPowerupDrop) {
                    float chance = cfg.powerUpChance;
                    if (state.gameType == GameType::ROGUE) {
                        chance = game::rogue::effectiveDropChance(state, cfg);
                    }
                    PowerUpSystem::spawnPowerUp(state, b.pos, chance);
                }

                return awardBrickPoints(b.maxHp, fireballActive);
            };

            // Fireball hit = instant-kill the hit brick + AoE explosion kills nearby bricks.
            if (fireballActive) {
                int explosionPts = 0;
                // Kill neighbors first so "last brick" detection can be correct.
                float r = cfg.fireballExplosionRadius;
                if (state.gameType == GameType::ROGUE) {
                    r *= std::max(0.25f, state.rogueFireballRadiusMult);
                }
                float r2 = r * r;

                for (auto& other : state.bricks) {
                    if (!other.alive) continue;
                    float dx = other.pos.x - br.pos.x;
                    float dz = other.pos.z - br.pos.z;
                    if ((dx * dx + dz * dz) <= r2) {
                        // Only the directly-hit brick can spawn a drop (prevents powerup floods).
                        bool allowDrop = (&other == &br);
                        int got = killBrick(other, allowDrop);
                        if (got > 0) explosionPts += got;
                    }
                }

                // If this explosion wiped the last brick (normal mode), keep a visual copy for the finisher.
                if (state.gameType == GameType::NORMAL && state.mode == GameMode::PLAYING) {
                    bool anyAlive = false;
                    for (const auto& b : state.bricks) {
                        if (b.alive) { anyAlive = true; break; }
                    }
                    if (!anyAlive) {
                        state.winFinisherHoldBrickValid = true;
                        state.winFinisherHoldBrickPos = br.pos;
                        state.winFinisherHoldBrickSize = br.size;
                        state.winFinisherHoldBrickMaxHp = br.maxHp;
                        state.winFinisherHoldBrickHp = 1;
                    }
                }

                // Spawn a UI explosion ring at the impact point.
                state.fireballExplosions.push_back({ br.pos, 0.0f });
                if (explosionPts > 0) {
                    // Show immediate +PTS popup near the score HUD (Endless only).
                    // Normal mode keeps score HUD hidden, so skip positive popups there.
                    if (state.gameType == GameType::ENDLESS) {
                        state.scorePopups.push_back({ explosionPts, 0.0f });
                    }
                }

                // "Break" feel: camera shake + debris shards
                state.fireballShakeTimer = cfg.fireballShakeDuration;
                state.fireballShakeAnchorPos = br.pos;
                {
                    auto frand = []() -> float { return (float)rand() / (float)RAND_MAX; };
                    for (int i = 0; i < cfg.fireballShardCount; ++i) {
                        float a = frand() * glm::two_pi<float>();
                        float r = 0.15f + frand() * 0.55f;
                        glm::vec3 p = br.pos + glm::vec3(std::cos(a) * r, 0.12f + frand() * 0.18f, std::sin(a) * r);

                        float dirA = frand() * glm::two_pi<float>();
                        float sp = cfg.fireballShardSpeed * (0.65f + frand() * 0.55f);
                        glm::vec3 v(std::cos(dirA) * sp, cfg.fireballShardUp * (0.65f + frand() * 0.55f), std::sin(dirA) * sp);

                        GameState::FireballShard s;
                        s.pos = p;
                        s.vel = v;
                        s.t = 0.0f;
                        state.fireballShards.push_back(s);
                    }
                }

                // One-shot: delete the fireball ball (no bounce).
                ball.alive = false;
                ball.isFireball = false;
                ball.vel = glm::vec3(0.0f);
                state.pendingRespawnAfterFireball = true;
                state.brickHitCooldown = 0.045f;
                info.broke = (info.bricksKilled > 0);
                info.hpAfter = 0;
                return info;
            } else {
                // Normal hit: damage 1
                int dmg = 1;
                if (state.gameType == GameType::ROGUE) {
                    dmg += std::max(0, state.rogueBrickDamageBonus);
                }
                br.hp -= dmg;
                if (br.hp <= 0) {
                    // If this was the LAST brick (normal mode), keep a visual copy for the brief slow-down
                    // so the player sees it "break" exactly when the finisher burst starts.
                    if (state.gameType == GameType::NORMAL && state.mode == GameMode::PLAYING) {
                        bool anyOtherAlive = false;
                        for (const auto& other : state.bricks) {
                            if (&other == &br) continue;
                            if (other.alive) { anyOtherAlive = true; break; }
                        }
                        if (!anyOtherAlive) {
                            state.winFinisherHoldBrickValid = true;
                            state.winFinisherHoldBrickPos = br.pos;
                            state.winFinisherHoldBrickSize = br.size;
                            state.winFinisherHoldBrickMaxHp = br.maxHp;
                            state.winFinisherHoldBrickHp = 1; // show the "about to break" look
                        }
                    }
                    (void)killBrick(br, true);
                    info.broke = true;
                }
                info.hpAfter = std::max(0, br.hp);
                if (!info.broke) {
                    info.damaged = true;
                    if (info.hpBefore > 1 && info.hpAfter == 1) info.cracked = true;
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
            return info;
        }
    }
    return info;
}

} // namespace game


