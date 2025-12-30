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
// NOTE: Menu layout calculator lives under game/ui
#include "game/ui/OverlayLayout.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <string>

namespace game {

static std::string endlessBestScorePath() {
    const char* home = std::getenv("HOME");
    if (home && *home) {
        return std::string(home) + "/.breakout3d_endless_best.txt";
    }
    return "breakout3d_endless_best.txt";
}

static void saveEndlessBestScore(int best) {
    std::ofstream f(endlessBestScorePath(), std::ios::trunc);
    if (!f) return;
    f << best;
}

static void commitEndlessStreak(GameState& state) {
    if (state.endlessStreakPoints == 0) return;
    // Bank can be negative (penalties). Clamp score at 0.
    state.score = std::max(0, state.score + state.endlessStreakPoints);
    state.endlessStreakPoints = 0;
    state.endlessStreakPosPoints = 0;
    state.endlessStreakNegPoints = 0;
    state.endlessStreakIdleTimer = 0.0f;
    state.endlessStreakBanking = false;
    state.endlessStreakBankTimer = 0.0f;
}

static void maybeUpdateEndlessBest(GameState& state) {
    if (state.gameType != GameType::ENDLESS) return;
    if (state.score > state.endlessBestScore) {
        state.endlessBestScore = state.score;
        saveEndlessBestScore(state.endlessBestScore);
    }
}

void Game::update(const engine::Input& input) {
    float dt = m_time.delta();

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
    if (m_state.mode == GameMode::MENU) {
        // Keep menu layout synced with the current framebuffer size and real font metrics.
        {
            auto [fbW, fbH] = m_window.getFramebufferSize();
            m_state.menuLayout = ui::calculateMenuLayout(m_renderer, fbW, fbH);
        }
        if (InputSystem::handleMenuInput(m_state, input, m_window)) {
            // If menu handled input and signaled init needed
            if (m_state.mode == GameMode::PLAYING) {
                init();
            }
        }
        return;
    }

    // Normal mode: when the last brick is destroyed, we play a short "camera break" finisher
    // before switching to the WIN overlay.
    if (m_state.winFinisherActive) {
        // Snappy parry-ish feel: tiny hit-stop, brief slow-mo, then ramp back to 1x.
        m_state.winFinisherRealTimer += dt;
        float tr = m_state.winFinisherRealTimer;

        // Keep it subtle: just enough slow-mo to feel "counter-parry", not enough to drag.
        const float hitStop = 0.045f; // seconds (freeze)
        const float slowDur = 0.200f; // seconds (slow)
        const float rampDur = 0.180f; // seconds (ease back to 1x)
        const float slowScale = 0.25f;

        float timeScale = 1.0f;
        if (tr < hitStop) {
            timeScale = 0.0f;
        } else if (tr < hitStop + slowDur) {
            timeScale = slowScale;
        } else if (tr < hitStop + slowDur + rampDur) {
            float u = (tr - (hitStop + slowDur)) / rampDur; // 0..1
            u = std::max(0.0f, std::min(1.0f, u));
            float ease = u * u * (3.0f - 2.0f * u);
            timeScale = slowScale + (1.0f - slowScale) * ease;
        } else {
            timeScale = 1.0f;
        }

        m_state.winFinisherTimer += dt * timeScale;

        const float dur = 1.05f;
        if (m_state.winFinisherTimer >= dur) {
            m_state.winFinisherActive = false;
            m_state.winFinisherTimer = 0.0f;
            m_state.winFinisherRealTimer = 0.0f;
            m_state.winFinisherHoldBrickValid = false;
            m_state.mode = GameMode::WIN;
        }
        return;
    }

    // Handle game input (pause, paddle movement, ball launch, etc.)
    InputSystem::handleGameInput(m_state, input, m_cfg, m_window, dt);

    // ROGUE: card pick UI logic (pause gameplay, click to choose).
    if (m_state.mode == GameMode::ROGUE_CARDS) {
        auto [fbW, fbH] = m_window.getFramebufferSize();
        auto [px, py_raw] = input.mousePosFbPx();
        float py = (float)fbH - py_raw;
        bool click = input.mousePressed(engine::MouseButton::Left);

        const auto L = game::ui::rogueCardOverlay(fbW, fbH);

        // Update hover state
        m_state.hoveredRogueCard = -1;
        m_state.hoveredRogueCardPickButton = -1;

        if (m_state.rogueOfferCount > 0 && L.cardA.contains(px, py)) m_state.hoveredRogueCard = 0;
        else if (m_state.rogueOfferCount > 1 && L.cardB.contains(px, py)) m_state.hoveredRogueCard = 1;
        else if (m_state.rogueOfferCount > 2 && L.cardC.contains(px, py)) m_state.hoveredRogueCard = 2;

        // Check if hover is specifically over the PICK button
        auto checkPickButtonHover = [&](const game::ui::Rect& cardRect, int cardIndex) {
            float pbW = cardRect.w * 0.56f;
            float pbH = 76.0f;
            float pbX = cardRect.x + (cardRect.w - pbW) * 0.5f;
            float pbY = cardRect.y + 26.0f;
            game::ui::Rect pickBtn{pbX, pbY, pbW, pbH};
            if (pickBtn.contains(px, py)) {
                m_state.hoveredRogueCardPickButton = cardIndex;
            }
        };

        if (m_state.hoveredRogueCard == 0) checkPickButtonHover(L.cardA, 0);
        else if (m_state.hoveredRogueCard == 1) checkPickButtonHover(L.cardB, 1);
        else if (m_state.hoveredRogueCard == 2) checkPickButtonHover(L.cardC, 2);

        if (click) {
            if (m_state.hoveredRogueCard >= 0 && m_state.hoveredRogueCard < m_state.rogueOfferCount) {
                game::rogue::applyPickedCard(m_state, m_cfg, m_state.rogueOffer[m_state.hoveredRogueCard]);
                // If we exited the card overlay (i.e., draft is done), start row spawn cadence.
                if (m_state.mode == GameMode::PLAYING) {
                    m_state.rogueRowSpawnTimer = 0.15f;
                }
                return;
            }
        }
        return;
    }

    // PAUSED: Click UI logic for buttons
    if (m_state.mode == GameMode::PAUSED) {
        auto [fbW, fbH] = m_window.getFramebufferSize();
        auto [px, py_raw] = input.mousePosFbPx();
        float py = (float)fbH - py_raw;
        bool click = input.mousePressed(engine::MouseButton::Left);

        // Use larger panel for ROGUE mode
        const auto L = (m_state.gameType == GameType::ROGUE && !m_state.rogueChosen.empty())
            ? game::ui::pauseOverlayRogue(fbW, fbH)
            : game::ui::pauseOverlay(fbW, fbH);

        // Update hover state for buttons
        m_state.hoveredOverlayButton = -1;
        if (L.leftBtn.contains(px, py)) m_state.hoveredOverlayButton = 0;
        else if (L.rightBtn.contains(px, py)) m_state.hoveredOverlayButton = 1;

        // Update hover state for Rogue cards (if in ROGUE mode)
        m_state.hoveredPauseRogueCard = -1;
        if (m_state.gameType == GameType::ROGUE && !m_state.rogueChosen.empty()) {
            float panelX = L.panel.x;
            float panelY = L.panel.y;
            float panelH = L.panel.h;

            // Badge grid positioning (must match rendering in PauseEndOverlays.cpp)
            float titleH = 22.0f;
            float titleY = panelY + panelH - titleH - 165.0f;

            float badgeStartY = titleY - 80.0f;  // More space below title
            float badgeBaseW = 50.0f;
            float badgeBaseH = 50.0f;
            float badgeGap = 10.0f;  // 10px spacing between columns
            float badgeRowGap = 80.0f;  // Increased spacing between rows
            float badgesPerRow = 10;  // 10 columns for 2 rows layout

            // Calculate starting X
            float totalWidth = badgesPerRow * badgeBaseW + (badgesPerRow - 1) * badgeGap;
            float badgeStartX = panelX + (L.panel.w - totalWidth) * 0.5f;

            int rowIdx = 0;
            int colIdx = 0;
            int cardCount = 0;

            for (size_t i = 0; i < m_state.rogueChosen.size() && cardCount < 20; ++i) {
                // Badge position (fixed, doesn't move on hover)
                float badgeX = badgeStartX + colIdx * (badgeBaseW + badgeGap);
                float badgeY = badgeStartY - rowIdx * badgeRowGap;  // Use badgeRowGap for more spacing
                float badgeW = badgeBaseW;
                float badgeH = badgeBaseH;

                // Check hit
                if (px >= badgeX && px <= badgeX + badgeW &&
                    py >= badgeY && py <= badgeY + badgeH) {
                    m_state.hoveredPauseRogueCard = (int)i;
                    break;
                }

                colIdx++;
                if (colIdx >= (int)badgesPerRow) {
                    colIdx = 0;
                    rowIdx++;
                }
                cardCount++;
            }
        }

        if (click) {
            // Left button: Restart
            if (L.leftBtn.contains(px, py)) {
                init();
                return;
            }
            // Right button: Back to Menu
            if (L.rightBtn.contains(px, py)) {
                m_state.mode = GameMode::MENU;
                m_state.currentMenuScreen = MenuScreen::MAIN;
                m_state.showInstructions = false;
                return;
            }
        }
        return;
    }

#ifdef BREAKOUT3D_DEBUG
    // Debug key for endless mode
    if (m_state.gameType == GameType::ENDLESS && input.keyPressed(engine::Key::K3)) {
        InitSystem::spawnIncrementalBricks(m_state, m_cfg, 12, m_state.wave);
        m_state.pendingSpawnBricks = 0;
        m_state.endlessSpawnCooldown = 0.5f;
        m_state.endlessAutoTimer = 0.0f;
    }
#endif

    // Update timers
    if (m_state.brickHitCooldown > 0.0f)
        m_state.brickHitCooldown = std::max(0.0f, m_state.brickHitCooldown - dt);
    if (m_state.endlessSpawnCooldown > 0.0f)
        m_state.endlessSpawnCooldown = std::max(0.0f, m_state.endlessSpawnCooldown - dt);
    if (m_state.gameType == GameType::ENDLESS) {
        m_state.endlessAutoTimer += dt;
        if (m_state.mode == GameMode::PLAYING) {
            m_state.endlessElapsedTime += dt;
        }
    }
    if (m_state.gameType == GameType::ROGUE) {
        if (m_state.rogueWaveCooldown > 0.0f) {
            m_state.rogueWaveCooldown = std::max(0.0f, m_state.rogueWaveCooldown - dt);
        }
        if (m_state.rogueRowSpawnTimer > 0.0f) {
            m_state.rogueRowSpawnTimer = std::max(0.0f, m_state.rogueRowSpawnTimer - dt);
        }
        if (m_state.mode == GameMode::PLAYING) {
            m_state.rogueWaveTimer += dt;
        }
    }
    if (m_state.expandTimer > 0.0f) m_state.expandTimer = std::max(0.0f, m_state.expandTimer - dt);
    if (m_state.slowTimer > 0.0f) m_state.slowTimer = std::max(0.0f, m_state.slowTimer - dt);
    if (m_state.fireballTimer > 0.0f) m_state.fireballTimer = std::max(0.0f, m_state.fireballTimer - dt);
    if (m_state.shieldTimer > 0.0f) m_state.shieldTimer = std::max(0.0f, m_state.shieldTimer - dt);
    if (m_state.reverseTimer > 0.0f) m_state.reverseTimer = std::max(0.0f, m_state.reverseTimer - dt);
    if (m_state.tinyTimer > 0.0f) m_state.tinyTimer = std::max(0.0f, m_state.tinyTimer - dt);

    // Fireball explosion FX timers
    if (!m_state.fireballExplosions.empty()) {
        for (size_t i = 0; i < m_state.fireballExplosions.size(); ) {
            m_state.fireballExplosions[i].t += dt;
            if (m_state.fireballExplosions[i].t >= m_cfg.fireballExplosionFxDuration) {
                m_state.fireballExplosions.erase(m_state.fireballExplosions.begin() + i);
            } else {
                ++i;
            }
        }
    }

    // Fireball shake timer
    if (m_state.fireballShakeTimer > 0.0f) {
        m_state.fireballShakeTimer = std::max(0.0f, m_state.fireballShakeTimer - dt);
    }

    // Fireball shard simulation (cheap debris)
    if (!m_state.fireballShards.empty()) {
        const float g = 12.0f;
        for (size_t i = 0; i < m_state.fireballShards.size(); ) {
            auto& s = m_state.fireballShards[i];
            s.t += dt;
            // simple drag on XZ
            float drag = m_cfg.fireballShardDrag;
            s.vel.x *= std::exp(-drag * dt);
            s.vel.z *= std::exp(-drag * dt);
            s.vel.y -= g * dt;
            s.pos += s.vel * dt;
            if (s.t >= m_cfg.fireballShardLife || s.pos.y < -0.25f) {
                m_state.fireballShards.erase(m_state.fireballShards.begin() + i);
            } else {
                ++i;
            }
        }
    }

    // Score popup timers (penalties, etc.)
    if (!m_state.scorePopups.empty()) {
        // Keep longer so penalty popups never feel "instant vanish".
        const float popupDur = 2.10f;
        for (size_t i = 0; i < m_state.scorePopups.size(); ) {
            m_state.scorePopups[i].t += dt;
            if (m_state.scorePopups[i].t >= popupDur) {
                m_state.scorePopups.erase(m_state.scorePopups.begin() + i);
            } else {
                ++i;
            }
        }
    }

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

    // GAME OVER / WIN: Click UI logic
    if (m_state.mode == GameMode::GAME_OVER || m_state.mode == GameMode::WIN) {
        auto [fbW, fbH] = m_window.getFramebufferSize();
        auto [px, py_raw] = input.mousePosFbPx();
        float py = (float)fbH - py_raw;
        bool click = input.mousePressed(engine::MouseButton::Left);

        const auto L = game::ui::endOverlay(fbW, fbH);

        // Update hover state
        m_state.hoveredOverlayButton = -1;
        if (L.leftBtn.contains(px, py)) m_state.hoveredOverlayButton = 0;
        else if (L.rightBtn.contains(px, py)) m_state.hoveredOverlayButton = 1;

        if (click) {
            // Left button: Restart (for LEVELS mode, restart same level)
            if (L.leftBtn.contains(px, py)) {
                if (m_state.gameType == GameType::LEVELS) {
                    // Restart the same level (keep currentLevel)
                    init();
                } else {
                    init();
                }
                return;
            }
            // Right button: Back to Menu
            if (L.rightBtn.contains(px, py)) {
                m_state.mode = GameMode::MENU;
                // LEVELS mode: return to level select
                if (m_state.gameType == GameType::LEVELS) {
                    m_state.currentMenuScreen = MenuScreen::LEVEL_SELECT;
                } else {
                    m_state.currentMenuScreen = MenuScreen::MAIN;
                }
                m_state.showInstructions = false;
                return;
            }
        }
        return;
    }

    // =========== PLAYING STATE ===========

    // Update physics
    PhysicsSystem::updateBalls(m_state, m_cfg, dt);

    // Calculate current paddle size for collisions
    glm::vec3 currentPaddleSize = m_cfg.paddleSize;
    if (m_state.gameType == GameType::ROGUE) currentPaddleSize.x *= game::rogue::basePaddleScaleX(m_state);
    if (m_state.expandTimer > 0.0f) currentPaddleSize.x *= m_cfg.expandScaleFactor;
    if (m_state.tinyTimer > 0.0f) currentPaddleSize.x *= m_cfg.tinyScaleFactor;

    // Handle collisions for each ball
    for (auto& ball : m_state.balls) {
        if (ball.attached) continue;

        // World collisions (walls)
        CollisionSystem::handleWorldCollisions(ball, m_cfg);

        // Paddle collision
        CollisionSystem::handlePaddleCollision(ball, m_state, m_state.paddlePos, currentPaddleSize, m_cfg);

        // Brick collisions
        CollisionSystem::handleBrickCollisions(ball, m_state, m_cfg);
    }

    // Remove balls that were killed by one-shot effects (e.g., Fireball).
    if (!m_state.balls.empty()) {
        m_state.balls.erase(
            std::remove_if(
                m_state.balls.begin(),
                m_state.balls.end(),
                [](const Ball& b) { return !b.alive; }
            ),
            m_state.balls.end()
        );
    }

    // If a one-shot fireball was consumed, respawn a normal attached ball
    // without costing a life.
    if (m_state.pendingRespawnAfterFireball && m_state.balls.empty() && !m_state.winFinisherActive) {
        Ball b;
        b.isFireball = false;
        PhysicsSystem::resetBallToPaddle(b, m_state.paddlePos, m_cfg);
        m_state.balls.push_back(b);
        m_state.pendingRespawnAfterFireball = false;
    }

    // NOTE: Rogue wave progression is handled later (quota/time), not by clearing all bricks.

    // Endless/Rogue score streak logic:
    // Points accumulate into `endlessStreakPoints` as bricks are destroyed.
    // If no new points arrive for ~2.5s, we play a short "banking" animation,
    // then commit the streak into the main score.
    if ((m_state.gameType == GameType::ENDLESS || m_state.gameType == GameType::ROGUE) && m_state.mode == GameMode::PLAYING) {
        float idleToBank = 2.5f;   // ~2-3 seconds
        if (m_state.gameType == GameType::ROGUE) {
            idleToBank *= std::max(0.35f, m_state.rogueBankIdleMult);
        }
        const float bankAnim = 0.55f;    // short float-up

        if (m_state.endlessStreakPoints != 0) {
            if (!m_state.endlessStreakBanking) {
                m_state.endlessStreakIdleTimer += dt;
                if (m_state.endlessStreakIdleTimer >= idleToBank) {
                    m_state.endlessStreakBanking = true;
                    m_state.endlessStreakBankTimer = 0.0f;
                }
            } else {
                m_state.endlessStreakBankTimer += dt;
                if (m_state.endlessStreakBankTimer >= bankAnim) {
                    commitEndlessStreak(m_state);
                    maybeUpdateEndlessBest(m_state);
                    RogueSystem::maybeUpdateRogueBest(m_state);
                }
            }
        }
    }

    // Rogue: wave progression is NOT "clear all bricks".
    // We advance when either:
    // - enough bricks were broken this wave (quota), OR
    // - enough time has elapsed (time limit),
    // gated by a small cooldown to prevent double-advances.
    if (m_state.gameType == GameType::ROGUE && m_state.mode == GameMode::PLAYING && m_state.rogueWaveCooldown <= 0.0f) {
        int required = RogueSystem::bricksRequiredForWave(m_state.wave);
        float minT = RogueSystem::minTimeForWave(m_state.wave);
        float limit = RogueSystem::timeLimitForWave(m_state.wave);
        bool quotaMet = (m_state.rogueWaveTimer >= minT) && (m_state.rogueBricksBrokenThisWave >= required);
        bool timeMet = (m_state.rogueWaveTimer >= limit);

        if (quotaMet || timeMet) {
            int clearedWave = std::max(1, m_state.wave);
            // Make sure banked points count before we move on.
            commitEndlessStreak(m_state);
            RogueSystem::maybeUpdateRogueBest(m_state);

            if (RogueSystem::shouldWinAfterClearingWave(m_state, clearedWave)) {
                m_state.mode = GameMode::WIN;
                m_state.balls.clear();
                m_state.powerups.clear();
                return;
            }

            m_state.wave = clearedWave + 1;
            RogueSystem::onWaveAdvanced(m_state);

            // Next wave: queue rows to insert. They will spawn AFTER the player picks a card,
            // gradually, not all at once.
            m_state.roguePendingRowsToSpawn = RogueSystem::rowsToInsertForWave(m_state, m_state.wave);

            if (RogueSystem::shouldOfferCardsAfterClearingWave(m_state, clearedWave)) {
                // OP pack on the START of waves 3/6/9/... (i.e. after clearing 2/5/8/...)
                int nextWave = std::max(1, clearedWave + 1);
                bool opPack = (nextWave % 3) == 0 && nextWave >= 3;
                game::rogue::dealOffer(m_state, 3, opPack, nextWave);
                m_state.mode = GameMode::ROGUE_CARDS;
                m_state.hoveredRogueCard = -1;
                m_state.hoveredRogueCardPickButton = -1;
            }
            return;
        }
    }

    // Rogue: after a card pick, insert queued rows gradually (one row per interval).
    if (m_state.gameType == GameType::ROGUE && m_state.mode == GameMode::PLAYING && m_state.roguePendingRowsToSpawn > 0) {
        if (m_state.rogueRowSpawnTimer <= 0.0f) {
            RogueSystem::spawnWaveRows(m_state, m_cfg, /*rowsToInsert=*/1);
            m_state.roguePendingRowsToSpawn--;
            m_state.rogueRowSpawnTimer = m_state.rogueRowSpawnInterval;
        }
    }

    // Perform queued brick spawns (endless mode)
    if (m_state.gameType == GameType::ENDLESS && m_state.pendingSpawnBricks > 0) {
        InitSystem::spawnIncrementalBricks(m_state, m_cfg, m_state.pendingSpawnBricks, m_state.wave);
        m_state.pendingSpawnBricks = 0;
    }

    // Endless mode: time-based pressure (rows spawn over time, but ramps up gently).
    if (m_state.gameType == GameType::ENDLESS && m_state.mode == GameMode::PLAYING && m_state.endlessSpawnCooldown <= 0.0f) {
        const float t = m_state.endlessElapsedTime;

        // Grace period: let the player stabilize first.
        const float grace = 60.0f;
        if (t < grace) {
            m_state.endlessAutoTimer = 0.0f;
        } else {
            // After grace, interval slowly decreases over time:
            // ~26s (just after grace) down to ~12s by ~10 minutes, clamped.
            float u = std::min(1.0f, (t - grace) / 540.0f); // 0..1 over 9 minutes
            float interval = 26.0f - (14.0f * u);
            interval = std::max(12.0f, interval);

            // Avoid stacking multiple rows at once.
            if (m_state.pendingSpawnBricks < 12 && m_state.endlessAutoTimer >= interval) {
                m_state.pendingSpawnBricks += 12;
                m_state.endlessSpawnCooldown = 0.50f;
                m_state.endlessAutoTimer = 0.0f;
            }
        }
    }

    // Endless mode: check for lose condition (brick reached paddle)
    if (m_state.gameType == GameType::ENDLESS) {
        float limitZ = m_state.paddlePos.z - 0.5f;
        float warningThresholdZ = limitZ - (1.33f * 3.0f);

        float maxZFound = -20.0f;
        bool anyBricksAliveInEndless = false;

        for (const auto& br : m_state.bricks) {
            if (!br.alive) continue;
            anyBricksAliveInEndless = true;
            float brMaxZ = br.pos.z + br.size.z * 0.5f;
            if (brMaxZ > maxZFound) maxZFound = brMaxZ;

            if (brMaxZ >= limitZ) {
                // About to lose: commit any pending streak so it counts.
                commitEndlessStreak(m_state);
                maybeUpdateEndlessBest(m_state);
                m_state.mode = GameMode::GAME_OVER;
                m_state.balls.clear();
                break;
            }
        }

        if (m_state.mode != GameMode::GAME_OVER) {
            m_state.endlessDangerActive = (anyBricksAliveInEndless && maxZFound >= warningThresholdZ);
            if (m_state.endlessDangerActive) {
                m_state.endlessDangerTimer += dt;
                m_state.endlessDangerMaxZ = maxZFound;
            } else {
                m_state.endlessDangerTimer = 0.0f;
                m_state.endlessDangerMaxZ = -20.0f;
            }
        }
    }

    // Rogue mode: danger warning + brick-reached-paddle lose condition (same visual language as Endless).
    if (m_state.gameType == GameType::ROGUE) {
        float limitZ = m_state.paddlePos.z - 0.5f;
        float warningThresholdZ = limitZ - (1.33f * 3.0f);

        float maxZFound = -20.0f;
        bool anyAlive = false;

        for (const auto& br : m_state.bricks) {
            if (!br.alive) continue;
            anyAlive = true;
            float brMaxZ = br.pos.z + br.size.z * 0.5f;
            if (brMaxZ > maxZFound) maxZFound = brMaxZ;

            if (brMaxZ >= limitZ) {
                // About to lose: commit any pending streak so it counts.
                commitEndlessStreak(m_state);
                RogueSystem::maybeUpdateRogueBest(m_state);
                m_state.mode = GameMode::GAME_OVER;
                m_state.balls.clear();
                break;
            }
        }

        if (m_state.mode != GameMode::GAME_OVER) {
            m_state.endlessDangerActive = (anyAlive && maxZFound >= warningThresholdZ);
            if (m_state.endlessDangerActive) {
                m_state.endlessDangerTimer += dt;
                m_state.endlessDangerMaxZ = maxZFound;
            } else {
                m_state.endlessDangerTimer = 0.0f;
                m_state.endlessDangerMaxZ = -20.0f;
            }
        }
    }

    // Update power-ups
    PowerUpSystem::updatePowerUps(m_state, m_cfg, dt);

    // Handle lives loss
    if (!m_state.winFinisherActive && m_state.balls.empty()) {
        m_state.lives--;
        // Losing a heart = -points (but Fireball one-shot respawn does NOT reach this path).
        int lossPenalty = m_cfg.lifeLossPenalty;
        if (m_state.gameType == GameType::ROGUE) {
            lossPenalty += m_state.rogueLifeLossPenaltyBonus;
            if (lossPenalty < 0) lossPenalty = 0;
        }
        if (lossPenalty > 0) {
            if (m_state.gameType == GameType::ENDLESS) {
                // Same "bank" logic as the white streak popup:
                // deduct into the bank now, commit later.
                m_state.endlessStreakPoints -= lossPenalty;
                m_state.endlessStreakNegPoints += lossPenalty;
                m_state.endlessStreakIdleTimer = 0.0f;
                m_state.endlessStreakBanking = false;
                m_state.endlessStreakBankTimer = 0.0f;
            } else if (m_state.gameType == GameType::ROGUE) {
                m_state.endlessStreakPoints -= lossPenalty;
                m_state.endlessStreakNegPoints += lossPenalty;
                m_state.endlessStreakIdleTimer = 0.0f;
                m_state.endlessStreakBanking = false;
                m_state.endlessStreakBankTimer = 0.0f;
            } else {
                m_state.score = std::max(0, m_state.score - lossPenalty);
                // Normal mode: don't show score popups.
            }
        }
        if (m_state.lives > 0) {
            Ball b;
            PhysicsSystem::resetBallToPaddle(b, m_state.paddlePos, m_cfg);
            m_state.balls.push_back(b);
        } else {
            if (m_state.gameType == GameType::ENDLESS) {
                commitEndlessStreak(m_state);
                maybeUpdateEndlessBest(m_state);
            }
            if (m_state.gameType == GameType::ROGUE) {
                commitEndlessStreak(m_state);
                RogueSystem::maybeUpdateRogueBest(m_state);
            }
            m_state.mode = InitSystem::anyBricksAlive(m_state) ? GameMode::GAME_OVER : GameMode::WIN;
        }
    }
}

} // namespace game


