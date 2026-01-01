// PhysicsSystem.hpp
#pragma once
#include "game/GameState.hpp"
#include "game/GameConfig.hpp"

namespace game {

/**
 * @file PhysicsSystem.hpp
 * @brief Movimento e simulação básica (paddle e bolas).
 *
 * Este sistema trata “apenas” de actualizar posições/velocidades.
 * Colisões/respostas ficam no CollisionSystem.
 */
class PhysicsSystem {
public:
    /**
     * @brief Actualiza a posição do paddle com base numa direcção (-1..+1).
     * @param dir Direcção: -1 esquerda, +1 direita, 0 parado.
     */
    static void updatePaddle(GameState& state, const GameConfig& cfg, float dir, float dt);

    /// Actualiza bolas (pos += vel * dt, estados attached, etc.).
    static void updateBalls(GameState& state, const GameConfig& cfg, float dt);

    /// Reposiciona uma bola para “colada” ao paddle (ex.: antes do launch / após reset).
    static void resetBallToPaddle(Ball& ball, const glm::vec3& paddlePos, const GameConfig& cfg);
};

} // namespace game
