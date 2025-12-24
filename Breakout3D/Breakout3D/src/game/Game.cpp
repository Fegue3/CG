#include "game/Game.hpp"
#include "engine/Input.hpp"
#include "game/GameAssets.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <glm/gtc/matrix_transform.hpp>

static float clampf(float v, float a, float b) { return std::max(a, std::min(v, b)); }

static bool sphereAabbXZ(const glm::vec3& c, float r, const glm::vec3& bpos, const glm::vec3& bsize) {
    float hx = bsize.x * 0.5f;
    float hz = bsize.z * 0.5f;

    float minX = bpos.x - hx, maxX = bpos.x + hx;
    float minZ = bpos.z - hz, maxZ = bpos.z + hz;

    float cx = clampf(c.x, minX, maxX);
    float cz = clampf(c.z, minZ, maxZ);

    float dx = c.x - cx;
    float dz = c.z - cz;
    return (dx * dx + dz * dz) <= (r * r);
}

namespace game {

static bool anyBricksAlive(const GameState& s) {
    for (const auto& b : s.bricks) if (b.alive) return true;
    return false;
}

static void resetBallToPaddle(Ball& b, const glm::vec3& paddlePos, const GameConfig& cfg) {
    b.attached = true;
    b.vel = glm::vec3(0.0f);
    b.pos = paddlePos + glm::vec3(
        0.0f, 0.0f,
        -(cfg.paddleSize.z * 0.5f + cfg.ballRadius + 0.15f)
    );
}

static void spawnPowerUp(GameState& s, const glm::vec3& pos, float chance) {
    float r = (float)rand() / (float)RAND_MAX;
    if (r > chance) return;

    PowerUp p;
    p.pos = pos;
    // Lift slightly off the ground initially
    p.pos.y = 0.4f;

    int r2 = rand() % 100;
    if (r2 < 2)        p.type = PowerUpType::EXTRA_LIFE; // 2% (Rarer)
    else if (r2 < 35)  p.type = PowerUpType::EXPAND;     // 33%
    else if (r2 < 68)  p.type = PowerUpType::SLOW;       // 33%
    else               p.type = PowerUpType::EXTRA_BALL; // 32%
    
    s.powerups.push_back(p);
}


static bool pointInRectPx(float px, float py, float x, float y, float w, float h) {
    return (px >= x && px <= x + w && py >= y && py <= y + h);
}

Game::Game(engine::Window& window, engine::Time& time, engine::Renderer& renderer, GameAssets& assets)
: m_window(window), m_time(time), m_renderer(renderer), m_assets(assets) {}

void Game::init() {
    srand(static_cast<unsigned int>(time(nullptr)));
    m_state.mode = GameMode::PLAYING;
    m_state.lives = 3;

    m_state.paddlePos = glm::vec3(
        0.0f, 0.0f,
        m_cfg.arenaMaxZ - (m_cfg.paddleSize.z * 0.5f) - 0.25f
    );

    m_state.balls.clear();
    Ball firstBall;
    resetBallToPaddle(firstBall, m_state.paddlePos, m_cfg);
    m_state.balls.push_back(firstBall);

    m_state.powerups.clear();
    m_state.expandTimer = 0.0f;
    m_state.slowTimer = 0.0f;

    m_state.brickHitCooldown = 0.0f;
    m_state.bricks.clear();

    // ✅ MAIS COLUNAS + 3 verdes e 2 do resto (total 9 filas)
    const int cols = 12;
    const int rows = 9; // 2 roxo + 2 azul + 2 amarelo + 3 verde

    // ✅ Slight gaps (0.05) on the sides and depth for better aesthetics.
    // Width: 12 * 2.95 = 35.4 (vs 36.0 arena) => 0.3 gap on each side.
    glm::vec3 brickSize(2.95f, 0.7f, 1.30f); 
    float gapX = 0.04f;
    float gapZ = 0.03f;

    float startZ = m_cfg.arenaMinZ + 0.85f;

    float totalW = cols * brickSize.x + (cols - 1) * gapX;
    float leftX  = -totalW * 0.5f + brickSize.x * 0.5f;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            Brick b;
            b.size = brickSize;
            b.alive = true;

            b.pos.x = leftX + c * (brickSize.x + gapX);
            b.pos.y = 0.0f;
            b.pos.z = startZ + r * (brickSize.z + gapZ);

            // ✅ 2 roxo, 2 azul, 2 amarelo, 3 verde
            // r: 0-1 => 4, 2-3 => 3, 4-5 => 2, 6-8 => 1
            if (r <= 1)      b.maxHp = b.hp = 4;
            else if (r <= 3) b.maxHp = b.hp = 3;
            else if (r <= 5) b.maxHp = b.hp = 2;
            else             b.maxHp = b.hp = 1;

            m_state.bricks.push_back(b);
        }
    }
}

