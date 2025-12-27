#pragma once
#include <glm/glm.hpp>

namespace game {

struct Ball {
    glm::vec3 pos = {0.0f, 0.0f, 0.0f};
    glm::vec3 vel = {0.0f, 0.0f, 0.0f};
    bool attached = false;
    bool alive = true;

    // One-shot fireball projectile (loaded onto paddle by FIREBALL power-up).
    // When it hits a brick, it explodes (AoE) and is removed (no bounce).
    bool isFireball = false;
};

} // namespace game


