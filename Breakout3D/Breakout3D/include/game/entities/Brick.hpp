#pragma once
#include <glm/glm.hpp>

namespace game {

struct Brick {
    glm::vec3 pos   = {0,0,0};
    glm::vec3 size  = {2.0f, 0.6f, 1.0f};
    bool alive      = true;
};

} // namespace game
