#pragma once
#include <glm/glm.hpp>

namespace game {

struct GameConfig {
    // Arena maior (plano XZ)
    float arenaMinX = -18.0f;
    float arenaMaxX =  18.0f;

    float arenaMinZ = -12.0f; // topo (bricks)
    float arenaMaxZ =  12.0f; // fundo (perde bola)

    // Paddle
    glm::vec3 paddleSize = {3.6f, 0.55f, 0.90f};
    float paddleSpeed = 14.0f;

    // Ball
    float ballRadius = 0.42f;
    float ballSpeed  = 10.5f;

    // Powerups
    float powerUpDropSpeed = 4.5f;
    float powerUpChance = 0.25f; // 25% chance
    float powerUpDuration = 7.0f;
    float powerUpVisualScale = 1.15f;
    float slowSpeedFactor = 0.5f;
    float expandScaleFactor = 1.6f;
};

} // namespace game
