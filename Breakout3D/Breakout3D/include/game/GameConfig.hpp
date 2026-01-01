// GameConfig.hpp
#pragma once
#include <glm/glm.hpp>

namespace game {

/**
 * @file GameConfig.hpp
 * @brief Configuração/tuning do jogo (arena, tamanhos, velocidades e parâmetros de power-ups).
 *
 * É o “site” central de números:
 * - limites da arena (plano XZ)
 * - tamanho/velocidade do paddle e bola
 * - probabilidades e durações de power-ups
 * - parâmetros de efeitos (fireball/shake/flash/shards, etc.)
 *
 * @note
 * Estes valores são defaults; o jogo pode aplicar multiplicadores/alterações via modos/cartas.
 */
struct GameConfig {
    // Arena (plano XZ)
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

    // Powerups (geral)
    float powerUpDropSpeed = 4.5f;
    float powerUpChance = 0.35f;
    float powerUpDuration = 7.0f;

    float powerUpVisualScale = 1.15f;
    float slowSpeedFactor = 0.5f;
    float expandScaleFactor = 1.6f;

    // Fireball (efeito + VFX)
    float fireballDuration = 7.0f;
    float fireballExplosionRadius = 4.2f;
    float fireballExplosionFxDuration = 0.45f;
    float fireballFlashMaxAlpha = 0.20f;
    float fireballShakeDuration = 0.32f;
    float fireballShakeStrength = 0.28f;

    int fireballShardCount = 18;
    float fireballShardLife = 0.55f;
    float fireballShardSpeed = 8.0f;
    float fireballShardUp = 4.5f;
    float fireballShardDrag = 2.2f;

    // Shield
    float shieldDuration = 6.5f;
    float shieldOffsetZ = 2.2f; // barreira atrás do paddle (em direcção ao arenaMaxZ)

    // Curses
    float reverseDuration = 4.25f;
    float tinyDuration = 6.0f;
    float tinyScaleFactor = 0.60f;

    // Penalizações/score
    int lifeLossPenalty = 200;
};

} // namespace game
