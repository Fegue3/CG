#include "game/systems/PowerUpSystem.hpp"
#include "game/entities/Ball.hpp"
#include "game/entities/PowerUp.hpp"
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
    if (r2 < 15)       p.type = PowerUpType::EXTRA_LIFE; // 15%
    else if (r2 < 43)  p.type = PowerUpType::EXPAND;     // 28%
    else if (r2 < 71)  p.type = PowerUpType::SLOW;       // 28%
    else               p.type = PowerUpType::EXTRA_BALL; // 29%
    
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
    }
}

void PowerUpSystem::updatePowerUps(GameState& state, const GameConfig& cfg, float dt) {
    glm::vec3 currentPaddleSize = cfg.paddleSize;
    if (state.expandTimer > 0.0f) {
        currentPaddleSize.x *= cfg.expandScaleFactor;
    }
    
    float halfX = currentPaddleSize.x * 0.5f;
    float halfZ = currentPaddleSize.z * 0.5f;
    
    for (size_t i = 0; i < state.powerups.size(); ) {
        PowerUp& p = state.powerups[i];
        p.pos.z += cfg.powerUpDropSpeed * dt;

        // Collision with paddle
        if (std::abs(p.pos.x - state.paddlePos.x) < halfX + cfg.ballRadius &&
            std::abs(p.pos.z - state.paddlePos.z) < halfZ + cfg.ballRadius) {
            
            // Endless scoring on pickup:
            // good powerups add points (into the streak bank),
            // bad powerups (currently SLOW/snail) subtract points.
            if (state.gameType == GameType::ENDLESS) {
                auto addStreak = [&](int pts) {
                    if (pts <= 0) return;
                    state.endlessStreakPoints += pts;
                    state.endlessStreakIdleTimer = 0.0f;
                    state.endlessStreakBanking = false;
                    state.endlessStreakBankTimer = 0.0f;
                };
                auto applyPenalty = [&](int pts) {
                    if (pts <= 0) return;
                    // take from unbanked streak first, then from score
                    int take = std::min(state.endlessStreakPoints, pts);
                    state.endlessStreakPoints -= take;
                    pts -= take;
                    if (pts > 0) {
                        state.score = std::max(0, state.score - pts);
                    }
                    state.endlessStreakIdleTimer = 0.0f;
                    state.endlessStreakBanking = false;
                    state.endlessStreakBankTimer = 0.0f;
                };

                if (p.type == PowerUpType::SLOW) {
                    applyPenalty(300);
                } else if (p.type == PowerUpType::EXTRA_LIFE) {
                    addStreak(500);
                } else if (p.type == PowerUpType::EXPAND) {
                    addStreak(250);
                } else if (p.type == PowerUpType::EXTRA_BALL) {
                    addStreak(350);
                }
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


