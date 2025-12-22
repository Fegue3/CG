#pragma once
#include <glm/glm.hpp>

namespace game {

struct Brick {
    glm::vec3 pos  = {0.0f, 0.0f, 0.0f};
    glm::vec3 size = {2.2f, 0.6f, 1.0f};

    bool alive = true;

    // resistência
    int hp    = 1;   // hits atuais
    int maxHp = 1;   // resistência total (1..4)
};

} // namespace game
