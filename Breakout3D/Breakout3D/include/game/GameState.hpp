#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "game/entities/Brick.hpp"
#include "game/entities/Ball.hpp"
#include "game/entities/PowerUp.hpp" // includes PowerUpType enum
#include "game/ui/OverlayLayout.hpp"
#include "game/rogue/RogueCardId.hpp"

namespace game {

enum class GameMode {
    MENU,
    PLAYING,
    PAUSED,
    ROGUE_CARDS, // Rogue mode: pick a reward card (gameplay paused)
    GAME_OVER,
    WIN
};

enum class GameType {
    NORMAL,   // Normal mode: Win when all bricks destroyed
    ENDLESS,  // Endless mode: Infinite waves, no WIN state
    ROGUE,    // Future: Rogue mode (placeholder)
    LEVELS    // Future: Levels mode (placeholder)
};

enum class MenuScreen {
    MAIN,       // Main menu: Play, Instructions, Options, Exit
    PLAY_MODES, // Play modes submenu: Normal, Endless, Rogue, Levels, Back
    INSTRUCTIONS, // Instructions screen: Controls, Powerups, Back
    OPTIONS     // Options submenu: Sound, Graphics, Back
};

// PowerUpType is now defined in game/entities/PowerUp.hpp

struct GameState {
    GameMode mode = GameMode::MENU;
    GameType gameType = GameType::NORMAL;

    int lives = 3;
    int score = 0;
    // Endless mode: persistent best score (loaded on init, updated/saved during play)
    int endlessBestScore = 0;
    // Endless mode: "streak bank" — points accumulate here and commit to score
    // after a short idle delay (or immediately on game over).
    int endlessStreakPoints = 0;
    // Split view of the bank so we can show "+gains" and "-penalties" simultaneously.
    // Net is still `endlessStreakPoints` (pos - neg).
    int endlessStreakPosPoints = 0;
    int endlessStreakNegPoints = 0;
    float endlessStreakIdleTimer = 0.0f;
    bool endlessStreakBanking = false;
    float endlessStreakBankTimer = 0.0f;
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
    float fireballTimer = 0.0f;
    float shieldTimer = 0.0f;
    float reverseTimer = 0.0f;
    float tinyTimer = 0.0f;

    struct FireballExplosionFx {
        glm::vec3 pos = glm::vec3(0.0f);
        float t = 0.0f; // seconds since spawn
    };
    std::vector<FireballExplosionFx> fireballExplosions;

    // Fireball impact "break" feel
    float fireballShakeTimer = 0.0f;
    glm::vec3 fireballShakeAnchorPos = glm::vec3(0.0f);

    struct FireballShard {
        glm::vec3 pos = glm::vec3(0.0f);
        glm::vec3 vel = glm::vec3(0.0f);
        float t = 0.0f;
    };
    std::vector<FireballShard> fireballShards;

    // Score popups (used for negative penalties and positive instant awards like Fireball)
    struct ScorePopup {
        int pts = 0;      // can be negative
        float t = 0.0f;   // seconds since spawn
    };
    std::vector<ScorePopup> scorePopups;

    // When a one-shot fireball projectile is consumed, we respawn a normal ball
    // without costing a life (so Fireball doesn't "eat" a heart).
    bool pendingRespawnAfterFireball = false;

    // evita múltiplos hits por overlap
    float brickHitCooldown = 0.0f;

    // Debug ping: shows a HUD flash when bricks spawn
    float spawnPingTimer = 0.0f;
    float endlessSpawnCooldown = 0.0f; // seconds between auto spawns
    float endlessAutoTimer = 0.0f;      // periodic auto-spawn timer
    float endlessElapsedTime = 0.0f;    // total time spent in endless mode (playing)

    // UI click edge
    bool mouseWasDown = false;
    int cameraMode = 1;
    int currentBg = -1;

    // --- Rogue mode (wave-based) ---
    // Rewards are presented as "cards" between waves (and at run start).
    int rogueRewardEvery = 1; // show cards after every wave (user request)
    // Current offer (up to 3 cards) + hover index.
    game::rogue::RogueCardId rogueOffer[3] = {
        game::rogue::RogueCardId::PU_EXPAND,
        game::rogue::RogueCardId::PU_EXTRA_BALL,
        game::rogue::RogueCardId::PU_EXTRA_LIFE
    };
    int rogueOfferCount = 0;
    int hoveredRogueCard = -1; // -1 none, 0..2

    // Chosen cards (no repeats) and "drop deck" (powerup types that can drop from bricks).
    std::vector<game::rogue::RogueCardId> rogueChosen;
    std::vector<PowerUpType> rogueDropDeck;

    // Remaining pools for no-repeat dealing.
    std::vector<game::rogue::RogueCardId> rogueRemainingNormal;
    std::vector<game::rogue::RogueCardId> rogueRemainingOp;

    // Starting draft: wave 1 begins with 3 rounds of 3-card choices before gameplay.
    int rogueStartingDraftRoundsLeft = 0;

