#pragma once

#include "engine/Renderer.hpp"
#include "engine/Time.hpp"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace game::render {

struct RenderContext {
    int fbW = 0;
    int fbH = 0;

    engine::Time& time;
    engine::Renderer& renderer;

    glm::mat4 V{1.0f};
    glm::mat4 P{1.0f};
    glm::vec3 camPos{0.0f};

    // UI pixels (origin at bottom). -1 if invalid/unknown.
    float dangerLineScreenY = -1.0f;
};

} // namespace game::render


