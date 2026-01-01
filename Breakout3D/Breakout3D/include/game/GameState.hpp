// GameState.hpp
#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

#include "game/entities/Brick.hpp"
#include "game/entities/Ball.hpp"
#include "game/entities/PowerUp.hpp" // inclui PowerUpType
#include "game/ui/OverlayLayout.hpp"
#include "game/rogue/RogueCardId.hpp"

namespace game {

/**
 * @file GameState.hpp
 * @brief Estado central do jogo (modo actual, entidades, timers, UI e dados específicos de cada modo).
 *
 * O `GameState` é o “cérebro” do runtime: guarda tudo o que muda durante o jogo.
 * Está organizado por blocos:
 * - modo e tipo de jogo (menu/playing/pausa/etc.)
 * - estado de gameplay (vidas, score, wave, entidades)
 * - timers e efeitos (power-ups, fireball VFX, finisher de vitória)
 * - estado de UI (hover/drag/click edge, layouts calculados)
 * - dados específicos de modos (Endless, Rogue, Levels)
 *
 * @note
 * Muitos “eventos” (ex.: power-ups apanhados) são registados aqui para o `Game::update`
 * conseguir tocar áudio e aplicar UI sem depender dos sistemas onde o evento aconteceu.
 */

enum class GameMode {
    MENU,
    PLAYING,
    PAUSED,
    ROGUE_CARDS, // Rogue: escolha de carta (gameplay em pausa)
    GAME_OVER,
    WIN
};

enum class GameType {
    NORMAL,   // Normal: ganha quando todos os bricks forem destruídos
    ENDLESS,  // Endless: waves infinitas, não existe estado WIN
    ROGUE,    // Rogue: waves + cartas (run)
    LEVELS    // Levels: níveis fixos (1..20)
};

enum class MenuScreen {
    MAIN,         // Menu principal
    PLAY_MODES,   // Submenu de modos
    LEVEL_SELECT, // Seleção de nível (Levels)
    INSTRUCTIONS, // Instruções
    OPTIONS,      // Opções
    SOUND         // Som (sliders)
};

// PowerUpType é definido em game/entities/PowerUp.hpp

struct GameState {
    // --- Estado global (máquina de estados) ---
    GameMode mode = GameMode::MENU;
    GameType gameType = GameType::NORMAL;

    int lives = 3;
    int score = 0;

