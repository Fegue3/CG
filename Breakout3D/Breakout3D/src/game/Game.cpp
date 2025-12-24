#include "game/Game.hpp"
#include "engine/Input.hpp"
#include "game/GameAssets.hpp"
#include <algorithm>
#include <cmath>
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

static void resetBallToPaddle(GameState& s, const GameConfig& cfg) {
    s.ballAttached = true;
    s.ballVel = glm::vec3(0.0f);
    s.ballPos = s.paddlePos + glm::vec3(
        0.0f, 0.0f,
        -(cfg.paddleSize.z * 0.5f + cfg.ballRadius + 0.15f)
    );
}

static bool pointInRectPx(float px, float py, float x, float y, float w, float h) {
    return (px >= x && px <= x + w && py >= y && py <= y + h);
}

Game::Game(engine::Window& window, engine::Time& time, engine::Renderer& renderer, GameAssets& assets)
: m_window(window), m_time(time), m_renderer(renderer), m_assets(assets) {}

void Game::init() {
    m_state.mode = GameMode::PLAYING;
    m_state.lives = 3;

    m_state.paddlePos = glm::vec3(
        0.0f, 0.0f,
        m_cfg.arenaMaxZ - (m_cfg.paddleSize.z * 0.5f) - 0.25f
    );

    resetBallToPaddle(m_state, m_cfg);

    m_state.brickHitCooldown = 0.0f;
    m_state.bricks.clear();

    // ✅ MAIS COLUNAS + 3 verdes e 2 do resto (total 9 filas)
    const int cols = 12;
    const int rows = 9; // 2 roxo + 2 azul + 2 amarelo + 3 verde

    glm::vec3 brickSize(2.6f, 0.7f, 1.2f);
    float gapX = 0.30f;
    float gapZ = 0.35f;

    float startZ = m_cfg.arenaMinZ + 2.2f;

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

    if (input.keyDown(engine::Key::Escape)) {
        m_window.requestClose();
        return;
    }

    if (m_state.brickHitCooldown > 0.0f)
        m_state.brickHitCooldown = std::max(0.0f, m_state.brickHitCooldown - dt);

    if (m_state.mode == GameMode::PLAYING && !anyBricksAlive(m_state)) {
        m_state.mode = GameMode::WIN;
        m_state.ballVel = glm::vec3(0.0f);
        m_state.ballAttached = true;
    }

    // ------------- GAME OVER / WIN: click UI -------------
    if (m_state.mode != GameMode::PLAYING) {
        auto [fbW, fbH] = m_window.getFramebufferSize();
        auto [px, py]   = input.mousePosFbPx();
        bool click      = input.mousePressed(engine::MouseButton::Left);

        // Panel dimensions
        float panelW = 450.0f;
        float panelH = 200.0f;
        float panelX = (fbW - panelW) * 0.5f;
        float panelY = (fbH - panelH) * 0.5f;
        
        // Button dimensions
        float btnW = 100.0f;
        float btnH = 60.0f;
        float btnGap = 50.0f;
        float btnX_left = panelX + (panelW - 2*btnW - btnGap) * 0.5f;
        float btnX_right = btnX_left + btnW + btnGap;
        float btnY = panelY + (panelH - btnH) * 0.5f;

        if (click) {
            // Green button: restart
            if (pointInRectPx(px, py, btnX_left, btnY, btnW, btnH)) { 
                init(); 
                return; 
            }
            // Red button: close window
            if (pointInRectPx(px, py, btnX_right, btnY, btnW, btnH)) { 
                m_window.requestClose(); 
                return; 
            }
        }
        return;
    }

    // ----------------- PLAYING -----------------
    float dir = 0.0f;
    if (input.keyDown(engine::Key::A) || input.keyDown(engine::Key::Left))  dir -= 1.0f;
    if (input.keyDown(engine::Key::D) || input.keyDown(engine::Key::Right)) dir += 1.0f;

    m_state.paddlePos.x += dir * m_cfg.paddleSpeed * dt;

    float paddleHalfX = m_cfg.paddleSize.x * 0.5f;
    m_state.paddlePos.x = std::clamp(
        m_state.paddlePos.x,
        m_cfg.arenaMinX + paddleHalfX,
        m_cfg.arenaMaxX - paddleHalfX
    );

    if (m_state.ballAttached) {
        resetBallToPaddle(m_state, m_cfg);

        if (input.keyDown(engine::Key::Space)) {
            m_state.ballAttached = false;
            glm::vec3 d = glm::normalize(glm::vec3(0.30f, 0.0f, -1.0f));
            m_state.ballVel = d * m_cfg.ballSpeed;
        }
        return;
    }

    m_state.ballPos += m_state.ballVel * dt;

    // walls bounce
    if (m_state.ballPos.x - m_cfg.ballRadius < m_cfg.arenaMinX) {
        m_state.ballPos.x = m_cfg.arenaMinX + m_cfg.ballRadius;
        m_state.ballVel.x = std::abs(m_state.ballVel.x);
    }
    if (m_state.ballPos.x + m_cfg.ballRadius > m_cfg.arenaMaxX) {
        m_state.ballPos.x = m_cfg.arenaMaxX - m_cfg.ballRadius;
        m_state.ballVel.x = -std::abs(m_state.ballVel.x);
    }
    if (m_state.ballPos.z - m_cfg.ballRadius < m_cfg.arenaMinZ) {
        m_state.ballPos.z = m_cfg.arenaMinZ + m_cfg.ballRadius;
        m_state.ballVel.z = std::abs(m_state.ballVel.z);
    }

    // Paddle collision
    {
        float paddleHalfX = m_cfg.paddleSize.x * 0.5f;
        float halfZ = m_cfg.paddleSize.z * 0.5f;

        float minX = m_state.paddlePos.x - paddleHalfX;
        float maxX = m_state.paddlePos.x + paddleHalfX;
        float minZ = m_state.paddlePos.z - halfZ;
        float maxZ = m_state.paddlePos.z + halfZ;

        float cx = clampf(m_state.ballPos.x, minX, maxX);
        float cz = clampf(m_state.ballPos.z, minZ, maxZ);

        float dx = m_state.ballPos.x - cx;
        float dz = m_state.ballPos.z - cz;

        if (dx*dx + dz*dz <= m_cfg.ballRadius*m_cfg.ballRadius && m_state.ballVel.z > 0.0f) {
            m_state.ballPos.z = minZ - m_cfg.ballRadius - 0.002f;

            float t = (m_state.ballPos.x - m_state.paddlePos.x) / paddleHalfX;
            t = clampf(t, -1.0f, 1.0f);

            float ang = t * glm::radians(60.0f);
            m_state.ballVel.x = std::sin(ang) * m_cfg.ballSpeed;
            m_state.ballVel.z = -std::cos(ang) * m_cfg.ballSpeed;
        }
    }

    // Brick collisions
    if (m_state.brickHitCooldown <= 0.0f) {
        for (auto& b : m_state.bricks) {
            if (!b.alive) continue;

            if (sphereAabbXZ(m_state.ballPos, m_cfg.ballRadius, b.pos, b.size)) {

                // ✅ HIT: só reduz HP (o render é que escolhe *qual OBJ* mostrar)
                b.hp--;
                if (b.hp <= 0) b.alive = false;

                glm::vec3 diff = m_state.ballPos - b.pos;
                float ax = std::abs(diff.x) / (b.size.x * 0.5f);
                float az = std::abs(diff.z) / (b.size.z * 0.5f);

                if (ax > az) {
                    m_state.ballVel.x = -m_state.ballVel.x;
                    float sign = (diff.x >= 0.0f) ? 1.0f : -1.0f;
                    m_state.ballPos.x = b.pos.x + sign * (b.size.x * 0.5f + m_cfg.ballRadius + 0.002f);
                } else {
                    m_state.ballVel.z = -m_state.ballVel.z;
                    float sign = (diff.z >= 0.0f) ? 1.0f : -1.0f;
                    m_state.ballPos.z = b.pos.z + sign * (b.size.z * 0.5f + m_cfg.ballRadius + 0.002f);
                }

                m_state.brickHitCooldown = 0.045f;
                break;
            }
        }
    }

    // Ball lost
    if (m_state.ballPos.z - m_cfg.ballRadius > m_cfg.arenaMaxZ) {
        m_state.lives--;

        if (m_state.lives > 0) {
            resetBallToPaddle(m_state, m_cfg);
        } else {
            m_state.mode = anyBricksAlive(m_state) ? GameMode::GAME_OVER : GameMode::WIN;
        }
    }
}

