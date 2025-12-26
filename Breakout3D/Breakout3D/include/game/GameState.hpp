#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "game/entities/Brick.hpp"

namespace game {

enum class GameMode {
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER,
    WIN
};

enum class PowerUpType {
    EXPAND,
    EXTRA_BALL,
    SLOW,
    EXTRA_LIFE
};

struct Ball {
    glm::vec3 pos = {0.0f, 0.0f, 0.0f};
    glm::vec3 vel = {0.0f, 0.0f, 0.0f};
    bool attached = false;
};

struct PowerUp {
    PowerUpType type;
    glm::vec3 pos = {0.0f, 0.0f, 0.0f};
    bool alive = true;
};

struct GameState {
    GameMode mode = GameMode::MENU;

    int lives = 3;

    glm::vec3 paddlePos = {0.0f, 0.0f, 0.0f};
    
    std::vector<Ball> balls;
    std::vector<PowerUp> powerups;

    std::vector<Brick> bricks;

    // Powerup Timers
    float expandTimer = 0.0f;
    float slowTimer = 0.0f;

    // evita múltiplos hits por overlap
    float brickHitCooldown = 0.0f;

    // UI click edge
    bool mouseWasDown = false;
    int cameraMode = 1;
    int currentBg = -1;

    // Menu state
    int selectedMenuOption = 0; // 0 = Play, 1 = Instructions, 2 = Exit
    bool showInstructions = false;
};

} // namespace game