void Game::update(const engine::Input& input) {
    float dt = m_time.delta();

    // Toggle Pause with Escape
    if (input.keyPressed(engine::Key::Escape)) {
        if (m_state.mode == GameMode::PLAYING) m_state.mode = GameMode::PAUSED;
        else if (m_state.mode == GameMode::PAUSED) m_state.mode = GameMode::PLAYING;
    }

    if (m_state.mode == GameMode::PAUSED) return;

    auto [fbW, fbH] = m_window.getFramebufferSize();

    // --- BG Selector HUD Logic ---
    {
        float boxSize = 30.0f;
        float gap = 10.0f;
        float startX = fbW - (boxSize + gap) * 5.0f - 20.0f;
        float startY = 20.0f; // Top padding

        auto [mx, my_raw] = input.mousePosFbPx();
        float my = (float)fbH - my_raw; 

        if (input.mousePressed(engine::MouseButton::Left)) {
            for (int i = -1; i < 4; i++) {
                float bx = startX + (i + 1) * (boxSize + gap);
                float by = fbH - startY - boxSize;
                if (mx >= bx && mx <= bx + boxSize && my >= by && my <= by + boxSize) {
                    m_state.currentBg = i;
                }
            }
        }
    }

    if (input.keyPressed(engine::Key::K1)) m_state.cameraMode = 1;
    if (input.keyPressed(engine::Key::K2)) m_state.cameraMode = 2;
    if (m_state.brickHitCooldown > 0.0f)
        m_state.brickHitCooldown = std::max(0.0f, m_state.brickHitCooldown - dt);

    if (m_state.mode == GameMode::PLAYING && !anyBricksAlive(m_state)) {
        m_state.mode = GameMode::WIN;
        m_state.balls.clear();
    }


    // ------------- GAME OVER / WIN: Click UI logic -------------
    if (m_state.mode == GameMode::GAME_OVER || m_state.mode == GameMode::WIN) {
        auto [px, py_raw]   = input.mousePosFbPx();
        float py = (float)fbH - py_raw; // Flip Y: Top-Left to Bottom-Left
        bool click      = input.mousePressed(engine::MouseButton::Left);

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
            if (pointInRectPx(px, py, btnX_left, btnY, btnW, btnH)) { 
                init(); 
                return; 
            }
            // Right button: Close window
            if (pointInRectPx(px, py, btnX_right, btnY, btnW, btnH)) { 
                m_window.requestClose(); 
                return; 
            }
        }
        return;
    }

    // ----------------- PLAYING -----------------
    // Timers
    if (m_state.expandTimer > 0.0f) m_state.expandTimer = std::max(0.0f, m_state.expandTimer - dt);
    if (m_state.slowTimer > 0.0f)   m_state.slowTimer = std::max(0.0f, m_state.slowTimer - dt);

    float currentPaddleSpeed = m_cfg.paddleSpeed;
    if (m_state.slowTimer > 0.0f) currentPaddleSpeed *= m_cfg.slowSpeedFactor;

    glm::vec3 currentPaddleSize = m_cfg.paddleSize;
    if (m_state.expandTimer > 0.0f) currentPaddleSize.x *= m_cfg.expandScaleFactor;

    float dir = 0.0f;
    if (input.keyDown(engine::Key::A) || input.keyDown(engine::Key::Left))  dir -= 1.0f;
    if (input.keyDown(engine::Key::D) || input.keyDown(engine::Key::Right)) dir += 1.0f;

    m_state.paddlePos.x += dir * currentPaddleSpeed * dt;

    float paddleHalfX = currentPaddleSize.x * 0.5f;
    m_state.paddlePos.x = std::clamp(
        m_state.paddlePos.x,
        m_cfg.arenaMinX + paddleHalfX,
        m_cfg.arenaMaxX - paddleHalfX
    );

    // --- BALLS UPDATE ---
    for (size_t i = 0; i < m_state.balls.size(); ) {
        Ball& b = m_state.balls[i];

        if (b.attached) {
            resetBallToPaddle(b, m_state.paddlePos, m_cfg);
            if (input.keyDown(engine::Key::Space)) {
                b.attached = false;
                glm::vec3 d = glm::normalize(glm::vec3(0.30f, 0.0f, -1.0f));
                b.vel = d * m_cfg.ballSpeed;
            }
            i++;
            continue;
        }

        b.pos += b.vel * dt;

        // walls bounce
        if (b.pos.x - m_cfg.ballRadius < m_cfg.arenaMinX) {
            b.pos.x = m_cfg.arenaMinX + m_cfg.ballRadius;
            b.vel.x = std::abs(b.vel.x);
        }
        if (b.pos.x + m_cfg.ballRadius > m_cfg.arenaMaxX) {
            b.pos.x = m_cfg.arenaMaxX - m_cfg.ballRadius;
            b.vel.x = -std::abs(b.vel.x);
        }
        if (b.pos.z - m_cfg.ballRadius < m_cfg.arenaMinZ) {
            b.pos.z = m_cfg.arenaMinZ + m_cfg.ballRadius;
            b.vel.z = std::abs(b.vel.z);
        }

        // Paddle collision
        {
            float halfZ = currentPaddleSize.z * 0.5f;
            float minX = m_state.paddlePos.x - paddleHalfX;
            float maxX = m_state.paddlePos.x + paddleHalfX;
            float minZ = m_state.paddlePos.z - halfZ;
            float maxZ = m_state.paddlePos.z + halfZ;

            float cx = clampf(b.pos.x, minX, maxX);
            float cz = clampf(b.pos.z, minZ, maxZ);

            float dx = b.pos.x - cx;
            float dz = b.pos.z - cz;

            if (dx*dx + dz*dz <= m_cfg.ballRadius*m_cfg.ballRadius) {
                // Hitting the front face
                if (cz == minZ && b.vel.z > 0.0f) {
                    b.pos.z = minZ - m_cfg.ballRadius - 0.002f;
                    float t = (b.pos.x - m_state.paddlePos.x) / paddleHalfX;
                    t = clampf(t, -1.0f, 1.0f);
                    float ang = t * glm::radians(60.0f);
                    b.vel.x = std::sin(ang) * m_cfg.ballSpeed;
                    b.vel.z = -std::cos(ang) * m_cfg.ballSpeed;
                }
                // Hitting the sides
                else if (cx == minX || cx == maxX) {
                    b.vel.x = -b.vel.x;
                    float sideSign = (b.pos.x < cx) ? -1.0f : 1.0f;
                    b.pos.x = cx + sideSign * (m_cfg.ballRadius + 0.002f);
                }
            }
        }

        // Brick collisions
        if (m_state.brickHitCooldown <= 0.0f) {
            for (auto& br : m_state.bricks) {
                if (!br.alive) continue;
                if (sphereAabbXZ(b.pos, m_cfg.ballRadius, br.pos, br.size)) {
                    br.hp--;
                    if (br.hp <= 0) {
                        br.alive = false;
                        spawnPowerUp(m_state, br.pos, m_cfg.powerUpChance);
                    }

                    glm::vec3 diff = b.pos - br.pos;
                    float ax = std::abs(diff.x) / (br.size.x * 0.5f);
                    float az = std::abs(diff.z) / (br.size.z * 0.5f);

                    if (ax > az) {
                        b.vel.x = -b.vel.x;
                        float sign = (diff.x >= 0.0f) ? 1.0f : -1.0f;
                        b.pos.x = br.pos.x + sign * (br.size.x * 0.5f + m_cfg.ballRadius + 0.002f);
                    } else {
                        b.vel.z = -b.vel.z;
                        float sign = (diff.z >= 0.0f) ? 1.0f : -1.0f;
                        b.pos.z = br.pos.z + sign * (br.size.z * 0.5f + m_cfg.ballRadius + 0.002f);
                    }
                    m_state.brickHitCooldown = 0.045f;
                    break;
                }
            }
        }

        // Ball lost check - fly away just a bit
        if (b.pos.z - m_cfg.ballRadius > 20.0f) {
            m_state.balls.erase(m_state.balls.begin() + i);
        } else {
            i++;
        }
    }

    // --- POWERUPS UPDATE ---
    for (size_t i = 0; i < m_state.powerups.size(); ) {
        PowerUp& p = m_state.powerups[i];
        p.pos.z += m_cfg.powerUpDropSpeed * dt;

        // Collision with paddle
        float halfX = currentPaddleSize.x * 0.5f;
        float halfZ = currentPaddleSize.z * 0.5f;
        if (std::abs(p.pos.x - m_state.paddlePos.x) < halfX + m_cfg.ballRadius &&
            std::abs(p.pos.z - m_state.paddlePos.z) < halfZ + m_cfg.ballRadius) {
            
            // Activate
            if (p.type == PowerUpType::EXTRA_LIFE) {
                m_state.lives++;
            } else if (p.type == PowerUpType::EXTRA_BALL) {
                for (int k = 0; k < 3; k++) {
                    Ball nb;
                    nb.pos = m_state.paddlePos + glm::vec3(0,0,-0.5f);
                    float ang = glm::radians(-30.0f + k * 30.0f);
                    nb.vel = glm::vec3(std::sin(ang), 0.0f, -std::cos(ang)) * m_cfg.ballSpeed;
                    nb.attached = false;
                    m_state.balls.push_back(nb);
                }
            } else if (p.type == PowerUpType::SLOW) {
                m_state.slowTimer = m_cfg.powerUpDuration;
            } else if (p.type == PowerUpType::EXPAND) {
                m_state.expandTimer = m_cfg.powerUpDuration;
            }

            p.alive = false;
        }

        if (!p.alive || p.pos.z > 20.0f) {
            m_state.powerups.erase(m_state.powerups.begin() + i);
        } else {
            i++;
        }
    }

    // --- LIVES LOSS ---
    if (m_state.balls.empty()) {
        m_state.lives--;
        if (m_state.lives > 0) {
            Ball b;
            resetBallToPaddle(b, m_state.paddlePos, m_cfg);
            m_state.balls.push_back(b);
        } else {
            m_state.mode = anyBricksAlive(m_state) ? GameMode::GAME_OVER : GameMode::WIN;
        }
    }
}