void Game::render() {
    auto [fbW, fbH] = m_window.getFramebufferSize();
    m_renderer.beginFrame(fbW, fbH);

    // -------- 3D PASS --------
    float arenaW = (m_cfg.arenaMaxX - m_cfg.arenaMinX);
    float arenaD = (m_cfg.arenaMaxZ - m_cfg.arenaMinZ);
    float base = std::max(arenaW, arenaD);

    glm::vec3 camPos(0.0f, base * 0.55f, base * 0.85f);
    glm::vec3 camTarget(0.0f, 0.0f, 0.0f);

    glm::mat4 V = glm::lookAt(camPos, camTarget, glm::vec3(0,1,0));
    glm::mat4 P = glm::perspective(glm::radians(58.0f), (float)fbW/(float)fbH, 0.1f, 300.0f);
    m_renderer.setCamera(V, P, camPos);

    // ✅ NÃO usamos tint para “dar cor” (só deixa as texturas falarem)
    glm::vec3 tint(1.0f);

    // Walls (usa brick01 como textura base)
    float zCenter = (m_cfg.arenaMinZ + m_cfg.arenaMaxZ) * 0.5f;
    float sideThickness = 1.2f;
    float topThickness  = 1.2f;
    float wallHeight    = 1.0f;

    m_renderer.drawMesh(m_assets.brick01,
        glm::vec3(m_cfg.arenaMinX - sideThickness*0.5f, 0.0f, zCenter),
        glm::vec3(sideThickness, wallHeight, arenaD + topThickness),
        tint
    );

    m_renderer.drawMesh(m_assets.brick01,
        glm::vec3(m_cfg.arenaMaxX + sideThickness*0.5f, 0.0f, zCenter),
        glm::vec3(sideThickness, wallHeight, arenaD + topThickness),
        tint
    );

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
    m_renderer.drawMesh(m_assets.paddle, m_state.paddlePos, m_cfg.paddleSize, tint);

    // Ball
    float d = m_cfg.ballRadius * 2.0f;
    m_renderer.drawMesh(m_assets.ball, m_state.ballPos, glm::vec3(d, d, d), tint);

    // -------- UI PASS (HUD 3D em ortho) --------
    m_renderer.beginUI(fbW, fbH);

    const float padX = 22.0f;
    const float padTop = 18.0f;

    const float hs = 56.0f;   // ✅ menor (era 70)
    const float gap = 12.0f;

    const float rx = glm::radians(18.0f);
    const float ry = glm::radians(-12.0f);

    for (int i = 0; i < 3; ++i) {
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

    // Game Over / Win overlay
    if (m_state.mode != GameMode::PLAYING) {
        // Black background panel
        float panelW = 450.0f;
        float panelH = 200.0f;
        float panelX = (fbW - panelW) * 0.5f;
        float panelY = (fbH - panelH) * 0.5f;
        
        m_renderer.drawUIQuad(panelX, panelY, panelW, panelH, glm::vec3(0.1f, 0.1f, 0.1f));
        
        // Buttons
        float btnW = 140.0f;
        float btnH = 70.0f;
        float btnGap = 50.0f;
        float btnX_left = panelX + (panelW - 2*btnW - btnGap) * 0.5f;
        float btnX_right = btnX_left + btnW + btnGap;
        float btnY = panelY + (panelH - btnH) * 0.5f;
        
        m_renderer.drawUIQuad(btnX_left, btnY, btnW, btnH, glm::vec3(0.2f, 0.8f, 0.2f));
        m_renderer.drawUIQuad(btnX_right, btnY, btnW, btnH, glm::vec3(0.8f, 0.2f, 0.2f));
        
        // Draw text on buttons - centered
        float labelH = 20.0f;
        float labelW = 14.0f;
        float labelSpacing = 4.0f;
        std::string leftLabel = "RESTART";
        std::string rightLabel = "SAIR";
        
        float leftTextWidth = leftLabel.size() * labelW + (leftLabel.size() - 1) * labelSpacing;
        float rightTextWidth = rightLabel.size() * labelW + (rightLabel.size() - 1) * labelSpacing;
        
        float leftX = btnX_left + (btnW - leftTextWidth) * 0.5f;
        float rightX = btnX_right + (btnW - rightTextWidth) * 0.5f;
        float labelY = btnY + (btnH - labelH) * 0.5f;
        
        m_renderer.drawUIText(leftX, labelY, leftLabel, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
        m_renderer.drawUIText(rightX, labelY, rightLabel, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
    }

    m_renderer.endUI();
    m_window.swapBuffers();
}

} // namespace game
