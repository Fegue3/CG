#pragma once
#include "engine/Window.hpp"
#include "engine/Time.hpp"
#include "engine/Renderer.hpp"
#include "engine/Input.hpp"
#include "game/GameConfig.hpp"
#include "game/GameState.hpp"
#include "game/AudioSystem.hpp"
#include <string>

// forward declare (não precisa incluir o header aqui)
namespace game { struct GameAssets; }

namespace game {

class Game {
public:
    Game(engine::Window& window, engine::Time& time, engine::Renderer& renderer, GameAssets& assets);

    void init();
    void update(const engine::Input& input);
    void render();

private:
    // Internal update helpers (to keep GameUpdate.cpp small and readable).
    void setMusic(const std::string& group, float fadeSeconds);
    bool updateMenu(const engine::Input& input);
    bool updateWinFinisher(float dt);
    bool updateRogueCardsOverlay(const engine::Input& input);
    bool updatePausedOverlay(const engine::Input& input);
    bool updateEndOverlay(const engine::Input& input);
    void updateTimers(const engine::Input& input, float dt);
    void updatePlayingFrame(
        const engine::Input& input,
        float dt,
        GameMode modeBefore,
        GameType typeBefore,
        int livesBefore,
        int waveBefore,
        int endlessRowsBefore,
        bool dangerBefore,
        int streakBefore,
        bool hadPowerupBefore
    );

    // Shared helpers for Endless/Rogue bookkeeping.
    static std::string endlessBestScorePath();
    static void saveEndlessBestScore(int best);
    static void commitEndlessStreak(GameState& state);
    static void maybeUpdateEndlessBest(GameState& state);

    engine::Window& m_window;
    engine::Time& m_time;
    engine::Renderer& m_renderer;
    GameAssets& m_assets;

    GameConfig m_cfg;
    GameState m_state;

    AudioSystem m_audio;

    // Previous-frame tracking for one-shot audio events
    GameMode m_prevMode = GameMode::MENU;
    GameType m_prevGameType = GameType::NORMAL;
    MenuScreen m_prevMenuScreen = MenuScreen::MAIN;
    int m_prevHoveredMenuButton = -1;
    int m_prevHoveredPlayModeButton = -1;
    bool m_prevShowInstructions = false;
    int m_prevInstructionsTab = 0;
    int m_prevHoveredCloseButton = 0;
    int m_prevHoveredPowerupNav = -1;

    int m_prevHoveredRogueCard = -1;
    int m_prevHoveredRoguePickBtn = -1;

    int m_prevLives = 3;
    int m_prevWave = 1;
    int m_prevEndlessRowsSpawned = 0;
    bool m_prevEndlessDangerActive = false;
    bool m_prevStreakBanking = false;
    int m_prevStreakPoints = 0;

    int m_endlessMusicTier = 0; // 0 low, 1 mid, 2 high
    std::string m_currentMusicGroup;

    // Instructions overlay extra UX SFX
    int m_prevPowerupInspectIndex = 0;
    bool m_prevRogueCardsInspectOpen = false;
};

} // namespace game
