#pragma once
#include <glm/glm.hpp>

namespace game {

struct Ball {
    glm::vec3 pos = {0.0f, 0.0f, 0.0f};
    glm::vec3 vel = {0.0f, 0.0f, 0.0f};
    bool attached = false;
};

} // namespace game


