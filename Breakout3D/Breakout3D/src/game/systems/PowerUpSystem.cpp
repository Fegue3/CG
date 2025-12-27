#include "game/systems/PowerUpSystem.hpp"
#include "game/entities/Ball.hpp"
#include "game/entities/PowerUp.hpp"
#include "game/systems/PhysicsSystem.hpp"
#include <cstdlib>
#include <ctime>
#include <cmath>

namespace game {

void PowerUpSystem::spawnPowerUp(GameState& state, const glm::vec3& pos, float chance) {
    float r = (float)rand() / (float)RAND_MAX;
    if (r > chance) return;

    PowerUp p;
    p.pos = pos;
    p.pos.y = 0.4f; // Lift slightly off the ground initially

    int r2 = rand() % 100;
    // Weighted distribution (feel free to tune):
    // - Good: extra ball, expand, shield, fireball, extra life
    // - Bad/curses: reverse, tiny
    // NOTE: bumped curse rates a bit so they actually show up in real play.
    if (r2 < 22)       p.type = PowerUpType::EXTRA_BALL; // 22%
    else if (r2 < 36)  p.type = PowerUpType::EXPAND;     // 14%
    else if (r2 < 48)  p.type = PowerUpType::SLOW;       // 12%
    else if (r2 < 58)  p.type = PowerUpType::EXTRA_LIFE; // 10%
    else if (r2 < 72)  p.type = PowerUpType::FIREBALL;   // 14%
    else if (r2 < 86)  p.type = PowerUpType::SHIELD;     // 14%
    else if (r2 < 93)  p.type = PowerUpType::REVERSE;    // 7%
    else               p.type = PowerUpType::TINY;       // 7%
    
    state.powerups.push_back(p);
}

void PowerUpSystem::applyPowerUpEffect(GameState& state, const GameConfig& cfg, PowerUpType type) {
    if (type == PowerUpType::EXTRA_LIFE) {
        state.lives++;
    } else if (type == PowerUpType::EXTRA_BALL) {
        // Spawn new balls from the first active ball, or paddle if none
        glm::vec3 spawnPos = state.paddlePos + glm::vec3(0, 0, -0.5f);
        if (!state.balls.empty()) {
            spawnPos = state.balls[0].pos;
        }
        for (int k = 0; k < 3; k++) {
            Ball nb;
            nb.pos = spawnPos;
            float ang = glm::radians(-30.0f + k * 30.0f);
            nb.vel = glm::vec3(std::sin(ang), 0.0f, -std::cos(ang)) * cfg.ballSpeed;
            nb.attached = false;
            state.balls.push_back(nb);
        }
    } else if (type == PowerUpType::SLOW) {
        state.slowTimer = cfg.powerUpDuration;
    } else if (type == PowerUpType::EXPAND) {
        state.expandTimer = cfg.powerUpDuration;
    } else if (type == PowerUpType::FIREBALL) {
        // One-shot fireball: load onto paddle immediately so player aims the shot.
        // Simplify by turning it into a single projectile ball.
        state.balls.clear();
        Ball fb;
        fb.alive = true;
        fb.isFireball = true;
        PhysicsSystem::resetBallToPaddle(fb, state.paddlePos, cfg);
        state.balls.push_back(fb);
        state.fireballTimer = 0.0f; // (legacy field; no longer used for behavior)
    } else if (type == PowerUpType::SHIELD) {
        state.shieldTimer = cfg.shieldDuration;
    } else if (type == PowerUpType::REVERSE) {
        state.reverseTimer = cfg.reverseDuration;
    } else if (type == PowerUpType::TINY) {
        state.tinyTimer = cfg.tinyDuration;
    }
}

void PowerUpSystem::updatePowerUps(GameState& state, const GameConfig& cfg, float dt) {
    glm::vec3 currentPaddleSize = cfg.paddleSize;
    if (state.expandTimer > 0.0f) {
        currentPaddleSize.x *= cfg.expandScaleFactor;
    }
    if (state.tinyTimer > 0.0f) {
        currentPaddleSize.x *= cfg.tinyScaleFactor;
    }
    
    float halfX = currentPaddleSize.x * 0.5f;
    float halfZ = currentPaddleSize.z * 0.5f;
    
    for (size_t i = 0; i < state.powerups.size(); ) {
        PowerUp& p = state.powerups[i];
        p.pos.z += cfg.powerUpDropSpeed * dt;

        // Collision with paddle
        if (std::abs(p.pos.x - state.paddlePos.x) < halfX + cfg.ballRadius &&
            std::abs(p.pos.z - state.paddlePos.z) < halfZ + cfg.ballRadius) {
            
            // Pickup scoring/penalties:
            // - Endless: good pickups add to streak bank; bad pickups subtract from (streak first, then score)
            // - Normal: apply directly to score
            auto addGood = [&](int pts) {
                if (pts <= 0) return;
                if (state.gameType == GameType::ENDLESS) {
                    state.endlessStreakPoints += pts;
                    state.endlessStreakPosPoints += pts;
                    state.endlessStreakIdleTimer = 0.0f;
                    state.endlessStreakBanking = false;
                    state.endlessStreakBankTimer = 0.0f;
                } else {
                    state.score += pts;
                }
            };

            auto applyPenalty = [&](int pts) {
                if (pts <= 0) return;
                if (state.gameType == GameType::ENDLESS) {
                    // User request: penalties should behave EXACTLY like the white streak bank:
                    // affect the BANK now (can go negative), and only affect real score on commit.
                    state.endlessStreakPoints -= pts;
                    state.endlessStreakNegPoints += pts;
                    state.endlessStreakIdleTimer = 0.0f;
                    state.endlessStreakBanking = false;
                    state.endlessStreakBankTimer = 0.0f;
                } else {
                    state.score = std::max(0, state.score - pts);
                    // Normal mode: don't show score popups.
                }
            };

            if (p.type == PowerUpType::SLOW) {
                applyPenalty(300);
            } else if (p.type == PowerUpType::EXTRA_LIFE) {
                addGood(500);
            } else if (p.type == PowerUpType::EXPAND) {
                addGood(250);
            } else if (p.type == PowerUpType::EXTRA_BALL) {
                addGood(350);
            } else if (p.type == PowerUpType::FIREBALL) {
                addGood(400);
            } else if (p.type == PowerUpType::SHIELD) {
                addGood(300);
            } else if (p.type == PowerUpType::REVERSE) {
                applyPenalty(250);
            } else if (p.type == PowerUpType::TINY) {
                applyPenalty(250);
            }

            applyPowerUpEffect(state, cfg, p.type);
            p.alive = false;
        }

        if (!p.alive || p.pos.z > 20.0f) {
            state.powerups.erase(state.powerups.begin() + i);
        } else {
            i++;
        }
    }
}

} // namespace game


