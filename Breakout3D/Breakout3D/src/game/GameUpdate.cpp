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

void Game::update(const engine::Input& input) {
    float dt = m_time.delta();

    // Tick audio (fades, loop gates). Audio may be disabled (init failure), that's OK.
    m_audio.update(dt);

    // Live-apply audio settings from the menu sliders.
    if (m_audio.isEnabled()) {
        m_audio.setMasterVolume(m_state.audioMasterVol);
        m_audio.setSfxVolume(m_state.audioSfxVol);
        m_audio.setMusicVolume(m_state.audioMusicVol);
        m_audio.setStingerVolume(m_state.audioStingerVol);
    }

    // Snapshot for diff-based audio triggers (we keep this lightweight; most events are inferred from state changes)
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

    // Preload powerup GIF previews asynchronously while the game is running,
    // so opening the Powerups inspector never stalls.
    m_assets.startPowerupVideoPreload();
    m_assets.pumpPowerupVideoPreload(6);

#ifdef BREAKOUT3D_DEBUG
    // Debug: spawn specific powerups (so you're not waiting on RNG).
    // These use TOP-ROW keys (not numpad).
    if (m_state.mode == GameMode::PLAYING) {
        auto spawnDebugDrop = [&](PowerUpType type) {
            PowerUp p;
            p.type = type;
            // Spawn near the brick/top side so it "drops" across the arena like normal drops.
            // Bricks start at cfg.arenaMinZ + 0.85f (see InitSystem.cpp).
            p.pos = glm::vec3(m_state.paddlePos.x, 0.4f, m_cfg.arenaMinZ + 0.85f);
            m_state.powerups.push_back(p);
        };

        // PowerUpType binds (top-row digits + '-' key)
        if (input.keyPressed(engine::Key::K8)) spawnDebugDrop(PowerUpType::EXPAND);
        if (input.keyPressed(engine::Key::K9)) spawnDebugDrop(PowerUpType::EXTRA_BALL);
        if (input.keyPressed(engine::Key::K0)) spawnDebugDrop(PowerUpType::EXTRA_LIFE);
        if (input.keyPressed(engine::Key::Minus)) spawnDebugDrop(PowerUpType::SLOW);

        if (input.keyPressed(engine::Key::K4)) spawnDebugDrop(PowerUpType::TINY);
        if (input.keyPressed(engine::Key::K5)) spawnDebugDrop(PowerUpType::FIREBALL);
        if (input.keyPressed(engine::Key::K6)) spawnDebugDrop(PowerUpType::SHIELD);
        if (input.keyPressed(engine::Key::K7)) spawnDebugDrop(PowerUpType::REVERSE);
    }

    // Debug: fill Rogue cards (press R in ROGUE mode to max out cards)
    if (m_state.gameType == GameType::ROGUE && input.keyPressed(engine::Key::R)) {
        // Fill with all available cards up to max 20
        m_state.rogueChosen.clear();

        // Add all cards from the enum
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

    // =========== MENU LOGIC ===========
    if (updateMenu(input)) return;

    if (updateWinFinisher(dt)) return;

    // Handle game input (pause, paddle movement, ball launch, etc.)
    InputSystem::handleGameInput(m_state, input, m_cfg, m_window, dt);

    // Pause toggle audio + music
    if (modeBefore != m_state.mode) {
        if (m_state.mode == GameMode::PAUSED) {
            m_audio.playSfx("ui/ui_pause_in");
            setMusic("music_pause_loop", 0.18f);
        } else if (modeBefore == GameMode::PAUSED && m_state.mode == GameMode::PLAYING) {
            m_audio.playSfx("ui/ui_pause_out");
        }
    }

    // Ball launch sound (attached -> free)
    int attachedAfter = 0;
    for (const auto& b : m_state.balls) if (b.attached) attachedAfter++;
    if (attachedAfter < attachedBefore) {
        m_audio.playSfx("game/ball_launch", -1.5f);
    }

    if (updateRogueCardsOverlay(input)) return;
    if (updatePausedOverlay(input)) return;

    updateTimers(input, dt);

    // Check for WIN condition (normal mode only)
    if (m_state.mode == GameMode::PLAYING && m_state.gameType == GameType::NORMAL && !InitSystem::anyBricksAlive(m_state)) {
        // Trigger finisher cinematic (delays WIN overlay).
        m_state.winFinisherActive = true;
        m_state.winFinisherTimer = 0.0f;
        m_state.winFinisherRealTimer = 0.0f;
        m_state.winFinisherAnchorValid = m_state.lastBrickDestroyedValid;
        m_state.winFinisherAnchorPos = m_state.lastBrickDestroyedPos;
        m_state.balls.clear();
        m_state.powerups.clear();
        // IMPORTANT: don't run the rest of the frame (otherwise "no balls" would cost a life).
        return;
    }

    // Check for LEVEL COMPLETE condition (LEVELS mode only)
    if (m_state.mode == GameMode::PLAYING && m_state.gameType == GameType::LEVELS && !InitSystem::anyBricksAlive(m_state)) {
        // Level completed! Unlock next level if needed
        if (m_state.currentLevel >= m_state.levelsBestLevel) {
            m_state.levelsBestLevel = m_state.currentLevel + 1;
            if (m_state.levelsBestLevel > 20) m_state.levelsBestLevel = 20;
        }

        // Trigger finisher cinematic (delays WIN overlay)
        m_state.winFinisherActive = true;
        m_state.winFinisherTimer = 0.0f;
        m_state.winFinisherRealTimer = 0.0f;
        m_state.winFinisherAnchorValid = m_state.lastBrickDestroyedValid;
        m_state.winFinisherAnchorPos = m_state.lastBrickDestroyedPos;
        m_state.balls.clear();
        m_state.powerups.clear();
        // IMPORTANT: don't run the rest of the frame (otherwise "no balls" would cost a life).
        return;
    }

    if (updateEndOverlay(input)) return;

    updatePlayingFrame(input, dt, modeBefore, typeBefore, livesBefore, waveBefore, endlessRowsBefore, dangerBefore, streakBefore, hadPowerupBefore);
    return;
}

} // namespace game


