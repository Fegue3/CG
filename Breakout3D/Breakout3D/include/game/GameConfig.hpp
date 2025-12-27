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
    float powerUpChance = 0.40f; // 40% chance (increased from 25%)
    float powerUpDuration = 7.0f;
    float powerUpVisualScale = 1.15f;
    float slowSpeedFactor = 0.5f;
    float expandScaleFactor = 1.6f;

    // New powerups/curses tuning
    float fireballDuration = 7.0f;
    float fireballExplosionRadius = 4.8f; // in world units (XZ)
    float fireballExplosionFxDuration = 0.45f; // UI ring duration

    float shieldDuration = 7.0f;
    float shieldOffsetZ = 2.4f; // barrier behind the paddle (towards arenaMaxZ)

    float reverseDuration = 5.0f;

    float tinyDuration = 7.0f;
    float tinyScaleFactor = 0.60f;

    int lifeLossPenalty = 250; // user request: losing a heart = -points
};

} // namespace game
