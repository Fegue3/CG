#include "game/systems/PhysicsSystem.hpp"
#include "game/entities/Ball.hpp"
#include <algorithm>
#include <glm/glm.hpp>

namespace game {

void PhysicsSystem::resetBallToPaddle(Ball& ball, const glm::vec3& paddlePos, const GameConfig& cfg) {
    ball.attached = true;
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
    
    for (size_t i = 0; i < state.balls.size(); ) {
        Ball& b = state.balls[i];
        
        if (b.attached) {
            resetBallToPaddle(b, state.paddlePos, cfg);
            i++;
            continue;
        }
        
        b.pos += b.vel * dt;
        
        // Remove balls that have gone too far
        if (b.pos.z - cfg.ballRadius > 20.0f) {
            state.balls.erase(state.balls.begin() + i);
        } else {
            i++;
        }
    }
}

} // namespace game