    // ------------------------- LEVELS MODE -------------------------
    int currentLevel = 1;        // nível actual (1-20)
    int levelsBestLevel = 1;     // melhor nível atingido (persistente)
    int levelsCompletedStars[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; // 0-3 estrelas por nível
    int hoveredLevelButton = -1; // -1 nenhum, 0-19 índice do nível
    int hoveredPauseRogueCard = -1; // hover na lista de cartas (pausa do Rogue)

    // ------------------------- ENDLESS MODE -------------------------
    int endlessBestScore = 0;    // recorde (carregado no init, guardado ao longo do jogo)

    // “Streak bank”: pontos acumulam e só entram no score após um curto delay de “idle”.
    int endlessStreakPoints = 0;
    int endlessStreakPosPoints = 0; // para UI: ganhos
    int endlessStreakNegPoints = 0; // para UI: penalizações
    float endlessStreakIdleTimer = 0.0f;
    bool endlessStreakBanking = false;
    float endlessStreakBankTimer = 0.0f;

    int wave = 1;
    int bricksDestroyedThisWave = 0;
    int endlessRowsSpawned = 0;
    int pendingSpawnBricks = 0; // spawn adiado para não mexer em listas durante iteração

    // ------------------------- ENTIDADES -------------------------
    glm::vec3 paddlePos = {0.0f, 0.0f, 0.0f};

    std::vector<Ball> balls;
    std::vector<PowerUp> powerups;
    std::vector<Brick> bricks;

    // --- Buffers de eventos para áudio (limpos/consumidos no Game::update) ---
    std::vector<PowerUpType> audioSpawnedPowerups;
    std::vector<PowerUpType> audioPickedPowerups;

    // ------------------------- TIMERS DE POWER-UPS -------------------------
    float expandTimer = 0.0f;
    float slowTimer = 0.0f;
    float fireballTimer = 0.0f;
    float shieldTimer = 0.0f;
    float reverseTimer = 0.0f;
    float tinyTimer = 0.0f;

    // ------------------------- FIREBALL (VFX/FEEDBACK) -------------------------
    struct FireballExplosionFx {
        glm::vec3 pos = glm::vec3(0.0f);
        float t = 0.0f; // segundos desde spawn
    };
    std::vector<FireballExplosionFx> fireballExplosions;

    float fireballShakeTimer = 0.0f;
    glm::vec3 fireballShakeAnchorPos = glm::vec3(0.0f);

    struct FireballShard {
        glm::vec3 pos = glm::vec3(0.0f);
        glm::vec3 vel = glm::vec3(0.0f);
        float t = 0.0f;
    };
    std::vector<FireballShard> fireballShards;

    // Popups de score (positivos e negativos).
    struct ScorePopup {
        int pts = 0;     // pode ser negativo
        float t = 0.0f;  // segundos desde spawn
    };
    std::vector<ScorePopup> scorePopups;

    // Quando um fireball one-shot é consumido, respawn de bola normal sem gastar vida.
    bool pendingRespawnAfterFireball = false;

    // Evita múltiplos hits por overlap no mesmo instante.
    float brickHitCooldown = 0.0f;

    // Debug/Endless timers auxiliares.
    float spawnPingTimer = 0.0f;
    float endlessSpawnCooldown = 0.0f;
    float endlessAutoTimer = 0.0f;
    float endlessElapsedTime = 0.0f;

    // ------------------------- UI / INPUT STATE -------------------------
    bool mouseWasDown = false; // edge de clique (para não repetir “click” enquanto está premido)
    int cameraMode = 1;
    int currentBg = -1;

    // ------------------------- ROGUE MODE -------------------------
    int rogueRewardEvery = 1; // mostrar cartas após cada wave (pedido do utilizador)

    // Oferta actual (até 3 cartas) + hover.
    game::rogue::RogueCardId rogueOffer[3] = {
        game::rogue::RogueCardId::PU_EXPAND,
        game::rogue::RogueCardId::PU_EXTRA_BALL,
        game::rogue::RogueCardId::PU_EXTRA_LIFE
    };
    int rogueOfferCount = 0;
    int hoveredRogueCard = -1; // -1 nenhum, 0..2
    int hoveredRogueCardPickButton = -1; // hover do botão PICK por carta

    // Picks (sem repetição) e deck de drops (powerups que podem cair dos bricks).
    std::vector<game::rogue::RogueCardId> rogueChosen;
    std::vector<PowerUpType> rogueDropDeck;

    // Pools restantes para garantir “sem repetição”.
    std::vector<game::rogue::RogueCardId> rogueRemainingNormal;
    std::vector<game::rogue::RogueCardId> rogueRemainingOp;

    int rogueStartingDraftRoundsLeft = 0;

    // Modificadores persistentes da run (aplicados ao escolher cartas).
    float rogueDropChanceMult = 1.0f;
    float rogueBasePaddleScaleX = 1.0f;
    float roguePaddleSpeedMult = 1.0f;
    float rogueBallSpeedMult = 1.0f;
    float rogueBrickPointsMult = 1.0f;
    float rogueBankIdleMult = 1.0f;
    int rogueBrickDamageBonus = 0;
    float rogueFireballRadiusMult = 1.0f;
    float rogueShieldDurationMult = 1.0f;
    int rogueLifeLossPenaltyBonus = 0;
    int rogueRowsPerWaveDelta = 0;

    // Ambiente (efeitos “de sala”).
    float rogueWindX = 0.0f;
    bool rogueRandomWindActive = false;
    float rogueRandomWindTimer = 0.0f;
    float roguePaddleClampMarginX = 0.0f;
    bool rogueStickyPaddle = false;

    int rogueMaxWaves = 10; // run termina (WIN) ao limpar esta wave
    int rogueBestScore = 0; // recorde Rogue (persistente, separado do Endless)

    // Avanço de wave no Rogue: por quota/tempo (não é “limpar todos os bricks”).
    int rogueBricksBrokenThisWave = 0;
    float rogueWaveTimer = 0.0f;
    float rogueWaveCooldown = 0.0f;
    int rogueRowsSpawned = 0;

    // Após avançar de wave, filas são enfileiradas e inseridas gradualmente (depois de escolher carta).
    int roguePendingRowsToSpawn = 0;
    float rogueRowSpawnTimer = 0.0f;
    float rogueRowSpawnInterval = 0.55f;

    // ------------------------- MENU STATE -------------------------
    MenuScreen currentMenuScreen = MenuScreen::MAIN;
    int selectedMenuOption = 0;

    int hoveredMenuButton = -1; // main menu buttons (0..3)
    int hoveredPlayModeButton = -1; // PLAY_MODES: botão PLAY por cartão (0..3)
    int hoveredOverlayButton = -1; // overlays (0 esquerda, 1 direita)
    bool hoveredCloseButton = false; // botão BACK do overlay de instruções
    bool hoveredTestBadge = false;
    bool showInstructions = false;
    int instructionsTab = 0; // 0 Controlos, 1 Power-ups, 2 Rogue Cards

    // Inspector de power-ups (instruções).
    int powerupInspectIndex = 0;
    float powerupInspectYaw = 0.0f;
    float powerupInspectPitch = 0.0f;
    bool powerupInspectDragging = false;
    glm::vec2 powerupInspectLastMouse = glm::vec2(0.0f);
    int hoveredPowerupNav = -1; // 0 left, 1 right

    // Browser de cartas Rogue (instruções).
    int hoveredRogueCardsItem = -1;
    game::rogue::RogueCardId rogueCardsSelected = game::rogue::RogueCardId::PU_EXPAND;
    bool rogueCardsInspectOpen = false;
    float rogueCardsScrollPowerups = 0.0f;
    float rogueCardsScrollModifiers = 0.0f;
    float rogueCardsScrollOp = 0.0f;

    bool testOneBrick = false;

    // ------------------------- ÁUDIO (sliders) -------------------------
    float audioMasterVol = 1.0f;
    float audioSfxVol = 1.0f;
    float audioMusicVol = 0.32f;
    float audioStingerVol = 0.40f;

    int hoveredSoundSlider = -1;  // 0 master, 1 sfx, 2 music, 3 stinger
    bool hoveredSoundBack = false;
    int draggingSoundSlider = -1;
    bool draggingSound = false;

    // Layout do menu (cacheado, calculado com métricas reais da fonte).
    ui::MenuLayout menuLayout;

    // Endless: aviso de danger zone.
    bool endlessDangerActive = false;
    float endlessDangerTimer = 0.0f;
    float endlessDangerMaxZ = 0.0f;

    // ------------------------- FINISHER DE VITÓRIA (modo Normal) -------------------------
    bool winFinisherActive = false;
    float winFinisherTimer = 0.0f;     // tempo cinematográfico do burst
    float winFinisherRealTimer = 0.0f; // tempo real desde início

    // Âncoras para efeitos (shockwave/flash) e para manter o último brick visível.
    bool lastBrickDestroyedValid = false;
    glm::vec3 lastBrickDestroyedPos = glm::vec3(0.0f);

    bool winFinisherAnchorValid = false;
    glm::vec3 winFinisherAnchorPos = glm::vec3(0.0f);

    bool winFinisherHoldBrickValid = false;
    glm::vec3 winFinisherHoldBrickPos = glm::vec3(0.0f);
    glm::vec3 winFinisherHoldBrickSize = glm::vec3(1.0f);
    int winFinisherHoldBrickMaxHp = 1;
    int winFinisherHoldBrickHp = 1;
};

} // namespace game
