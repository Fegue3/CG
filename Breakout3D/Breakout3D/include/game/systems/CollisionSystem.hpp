// CollisionSystem.hpp
#pragma once
#include "game/GameState.hpp"
#include "game/GameConfig.hpp"
#include "game/entities/Ball.hpp"

namespace game {

/**
 * @file CollisionSystem.hpp
 * @brief Colisões do jogo (paredes/mundo, paddle e bricks).
 *
 * Este sistema trata das respostas a colisões:
 * - limites do mundo (paredes/tecto/chão/zonas de perda)
 * - paddle (inclui ajuste de direcção para dar controlo ao jogador)
 * - bricks (dano, destruição e casos especiais como fireball)
 */
class CollisionSystem {
public:
    /**
     * @brief Colisões com o “mundo” (paredes/limites).
     * @return true se houve colisão (útil para SFX/feedback).
     */
    static bool handleWorldCollisions(Ball& ball, const GameConfig& cfg);

    /**
     * @brief Colisão bola <-> paddle.
     *
     * Normalmente:
     * - reflecte a velocidade no eixo adequado,
     * - ajusta o ângulo conforme a zona do paddle onde a bola bateu (mais controlo).
     *
     * @param paddlePos / paddleSize usados para AABB do paddle no mundo.
     * @return true se houve colisão.
     */
    static bool handlePaddleCollision(Ball& ball, const GameState& state, const glm::vec3& paddlePos,
                                      const glm::vec3& paddleSize, const GameConfig& cfg);

    /**
     * @brief Informação de resultado de colisão com bricks (para VFX/SFX/score).
     *
     * Esta struct existe para o gameplay conseguir reagir sem re-calcular:
     * - se acertou, se foi fireball, se partiu/crack/danificou, quantos bricks morreram, etc.
     */
    struct BrickCollisionInfo {
        bool hit = false;       ///< houve colisão com pelo menos um brick
        bool fireball = false;  ///< a bola era fireball (impacto especial)
        bool damaged = false;   ///< acertou mas não destruiu (hp desceu, brick ficou vivo)
        bool cracked = false;   ///< entrou num estado “low hp” (para feedback visual)
        bool broke = false;     ///< destruiu pelo menos 1 brick
        int bricksKilled = 0;   ///< fireball pode destruir vários
        int hpBefore = 0;       ///< hp do brick principal antes do impacto
        int hpAfter = 0;        ///< hp depois do impacto
        int maxHp = 0;          ///< resistência total (para VFX/cores)
    };

    /**
     * @brief Colisão bola <-> bricks (dano, destruição, fireball/AoE).
     * @return Info detalhada do impacto (hit/dano/destruição/quantos morreram).
     */
    static BrickCollisionInfo handleBrickCollisions(Ball& ball, GameState& state, const GameConfig& cfg);
};

} // namespace game
