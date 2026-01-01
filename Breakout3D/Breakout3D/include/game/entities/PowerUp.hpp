// PowerUp.hpp
#pragma once
#include <glm/glm.hpp>

namespace game {

/**
 * @file PowerUp.hpp
 * @brief Definição de power-ups e “curses” (tipo + posição + flag de vida).
 *
 * Notas:
 * - `PowerUpType` descreve o efeito aplicado ao jogador (ou penalização).
 * - `PowerUp` é a entidade “pickup” no mundo (cai, pode ser apanhada, etc.).
 */
enum class PowerUpType {
    EXPAND,
    EXTRA_BALL,
    SLOW,
    EXTRA_LIFE,
    FIREBALL, // ball becomes a fireball (orange trail) + AoE explosion on brick hit
    SHIELD,   // temporary barrier behind the paddle that saves balls
    REVERSE,  // curse: invert left/right controls
    TINY      // curse: shrink paddle
};

/**
 * @brief Pickup de power-up no mundo.
 *
 * Notas:
 * - `type` define o efeito quando apanhado.
 * - `alive` permite desactivar/remover quando apanhado ou expirado.
 */
struct PowerUp {
    PowerUpType type;
    glm::vec3 pos = {0.0f, 0.0f, 0.0f};
    bool alive = true;
};

} // namespace game
