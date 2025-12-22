#pragma once
#include <glm/glm.hpp>

namespace game {

struct GameConfig {
    // Arena maior (plano XZ)
    float arenaMinX = -14.0f;
    float arenaMaxX =  14.0f;

    float arenaMinZ = -12.0f; // topo (bricks)
    float arenaMaxZ =  12.0f; // fundo (perde bola)

    // Paddle
    glm::vec3 paddleSize = {3.6f, 0.55f, 0.90f};
    float paddleSpeed = 14.0f;

    // Ball
    float ballRadius = 0.32f;
    float ballSpeed  = 10.5f;
};

} // namespace game
