#include "game/systems/PhysicsSystem.hpp"
#include "game/entities/Ball.hpp"
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
    if (state.slowTimer > 0.0f) {
        currentPaddleSpeed *= cfg.slowSpeedFactor;
    }
    
    state.paddlePos.x += dir * currentPaddleSpeed * dt;
    
    glm::vec3 currentPaddleSize = cfg.paddleSize;
    if (state.expandTimer > 0.0f) {
        currentPaddleSize.x *= cfg.expandScaleFactor;
    }
    if (state.tinyTimer > 0.0f) {
        currentPaddleSize.x *= cfg.tinyScaleFactor;
    }
    
    float paddleHalfX = currentPaddleSize.x * 0.5f;
    state.paddlePos.x = std::clamp(
        state.paddlePos.x,
        cfg.arenaMinX + paddleHalfX,
        cfg.arenaMaxX - paddleHalfX
    );
}

void PhysicsSystem::updateBalls(GameState& state, const GameConfig& cfg, float dt) {
    glm::vec3 currentPaddleSize = cfg.paddleSize;
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


