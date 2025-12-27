#include "game/Game.hpp"
#include "engine/Input.hpp"
#include "game/GameAssets.hpp"
#include "game/systems/InitSystem.hpp"
#include "game/systems/InputSystem.hpp"
#include "game/systems/PhysicsSystem.hpp"
#include "game/systems/CollisionSystem.hpp"
#include "game/systems/PowerUpSystem.hpp"
#include "game/effects/WinFinisher.hpp"
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

    // =========== MENU LOGIC ===========
    if (m_state.mode == GameMode::MENU) {
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

        // Calculate button positions (matching render layout)
        float panelW = 450.0f;
        float panelH = 280.0f; // Taller to fit PAUSED text + buttons
        float panelX = (fbW - panelW) * 0.5f;
        float panelY = (fbH - panelH) * 0.5f;
        
        float btnW = 140.0f;
        float btnH = 60.0f;
        float btnGap = 50.0f;
        float btnX_left = panelX + (panelW - 2*btnW - btnGap) * 0.5f;
        float btnX_right = btnX_left + btnW + btnGap;
        float btnY = panelY + 40.0f; // Position at bottom of panel

        if (click) {
            // Left button: Restart
            if (px >= btnX_left && px <= btnX_left + btnW && py >= btnY && py <= btnY + btnH) {
                init();
                return;
            }
            // Right button: Back to Menu
            if (px >= btnX_right && px <= btnX_right + btnW && py >= btnY && py <= btnY + btnH) {
                m_state.mode = GameMode::MENU;
                m_state.showInstructions = false;
                return;
            }
        }
        return;
    }

    // Debug key for endless mode
    if (m_state.gameType == GameType::ENDLESS && input.keyPressed(engine::Key::K3)) {
        InitSystem::spawnIncrementalBricks(m_state, m_cfg, 12, m_state.wave);
        m_state.pendingSpawnBricks = 0;
        m_state.endlessSpawnCooldown = 0.5f;
        m_state.endlessAutoTimer = 0.0f;
    }

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

        float panelW = 450.0f;
        float panelH = 200.0f;
        float panelX = (fbW - panelW) * 0.5f;
        float panelY = (fbH - panelH) * 0.5f;
        
        float btnW = 140.0f;
        float btnH = 60.0f;
        float btnGap = 50.0f;
        float btnX_left = panelX + (panelW - 2*btnW - btnGap) * 0.5f;
        float btnX_right = btnX_left + btnW + btnGap;
        float btnY = panelY + 40.0f;

        if (click) {
            // Left button: Restart
            if (px >= btnX_left && px <= btnX_left + btnW && py >= btnY && py <= btnY + btnH) {
                init();
                return;
            }
            // Right button: Back to Menu
            if (px >= btnX_right && px <= btnX_right + btnW && py >= btnY && py <= btnY + btnH) {
                m_state.mode = GameMode::MENU;
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

    auto fitScaleToWidth = [&](const std::string& text, float desiredScale, float maxWidthPx) -> float {
        float s = desiredScale;
        if (maxWidthPx <= 1.0f) return s;
        float w = m_renderer.measureUITextWidth(text, s);
        if (w > maxWidthPx && w > 1.0f) {
            s *= (maxWidthPx / w);
        }
        return s;
    };

    // =========== MENU RENDER ===========
    if (m_state.mode == GameMode::MENU) {
        // Dark background
        m_renderer.beginUI(fbW, fbH);
        m_renderer.drawUIQuad(0, 0, (float)fbW, (float)fbH, glm::vec4(0.05f, 0.05f, 0.08f, 1.0f));

        // Menu layout: title on top, menu panel BELOW it (no overlap).
        float panelW = 500.0f;
        float panelH = 480.0f;
        float panelX = (fbW - panelW) * 0.5f;

        const float topMargin = 28.0f;
        const float bottomMargin = 24.0f;
        const float titlePanelGap = 26.0f;

        // Menu title (BIG, centered on screen)
        std::string title = "BREAKOUT 3D";
        float titleScale = fitScaleToWidth(title, 12.0f, (float)fbW * 0.92f);
        {
            // Ensure there's enough vertical room for: [title][gap][panel]
            float maxTitleH = (float)fbH - topMargin - bottomMargin - titlePanelGap - panelH;
            if (maxTitleH > 8.0f) {
                float titleH = m_renderer.getUIFontLineHeight(titleScale);
                if (titleH > maxTitleH && titleH > 1.0f) {
                    titleScale *= (maxTitleH / titleH);
                }
            }
        }

        float titleW = m_renderer.measureUITextWidth(title, titleScale);
        float titleH = m_renderer.getUIFontLineHeight(titleScale);
        float titleX = ((float)fbW - titleW) * 0.5f;
        float titleY = (float)fbH - topMargin - titleH;

        // Panel sits below the title
        float panelY = titleY - titlePanelGap - panelH;
        if (panelY < bottomMargin) {
            panelY = bottomMargin;
            // If we had to clamp, keep title just above the panel.
            titleY = panelY + panelH + titlePanelGap;
            if (titleY + titleH > (float)fbH - topMargin) {
                titleY = (float)fbH - topMargin - titleH;
            }
        }

        m_renderer.drawUIText(titleX, titleY, title, titleScale, glm::vec3(0.2f, 0.8f, 1.0f));

        m_renderer.drawUIQuad(panelX, panelY, panelW, panelH, glm::vec4(0.08f, 0.08f, 0.12f, 0.95f));
        
        // Border
        float borderThickness = 3.0f;
        glm::vec4 borderColor(0.2f, 0.8f, 1.0f, 1.0f);
        m_renderer.drawUIQuad(panelX - borderThickness, panelY - borderThickness, panelW + 2*borderThickness, borderThickness, borderColor);
        m_renderer.drawUIQuad(panelX - borderThickness, panelY + panelH, panelW + 2*borderThickness, borderThickness, borderColor);
        m_renderer.drawUIQuad(panelX - borderThickness, panelY, borderThickness, panelH, borderColor);
        m_renderer.drawUIQuad(panelX + panelW, panelY, borderThickness, panelH, borderColor);

        // Buttons
        float btnW = 200.0f;
        float btnH = 70.0f;
        float btnX = panelX + (panelW - btnW) * 0.5f;

        // Button positions (from top to bottom)
        float btn1Y = panelY + 360.0f; // Normal Mode (top)
        float btn2Y = panelY + 250.0f; // Endless Mode
        float btn3Y = panelY + 140.0f; // Instructions
        float btn4Y = panelY + 30.0f;  // Exit (bottom)

        // Normal Mode button
        m_renderer.drawUIQuad(btnX, btn1Y, btnW, btnH, glm::vec4(0.2f, 0.8f, 0.2f, 1.0f));
        float normalScale = 1.0f;
        float normalLabelW = m_renderer.measureUITextWidth("NORMAL", normalScale);
        float normalH = m_renderer.getUIFontLineHeight(normalScale);
        m_renderer.drawUIText(btnX + (btnW - normalLabelW) * 0.5f, btn1Y + (btnH - normalH) * 0.5f, "NORMAL", normalScale, glm::vec3(1, 1, 1));

        // Endless Mode button
        m_renderer.drawUIQuad(btnX, btn2Y, btnW, btnH, glm::vec4(0.8f, 0.5f, 0.2f, 1.0f));
        float endlessScale = 1.0f;
        float endlessLabelW = m_renderer.measureUITextWidth("ENDLESS", endlessScale);
        float endlessH = m_renderer.getUIFontLineHeight(endlessScale);
        m_renderer.drawUIText(btnX + (btnW - endlessLabelW) * 0.5f, btn2Y + (btnH - endlessH) * 0.5f, "ENDLESS", endlessScale, glm::vec3(1, 1, 1));

        // Instructions button
        m_renderer.drawUIQuad(btnX, btn3Y, btnW, btnH, glm::vec4(0.3f, 0.5f, 0.8f, 1.0f));
        float instrScale = 0.85f;
        float instrLabelW = m_renderer.measureUITextWidth("INSTRUCTIONS", instrScale);
        float instrH = m_renderer.getUIFontLineHeight(instrScale);
        m_renderer.drawUIText(btnX + (btnW - instrLabelW) * 0.5f, btn3Y + (btnH - instrH) * 0.5f, "INSTRUCTIONS", instrScale, glm::vec3(1, 1, 1));

        // Exit button
        m_renderer.drawUIQuad(btnX, btn4Y, btnW, btnH, glm::vec4(0.8f, 0.2f, 0.2f, 1.0f));
        float exitScale = 1.2f;
        float exitLabelW = m_renderer.measureUITextWidth("EXIT", exitScale);
        float exitH = m_renderer.getUIFontLineHeight(exitScale);
        m_renderer.drawUIText(btnX + (btnW - exitLabelW) * 0.5f, btn4Y + (btnH - exitH) * 0.5f, "EXIT", exitScale, glm::vec3(1, 1, 1));

        // Small clickable "4" badge for the one-brick test mode.
        {
            float badgeW = 48.0f;
            float badgeH = btnH - 16.0f;
            float badgeX = btnX + btnW - 8.0f - badgeW;
            float badgeY = btn4Y + 8.0f;
            m_renderer.drawUIQuad(badgeX, badgeY, badgeW, badgeH, glm::vec4(0.2f, 0.8f, 1.0f, 1.0f));
            m_renderer.drawUIQuad(badgeX + 2.0f, badgeY + 2.0f, badgeW - 4.0f, badgeH - 4.0f, glm::vec4(0.06f, 0.06f, 0.10f, 1.0f));

            std::string k = "4";
            float kScale = 1.4f;
            float kW = m_renderer.measureUITextWidth(k, kScale);
            float kH = m_renderer.getUIFontLineHeight(kScale);
            m_renderer.drawUIText(badgeX + (badgeW - kW) * 0.5f, badgeY + (badgeH - kH) * 0.5f, k, kScale, glm::vec3(0.2f, 0.8f, 1.0f));

            // Hint text under the button
            std::string hint = "ONE BRICK";
            float hScale = 0.58f;
            float hW = m_renderer.measureUITextWidth(hint, hScale);
            m_renderer.drawUIText(btnX + (btnW - hW) * 0.5f, btn4Y - 20.0f, hint, hScale, glm::vec3(0.6f, 0.85f, 1.0f));
        }

        // Show instructions if toggled
        if (m_state.showInstructions) {
            // Instructions panel
            float instrW = 600.0f;
            float instrH = 320.0f;
            float instrX = (fbW - instrW) * 0.5f;
            float instrY = (fbH - instrH) * 0.5f;

            m_renderer.drawUIQuad(instrX, instrY, instrW, instrH, glm::vec4(0.05f, 0.05f, 0.1f, 0.98f));
            
            // Instructions border
            glm::vec4 instrBorder(0.4f, 0.6f, 0.9f, 1.0f);
            m_renderer.drawUIQuad(instrX - borderThickness, instrY - borderThickness, instrW + 2*borderThickness, borderThickness, instrBorder);
            m_renderer.drawUIQuad(instrX - borderThickness, instrY + instrH, instrW + 2*borderThickness, borderThickness, instrBorder);
            m_renderer.drawUIQuad(instrX - borderThickness, instrY, borderThickness, instrH, instrBorder);
            m_renderer.drawUIQuad(instrX + instrW, instrY, borderThickness, instrH, instrBorder);

            // Instructions title
            std::string instrTitle = "HOW TO PLAY";
            float instrTitleScale = 1.5f;
            float instrTitleW = m_renderer.measureUITextWidth(instrTitle, instrTitleScale);
            m_renderer.drawUIText(instrX + (instrW - instrTitleW) * 0.5f, instrY + instrH - 50.0f, instrTitle, instrTitleScale, glm::vec3(0.4f, 0.8f, 1.0f));

            // Instructions text
            float textY = instrY + instrH - 100.0f;
            float textX = instrX + 20.0f;
            float lineGap = 28.0f;

            std::vector<std::string> instructions = {
                "A/D or ARROW KEYS: Move paddle",
                "SPACE: Launch ball",
                "ESC: Pause/Resume game",
                "1/2: Change camera view",
                "Destroy all bricks to win!",
                "Click here to go back"
            };

            for (const auto& line : instructions) {
                m_renderer.drawUIText(textX, textY, line, 0.65f, glm::vec3(0.8f, 0.9f, 1.0f));
                textY -= lineGap;
            }
        }

        m_renderer.endUI();
        m_window.swapBuffers();
        return;
    }

    if (m_state.currentBg != -1) {
        m_renderer.drawBackground(m_assets.backgroundTexs[m_state.currentBg].id);
    }

    // -------- 3D PASS --------
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

    // ✅ NÃO usamos tint para “dar cor” (só deixa as texturas falarem)
    glm::vec3 tint(1.0f);

    // Walls (Side rails extended ONLY towards camera)
    float sideThickness = 1.2f;
    float topThickness  = 1.2f;
    float wallHeight    = 1.0f;
    float railLen       = 50.0f;
    float railZStart    = m_cfg.arenaMinZ - topThickness;
    float railZCenter   = railZStart + railLen * 0.5f;

    // Left Rail
    m_renderer.drawMesh(m_assets.brick01,
        glm::vec3(m_cfg.arenaMinX - sideThickness*0.5f, 0.0f, railZCenter),
        glm::vec3(sideThickness, wallHeight, railLen),
        tint
    );

    // Right Rail
    m_renderer.drawMesh(m_assets.brick01,
        glm::vec3(m_cfg.arenaMaxX + sideThickness*0.5f, 0.0f, railZCenter),
        glm::vec3(sideThickness, wallHeight, railLen),
        tint
    );

    // Top Border
    m_renderer.drawMesh(m_assets.brick01,
        glm::vec3(0.0f, 0.0f, m_cfg.arenaMinZ - topThickness*0.5f),
        glm::vec3(arenaW + sideThickness*2.0f, wallHeight, topThickness),
        tint
    );

    // ✅ BRICKS: trocar para *_1hit / *_2hit / *_3hit (texturas/OBJ)
    auto pickBrickMesh = [&](int maxHp, int hp) -> const engine::Mesh* {
        const engine::Mesh* m = &m_assets.brick01;
        if (maxHp == 4) {
            if (hp == 4)      m = &m_assets.brick04;
            else if (hp == 3) m = &m_assets.brick04_1hit;
            else if (hp == 2) m = &m_assets.brick04_2hit;
            else              m = &m_assets.brick04_3hit;
        } else if (maxHp == 3) {
            if (hp == 3)      m = &m_assets.brick03;
            else if (hp == 2) m = &m_assets.brick03_1hit;
            else              m = &m_assets.brick03_2hit;
        } else if (maxHp == 2) {
            if (hp == 2)      m = &m_assets.brick02;
            else              m = &m_assets.brick02_1hit;
        } else {
            m = &m_assets.brick01;
        }
        return m;
    };

    for (const auto& b : m_state.bricks) {
        if (!b.alive) continue;

        const engine::Mesh* m = pickBrickMesh(b.maxHp, b.hp);

        m_renderer.drawMesh(*m, b.pos, b.size, tint);
    }

    // Paddle
    glm::vec3 currentPaddleSize = m_cfg.paddleSize;
    if (m_state.expandTimer > 0.0f) currentPaddleSize.x *= m_cfg.expandScaleFactor;
    if (m_state.tinyTimer > 0.0f) currentPaddleSize.x *= m_cfg.tinyScaleFactor;
    m_renderer.drawMesh(m_assets.paddle, m_state.paddlePos, currentPaddleSize, tint);

    // Shield barrier (behind paddle)
    if (m_state.shieldTimer > 0.0f) {
        float barrierZ = m_state.paddlePos.z + m_cfg.shieldOffsetZ;
        barrierZ = std::min(barrierZ, 19.0f);
        glm::vec3 barrierPos(0.0f, 0.0f, barrierZ);
        glm::vec3 barrierSize((m_cfg.arenaMaxX - m_cfg.arenaMinX) * 1.10f, 1.0f, 0.30f);
        m_renderer.drawMesh(m_assets.shield, barrierPos, barrierSize, glm::vec3(0.25f, 0.90f, 1.00f));
    }

    // Balls
    float ballD = m_cfg.ballRadius * 2.0f;
    for (const auto& b : m_state.balls) {
        if (b.isFireball) {
            // Fireball: use model + simple orange "trail" ghosts
            glm::vec3 fireTint(1.00f, 0.55f, 0.15f);
            m_renderer.drawMesh(m_assets.fireball, b.pos, glm::vec3(ballD), fireTint);

            // Cheap trail: offset copies opposite the velocity (no history needed)
            float speed2 = b.vel.x * b.vel.x + b.vel.z * b.vel.z;
            if (!b.attached && speed2 > 1e-6f) {
                glm::vec3 dir = glm::normalize(glm::vec3(b.vel.x, 0.0f, b.vel.z));
                const int trailCount = 6;
                const float spacing = 0.55f;
                for (int i = 1; i <= trailCount; ++i) {
                    float u = (float)i / (float)(trailCount + 1);
                    glm::vec3 tp = b.pos - dir * (spacing * (float)i);
                    float s = ballD * (0.90f - 0.55f * u);
                    glm::vec3 tcol(1.0f, 0.55f + 0.25f * (1.0f - u), 0.20f);
                    m_renderer.drawMesh(m_assets.fireball, tp, glm::vec3(s), tcol);
                }
            }
        } else {
            m_renderer.drawMesh(m_assets.ball, b.pos, glm::vec3(ballD), tint);
        }
    }

    // (Danger warning moved to UI pass for better visibility)

    // Powerups
    float powerUpSpin = m_time.now() * 2.5f;
    // 90 degrees (stand up) + ~33 degrees (tilt towards camera) = ~123
    float powerUpTilt = glm::radians(123.0f); 
    float powerUpBob  = std::sin(m_time.now() * 4.0f) * 0.25f;

    for (const auto& p : m_state.powerups) {
        const engine::Mesh* m = &m_assets.ball;

        if (p.type == PowerUpType::EXPAND)           m = &m_assets.expand;
        else if (p.type == PowerUpType::EXTRA_BALL)  m = &m_assets.extraBall;
        else if (p.type == PowerUpType::SLOW)        m = &m_assets.slow;
        else if (p.type == PowerUpType::EXTRA_LIFE)  m = &m_assets.extraLife;
        else if (p.type == PowerUpType::FIREBALL)    m = &m_assets.fireball;
        else if (p.type == PowerUpType::SHIELD)      m = &m_assets.shield;
        else if (p.type == PowerUpType::REVERSE)     m = &m_assets.skull;
        else if (p.type == PowerUpType::TINY)        m = &m_assets.minus;
        
        glm::mat4 M(1.0f);
        M = glm::translate(M, p.pos + glm::vec3(0, 0.5f + powerUpBob, 0));
        M = glm::rotate(M, powerUpTilt, glm::vec3(1, 0, 0));
        
        // Snail specific: rotate 90 degrees on Y so its side profile is visible
        float rotation = powerUpSpin;
        if (p.type == PowerUpType::SLOW) rotation += glm::radians(90.0f);
        
        M = glm::rotate(M, rotation, glm::vec3(0, 1, 0));
        
        if (p.type == PowerUpType::EXTRA_LIFE) {
            // Flip 180 on X for the powerup heart specifically
            M = glm::rotate(M, glm::radians(180.0f), glm::vec3(1, 0, 0));
        }

        M = glm::scale(M, glm::vec3(m_cfg.powerUpVisualScale));

        // Fix "Minus looks like a cube": sculpt it into a minus bar in render-space.
        if (p.type == PowerUpType::TINY) {
            M = glm::scale(M, glm::vec3(2.35f, 0.22f, 0.60f));
        }

        // Distinct, readable colors per powerup (more representative).
        glm::vec3 col = tint;
        // Good
        if (p.type == PowerUpType::EXPAND)      col = glm::vec3(0.25f, 0.95f, 0.25f); // growth = green
        if (p.type == PowerUpType::EXTRA_BALL)  col = glm::vec3(0.15f, 0.70f, 1.00f); // extra = blue
        if (p.type == PowerUpType::EXTRA_LIFE)  col = glm::vec3(1.00f, 0.18f, 0.22f); // heart = red
        if (p.type == PowerUpType::FIREBALL)    col = glm::vec3(1.00f, 0.55f, 0.15f); // fire = orange
        if (p.type == PowerUpType::SHIELD)      col = glm::vec3(0.20f, 0.95f, 1.00f); // shield = cyan

        // Bad / curses
        if (p.type == PowerUpType::SLOW)        col = glm::vec3(0.55f, 0.38f, 0.22f); // snail = brown
        if (p.type == PowerUpType::REVERSE)     col = glm::vec3(0.95f, 0.20f, 0.90f); // cursed = magenta
        if (p.type == PowerUpType::TINY)        col = glm::vec3(1.00f, 0.85f, 0.10f); // shrink = yellow
        m_renderer.drawMesh(*m, M, col);
    }

    // -------- UI PASS (HUD 3D em ortho) --------
    m_renderer.beginUI(fbW, fbH);

    // --- WIN FINISHER OVERLAY: flash + expanding shockwave ("blown air") ---
    if (m_state.winFinisherActive) {
        auto o = game::effects::computeWinFinisherOverlay(
            fbW, fbH, V, P,
            m_state.winFinisherAnchorPos,
            m_state.winFinisherTimer,
            m_state.winFinisherAnchorValid
        );

        // White flash
        float flashA = o.flashAlpha;
        if (flashA > 0.01f) {
            m_renderer.drawUIQuad(0.0f, 0.0f, (float)fbW, (float)fbH, glm::vec4(1.0f, 1.0f, 1.0f, flashA));
        }

        // Expanding ring (drawn with triangles)
        glm::vec2 c = o.centerPx;
        float r = o.ringRadiusPx;
        float thick = o.ringThicknessPx;

        float ringA = o.ringAlpha;
        glm::vec4 ringCol(0.85f, 0.95f, 1.0f, ringA);

        auto drawRing = [&](const glm::vec2& center, float radius, float thickness, const glm::vec4& col) {
            const int segs = 72;
            float r0 = std::max(0.0f, radius - thickness * 0.5f);
            float r1 = radius + thickness * 0.5f;
            for (int i = 0; i < segs; ++i) {
                float a0 = (float)i / (float)segs * glm::two_pi<float>();
                float a1 = (float)(i + 1) / (float)segs * glm::two_pi<float>();
                glm::vec2 d0(std::cos(a0), std::sin(a0));
                glm::vec2 d1(std::cos(a1), std::sin(a1));

                glm::vec2 p0o = center + d0 * r1;
                glm::vec2 p0i = center + d0 * r0;
                glm::vec2 p1o = center + d1 * r1;
                glm::vec2 p1i = center + d1 * r0;

                m_renderer.drawUITriangle(p0o, p0i, p1o, col);
                m_renderer.drawUITriangle(p0i, p1o, p1i, col);
            }
        };

        if (ringA > 0.01f) {
            drawRing(c, r, thick, ringCol);
        }
    }

    // --- FIREBALL EXPLOSION FX (orange expanding ring at impact point) ---
    if (!m_state.fireballExplosions.empty()) {
        auto drawRing = [&](const glm::vec2& center, float radius, float thickness, const glm::vec4& col) {
            const int segs = 56;
            float r0 = std::max(0.0f, radius - thickness * 0.5f);
            float r1 = radius + thickness * 0.5f;
            for (int i = 0; i < segs; ++i) {
                float a0 = (float)i / (float)segs * glm::two_pi<float>();
                float a1 = (float)(i + 1) / (float)segs * glm::two_pi<float>();
                glm::vec2 d0(std::cos(a0), std::sin(a0));
                glm::vec2 d1(std::cos(a1), std::sin(a1));

                glm::vec2 p0o = center + d0 * r1;
                glm::vec2 p0i = center + d0 * r0;
                glm::vec2 p1o = center + d1 * r1;
                glm::vec2 p1i = center + d1 * r0;

                m_renderer.drawUITriangle(p0o, p0i, p1o, col);
                m_renderer.drawUITriangle(p0i, p1o, p1i, col);
            }
        };

        float dur = std::max(0.001f, m_cfg.fireballExplosionFxDuration);
        for (const auto& fx : m_state.fireballExplosions) {
            float u = fx.t / dur;
            if (u < 0.0f) u = 0.0f;
            if (u > 1.0f) u = 1.0f;

            // Project explosion center to UI pixels
            glm::vec4 clip = P * V * glm::vec4(fx.pos, 1.0f);
            if (std::abs(clip.w) < 1e-6f) continue;
            float ndcX = clip.x / clip.w;
            float ndcY = clip.y / clip.w;
            if (clip.w <= 0.0f) continue;

            glm::vec2 cPx(
                (ndcX * 0.5f + 0.5f) * (float)fbW,
                (ndcY * 0.5f + 0.5f) * (float)fbH
            );

            float radiusPx = 26.0f + 170.0f * u;
            float thickPx = 18.0f - 10.0f * u;
            float a = (1.0f - u) * 0.85f;
            glm::vec4 col(1.0f, 0.55f, 0.10f, a);
            if (a > 0.01f) drawRing(cPx, radiusPx, thickPx, col);
        }
    }

    const float padX = 22.0f;
    const float padTop = 18.0f;

    const float hs = 56.0f;
    const float gap = 12.0f;

    const float rx = glm::radians(18.0f);
    const float ry = glm::radians(-12.0f);

    int heartCount = std::max(3, m_state.lives);
    for (int i = 0; i < heartCount; ++i) {
        float xCenter = padX + hs * 0.5f + i * (hs + gap);
        float yCenter = (float)fbH - (padTop + hs * 0.5f);

        glm::mat4 M(1.0f);
        M = glm::translate(M, glm::vec3(xCenter, yCenter, 0.0f));
        M = glm::rotate(M, ry, glm::vec3(0,1,0));
        M = glm::rotate(M, rx, glm::vec3(1,0,0));
        M = glm::scale(M, glm::vec3(hs, hs, hs * 0.55f));

        glm::vec3 col = (i < m_state.lives)
            ? glm::vec3(1.0f, 0.20f, 0.25f)
            : glm::vec3(0.20f, 0.20f, 0.22f);

        m_renderer.drawMesh(m_assets.heart, M, col);
    }

    // --- DANGER ZONE FILTER (draw BEFORE pause/gameover overlay so overlays stay on top) ---
    if (m_state.gameType == GameType::ENDLESS &&
        m_state.endlessDangerActive &&
        (m_state.mode == GameMode::PLAYING || m_state.mode == GameMode::PAUSED) &&
        (
            // Keep the "early warning" capped (so it doesn't flash forever),
            // but ALWAYS show again when bricks are very close to the death line (1-2 rows away).
            m_state.endlessDangerTimer < 10.0f ||
            ((m_state.paddlePos.z - 0.5f) - m_state.endlessDangerMaxZ) <= (1.33f * 2.0f)
        )) {

        float screenY = (dangerLineScreenY >= 0.0f) ? dangerLineScreenY : (float)fbH * 0.30f;
        screenY = std::max(0.0f, std::min((float)fbH, screenY));

        // Clean "real warning" flash: smooth fade in/out (no harsh on/off, no double-strobe).
        // Keep in sync with text/signs by using the same intensity curve.
        const float period = 0.95f; // seconds
        float ph = std::fmod(m_state.endlessDangerTimer, period);
        if (ph < 0.0f) ph += period;
        float x = ph / period; // 0..1
        // triangle wave 0..1..0
        float tri = 1.0f - std::abs(2.0f * x - 1.0f);
        // smoothstep for clean easing
        float pulse = tri * tri * (3.0f - 2.0f * tri);
        // slightly emphasize the peak
        pulse = std::pow(pulse, 1.15f);

        float baseA = 0.06f + 0.30f * pulse; // fades in/out with the warning
        if (baseA > 0.01f) {
            m_renderer.drawUIQuad(0.0f, 0.0f, (float)fbW, screenY, glm::vec4(1.0f, 0.0f, 0.0f, baseA));
        }
    }

    // (Overlay is drawn at the end of UI so it truly sits on top of everything.)

    // --- SCORE AND WAVE HUD ---
    {
        if (m_state.gameType == GameType::ENDLESS) {
            // Endless: HUD at the top-center (white).
            std::string bestStr = std::to_string(m_state.endlessBestScore);
            std::string scoreStr = std::to_string(m_state.score);

            // Bigger, more readable score HUD
            float bestScale = 1.30f;
            float scoreScale = 2.35f;

            float bestH = m_renderer.getUIFontLineHeight(bestScale);
            float scoreH = m_renderer.getUIFontLineHeight(scoreScale);

            float bestTotalW = m_renderer.measureUITextWidth(bestStr, bestScale);
            float scoreTotalW = m_renderer.measureUITextWidth(scoreStr, scoreScale);

            float bestX = (float)fbW * 0.5f - bestTotalW * 0.5f;
            float scoreX = (float)fbW * 0.5f - scoreTotalW * 0.5f;

            const float topMargin = 18.0f;
            const float lineGap = 8.0f;
            float bestY = (float)fbH - topMargin - bestH;
            float scoreY = bestY - lineGap - scoreH;

            m_renderer.drawUIText(bestX, bestY, bestStr, bestScale, glm::vec4(1.0f, 1.0f, 1.0f, 0.75f));
            m_renderer.drawUIText(scoreX, scoreY, scoreStr, scoreScale, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

            // Split bank display:
            // - gains: white "+XXX" on the RIGHT (same as original)
            // - penalties: red "-XXX" on the LEFT (same animation, mirrored)
            {
                const float addScale = 1.45f;
                const float bankAnim = 0.55f;
                float t = m_state.endlessStreakBanking
                    ? std::min(1.0f, std::max(0.0f, m_state.endlessStreakBankTimer / bankAnim))
                    : 0.0f;
                float ease = t * t * (3.0f - 2.0f * t);
                float yOffset = ease * 22.0f;
                float alpha = 1.0f - 0.65f * ease;
                float baseY = scoreY - scoreH * 0.65f + yOffset;

                if (m_state.endlessStreakPosPoints > 0) {
                    std::string s = "+" + std::to_string(m_state.endlessStreakPosPoints);
                    float w = m_renderer.measureUITextWidth(s, addScale);
                    float x = scoreX + scoreTotalW - w * 0.25f; // close-right
                    m_renderer.drawUIText(x, baseY, s, addScale, glm::vec4(1.0f, 1.0f, 1.0f, alpha));
                }
                if (m_state.endlessStreakNegPoints > 0) {
                    std::string s = "-" + std::to_string(m_state.endlessStreakNegPoints);
                    float w = m_renderer.measureUITextWidth(s, addScale);
                    float x = scoreX - w * 0.75f; // mirrored to left
                    m_renderer.drawUIText(x, baseY, s, addScale, glm::vec4(1.0f, 0.15f, 0.15f, alpha));
                }
            }

            // Score popups:
            // - negatives: red, to the LEFT of the score
            // - positives: green, under/near the score (used e.g. by Fireball explosion)
            if (!m_state.scorePopups.empty()) {
                const float popupDur = 2.10f;
                float scale = 1.35f;
                float negMaxW = 0.0f;
                float posMaxW = 0.0f;
                for (const auto& sp : m_state.scorePopups) {
                    std::string s = (sp.pts >= 0) ? ("+" + std::to_string(sp.pts)) : std::to_string(sp.pts);
                    float w = m_renderer.measureUITextWidth(s, scale);
                    if (sp.pts < 0) negMaxW = std::max(negMaxW, w);
                    else            posMaxW = std::max(posMaxW, w);
                }

                // Mirror the white popup's placement:
                // white: addX = scoreX + scoreTotalW - w*0.25
                // red:   negX = scoreX - w*0.75
                float negX = scoreX - negMaxW * 0.75f;
                float posX = scoreX + (scoreTotalW - posMaxW) * 0.5f;

                // Match the white streak popup height baseline exactly.
                float yBase = scoreY - scoreH * 0.65f;
                int negIdx = 0;
                int posIdx = 0;

                for (const auto& sp : m_state.scorePopups) {
                    std::string s = (sp.pts >= 0) ? ("+" + std::to_string(sp.pts)) : std::to_string(sp.pts);
                    if (sp.pts < 0) {
                        // User request: same float/fade feel as the white streak popup.
                        const float bankAnim = 0.55f;
                        float t = std::min(1.0f, std::max(0.0f, sp.t / bankAnim));
                        float ease = t * t * (3.0f - 2.0f * t);
                        float yOffset = ease * 22.0f;
                        float a = 1.0f - 0.65f * ease;
                        float y = yBase + yOffset + (float)negIdx * 20.0f;
                        m_renderer.drawUIText(negX, y, s, scale, glm::vec4(1.0f, 0.15f, 0.15f, a));
                        negIdx++;
                    } else if (sp.pts > 0) {
                        float u = std::min(1.0f, std::max(0.0f, sp.t / popupDur));
                        float ease = u * u * (3.0f - 2.0f * u);
                        float a = 1.0f - 0.75f * ease; // green can stay "snappier"
                        float y = (scoreY - scoreH * 1.25f) + ease * 26.0f + (float)posIdx * 20.0f;
                        m_renderer.drawUIText(posX, y, s, scale, glm::vec4(0.35f, 1.0f, 0.35f, a));
                        posIdx++;
                    }
                }
            }

            if (m_state.endlessDangerActive && (m_state.mode == GameMode::PLAYING || m_state.mode == GameMode::PAUSED)) {
                bool veryCloseToDeath = (((m_state.paddlePos.z - 0.5f) - m_state.endlessDangerMaxZ) <= (1.33f * 2.0f));
                if (m_state.endlessDangerTimer < 10.0f || veryCloseToDeath) {
                    // Same clean fade in/out as the filter above (keeps everything synced).
                    const float period = 0.95f;
                    float ph = std::fmod(m_state.endlessDangerTimer, period);
                    if (ph < 0.0f) ph += period;
                    float x = ph / period; // 0..1
                    float tri = 1.0f - std::abs(2.0f * x - 1.0f);
                    float pulse = tri * tri * (3.0f - 2.0f * tri);
                    pulse = std::pow(pulse, 1.15f);

                    float triAlpha = 0.10f + 0.90f * pulse;
                    if (triAlpha < 0.02f) {
                        // effectively off (clean gap)
                    } else {
                        // 2. Signs (Red, Big & Transparent) — centered within the red danger band
                        std::string dMsg = "DANGER!";
                        float dScale = fitScaleToWidth(dMsg, 4.0f, (float)fbW * 0.92f);
                        float dW = m_renderer.measureUITextWidth(dMsg, dScale);
                        float xPos = (float)fbW * 0.5f - dW * 0.5f;

                        // Center vertically inside the danger band (y in UI coords is bottom-up)
                        float screenY = (dangerLineScreenY >= 0.0f) ? dangerLineScreenY : (float)fbH * 0.30f;
                        screenY = std::max(0.0f, std::min((float)fbH, screenY));
                        float th = m_renderer.getUIFontLineHeight(dScale);
                        float yPos = (screenY - th) * 0.5f;
                        yPos = std::max(12.0f, yPos);
                        if (yPos + th > screenY - 8.0f) yPos = std::max(12.0f, screenY - th - 8.0f);
                        float yMid = yPos + th * 0.5f;

                        // Make it read "bold" without overpowering: a tiny thickness pass + main text.
                        {
                            const float o = 1.25f;
                            const float a = triAlpha * 0.16f;
                            glm::vec4 thick(1.0f, 0.0f, 0.0f, a);
                            m_renderer.drawUIText(xPos - o, yPos, dMsg, dScale, thick);
                            m_renderer.drawUIText(xPos + o, yPos, dMsg, dScale, thick);
                            m_renderer.drawUIText(xPos, yPos - o, dMsg, dScale, thick);
                            m_renderer.drawUIText(xPos, yPos + o, dMsg, dScale, thick);
                        }

                        // Main crisp text (kept subtle)
                        m_renderer.drawUIText(xPos, yPos, dMsg, dScale, glm::vec4(1.0f, 0.0f, 0.0f, triAlpha * 0.30f));

                        auto drawTriangleSign = [&](float x) {
                            float triW = 100.0f; 
                            float triH = 85.0f;  
                            float yTri = yMid - triH * 0.5f; 
                            
                            glm::vec2 pTop(x, yTri + triH);
                            glm::vec2 pLeft(x - triW*0.5f, yTri);
                            glm::vec2 pRight(x + triW*0.5f, yTri);
                            
                            auto drawEdge = [&](const glm::vec2& a, const glm::vec2& b, float t) {
                                glm::vec2 d = b - a;
                                float len = glm::length(d);
                                if (len < 0.01f) return;
                                glm::vec2 dir = d / len;
                                glm::vec2 perp(-dir.y, dir.x);
                                glm::vec2 c1 = a + perp * (t * 0.5f);
                                glm::vec2 c2 = a - perp * (t * 0.5f);
                                glm::vec2 c3 = b + perp * (t * 0.5f);
                                glm::vec2 c4 = b - perp * (t * 0.5f);
                                m_renderer.drawUITriangle(c1, c2, c3, glm::vec4(1.0f, 0.0f, 0.0f, triAlpha));
                                m_renderer.drawUITriangle(c2, c3, c4, glm::vec4(1.0f, 0.0f, 0.0f, triAlpha));
                            };

                            float edgeT = 6.0f;
                            drawEdge(pLeft, pRight, edgeT);
                            drawEdge(pLeft, pTop, edgeT);
                            drawEdge(pRight, pTop, edgeT);

                            m_renderer.drawUIQuad(x - 3, yTri + 30, 6, 40, glm::vec4(1.0f, 0.0f, 0.0f, triAlpha));
                            m_renderer.drawUIQuad(x - 3, yTri + 16, 6, 6, glm::vec4(1.0f, 0.0f, 0.0f, triAlpha));
                        };

                        // Symmetric composition around center
                        drawTriangleSign((float)fbW * 0.22f);
                        drawTriangleSign((float)fbW * 0.78f);
                    }
                }
            }
        } else {
            // Normal mode: score HUD removed (per request).
            // Normal mode: don't show score popups (green or red).
        }

    }

    // --- BG Selector HUD ---
    {
        float boxSize = 30.0f;
        float gap = 10.0f;
        float startX = (float)fbW - (boxSize + gap) * 5.0f - 20.0f;
        float startY = (float)fbH - 20.0f - boxSize;

        glm::vec3 colors[] = {
            {0.15f, 0.15f, 0.2f}, // None (Dark Blueish)
            {0.2f, 0.4f, 0.9f}, // Blueish
            {0.6f, 0.2f, 0.9f}, // Purplish
            {0.2f, 0.8f, 0.4f}, // Greenish (Swapped)
            {0.9f, 0.2f, 0.3f}  // Redish (Swapped)
        };

        for (int i = -1; i < 4; i++) {
            float bx = startX + (i + 1) * (boxSize + gap);
            glm::vec3 col = colors[i + 1];
            
            // Draw border if selected
            if (m_state.currentBg == i) {
                float border = 2.0f;
                m_renderer.drawUIQuad(bx - border, startY - border, boxSize + border * 2.0f, boxSize + border * 2.0f, glm::vec4(1, 1, 1, 1.0f));
            } else {
                col *= 0.6f;
            }

            m_renderer.drawUIQuad(bx, startY, boxSize, boxSize, glm::vec4(col, 1.0f));

            if (i == -1) {
                // Draw "/" inside the first box
                float tx = bx + (boxSize - 10.0f) * 0.5f;
                float ty = startY + (boxSize - 16.0f) * 0.5f;
                m_renderer.drawUIText(tx, ty, "/", 0.8f, glm::vec3(1, 1, 1));
            }
        }
    }

    // Overlay (Pause / Game Over / Win) — draw LAST so it overlaps everything (danger, HUD, etc.)
    if (m_state.mode != GameMode::PLAYING) {
        float panelW = 650.0f;
        float panelH = 260.0f;
        float panelX = (fbW - panelW) * 0.5f;
        float panelY = (fbH - panelH) * 0.5f;
        
        if (m_state.mode == GameMode::PAUSED) {
            // Calculate panel and button positions
            float panelW = 650.0f;
            float panelH = 320.0f; // Taller to fit bigger PAUSED text + buttons
            float panelX = (fbW - panelW) * 0.5f;
            float panelY = (fbH - panelH) * 0.5f;
            
            // Full screen masked overlay (dimmed outside, clear inside)
            glm::vec2 maskMin(panelX, panelY);
            glm::vec2 maskMax(panelX + panelW, panelY + panelH);
            m_renderer.drawUIQuad(0, 0, (float)fbW, (float)fbH, glm::vec4(0, 0, 0, 0.82f), true, maskMin, maskMax);

            // Panel background
            m_renderer.drawUIQuad(panelX, panelY, panelW, panelH, glm::vec4(0.06f, 0.06f, 0.06f, 1.0f));
            
            // PAUSED text at the top
            std::string msg = "PAUSED";
            float scale = fitScaleToWidth(msg, 5.5f, panelW - 60.0f);
            float tw = m_renderer.measureUITextWidth(msg, scale);
            float th = m_renderer.getUIFontLineHeight(scale);
            
            float tx = panelX + (panelW - tw) * 0.5f;
            float ty = panelY + panelH - th - 55.0f;
            m_renderer.drawUIText(tx, ty, msg, scale, glm::vec3(1, 1, 1));
            
            // Buttons (Restart and Menu)
            float btnW = 140.0f;
            float btnH = 60.0f;
            float btnGap = 50.0f;
            float btnX_left = panelX + (panelW - 2*btnW - btnGap) * 0.5f;
            float btnX_right = btnX_left + btnW + btnGap;
            float btnY = panelY + 40.0f;
            
            m_renderer.drawUIQuad(btnX_left, btnY, btnW, btnH, glm::vec4(0.8f, 0.2f, 0.2f, 1.0f));
            m_renderer.drawUIQuad(btnX_right, btnY, btnW, btnH, glm::vec4(0.2f, 0.8f, 0.2f, 1.0f));
            
            std::string leftLabel = "RESTART";
            std::string rightLabel = "MENU";

            float btnLabelScale = 1.0f;
            float ltw = m_renderer.measureUITextWidth(leftLabel, btnLabelScale);
            float rtw = m_renderer.measureUITextWidth(rightLabel, btnLabelScale);
            float lth = m_renderer.getUIFontLineHeight(btnLabelScale);
            
            m_renderer.drawUIText(btnX_left + (btnW - ltw) * 0.5f,  btnY + (btnH - lth) * 0.5f, leftLabel, btnLabelScale, glm::vec3(1, 1, 1));
            m_renderer.drawUIText(btnX_right + (btnW - rtw) * 0.5f, btnY + (btnH - lth) * 0.5f, rightLabel, btnLabelScale, glm::vec3(1, 1, 1));
        } else {
            // Full screen masked overlay for Game Over / Win
            glm::vec2 maskMin(panelX, panelY);
            glm::vec2 maskMax(panelX + panelW, panelY + panelH);
            m_renderer.drawUIQuad(0, 0, (float)fbW, (float)fbH, glm::vec4(0, 0, 0, 0.86f), true, maskMin, maskMax);

            // Panel for Game Over / Win - Fully opaque
            m_renderer.drawUIQuad(panelX, panelY, panelW, panelH, glm::vec4(0.08f, 0.08f, 0.08f, 1.0f));
            
            std::string title = (m_state.mode == GameMode::GAME_OVER) ? "GAME OVER" : "WINNER!";
            float titleScale = fitScaleToWidth(title, 5.8f, panelW - 60.0f);
            float tw = m_renderer.measureUITextWidth(title, titleScale);
            float th = m_renderer.getUIFontLineHeight(titleScale);
            float tx = panelX + (panelW - tw) * 0.5f;
            float ty = panelY + panelH - th - 45.0f;
            m_renderer.drawUIText(tx, ty, title, titleScale, glm::vec3(1, 1, 1));

            // Buttons
            float btnW = 140.0f;
            float btnH = 60.0f;
            float btnGap = 50.0f;
            float btnX_left = panelX + (panelW - 2*btnW - btnGap) * 0.5f;
            float btnX_right = btnX_left + btnW + btnGap;
            float btnY = panelY + 40.0f;
            
            m_renderer.drawUIQuad(btnX_left, btnY, btnW, btnH, glm::vec4(0.8f, 0.2f, 0.2f, 1.0f));
            m_renderer.drawUIQuad(btnX_right, btnY, btnW, btnH, glm::vec4(0.2f, 0.8f, 0.2f, 1.0f));
            
            std::string leftLabel = "RETRY";
            std::string rightLabel = "MENU";

            float btnLabelScale = 1.0f;
            float ltw = m_renderer.measureUITextWidth(leftLabel, btnLabelScale);
            float rtw = m_renderer.measureUITextWidth(rightLabel, btnLabelScale);
            float lth = m_renderer.getUIFontLineHeight(btnLabelScale);
            
            m_renderer.drawUIText(btnX_left + (btnW - ltw) * 0.5f,  btnY + (btnH - lth) * 0.5f, leftLabel, btnLabelScale, glm::vec3(1, 1, 1));
            m_renderer.drawUIText(btnX_right + (btnW - rtw) * 0.5f, btnY + (btnH - lth) * 0.5f, rightLabel, btnLabelScale, glm::vec3(1, 1, 1));
        }
    }

    m_renderer.endUI();
    m_window.swapBuffers();
}

} // namespace game
