#include "game/Game.hpp"

// ⚠️ ORDEM IMPORTANTE: GLEW antes de GLFW (senão dá gl.h before glew.h)
#include <GL/glew.h>
#include <GLFW/glfw3.h>

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

// ------------------- UI helpers (2D) -------------------
static void drawRect2D(engine::Renderer& r, float x, float y, float w, float h, const glm::vec3& color, float z = 0.0f) {
    glm::vec3 center(x + w * 0.5f, y + h * 0.5f, z);
    glm::vec3 size(w, h, 1.0f);
    r.drawCube({}, center, size, color);
}

static bool pointInRectPx(float px, float py, float x, float y, float w, float h) {
    return (px >= x && px <= x + w && py >= y && py <= y + h);
}

static void drawHeart2D(engine::Renderer& r, float x, float y, float s, const glm::vec3& color) {
    float w = s, h = s;

    drawRect2D(r, x + w*0.20f, y + h*0.15f, w*0.30f, h*0.30f, color);
    drawRect2D(r, x + w*0.50f, y + h*0.15f, w*0.30f, h*0.30f, color);

    drawRect2D(r, x + w*0.20f, y + h*0.38f, w*0.60f, h*0.30f, color);

    drawRect2D(r, x + w*0.32f, y + h*0.62f, w*0.36f, h*0.22f, color);
    drawRect2D(r, x + w*0.41f, y + h*0.78f, w*0.18f, h*0.18f, color);
}

// fonte 5x7 mínima (só letras que usamos)
static const unsigned char* glyph(char c) {
    static unsigned char A[7] = {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11};
    static unsigned char C[7] = {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E};
    static unsigned char E[7] = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F};
    static unsigned char F[7] = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10};
    static unsigned char G[7] = {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F};
    static unsigned char H[7] = {0x11,0x11,0x11,0x1F,0x11,0x11,0x11};
    static unsigned char I[7] = {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E};
    static unsigned char N[7] = {0x11,0x19,0x15,0x13,0x11,0x11,0x11};
    static unsigned char O[7] = {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E};
    static unsigned char R[7] = {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11};
    static unsigned char S[7] = {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E};
    static unsigned char U[7] = {0x11,0x11,0x11,0x11,0x11,0x11,0x0E};
    static unsigned char V[7] = {0x11,0x11,0x11,0x11,0x11,0x0A,0x04};
    static unsigned char W[7] = {0x11,0x11,0x11,0x15,0x15,0x15,0x0A};
    static unsigned char Y[7] = {0x11,0x11,0x0A,0x04,0x04,0x04,0x04};
    static unsigned char _[7] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    static unsigned char SP[7]= {0x00,0x00,0x00,0x00,0x00,0x00,0x00};

    switch (c) {
        case 'A': return A; case 'C': return C; case 'E': return E; case 'F': return F;
        case 'G': return G; case 'H': return H; case 'I': return I; case 'N': return N;
        case 'O': return O; case 'R': return R; case 'S': return S; case 'U': return U;
        case 'V': return V; case 'W': return W; case 'Y': return Y; case ' ': return SP;
        default:  return _;
    }
}

static void drawText2D(engine::Renderer& r, float x, float y, float scale, const glm::vec3& color, const char* text) {
    float cx = x;
    for (const char* p = text; *p; ++p) {
        char c = *p;
        if (c >= 'a' && c <= 'z') c = char(c - 'a' + 'A');

        const unsigned char* g = glyph(c);

        for (int row = 0; row < 7; ++row) {
            unsigned char bits = g[row];
            for (int col = 0; col < 5; ++col) {
                bool on = (bits & (1 << (4 - col))) != 0;
                if (!on) continue;

                float px = cx + col * scale;
                float py = y  + row * scale;
                drawRect2D(r, px, py, scale, scale, color);
            }
        }
        cx += 6 * scale;
    }
}

static int textWidthPx(const char* text, float scale) {
    int len = 0;
    for (const char* p = text; *p; ++p) ++len;
    return (int)(len * 6 * scale);
}

namespace game {

Game::Game(engine::Window& window, engine::Time& time, engine::Renderer& renderer)
: m_window(window), m_time(time), m_renderer(renderer) {}

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

void Game::init() {
    m_state.mode = GameMode::PLAYING;
    m_state.lives = 3;

    m_state.paddlePos = glm::vec3(
        0.0f, 0.0f,
        m_cfg.arenaMaxZ - (m_cfg.paddleSize.z * 0.5f) - 0.25f
    );

    resetBallToPaddle(m_state, m_cfg);

    m_state.brickHitCooldown = 0.0f;
    m_state.mouseWasDown = false;

    m_state.bricks.clear();

    int cols = 10;
    int rows = 6;

    glm::vec3 brickSize(2.2f, 0.6f, 1.0f);
    float gapX = 0.35f;
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

            if (r == 0)      b.maxHp = b.hp = 4;
            else if (r == 1) b.maxHp = b.hp = 3;
            else if (r == 2) b.maxHp = b.hp = 2;
            else             b.maxHp = b.hp = 1;

            m_state.bricks.push_back(b);
        }
    }
}

