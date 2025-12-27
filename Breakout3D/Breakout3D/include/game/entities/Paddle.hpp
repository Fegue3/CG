#pragma once
#include <glm/glm.hpp>

namespace game {

struct Paddle {
    glm::vec3 pos = {0.0f, 0.0f, 0.0f};
    // Paddle properties are stored in GameConfig
    // This struct can be extended with paddle-specific state if needed
};

} // namespace game


