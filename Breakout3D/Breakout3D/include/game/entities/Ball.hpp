// Ball.hpp
#pragma once
#include <glm/glm.hpp>

namespace game {

/**
 * @file Ball.hpp
 * @brief Estado da bola (posição/velocidade/flags de gameplay).
 *
 * Notas:
 * - `pos` e `vel` estão em world space (unidades do teu mundo).
 * - `attached` indica se a bola está “presa” ao paddle (antes do lançamento).
 * - `alive` permite remover/desactivar a bola sem a apagar logo da lista.
 * - `isFireball` activa o comportamento especial do power-up FIREBALL (one-shot + AoE).
 */
struct Ball {
    glm::vec3 pos = {0.0f, 0.0f, 0.0f};
    glm::vec3 vel = {0.0f, 0.0f, 0.0f};

    /// Se true, segue o paddle e não é simulada como bola “livre”.
    bool attached = false;

    /// Se false, a bola deve ser ignorada por update/render e eventualmente removida.
    bool alive = true;

    /**
     * @brief Modo fireball (projectil de uso único).
     *
     * @details
     * Carregado no paddle via power-up FIREBALL.
     * Ao acertar num brick: explode (AoE) e é removido (sem bounce).
     */
    bool isFireball = false;
};

} // namespace game
