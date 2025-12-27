#include "game/Game.hpp"
#include "engine/Input.hpp"
#include "game/GameAssets.hpp"
#include "game/systems/InitSystem.hpp"
#include "game/systems/InputSystem.hpp"
#include "game/systems/PhysicsSystem.hpp"
#include "game/systems/CollisionSystem.hpp"
#include "game/systems/PowerUpSystem.hpp"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace game {

Game::Game(engine::Window& window, engine::Time& time, engine::Renderer& renderer, GameAssets& assets)
: m_window(window), m_time(time), m_renderer(renderer), m_assets(assets) {}

void Game::init() {
    InitSystem::initGame(m_state, m_cfg);
}

void Game::update(const engine::Input& input) {
    float dt = m_time.delta();

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
    if (m_state.gameType == GameType::ENDLESS)
        m_state.endlessAutoTimer += dt;
    if (m_state.expandTimer > 0.0f) m_state.expandTimer = std::max(0.0f, m_state.expandTimer - dt);
    if (m_state.slowTimer > 0.0f) m_state.slowTimer = std::max(0.0f, m_state.slowTimer - dt);

    // Check for WIN condition (normal mode only)
    if (m_state.mode == GameMode::PLAYING && m_state.gameType == GameType::NORMAL && !InitSystem::anyBricksAlive(m_state)) {
        m_state.mode = GameMode::WIN;
        m_state.balls.clear();
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

    // Perform queued brick spawns (endless mode)
    if (m_state.gameType == GameType::ENDLESS && m_state.pendingSpawnBricks > 0) {
        InitSystem::spawnIncrementalBricks(m_state, m_cfg, m_state.pendingSpawnBricks, m_state.wave);
        m_state.pendingSpawnBricks = 0;
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
    if (m_state.balls.empty()) {
        m_state.lives--;
        if (m_state.lives > 0) {
            Ball b;
            PhysicsSystem::resetBallToPaddle(b, m_state.paddlePos, m_cfg);
            m_state.balls.push_back(b);
        } else {
            m_state.mode = InitSystem::anyBricksAlive(m_state) ? GameMode::GAME_OVER : GameMode::WIN;
        }
    }
}

void Game::render() {
    auto [fbW, fbH] = m_window.getFramebufferSize();
    m_renderer.beginFrame(fbW, fbH);

    // =========== MENU RENDER ===========
    if (m_state.mode == GameMode::MENU) {
        // Dark background
        m_renderer.beginUI(fbW, fbH);
        m_renderer.drawUIQuad(0, 0, (float)fbW, (float)fbH, glm::vec4(0.05f, 0.05f, 0.08f, 1.0f));

        // Menu title
        std::string title = "BREAKOUT 3D";
        float titleScale = 4.0f;
        float titleW = title.size() * 14.0f * titleScale;
        float titleX = (fbW - titleW) * 0.5f;
        float titleY = fbH - 100.0f;
        m_renderer.drawUIText(titleX, titleY, title, titleScale, glm::vec3(0.2f, 0.8f, 1.0f));

        // Menu panel
        float panelW = 500.0f;
        float panelH = 480.0f;
        float panelX = (fbW - panelW) * 0.5f;
        float panelY = (fbH - panelH) * 0.5f;

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
        float normalLabelW = 6.0f * 14.0f;
        m_renderer.drawUIText(btnX + (btnW - normalLabelW) * 0.5f, btn1Y + (btnH - 20.0f) * 0.5f, "NORMAL", 1.0f, glm::vec3(1, 1, 1));

        // Endless Mode button
        m_renderer.drawUIQuad(btnX, btn2Y, btnW, btnH, glm::vec4(0.8f, 0.5f, 0.2f, 1.0f));
        float endlessLabelW = 7.0f * 14.0f;
        m_renderer.drawUIText(btnX + (btnW - endlessLabelW) * 0.5f, btn2Y + (btnH - 20.0f) * 0.5f, "ENDLESS", 1.0f, glm::vec3(1, 1, 1));

        // Instructions button
        m_renderer.drawUIQuad(btnX, btn3Y, btnW, btnH, glm::vec4(0.3f, 0.5f, 0.8f, 1.0f));
        float instrLabelW = 12.0f * 14.0f * 0.7f;
        m_renderer.drawUIText(btnX + (btnW - instrLabelW) * 0.5f, btn3Y + (btnH - 20.0f) * 0.5f, "INSTRUCTIONS", 0.85f, glm::vec3(1, 1, 1));

        // Exit button
        m_renderer.drawUIQuad(btnX, btn4Y, btnW, btnH, glm::vec4(0.8f, 0.2f, 0.2f, 1.0f));
        float exitLabelW = 4.0f * 14.0f;
        m_renderer.drawUIText(btnX + (btnW - exitLabelW) * 0.5f, btn4Y + (btnH - 20.0f) * 0.5f, "EXIT", 1.2f, glm::vec3(1, 1, 1));

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
            float instrTitleW = instrTitle.size() * 14.0f * 1.5f;
            m_renderer.drawUIText(instrX + (instrW - instrTitleW) * 0.5f, instrY + instrH - 50.0f, instrTitle, 1.5f, glm::vec3(0.4f, 0.8f, 1.0f));

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

    glm::mat4 V = glm::lookAt(camPos, camTarget, glm::vec3(0,1,0));
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
    for (const auto& b : m_state.bricks) {
        if (!b.alive) continue;

        const engine::Mesh* m = &m_assets.brick01;

        if (b.maxHp == 4) {
            if (b.hp == 4)      m = &m_assets.brick04;
            else if (b.hp == 3) m = &m_assets.brick04_1hit;
            else if (b.hp == 2) m = &m_assets.brick04_2hit;
            else                m = &m_assets.brick04_3hit;
        } else if (b.maxHp == 3) {
            if (b.hp == 3)      m = &m_assets.brick03;
            else if (b.hp == 2) m = &m_assets.brick03_1hit;
            else                m = &m_assets.brick03_2hit;
        } else if (b.maxHp == 2) {
            if (b.hp == 2)      m = &m_assets.brick02;
            else                m = &m_assets.brick02_1hit;
        } else {
            m = &m_assets.brick01;
        }

        m_renderer.drawMesh(*m, b.pos, b.size, tint);
    }

    // Paddle
    glm::vec3 currentPaddleSize = m_cfg.paddleSize;
    if (m_state.expandTimer > 0.0f) currentPaddleSize.x *= m_cfg.expandScaleFactor;
    m_renderer.drawMesh(m_assets.paddle, m_state.paddlePos, currentPaddleSize, tint);

    // Balls
    float ballD = m_cfg.ballRadius * 2.0f;
    for (const auto& b : m_state.balls) {
        m_renderer.drawMesh(m_assets.ball, b.pos, glm::vec3(ballD), tint);
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

        m_renderer.drawMesh(*m, M, tint);
    }

    // -------- UI PASS (HUD 3D em ortho) --------
    m_renderer.beginUI(fbW, fbH);

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
        m_state.endlessDangerTimer < 10.0f) {

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
        float padX = 22.0f;
        float padTop = 100.0f;

        // Score display
        std::string scoreStr = "Score: " + std::to_string(m_state.score);
        m_renderer.drawUIText(padX, (float)fbH - padTop, scoreStr, 1.0f, glm::vec3(1.0f, 0.8f, 0.2f));

        // Wave + Destroyed display (only in endless mode)
        if (m_state.gameType == GameType::ENDLESS) {
            std::string waveStr = "Wave: " + std::to_string(m_state.wave);
            m_renderer.drawUIText(padX, (float)fbH - padTop - 35.0f, waveStr, 1.0f, glm::vec3(0.2f, 0.8f, 1.0f));

            if (m_state.endlessDangerActive && (m_state.mode == GameMode::PLAYING || m_state.mode == GameMode::PAUSED)) {
                if (m_state.endlessDangerTimer < 10.0f) {
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
                        float dScale = 2.8f; 
                        // Proper width calculation (Renderer::drawUIText uses 14*scale + 4*scale spacing)
                        float labelW = 14.0f * dScale;
                        float labelSpacing = 4.0f * dScale;
                        float dW = dMsg.size() * labelW + (dMsg.size() - 1) * labelSpacing;
                        float xPos = (float)fbW * 0.5f - dW * 0.5f;

                        // Center vertically inside the danger band (y in UI coords is bottom-up)
                        float screenY = (dangerLineScreenY >= 0.0f) ? dangerLineScreenY : (float)fbH * 0.30f;
                        screenY = std::max(0.0f, std::min((float)fbH, screenY));
                        float th = 20.0f * dScale;
                        float yPos = std::max(12.0f, (screenY * 0.5f) - th * 0.5f);
                        float yMid = yPos + th * 0.5f;

                        // When paused, add a subtle glow (kept light so it still reads "clean")
                        if (m_state.mode == GameMode::PAUSED) {
                            glm::vec4 glow(1.0f, 0.0f, 0.0f, triAlpha * 0.14f);
                            const float o = 2.0f;
                            m_renderer.drawUIText(xPos - o, yPos, dMsg, dScale, glow);
                            m_renderer.drawUIText(xPos + o, yPos, dMsg, dScale, glow);
                            m_renderer.drawUIText(xPos, yPos - o, dMsg, dScale, glow);
                            m_renderer.drawUIText(xPos, yPos + o, dMsg, dScale, glow);
                        }

                        // Main crisp text
                        m_renderer.drawUIText(xPos, yPos, dMsg, dScale, glm::vec4(1.0f, 0.0f, 0.0f, triAlpha));

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
        float panelW = 450.0f;
        float panelH = 200.0f;
        float panelX = (fbW - panelW) * 0.5f;
        float panelY = (fbH - panelH) * 0.5f;
        
        if (m_state.mode == GameMode::PAUSED) {
            // Calculate panel and button positions
            float panelW = 450.0f;
            float panelH = 280.0f; // Taller to fit PAUSED text + buttons
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
            float scale = 3.0f; 
            float charW = 14.0f * scale;
            float charSpacing = 4.0f * scale;
            float tw = msg.size() * charW + (msg.size() - 1) * charSpacing;
            float th = 20.0f * scale;
            
            float tx = (fbW - tw) * 0.5f;
            float ty = panelY + panelH - th - 40.0f;
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
            
            float labelW = 14.0f;
            float labelSpacing = 4.0f;
            float ltw = leftLabel.size() * labelW + (leftLabel.size() - 1) * labelSpacing;
            float rtw = rightLabel.size() * labelW + (rightLabel.size() - 1) * labelSpacing;
            
            m_renderer.drawUIText(btnX_left + (btnW - ltw) * 0.5f,  btnY + (btnH - 20.0f) * 0.5f, leftLabel, 1.0f, glm::vec3(1, 1, 1));
            m_renderer.drawUIText(btnX_right + (btnW - rtw) * 0.5f, btnY + (btnH - 20.0f) * 0.5f, rightLabel, 1.0f, glm::vec3(1, 1, 1));
        } else {
            // Full screen masked overlay for Game Over / Win
            glm::vec2 maskMin(panelX, panelY);
            glm::vec2 maskMax(panelX + panelW, panelY + panelH);
            m_renderer.drawUIQuad(0, 0, (float)fbW, (float)fbH, glm::vec4(0, 0, 0, 0.86f), true, maskMin, maskMax);

            // Panel for Game Over / Win - Fully opaque
            m_renderer.drawUIQuad(panelX, panelY, panelW, panelH, glm::vec4(0.08f, 0.08f, 0.08f, 1.0f));
            
            std::string title = (m_state.mode == GameMode::GAME_OVER) ? "GAME OVER" : "WINNER!";
            float tw = title.size() * 20.0f + (title.size() - 1) * 6.0f;
            float tx = panelX + (panelW - tw * 1.5f) * 0.5f;
            m_renderer.drawUIText(tx, panelY + panelH - 60.0f, title, 1.5f, glm::vec3(1, 1, 1));

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
            
            float labelW = 14.0f;
            float labelSpacing = 4.0f;
            float ltw = leftLabel.size() * labelW + (leftLabel.size() - 1) * labelSpacing;
            float rtw = rightLabel.size() * labelW + (rightLabel.size() - 1) * labelSpacing;
            
            m_renderer.drawUIText(btnX_left + (btnW - ltw) * 0.5f,  btnY + (btnH - 20.0f) * 0.5f, leftLabel, 1.0f, glm::vec3(1, 1, 1));
            m_renderer.drawUIText(btnX_right + (btnW - rtw) * 0.5f, btnY + (btnH - 20.0f) * 0.5f, rightLabel, 1.0f, glm::vec3(1, 1, 1));
        }
    }

    m_renderer.endUI();
    m_window.swapBuffers();
}

} // namespace game
