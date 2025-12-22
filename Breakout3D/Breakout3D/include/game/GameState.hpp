#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "game/entities/Brick.hpp"

namespace game {

struct GameState {
    glm::vec3 paddlePos = {0.0f, 0.0f, 0.0f};

    glm::vec3 ballPos   = {0.0f, 0.0f, 0.0f};
    glm::vec3 ballVel   = {0.0f, 0.0f, 0.0f};
    bool ballAttached   = true;

    std::vector<Brick> bricks;
};

} // namespace game
