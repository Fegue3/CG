// Brick.hpp
#pragma once
#include <glm/glm.hpp>

namespace game {

/**
 * @file Brick.hpp
 * @brief Brick do nível (transform básico + vida/resistência).
 *
 * Notas:
 * - `size` define dimensões do brick (para colisões/render).
 * - `hp` diminui com hits; quando chega a 0, o brick deixa de estar `alive`.
 * - `maxHp` é útil para cores/feedback visual (1..4, etc.).
 */
struct Brick {
    glm::vec3 pos  = {0.0f, 0.0f, 0.0f};
    glm::vec3 size = {2.2f, 0.6f, 1.0f};

    /// Se false, o brick não participa em colisões/render.
    bool alive = true;

    /// Hits actuais (vida). Normalmente decrementa a cada colisão válida.
    int hp    = 1;

    /// Resistência total (para UI/cores/níveis). Tipicamente 1..4.
    int maxHp = 1;
};

} // namespace game
