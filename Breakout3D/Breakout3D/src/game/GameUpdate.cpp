#include "game/Game.hpp"
#include "engine/Input.hpp"

#include "game/GameAssets.hpp"
#include "game/systems/InitSystem.hpp"
#include "game/systems/InputSystem.hpp"
#include "game/systems/PhysicsSystem.hpp"
#include "game/systems/CollisionSystem.hpp"
#include "game/systems/PowerUpSystem.hpp"
#include "game/systems/RogueSystem.hpp"
#include "game/rogue/RogueCards.hpp"
#include "game/ui/OverlayLayout.hpp"
#include "game/ui/RogueCardLayout.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <string>

namespace game {

/*
    Game::update:
    - Faz tick do áudio (fades/loops)
    - Aplica settings de volume (sliders do menu)
    - Faz snapshot do estado para triggers de SFX (diferenças entre frames)
    - Pré-carrega GIFs dos powerups em background
    - Processa menu/win finisher/input/overlays/timers/win conditions
    - Por fim, corre a simulação do frame (updatePlayingFrame)
*/
void Game::update(const engine::Input& input) {
    float dt = m_time.delta();

    // Tick áudio (fades, gating de loops). Pode estar desactivado.
    m_audio.update(dt);

    // Aplicar volumes em tempo real (sliders do menu)
    if (m_audio.isEnabled()) {
        m_audio.setMasterVolume(m_state.audioMasterVol);
        m_audio.setSfxVolume(m_state.audioSfxVol);
        m_audio.setMusicVolume(m_state.audioMusicVol);
        m_audio.setStingerVolume(m_state.audioStingerVol);
    }

    // Snapshot para triggers baseados em diferenças
    GameMode modeBefore = m_state.mode;
    GameType typeBefore = m_state.gameType;
    int livesBefore = m_state.lives;
    int waveBefore = m_state.wave;
    int endlessRowsBefore = m_state.endlessRowsSpawned;
    bool dangerBefore = m_state.endlessDangerActive;
    int streakBefore = m_state.endlessStreakPoints;
    bool hadPowerupBefore = !m_state.powerups.empty();

    int attachedBefore = 0;
    for (const auto& b : m_state.balls) if (b.attached) attachedBefore++;

    // Pré-carregar GIFs de powerups (assíncrono + upload incremental)
    m_assets.startPowerupVideoPreload();
    m_assets.pumpPowerupVideoPreload(6);

#ifdef BREAKOUT3D_DEBUG
    // Debug: spawn de powerups específicos (teclas da fila de cima).
    if (m_state.mode == GameMode::PLAYING) {
        auto spawnDebugDrop = [&](PowerUpType type) {
            PowerUp p;
            p.type = type;
            // Bricks começam em cfg.arenaMinZ + 0.85f (InitSystem.cpp).
            p.pos = glm::vec3(m_state.paddlePos.x, 0.4f, m_cfg.arenaMinZ + 0.85f);
            m_state.powerups.push_back(p);
        };

        if (input.keyPressed(engine::Key::K8)) spawnDebugDrop(PowerUpType::EXPAND);
        if (input.keyPressed(engine::Key::K9)) spawnDebugDrop(PowerUpType::EXTRA_BALL);
        if (input.keyPressed(engine::Key::K0)) spawnDebugDrop(PowerUpType::EXTRA_LIFE);
        if (input.keyPressed(engine::Key::Minus)) spawnDebugDrop(PowerUpType::SLOW);

        if (input.keyPressed(engine::Key::K4)) spawnDebugDrop(PowerUpType::TINY);
        if (input.keyPressed(engine::Key::K5)) spawnDebugDrop(PowerUpType::FIREBALL);
        if (input.keyPressed(engine::Key::K6)) spawnDebugDrop(PowerUpType::SHIELD);
        if (input.keyPressed(engine::Key::K7)) spawnDebugDrop(PowerUpType::REVERSE);
    }

    // Debug: encher cartas Rogue (R em modo ROGUE)
    if (m_state.gameType == GameType::ROGUE && input.keyPressed(engine::Key::R)) {
        m_state.rogueChosen.clear();

        const game::rogue::RogueCardId allCards[] = {
            game::rogue::RogueCardId::PU_EXPAND,
            game::rogue::RogueCardId::PU_EXTRA_BALL,
            game::rogue::RogueCardId::PU_EXTRA_LIFE,
            game::rogue::RogueCardId::PU_FIREBALL,
            game::rogue::RogueCardId::PU_SHIELD,
            game::rogue::RogueCardId::PU_SLOW,
            game::rogue::RogueCardId::PU_REVERSE,
            game::rogue::RogueCardId::PU_TINY,
            game::rogue::RogueCardId::MOD_WIDE_PADDLE_SLOW,
            game::rogue::RogueCardId::MOD_WIDE_PADDLE_LIGHT_BALL,
            game::rogue::RogueCardId::MOD_FAST_PADDLE_TINY_PADDLE,
            game::rogue::RogueCardId::MOD_FAST_BALL_SLOW_PADDLE,
            game::rogue::RogueCardId::MOD_LUCKY_DROPS_BRITTLE,
            game::rogue::RogueCardId::MOD_LUCKY_DROPS_SLOW_BALL,
            game::rogue::RogueCardId::MOD_BONUS_LIFE_TAX,
            game::rogue::RogueCardId::MOD_GLASS_CANNON,
            game::rogue::RogueCardId::MOD_STREAK_GREED,
            game::rogue::RogueCardId::MOD_SAFE_BANKER,
            game::rogue::RogueCardId::MOD_SHIELD_LONG_SLOW,
            game::rogue::RogueCardId::MOD_FIREBALL_WIDE_SLOW,
        };

        for (const auto& card : allCards) {
            if (m_state.rogueChosen.size() >= 20) break;
            m_state.rogueChosen.push_back(card);
        }
    }
#endif

    // MENU
    if (updateMenu(input)) return;

    // Finisher (se activo, consome o frame)
    if (updateWinFinisher(dt)) return;

    // Input do jogo (pause, movimento, launch, etc.)
    InputSystem::handleGameInput(m_state, input, m_cfg, m_window, dt);

    // Transição PAUSED: SFX + música
    if (modeBefore != m_state.mode) {
        if (m_state.mode == GameMode::PAUSED) {
            m_audio.playSfx("ui/ui_pause_in");
            setMusic("music_pause_loop", 0.18f);
        } else if (modeBefore == GameMode::PAUSED && m_state.mode == GameMode::PLAYING) {
            m_audio.playSfx("ui/ui_pause_out");
        }
    }

    // Som de launch (attached -> livre)
    int attachedAfter = 0;
    for (const auto& b : m_state.balls) if (b.attached) attachedAfter++;
    if (attachedAfter < attachedBefore) {
        m_audio.playSfx("game/ball_launch", -1.5f);
    }

    // Overlays/UI modais
    if (updateRogueCardsOverlay(input)) return;
    if (updatePausedOverlay(input)) return;

    // Timers (cooldowns, auto-spawns, etc.)
    updateTimers(input, dt);

    // WIN (modo NORMAL): quando não há bricks vivos
    if (m_state.mode == GameMode::PLAYING &&
        m_state.gameType == GameType::NORMAL &&
        !InitSystem::anyBricksAlive(m_state)) {
        m_state.winFinisherActive = true;
        m_state.winFinisherTimer = 0.0f;
        m_state.winFinisherRealTimer = 0.0f;
        m_state.winFinisherAnchorValid = m_state.lastBrickDestroyedValid;
        m_state.winFinisherAnchorPos = m_state.lastBrickDestroyedPos;
        m_state.balls.clear();
        m_state.powerups.clear();
        return; // importante: evitar perder vida por "no balls"
    }

    // LEVEL COMPLETE (modo LEVELS)
    if (m_state.mode == GameMode::PLAYING &&
        m_state.gameType == GameType::LEVELS &&
        !InitSystem::anyBricksAlive(m_state)) {

        if (m_state.currentLevel >= m_state.levelsBestLevel) {
            m_state.levelsBestLevel = m_state.currentLevel + 1;
            if (m_state.levelsBestLevel > 20) m_state.levelsBestLevel = 20;
        }

        m_state.winFinisherActive = true;
        m_state.winFinisherTimer = 0.0f;
        m_state.winFinisherRealTimer = 0.0f;
        m_state.winFinisherAnchorValid = m_state.lastBrickDestroyedValid;
        m_state.winFinisherAnchorPos = m_state.lastBrickDestroyedPos;
        m_state.balls.clear();
        m_state.powerups.clear();
        return;
    }

    // End overlay (Game Over / Win)
    if (updateEndOverlay(input)) return;

    // Frame principal (física, colisões, rogue/endless logic, score, música, etc.)
    updatePlayingFrame(
        input, dt,
        modeBefore, typeBefore,
        livesBefore, waveBefore,
        endlessRowsBefore, dangerBefore,
        streakBefore, hadPowerupBefore
    );
}

} // namespace game
