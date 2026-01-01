// Paddle.hpp
#pragma once
#include <glm/glm.hpp>

namespace game {

/**
 * @file Paddle.hpp
 * @brief Estado do paddle (transform mínimo).
 *
 * Notas:
 * - Propriedades (velocidade, limites, tamanhos, etc.) vivem no GameConfig.
 * - Este struct é propositadamente leve; podes estender com estado específico (ex.: timers, efeitos).
 */
struct Paddle {
    glm::vec3 pos = {0.0f, 0.0f, 0.0f};
    // Paddle properties are stored in GameConfig
    // This struct can be extended with paddle-specific state if needed
};

} // namespace game