void Game::update() {
    GLFWwindow* w = m_window.getHandle();   // ✅ FIX: era handle()
    float dt = m_time.delta();

    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(w, 1);

    if (m_state.brickHitCooldown > 0.0f)
        m_state.brickHitCooldown = std::max(0.0f, m_state.brickHitCooldown - dt);

    if (m_state.mode == GameMode::PLAYING && !anyBricksAlive(m_state)) {
        m_state.mode = GameMode::WIN;
        m_state.ballVel = glm::vec3(0.0f);
        m_state.ballAttached = true;
    }

    // ------------- GAME OVER / WIN: click 2D -------------
    if (m_state.mode != GameMode::PLAYING) {
        auto [fbW, fbH] = m_window.getFramebufferSize(); // ✅ FIX: era getFramebufferSize(fbW, fbH)

        int ww = 1, wh = 1;
        glfwGetWindowSize(w, &ww, &wh);
        ww = std::max(1, ww);
        wh = std::max(1, wh);

        double mx, my;
        glfwGetCursorPos(w, &mx, &my);

        float sx = (float)fbW / (float)ww;
        float sy = (float)fbH / (float)wh;

        float px = (float)mx * sx;
        float py = (float)my * sy;

        bool down = (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
        bool click = (down && !m_state.mouseWasDown);
        m_state.mouseWasDown = down;

        float bw = 220.0f, bh = 60.0f;
        float gap = 30.0f;
        float cx = fbW * 0.5f;
        float cy = fbH * 0.55f;

        float rx = cx - bw - gap * 0.5f;
        float ry = cy;
        float fx = cx + gap * 0.5f;
        float fy = cy;

        if (click) {
            if (pointInRectPx(px, py, rx, ry, bw, bh)) { init(); return; }
            if (pointInRectPx(px, py, fx, fy, bw, bh)) { glfwSetWindowShouldClose(w, 1); return; }
        }
        return;
    }

    // ----------------- PLAYING -----------------
    float dir = 0.0f;
    if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_LEFT) == GLFW_PRESS) dir -= 1.0f;
    if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_RIGHT) == GLFW_PRESS) dir += 1.0f;

    m_state.paddlePos.x += dir * m_cfg.paddleSpeed * dt;

    float paddleHalfX = m_cfg.paddleSize.x * 0.5f;
    m_state.paddlePos.x = std::clamp(
        m_state.paddlePos.x,
        m_cfg.arenaMinX + paddleHalfX,
        m_cfg.arenaMaxX - paddleHalfX
    );

    if (m_state.ballAttached) {
        resetBallToPaddle(m_state, m_cfg);

        if (glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS) {
            m_state.ballAttached = false;
            glm::vec3 d = glm::normalize(glm::vec3(0.30f, 0.0f, -1.0f));
            m_state.ballVel = d * m_cfg.ballSpeed;
        }
        return;
    }

    m_state.ballPos += m_state.ballVel * dt;

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

    // Brick collisions (cooldown anti double hit)
    if (m_state.brickHitCooldown <= 0.0f) {
        for (auto& b : m_state.bricks) {
            if (!b.alive) continue;

            if (sphereAabbXZ(m_state.ballPos, m_cfg.ballRadius, b.pos, b.size)) {
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
            if (anyBricksAlive(m_state)) m_state.mode = GameMode::GAME_OVER;
            else m_state.mode = GameMode::WIN;
        }
    }
}