void Game::render() {
    auto [fbW, fbH] = m_window.getFramebufferSize();
    m_renderer.beginFrame(fbW, fbH);

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

    // Overlay (Pause / Game Over / Win)
    if (m_state.mode != GameMode::PLAYING) {
        float panelW = 450.0f;
        float panelH = 200.0f;
        float panelX = (fbW - panelW) * 0.5f;
        float panelY = (fbH - panelH) * 0.5f;
        
        if (m_state.mode == GameMode::PAUSED) {
            // Just text for Pause - Higher centered
            std::string msg = "PAUSED";
            float tw = msg.size() * 20.0f + (msg.size() - 1) * 6.0f;
            float tx = (fbW - tw * 1.5f) * 0.5f;
            m_renderer.drawUIText(tx, fbH * 0.65f, msg, 1.5f, glm::vec3(1, 1, 1));
        } else {
            // Panel for Game Over / Win
            m_renderer.drawUIQuad(panelX, panelY, panelW, panelH, glm::vec3(0.08f, 0.08f, 0.08f));
            
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
            
            m_renderer.drawUIQuad(btnX_left, btnY, btnW, btnH, glm::vec3(0.2f, 0.8f, 0.2f));
            m_renderer.drawUIQuad(btnX_right, btnY, btnW, btnH, glm::vec3(0.8f, 0.2f, 0.2f));
            
            std::string leftLabel = "RESTART";
            std::string rightLabel = "SAIR";
            
            float labelW = 14.0f;
            float labelSpacing = 4.0f;
            float ltw = leftLabel.size() * labelW + (leftLabel.size() - 1) * labelSpacing;
            float rtw = rightLabel.size() * labelW + (rightLabel.size() - 1) * labelSpacing;
            
            m_renderer.drawUIText(btnX_left + (btnW - ltw) * 0.5f,  btnY + (btnH - 20.0f) * 0.5f, leftLabel, 1.0f, glm::vec3(1, 1, 1));
            m_renderer.drawUIText(btnX_right + (btnW - rtw) * 0.5f, btnY + (btnH - 20.0f) * 0.5f, rightLabel, 1.0f, glm::vec3(1, 1, 1));
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
            {0.9f, 0.2f, 0.3f}, // Redish
            {0.2f, 0.8f, 0.4f}  // Greenish
        };

        for (int i = -1; i < 4; i++) {
            float bx = startX + (i + 1) * (boxSize + gap);
            glm::vec3 col = colors[i + 1];
            
            // Draw border if selected
            if (m_state.currentBg == i) {
                float border = 2.0f;
                m_renderer.drawUIQuad(bx - border, startY - border, boxSize + border * 2.0f, boxSize + border * 2.0f, glm::vec3(1, 1, 1));
            } else {
                col *= 0.6f;
            }

            m_renderer.drawUIQuad(bx, startY, boxSize, boxSize, col);

            if (i == -1) {
                // Draw "/" inside the first box
                float tx = bx + (boxSize - 10.0f) * 0.5f;
                float ty = startY + (boxSize - 16.0f) * 0.5f;
                m_renderer.drawUIText(tx, ty, "/", 0.8f, glm::vec3(1, 1, 1));
            }
        }
    }

    m_renderer.endUI();
    m_window.swapBuffers();
}

} // namespace game