    // Persistent modifiers (applied immediately on pick; many have trade-offs).
    float rogueDropChanceMult = 1.0f;
    float rogueBasePaddleScaleX = 1.0f;
    float roguePaddleSpeedMult = 1.0f;
    float rogueBallSpeedMult = 1.0f;
    float rogueBrickPointsMult = 1.0f; // affects brick point awards
    float rogueBankIdleMult = 1.0f;    // >1 = safer (longer to bank), <1 = riskier
    int rogueBrickDamageBonus = 0;     // extra damage per normal hit (e.g. Pierce Training)
    float rogueFireballRadiusMult = 1.0f;
    float rogueShieldDurationMult = 1.0f;
    int rogueLifeLossPenaltyBonus = 0; // additive penalty in addition to cfg.lifeLossPenalty
    int rogueRowsPerWaveDelta = 0;      // negative reduces rows inserted per wave (Row Control)
    float rogueWindX = 0.0f;            // environment: constant lateral wind applied to balls
    float roguePaddleClampMarginX = 0.0f; // environment: reduces paddle range from walls
    bool rogueStickyPaddle = false;     // environment: balls stick to paddle on hit (non-fireball)
    int rogueMaxWaves = 10; // run ends (WIN) after clearing this wave number
    int rogueBestScore = 0; // persistent best score for Rogue (separate from Endless)

    // Wave advancement in Rogue is NOT "clear all bricks". It's quota/time based:
    int rogueBricksBrokenThisWave = 0;
    float rogueWaveTimer = 0.0f;
    float rogueWaveCooldown = 0.0f; // small gate to prevent double-advance in the same frame
    int rogueRowsSpawned = 0; // counts rows inserted so far (used for pacing/difficulty if needed)

    // After a wave advance, we queue row insertions and spawn them gradually
    // AFTER the player picks a card.
    int roguePendingRowsToSpawn = 0;
    float rogueRowSpawnTimer = 0.0f;
    float rogueRowSpawnInterval = 0.55f; // seconds between each inserted row (tune)

    // Menu state
    MenuScreen currentMenuScreen = MenuScreen::MAIN;
    int selectedMenuOption = 0;
    int hoveredMenuButton = -1; // -1 = none, 0-3 for main menu buttons
    int hoveredOverlayButton = -1; // -1 = none, 0 = left button, 1 = right button (pause/gameover/win)
    bool hoveredCloseButton = false; // Instructions overlay BACK button (bottom-left)
    bool hoveredTestBadge = false; // ONE BRICK test badge
    bool showInstructions = false;
    int instructionsTab = 0; // 0 = Controls, 1 = Powerups, 2 = Rogue Cards
    // Powerups inspector (Instructions -> Powerups overlay)
    int powerupInspectIndex = 0;
    float powerupInspectYaw = 0.0f;
    float powerupInspectPitch = 0.0f;
    bool powerupInspectDragging = false;
    glm::vec2 powerupInspectLastMouse = glm::vec2(0.0f);
    int hoveredPowerupNav = -1; // -1 none, 0 left, 1 right

    // Rogue cards browser (Instructions -> Rogue Cards)
    int hoveredRogueCardsItem = -1; // -1 none, else index into current slots list
    game::rogue::RogueCardId rogueCardsSelected = game::rogue::RogueCardId::PU_EXPAND;
    bool rogueCardsInspectOpen = false; // modal card inspect overlay
    float rogueCardsScrollPowerups = 0.0f;  // px
    float rogueCardsScrollModifiers = 0.0f; // px
    float rogueCardsScrollOp = 0.0f;        // px
    bool testOneBrick = false;  // Test feature: spawn a single brick instead of the full wall

    // Cached menu layout (computed using real font metrics to keep input + render aligned).
    ui::MenuLayout menuLayout;
    // Endless danger warning
    bool endlessDangerActive = false;
    float endlessDangerTimer = 0.0f;
    float endlessDangerMaxZ = 0.0f;

    // --- Normal mode "last brick" finisher (short cinematic before WIN overlay) ---
    bool winFinisherActive = false;
    float winFinisherTimer = 0.0f;      // burst time (camera break + shockwave)
    float winFinisherRealTimer = 0.0f;  // real time since start (unscaled)

    // Brick impact tracking (used to anchor VFX like the WIN finisher shockwave)
    bool lastBrickDestroyedValid = false;
    glm::vec3 lastBrickDestroyedPos = glm::vec3(0.0f);

    bool winFinisherAnchorValid = false;
    glm::vec3 winFinisherAnchorPos = glm::vec3(0.0f);

    // During the brief win slow-down, keep the last brick visible (so it "breaks" on the burst).
    bool winFinisherHoldBrickValid = false;
    glm::vec3 winFinisherHoldBrickPos = glm::vec3(0.0f);
    glm::vec3 winFinisherHoldBrickSize = glm::vec3(1.0f);
    int winFinisherHoldBrickMaxHp = 1;
    int winFinisherHoldBrickHp = 1;
};

} // namespace game
