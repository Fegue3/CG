#pragma once
#include <glm/glm.hpp>

namespace game {

enum class PowerUpType {
    EXPAND,
    EXTRA_BALL,
    SLOW,
    EXTRA_LIFE,

    // New powerups/curses
    FIREBALL, // ball becomes a fireball (orange trail) + AoE explosion on brick hit
    SHIELD,   // temporary barrier behind the paddle that saves balls
    REVERSE,  // curse: invert left/right controls
    TINY      // curse: shrink paddle
};

struct PowerUp {
    PowerUpType type;
    glm::vec3 pos = {0.0f, 0.0f, 0.0f};
    bool alive = true;
};

} // namespace game

