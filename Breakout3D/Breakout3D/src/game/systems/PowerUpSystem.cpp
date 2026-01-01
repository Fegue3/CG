/**
 * @file PowerUpSystem.cpp
 * @brief Spawn / pickup / efeitos de powerups.
 *
 * Regras:
 *  - Rogue: não existe pool aleatória; só spawna powerups que estão no rogueDropDeck (cartas escolhidas).
 *  - Outros modos: spawn random com distribuição “weighted”.
 *  - Pickup:
 *      - Endless/Rogue: pontos positivos/penalidades vão para o streak bank (pode ir negativo).
 *      - Normal: score direto (sem popups de penalidade).
 *  - Efeitos:
 *      - EXTRA_BALL: spawna 3 bolas com ângulos (-30/0/+30)
 *      - FIREBALL: troca para um único projétil fireball attached (one-shot)
 *      - SHIELD: duração pode ser multiplicada por rogueShieldDurationMult
 */
 #include "game/systems/PowerUpSystem.hpp"
 #include "game/entities/Ball.hpp"
 #include "game/entities/PowerUp.hpp"
 #include "game/systems/PhysicsSystem.hpp"
 #include "game/rogue/RogueCards.hpp"
 #include <cstdlib>
 #include <ctime>
 #include <cmath>
 
 namespace game {
 
 void PowerUpSystem::spawnPowerUp(GameState& state, const glm::vec3& pos, float chance) {
     // Rogue: deck only
     if (state.gameType == GameType::ROGUE) {
         if (state.rogueDropDeck.empty()) return;
 
         float r = (float)rand() / (float)RAND_MAX;
         if (r > chance) return;
 
         PowerUp p;
         p.pos = pos;
         p.pos.y = 0.4f;
         p.type = state.rogueDropDeck[(size_t)(rand() % state.rogueDropDeck.size())];
 
         state.powerups.push_back(p);
         state.audioSpawnedPowerups.push_back(p.type);
         return;
     }
 
     // Outros modos: chance global
     float r = (float)rand() / (float)RAND_MAX;
     if (r > chance) return;
 
     PowerUp p;
     p.pos = pos;
     p.pos.y = 0.4f;
 
     int r2 = rand() % 100;
     // Distribuição weighted (ajustada para curses aparecerem mais)
     if (r2 < 22)       p.type = PowerUpType::EXTRA_BALL; // 22%
     else if (r2 < 36)  p.type = PowerUpType::EXPAND;     // 14%
     else if (r2 < 48)  p.type = PowerUpType::SLOW;       // 12%
     else if (r2 < 58)  p.type = PowerUpType::EXTRA_LIFE; // 10%
     else if (r2 < 72)  p.type = PowerUpType::FIREBALL;   // 14%
     else if (r2 < 86)  p.type = PowerUpType::SHIELD;     // 14%
     else if (r2 < 93)  p.type = PowerUpType::REVERSE;    // 7%
     else               p.type = PowerUpType::TINY;       // 7%
 
     state.powerups.push_back(p);
     state.audioSpawnedPowerups.push_back(p.type);
 }
 
 void PowerUpSystem::applyPowerUpEffect(GameState& state, const GameConfig& cfg, PowerUpType type) {
     if (type == PowerUpType::EXTRA_LIFE) {
         state.lives++;
     } else if (type == PowerUpType::EXTRA_BALL) {
         // Spawn 3 bolas a partir da primeira bola ativa (ou do paddle)
         glm::vec3 spawnPos = state.paddlePos + glm::vec3(0, 0, -0.5f);
         if (!state.balls.empty()) {
             spawnPos = state.balls[0].pos;
         }
 
         for (int k = 0; k < 3; k++) {
             Ball nb;
             nb.pos = spawnPos;
 
             float ang = glm::radians(-30.0f + k * 30.0f);
             float sp = cfg.ballSpeed;
             if (state.gameType == GameType::ROGUE) sp *= game::rogue::ballSpeedMult(state);
 
             nb.vel = glm::vec3(std::sin(ang), 0.0f, -std::cos(ang)) * sp;
             nb.attached = false;
             state.balls.push_back(nb);
         }
     } else if (type == PowerUpType::SLOW) {
         state.slowTimer = cfg.powerUpDuration;
     } else if (type == PowerUpType::EXPAND) {
         state.expandTimer = cfg.powerUpDuration;
     } else if (type == PowerUpType::FIREBALL) {
         // One-shot fireball: fica attached para o player apontar.
         state.balls.clear();
         Ball fb;
         fb.alive = true;
         fb.isFireball = true;
 
         PhysicsSystem::resetBallToPaddle(fb, state.paddlePos, cfg);
         state.balls.push_back(fb);
 
         state.fireballTimer = 0.0f; // legacy
     } else if (type == PowerUpType::SHIELD) {
         float dur = cfg.shieldDuration;
         if (state.gameType == GameType::ROGUE) dur *= std::max(0.25f, state.rogueShieldDurationMult);
         state.shieldTimer = dur;
     } else if (type == PowerUpType::REVERSE) {
         state.reverseTimer = cfg.reverseDuration;
     } else if (type == PowerUpType::TINY) {
         state.tinyTimer = cfg.tinyDuration;
     }
 }
 
 void PowerUpSystem::updatePowerUps(GameState& state, const GameConfig& cfg, float dt) {
     // Paddle size para colisão com pickups (NOTA: aqui não aplica basePaddleScaleX do Rogue)
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
 
         // Cai para o paddle (Z aumenta em direção ao player)
         p.pos.z += cfg.powerUpDropSpeed * dt;
 
         // Colisão com paddle (AABB-ish em XZ)
         if (std::abs(p.pos.x - state.paddlePos.x) < halfX + cfg.ballRadius &&
             std::abs(p.pos.z - state.paddlePos.z) < halfZ + cfg.ballRadius) {
 
             // Helpers para score/bank
             auto addGood = [&](int pts) {
                 if (pts <= 0) return;
                 if (state.gameType == GameType::ENDLESS || state.gameType == GameType::ROGUE) {
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
                 if (state.gameType == GameType::ENDLESS || state.gameType == GameType::ROGUE) {
                     state.endlessStreakPoints -= pts;
                     state.endlessStreakNegPoints += pts;
                     state.endlessStreakIdleTimer = 0.0f;
                     state.endlessStreakBanking = false;
                     state.endlessStreakBankTimer = 0.0f;
                 } else {
                     state.score = std::max(0, state.score - pts);
                 }
             };
 
             // Pontos/penalidades por pickup
             if (p.type == PowerUpType::SLOW) {
                 applyPenalty(220);
             } else if (p.type == PowerUpType::EXTRA_LIFE) {
                 addGood(400);
             } else if (p.type == PowerUpType::EXPAND) {
                 addGood(220);
             } else if (p.type == PowerUpType::EXTRA_BALL) {
                 addGood(300);
             } else if (p.type == PowerUpType::FIREBALL) {
                 addGood(250);
             } else if (p.type == PowerUpType::SHIELD) {
                 addGood(250);
             } else if (p.type == PowerUpType::REVERSE) {
                 applyPenalty(250);
             } else if (p.type == PowerUpType::TINY) {
                 applyPenalty(250);
             }
 
             applyPowerUpEffect(state, cfg, p.type);
             state.audioPickedPowerups.push_back(p.type);
             p.alive = false;
         }
 
         // Cleanup
         if (!p.alive || p.pos.z > 20.0f) {
             state.powerups.erase(state.powerups.begin() + i);
         } else {
             i++;
         }
     }
 }
 
 } // namespace game
 