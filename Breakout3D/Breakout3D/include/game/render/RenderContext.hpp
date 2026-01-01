// RenderContext.hpp
#pragma once

#include "engine/Renderer.hpp"
#include "engine/Time.hpp"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace game::render {

/**
 * @file RenderContext.hpp
 * @brief Bundle de dados “do frame” para render (mundo + UI).
 *
 * Mantém o que é comum entre renderWorld/renderUI/renderMenu:
 * - tamanho do framebuffer
 * - referências ao renderer e ao relógio
 * - câmara final (V/P/camPos)
 * - alguns valores já projectados para UI (ex.: danger line em screen space)
 */
struct RenderContext {
    int fbW = 0;
    int fbH = 0;

    engine::Time& time;
    engine::Renderer& renderer;

    glm::mat4 V{1.0f};
    glm::mat4 P{1.0f};
    glm::vec3 camPos{0.0f};

    // UI pixels (origem em baixo). -1 = inválido/desconhecido.
    float dangerLineScreenY = -1.0f;
};

} // namespace game::render
