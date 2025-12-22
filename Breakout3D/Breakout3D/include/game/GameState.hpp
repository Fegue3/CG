#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "game/entities/Brick.hpp"

namespace game {

enum class GameMode {
    PLAYING,
    GAME_OVER,
    WIN
};

struct GameState {
    GameMode mode = GameMode::PLAYING;

    int lives = 3;

    glm::vec3 paddlePos = {0.0f, 0.0f, 0.0f};

    glm::vec3 ballPos = {0.0f, 0.0f, 0.0f};
    glm::vec3 ballVel = {0.0f, 0.0f, 0.0f};
    bool ballAttached = true;

    std::vector<Brick> bricks;

    // evita múltiplos hits por overlap
    float brickHitCooldown = 0.0f;

    // UI click edge
    bool mouseWasDown = false;
};

} // namespace game