void Game::render() {
    auto [fbW, fbH] = m_window.getFramebufferSize(); // ✅ FIX
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

    // Walls
    float zCenter = (m_cfg.arenaMinZ + m_cfg.arenaMaxZ) * 0.5f;
    glm::vec3 wallColor(0.20f, 0.30f, 0.70f);
    float sideThickness = 1.2f;
    float topThickness  = 1.2f;
    float wallHeight    = 1.0f;

    m_renderer.drawCube({}, glm::vec3(m_cfg.arenaMinX - sideThickness*0.5f, 0.0f, zCenter),
                        glm::vec3(sideThickness, wallHeight, arenaD + topThickness), wallColor);

    m_renderer.drawCube({}, glm::vec3(m_cfg.arenaMaxX + sideThickness*0.5f, 0.0f, zCenter),
                        glm::vec3(sideThickness, wallHeight, arenaD + topThickness), wallColor);

    m_renderer.drawCube({}, glm::vec3(0.0f, 0.0f, m_cfg.arenaMinZ - topThickness*0.5f),
                        glm::vec3(arenaW + sideThickness*2.0f, wallHeight, topThickness), wallColor);

    // Bricks
    for (const auto& b : m_state.bricks) {
        if (!b.alive) continue;

        glm::vec3 color;
        if (b.maxHp == 4) {
            if (b.hp == 4)      color = {0.45f, 0.15f, 0.65f};
            else if (b.hp == 3) color = {0.55f, 0.25f, 0.75f};
            else if (b.hp == 2) color = {0.70f, 0.45f, 0.85f};
            else                color = {0.85f, 0.70f, 0.95f};
        } else if (b.maxHp == 3) {
            if (b.hp == 3)      color = {0.20f, 0.30f, 0.85f};
            else if (b.hp == 2) color = {0.45f, 0.55f, 0.95f};
            else                color = {0.75f, 0.82f, 1.00f};
        } else if (b.maxHp == 2) {
            if (b.hp == 2)      color = {0.90f, 0.80f, 0.20f};
            else                color = {0.98f, 0.95f, 0.55f};
        } else {
            color = {0.20f, 0.80f, 0.35f};
        }

        m_renderer.drawCube({}, b.pos, b.size, color);
    }

    // Paddle
    glm::vec3 paddleColor(0.95f, 0.95f, 0.95f);
    glm::vec3 bodySize = m_cfg.paddleSize;
    glm::vec3 capSize  = glm::vec3(bodySize.z, bodySize.y, bodySize.z);

    float halfX = bodySize.x * 0.5f;
    float capOffset = halfX - capSize.x * 0.5f;

    glm::vec3 bodyMidSize = bodySize;
    bodyMidSize.x = bodySize.x - capSize.x;

    m_renderer.drawCube({}, m_state.paddlePos, bodyMidSize, paddleColor);
    m_renderer.drawCube({}, m_state.paddlePos + glm::vec3(-capOffset, 0, 0), capSize, paddleColor);
    m_renderer.drawCube({}, m_state.paddlePos + glm::vec3( capOffset, 0, 0), capSize, paddleColor);

    // Ball
    glm::vec3 ballColor(0.95f, 0.35f, 0.35f);
    float d = m_cfg.ballRadius * 2.0f;
    m_renderer.drawCube({}, m_state.ballPos, glm::vec3(d, d, d), ballColor);

    // -------- 2D PASS (HUD + OVERLAY) --------
    glDisable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    glm::mat4 Vui = glm::mat4(1.0f);
    glm::mat4 Pui = glm::ortho(0.0f, (float)fbW, (float)fbH, 0.0f, -10.0f, 10.0f);
    m_renderer.setCamera(Vui, Pui, glm::vec3(0,0,1));

    glm::vec3 heartOn(0.95f, 0.20f, 0.25f);
    glm::vec3 heartOff(0.22f, 0.22f, 0.26f);

    float hx = 18.0f, hy = 18.0f;
    float hs = 32.0f;
    float gap = 10.0f;

    for (int i = 0; i < 3; ++i) {
        glm::vec3 col = (i < m_state.lives) ? heartOn : heartOff;
        drawHeart2D(m_renderer, hx + i*(hs + gap), hy, hs, col);
    }

    if (m_state.mode != GameMode::PLAYING) {
        // ✅ Menos escuro (para não parecer ecrã preto)
        drawRect2D(m_renderer, 0.0f, 0.0f, (float)fbW, (float)fbH, glm::vec3(0.10f,0.10f,0.12f));

        const char* title = (m_state.mode == GameMode::WIN) ? "YOU WIN" : "GAME OVER";
        float titleScale = 7.0f;
        float titleX = (fbW * 0.5f) - (textWidthPx(title, titleScale) * 0.5f);
        float titleY = fbH * 0.30f;
        drawText2D(m_renderer, titleX, titleY, titleScale, glm::vec3(0.96f,0.96f,0.96f), title);

        float bw = 220.0f, bh = 60.0f;
        float g = 30.0f;
        float cx = fbW * 0.5f;
        float y  = fbH * 0.55f;

        float rx = cx - bw - g*0.5f;
        float fx = cx + g*0.5f;

        GLFWwindow* w = m_window.getHandle(); // ✅ FIX
        int ww = 1, wh = 1;
        glfwGetWindowSize(w, &ww, &wh);
        ww = std::max(1, ww);
        wh = std::max(1, wh);

        double mx, my;
        glfwGetCursorPos(w, &mx, &my);

        float sx = (float)fbW / (float)ww;
        float sy = (float)fbH / (float)wh;
        float px = (float)mx * sx;
        float py = (float)my * sy;

        bool hoverR = pointInRectPx(px, py, rx, y, bw, bh);
        bool hoverF = pointInRectPx(px, py, fx, y, bw, bh);

        glm::vec3 rCol = hoverR ? glm::vec3(0.30f,0.90f,0.35f) : glm::vec3(0.22f,0.75f,0.28f);
        glm::vec3 fCol = hoverF ? glm::vec3(0.95f,0.30f,0.30f) : glm::vec3(0.78f,0.22f,0.22f);

        drawRect2D(m_renderer, rx, y, bw, bh, rCol);
        drawRect2D(m_renderer, fx, y, bw, bh, fCol);

        drawText2D(m_renderer, rx + 28.0f, y + 18.0f, 4.0f, glm::vec3(0.05f,0.05f,0.05f), "REINICIAR");
        drawText2D(m_renderer, fx + 58.0f, y + 18.0f, 4.0f, glm::vec3(0.05f,0.05f,0.05f), "FECHAR");
    }

    glEnable(GL_DEPTH_TEST);

    m_window.swapBuffers();
}

} // namespace game
