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
    float powerUpChance = 0.35f; // tuned: a bit less spammy
    float powerUpDuration = 7.0f;
    float powerUpVisualScale = 1.15f;
    float slowSpeedFactor = 0.5f;
    float expandScaleFactor = 1.6f;

    // New powerups/curses tuning
    float fireballDuration = 7.0f;
    float fireballExplosionRadius = 4.2f; // tuned: big but not screen-clearing
    float fireballExplosionFxDuration = 0.45f; // UI ring duration
    float fireballFlashMaxAlpha = 0.20f; // full-screen flash on impact
    float fireballShakeDuration = 0.32f;
    float fireballShakeStrength = 0.28f;
    int fireballShardCount = 18;
    float fireballShardLife = 0.55f;
    float fireballShardSpeed = 8.0f;
    float fireballShardUp = 4.5f;
    float fireballShardDrag = 2.2f;

    float shieldDuration = 6.5f;
    float shieldOffsetZ = 2.2f; // barrier behind the paddle (towards arenaMaxZ)

    float reverseDuration = 4.25f; // tuned: less annoying

    float tinyDuration = 6.0f;
    float tinyScaleFactor = 0.60f;

    int lifeLossPenalty = 200; // tuned
};

} // namespace game
