/**
 * @file PhysicsSystem.cpp
 * @brief Atualização física simples: paddle (movimento + clamp) e bolas (integração + vento + shield + remoção).
 *
 * Ideias chave:
 *  - Paddle speed e tamanho são “dinâmicos”: Rogue modifiers + timers (SLOW/EXPAND/TINY).
 *  - Clamp do paddle respeita arena + (Rogue) margem extra para “CENTERED ARENA”.
 *  - Balls: se attached, seguem o paddle; caso contrário, integram posição e aplicam efeitos de ambiente.
 *  - Rogue wind:
 *      - rogueWindX: vento constante (pequeno empurrão lateral) com renormalização de speed.
 *      - rogueRandomWindActive: rajadas aleatórias a cada 0.3s (também renormaliza).
 *  - Shield: cria uma barreira atrás do paddle que reflete bolas em vez de perder.
 *  - Remoção: bola que passa z > 20 (abaixo do campo) é removida.
 */
 #include "game/systems/PhysicsSystem.hpp"
 #include "game/entities/Ball.hpp"
 #include "game/rogue/RogueCards.hpp"
 #include <algorithm>
 #include <glm/glm.hpp>
 
 namespace game {
 
 void PhysicsSystem::resetBallToPaddle(Ball& ball, const glm::vec3& paddlePos, const GameConfig& cfg) {
     // "Attached": cola a bola ao paddle (pronta para lançar)
     ball.attached = true;
     ball.alive = true;
     ball.vel = glm::vec3(0.0f);
 
     // Posiciona ligeiramente à frente do paddle (para o lado dos bricks, -Z)
     ball.pos = paddlePos + glm::vec3(
         0.0f, 0.0f,
         -(cfg.paddleSize.z * 0.5f + cfg.ballRadius + 0.15f)
     );
 }
 
 void PhysicsSystem::updatePaddle(GameState& state, const GameConfig& cfg, float dir, float dt) {
     // Velocidade base + modificadores
     float currentPaddleSpeed = cfg.paddleSpeed;
     if (state.gameType == GameType::ROGUE) {
         currentPaddleSpeed *= game::rogue::paddleSpeedMult(state);
     }
     if (state.slowTimer > 0.0f) {
         currentPaddleSpeed *= cfg.slowSpeedFactor;
     }
 
     // Move em X
     state.paddlePos.x += dir * currentPaddleSpeed * dt;
 
     // Tamanho atual do paddle (para clamp correto)
     glm::vec3 currentPaddleSize = cfg.paddleSize;
     if (state.gameType == GameType::ROGUE) {
         currentPaddleSize.x *= game::rogue::basePaddleScaleX(state);
     }
     if (state.expandTimer > 0.0f) {
         currentPaddleSize.x *= cfg.expandScaleFactor;
     }
     if (state.tinyTimer > 0.0f) {
         currentPaddleSize.x *= cfg.tinyScaleFactor;
     }
 
     // Clamp dentro da arena
     float paddleHalfX = currentPaddleSize.x * 0.5f;
     float rogueMargin = (state.gameType == GameType::ROGUE ? state.roguePaddleClampMarginX : 0.0f);
 
     state.paddlePos.x = std::clamp(
         state.paddlePos.x,
         cfg.arenaMinX + paddleHalfX + rogueMargin,
         cfg.arenaMaxX - paddleHalfX - rogueMargin
     );
 }
 
 void PhysicsSystem::updateBalls(GameState& state, const GameConfig& cfg, float dt) {
     // Recalcula paddle size (para coisas como shield/barreira e attached positioning)
     glm::vec3 currentPaddleSize = cfg.paddleSize;
     if (state.gameType == GameType::ROGUE) {
         currentPaddleSize.x *= game::rogue::basePaddleScaleX(state);
     }
     if (state.expandTimer > 0.0f) {
         currentPaddleSize.x *= cfg.expandScaleFactor;
     }
     if (state.tinyTimer > 0.0f) {
         currentPaddleSize.x *= cfg.tinyScaleFactor;
     }
 
     for (size_t i = 0; i < state.balls.size(); ) {
         Ball& b = state.balls[i];
 
         // Se attached, forçamos a bola a seguir o paddle (sem “descolar”)
         if (b.attached) {
             resetBallToPaddle(b, state.paddlePos, cfg);
             i++;
             continue;
         }
 
         // Integração simples
         b.pos += b.vel * dt;
 
         // ---------------- Rogue: vento constante (lateral) ----------------
         if (state.gameType == GameType::ROGUE && !b.attached && std::abs(state.rogueWindX) > 1e-4f) {
             float sp = glm::length(glm::vec2(b.vel.x, b.vel.z));
 
             // empurra X
             b.vel.x += state.rogueWindX * dt;
 
             // renormaliza para manter magnitude (não acelerar/desacelerar por acidente)
             float sp2 = glm::length(glm::vec2(b.vel.x, b.vel.z));
             if (sp > 1e-4f && sp2 > 1e-4f) {
                 float k = sp / sp2;
                 b.vel.x *= k;
                 b.vel.z *= k;
             }
         }
 
         // ---------------- Rogue: vento caótico (rajadas) ----------------
         if (state.gameType == GameType::ROGUE && !b.attached && state.rogueRandomWindActive) {
             state.rogueRandomWindTimer += dt;
             if (state.rogueRandomWindTimer >= 0.3f) {
                 float windForce = 2.8f + (float)(rand() % 20) * 0.08f; // 2.8 .. 4.4
                 if (rand() % 2 == 0) windForce = -windForce;
 
                 float sp = glm::length(glm::vec2(b.vel.x, b.vel.z));
                 b.vel.x += windForce;
 
                 float sp2 = glm::length(glm::vec2(b.vel.x, b.vel.z));
                 if (sp > 1e-4f && sp2 > 1e-4f) {
                     float k = sp / sp2;
                     b.vel.x *= k;
                     b.vel.z *= k;
                 }
 
                 state.rogueRandomWindTimer = 0.0f;
             }
         }
 
         // ---------------- Shield barrier ----------------
         if (state.shieldTimer > 0.0f) {
             float barrierZ = state.paddlePos.z + cfg.shieldOffsetZ;
             barrierZ = std::min(barrierZ, 19.0f);
 
             // Só reflete se a bola estiver a ir “para trás” (Z positivo)
             if (b.vel.z > 0.0f && (b.pos.z + cfg.ballRadius) >= barrierZ) {
                 b.pos.z = barrierZ - cfg.ballRadius - 0.002f;
                 b.vel.z = -std::abs(b.vel.z);
             }
         }
 
         // Remoção de bolas fora do campo
         if (b.pos.z - cfg.ballRadius > 20.0f) {
             state.balls.erase(state.balls.begin() + i);
         } else {
             i++;
         }
     }
 }
 
 } // namespace game
 