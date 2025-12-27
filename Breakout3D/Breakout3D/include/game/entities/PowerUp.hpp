#pragma once
#include <glm/glm.hpp>

namespace game {

enum class PowerUpType {
    EXPAND,
    EXTRA_BALL,
    SLOW,
    EXTRA_LIFE
};

struct PowerUp {
    PowerUpType type;
    glm::vec3 pos = {0.0f, 0.0f, 0.0f};
    bool alive = true;
};

} // namespace game

