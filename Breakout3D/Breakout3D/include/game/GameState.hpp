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

enum class GameType {
    NORMAL,   // Normal mode: Win when all bricks destroyed
    ENDLESS   // Endless mode: Infinite waves, no WIN state
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
    GameType gameType = GameType::NORMAL;

    int lives = 3;
    int score = 0;
    int wave = 1;  // Endless mode wave counter
    int bricksDestroyedThisWave = 0;  // Counter for endless mode spawning
    int endlessRowsSpawned = 0;       // Total rows spawned in endless (for HP scaling)
    int pendingSpawnBricks = 0;       // Defer spawn to avoid mutating while iterating

    glm::vec3 paddlePos = {0.0f, 0.0f, 0.0f};
    
    std::vector<Ball> balls;
    std::vector<PowerUp> powerups;

    std::vector<Brick> bricks;

    // Powerup Timers
    float expandTimer = 0.0f;
    float slowTimer = 0.0f;

    // evita múltiplos hits por overlap
    float brickHitCooldown = 0.0f;

    // Debug ping: shows a HUD flash when bricks spawn
    float spawnPingTimer = 0.0f;
    float endlessSpawnCooldown = 0.0f; // seconds between auto spawns
    float endlessAutoTimer = 0.0f;      // periodic auto-spawn timer

    // UI click edge
    bool mouseWasDown = false;
    int cameraMode = 1;
    int currentBg = -1;

    // Menu state
    int selectedMenuOption = 0; // 0 = Normal, 1 = Endless, 2 = Instructions, 3 = Exit
    bool showInstructions = false;
    // Endless danger warning
    bool endlessDangerActive = false;
    float endlessDangerTimer = 0.0f;
    float endlessDangerMaxZ = 0.0f;
};

} // namespace game
