#include "game/Game.hpp"
#include "engine/Input.hpp"
#include "game/GameAssets.hpp"
#include "game/systems/InitSystem.hpp"
#include "game/systems/InputSystem.hpp"
#include "game/systems/PhysicsSystem.hpp"
#include "game/systems/CollisionSystem.hpp"
#include "game/systems/PowerUpSystem.hpp"
#include "game/effects/WinFinisher.hpp"
#include "game/render/RenderContext.hpp"
#include "game/render/UIRender.hpp"
#include "game/render/WorldRender.hpp"
#include "game/ui/OverlayLayout.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <string>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

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

// UI text now uses a real TTF font (stb_truetype) via Renderer::drawUIText.

Game::Game(engine::Window& window, engine::Time& time, engine::Renderer& renderer, GameAssets& assets)
: m_window(window), m_time(time), m_renderer(renderer), m_assets(assets) {}

void Game::init() {
    InitSystem::initGame(m_state, m_cfg);
}

void Game::update(const engine::Input& input) {
    float dt = m_time.delta();

#ifdef BREAKOUT3D_DEBUG
    // Debug: spawn specific powerups at the paddle (so you're not waiting on RNG).
    // K4: TINY, K5: FIREBALL, K6: SHIELD, K7: REVERSE
    if (m_state.mode == GameMode::PLAYING) {
        if (input.keyPressed(engine::Key::K4)) {
            PowerUp p; p.type = PowerUpType::TINY; p.pos = m_state.paddlePos + glm::vec3(0, 0.4f, -2.0f);
            m_state.powerups.push_back(p);
        }
        if (input.keyPressed(engine::Key::K5)) {
            PowerUp p; p.type = PowerUpType::FIREBALL; p.pos = m_state.paddlePos + glm::vec3(0, 0.4f, -2.0f);
            m_state.powerups.push_back(p);
        }
        if (input.keyPressed(engine::Key::K6)) {
            PowerUp p; p.type = PowerUpType::SHIELD; p.pos = m_state.paddlePos + glm::vec3(0, 0.4f, -2.0f);
            m_state.powerups.push_back(p);
        }
        if (input.keyPressed(engine::Key::K7)) {
            PowerUp p; p.type = PowerUpType::REVERSE; p.pos = m_state.paddlePos + glm::vec3(0, 0.4f, -2.0f);
            m_state.powerups.push_back(p);
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

    // PAUSED: Click UI logic for buttons
    if (m_state.mode == GameMode::PAUSED) {
        auto [fbW, fbH] = m_window.getFramebufferSize();
        auto [px, py_raw] = input.mousePosFbPx();
        float py = (float)fbH - py_raw;
        bool click = input.mousePressed(engine::MouseButton::Left);

        const auto L = game::ui::pauseOverlay(fbW, fbH);

        // Update hover state
        m_state.hoveredOverlayButton = -1;
        if (L.leftBtn.contains(px, py)) m_state.hoveredOverlayButton = 0;
        else if (L.rightBtn.contains(px, py)) m_state.hoveredOverlayButton = 1;

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

    // =========== PLAYING STATE ===========
    
    // Update physics
    PhysicsSystem::updateBalls(m_state, m_cfg, dt);

    // Calculate current paddle size for collisions
    glm::vec3 currentPaddleSize = m_cfg.paddleSize;
    if (m_state.expandTimer > 0.0f) currentPaddleSize.x *= m_cfg.expandScaleFactor;
    if (m_state.tinyTimer > 0.0f) currentPaddleSize.x *= m_cfg.tinyScaleFactor;

    // Handle collisions for each ball
    for (auto& ball : m_state.balls) {
        if (ball.attached) continue;

        // World collisions (walls)
        CollisionSystem::handleWorldCollisions(ball, m_cfg);

        // Paddle collision
        CollisionSystem::handlePaddleCollision(ball, m_state.paddlePos, currentPaddleSize, m_cfg);

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

    // Endless score streak logic:
    // Points accumulate into `endlessStreakPoints` as bricks are destroyed.
    // If no new points arrive for ~2.5s, we play a short "banking" animation,
    // then commit the streak into the main score.
    if (m_state.gameType == GameType::ENDLESS && m_state.mode == GameMode::PLAYING) {
        const float idleToBank = 2.5f;   // ~2-3 seconds
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
                }
            }
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

    // Update power-ups
    PowerUpSystem::updatePowerUps(m_state, m_cfg, dt);

    // Handle lives loss
    if (!m_state.winFinisherActive && m_state.balls.empty()) {
        m_state.lives--;
        // Losing a heart = -points (but Fireball one-shot respawn does NOT reach this path).
        if (m_cfg.lifeLossPenalty > 0) {
            if (m_state.gameType == GameType::ENDLESS) {
                // Same "bank" logic as the white streak popup:
                // deduct into the bank now, commit later.
                m_state.endlessStreakPoints -= m_cfg.lifeLossPenalty;
                m_state.endlessStreakNegPoints += m_cfg.lifeLossPenalty;
                m_state.endlessStreakIdleTimer = 0.0f;
                m_state.endlessStreakBanking = false;
                m_state.endlessStreakBankTimer = 0.0f;
            } else {
                m_state.score = std::max(0, m_state.score - m_cfg.lifeLossPenalty);
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
            m_state.mode = InitSystem::anyBricksAlive(m_state) ? GameMode::GAME_OVER : GameMode::WIN;
        }
    }
}

void Game::render() {
    auto [fbW, fbH] = m_window.getFramebufferSize();
    m_renderer.beginFrame(fbW, fbH);

    // =========== MENU RENDER ===========
    if (m_state.mode == GameMode::MENU) {
        // Defensive: ensure layout is available even if render runs before first update.
        m_state.menuLayout = ui::calculateMenuLayout(m_renderer, fbW, fbH);
        game::render::RenderContext ctx{fbW, fbH, m_time, m_renderer};
        game::render::renderMenu(ctx, m_state, m_assets);
        m_window.swapBuffers();
        return;
    }

    if (m_state.currentBg != -1) {
        m_renderer.drawBackground(m_assets.backgroundTexs[m_state.currentBg].id);
    }

    // -------- 3D PASS (camera setup) --------
    float arenaW = (m_cfg.arenaMaxX - m_cfg.arenaMinX);
    float arenaD = (m_cfg.arenaMaxZ - m_cfg.arenaMinZ);
    float base   = std::max(arenaW, arenaD);
    float zMid   = (m_cfg.arenaMinZ + m_cfg.arenaMaxZ) * 0.5f;

    glm::vec3 camPos, camTarget;
    float fov = 45.0f;

    if (m_state.cameraMode == 1) {
        // Mode 1: Top-down centric - Slightly Zoomed Out
        camPos    = glm::vec3(0.0f, base * 1.03f, zMid + 0.5f);
        camTarget = glm::vec3(0.0f, 0.0f, zMid);
        fov       = 45.0f;
    } else {
        // Mode 2: Angled View - Ultra-tight Zoom
        camPos    = glm::vec3(0.0f, base * 0.62f, base * 0.82f);
        camTarget = glm::vec3(0.0f, 0.0f, 0.8f);
        fov       = 45.0f;
    }

    // --- WIN FINISHER CAMERA (pure math lives in game/effects) ---
    float rollRad = 0.0f;
    if (m_state.winFinisherActive) {
        auto cam = game::effects::computeWinFinisherCamera(
            camPos, camTarget, fov, base,
            m_state.winFinisherTimer,
            m_state.winFinisherRealTimer
        );
        camPos = cam.camPos;
        camTarget = cam.camTarget;
        fov = cam.fovDeg;
        rollRad = cam.rollRad;
    }

    // Fireball shake (subtle camera jitter on impact)
    if (m_state.fireballShakeTimer > 0.0f && m_cfg.fireballShakeDuration > 1e-4f) {
        float u = m_state.fireballShakeTimer / m_cfg.fireballShakeDuration; // 1..0
        u = std::max(0.0f, std::min(1.0f, u));
        // ease-out
        float e = u * u;
        float amp = m_cfg.fireballShakeStrength * e;
        float t = m_time.now();
        float ox = std::sin(t * 44.0f) * amp;
        float oy = std::sin(t * 57.0f + 1.7f) * amp * 0.65f;
        camPos += glm::vec3(ox, oy, 0.0f);
        camTarget += glm::vec3(ox * 0.55f, oy * 0.55f, 0.0f);
    }

    glm::mat4 V = glm::lookAt(camPos, camTarget, glm::vec3(0,1,0));
    if (rollRad != 0.0f) {
        V = glm::rotate(glm::mat4(1.0f), rollRad, glm::vec3(0, 0, 1)) * V;
    }

    glm::mat4 P = glm::perspective(glm::radians(fov), (float)fbW/(float)fbH, 0.1f, 300.0f);
    m_renderer.setCamera(V, P, camPos);

    // Precompute (in 3D camera space) the screen-space Y of the "danger line"
    // so the UI pass can apply a filter only from that line downwards.
    // User requested a clean rectangular band (straight line), so we use a single Y.
    float dangerLineScreenY = -1.0f; // UI pixels, origin at bottom
    if (m_state.gameType == GameType::ENDLESS && m_state.endlessDangerActive) {
        // NOTE: brick row step in InitSystem is ~1.33 (brickSize.z 1.30 + gapZ ~0.03).
        // Using +1.33f starts the filter one row BELOW the bricks. We want it to cover the next row up,
        // so we project the actual front edge of the bricks (no extra offset).
        glm::vec3 worldPos(0.0f, 0.0f, m_state.endlessDangerMaxZ);
        glm::vec4 clip = P * V * glm::vec4(worldPos, 1.0f);
        if (std::abs(clip.w) > 1e-6f) {
            float ndcY = clip.y / clip.w; // -1..1
            dangerLineScreenY = (ndcY * 0.5f + 0.5f) * (float)fbH;
            dangerLineScreenY = std::max(0.0f, std::min((float)fbH, dangerLineScreenY));
        }
    }

    game::render::RenderContext ctx{fbW, fbH, m_time, m_renderer};
    ctx.V = V;
    ctx.P = P;
    ctx.camPos = camPos;
    ctx.dangerLineScreenY = dangerLineScreenY;

    game::render::renderWorld(ctx, m_state, m_cfg, m_assets);
    game::render::renderUI(ctx, m_state, m_cfg, m_assets);
    m_window.swapBuffers();
}

} // namespace game
