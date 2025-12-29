#include "game/systems/PhysicsSystem.hpp"
#include "game/entities/Ball.hpp"
#include "game/rogue/RogueCards.hpp"
#include <algorithm>
#include <glm/glm.hpp>

namespace game {

void PhysicsSystem::resetBallToPaddle(Ball& ball, const glm::vec3& paddlePos, const GameConfig& cfg) {
    ball.attached = true;
    ball.alive = true;
    ball.vel = glm::vec3(0.0f);
    ball.pos = paddlePos + glm::vec3(
        0.0f, 0.0f,
        -(cfg.paddleSize.z * 0.5f + cfg.ballRadius + 0.15f)
    );
}

void PhysicsSystem::updatePaddle(GameState& state, const GameConfig& cfg, float dir, float dt) {
    float currentPaddleSpeed = cfg.paddleSpeed;
    if (state.gameType == GameType::ROGUE) {
        currentPaddleSpeed *= game::rogue::paddleSpeedMult(state);
    }
    if (state.slowTimer > 0.0f) {
        currentPaddleSpeed *= cfg.slowSpeedFactor;
    }
    
    state.paddlePos.x += dir * currentPaddleSpeed * dt;
    
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
    
    float paddleHalfX = currentPaddleSize.x * 0.5f;
    state.paddlePos.x = std::clamp(
        state.paddlePos.x,
        cfg.arenaMinX + paddleHalfX + (state.gameType == GameType::ROGUE ? state.roguePaddleClampMarginX : 0.0f),
        cfg.arenaMaxX - paddleHalfX - (state.gameType == GameType::ROGUE ? state.roguePaddleClampMarginX : 0.0f)
    );
}

void PhysicsSystem::updateBalls(GameState& state, const GameConfig& cfg, float dt) {
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
        
        if (b.attached) {
            resetBallToPaddle(b, state.paddlePos, cfg);
            i++;
            continue;
        }
        
        b.pos += b.vel * dt;

        // Rogue environment: constant lateral wind, keeps ball speed magnitude stable.
        if (state.gameType == GameType::ROGUE && !b.attached && std::abs(state.rogueWindX) > 1e-4f) {
            float sp = glm::length(glm::vec2(b.vel.x, b.vel.z));
            b.vel.x += state.rogueWindX * dt;
            float sp2 = glm::length(glm::vec2(b.vel.x, b.vel.z));
            if (sp > 1e-4f && sp2 > 1e-4f) {
                float k = sp / sp2;
                b.vel.x *= k;
                b.vel.z *= k;
            }
        }

        // Shield barrier: bounce balls back instead of losing them.
        // Positioned a bit "behind" the paddle (towards arenaMaxZ).
        if (state.shieldTimer > 0.0f) {
            float barrierZ = state.paddlePos.z + cfg.shieldOffsetZ;
            // Keep barrier inside the playable removal zone.
            barrierZ = std::min(barrierZ, 19.0f);

            if (b.vel.z > 0.0f && (b.pos.z + cfg.ballRadius) >= barrierZ) {
                b.pos.z = barrierZ - cfg.ballRadius - 0.002f;
                b.vel.z = -std::abs(b.vel.z);
            }
        }
        
        // Remove balls that have gone too far
        if (b.pos.z - cfg.ballRadius > 20.0f) {
            state.balls.erase(state.balls.begin() + i);
        } else {
            i++;
        }
    }
}

} // namespace game


