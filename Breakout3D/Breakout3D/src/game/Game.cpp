#include "game/Game.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

static float clampf(float v, float a, float b) { return std::max(a, std::min(v, b)); }

// sphere vs AABB no plano XZ (Y ignorado)
static bool sphereAabbXZ(const glm::vec3& c, float r, const glm::vec3& bpos, const glm::vec3& bsize) {
    float hx = bsize.x * 0.5f;
    float hz = bsize.z * 0.5f;

    float minX = bpos.x - hx, maxX = bpos.x + hx;
    float minZ = bpos.z - hz, maxZ = bpos.z + hz;

    float cx = clampf(c.x, minX, maxX);
    float cz = clampf(c.z, minZ, maxZ);

    float dx = c.x - cx;
    float dz = c.z - cz;
    return (dx*dx + dz*dz) <= (r*r);
}

namespace game {

Game::Game(engine::Window& window, engine::Time& time, engine::Renderer& renderer)
: m_window(window), m_time(time), m_renderer(renderer) {}

void Game::init() {
    // paddle ao fundo (ajusta a margem como quiseres)
    m_state.paddlePos = glm::vec3(
        0.0f, 0.0f,
        m_cfg.arenaMaxZ - (m_cfg.paddleSize.z * 0.5f) - 0.25f
    );

    // bola presa
    m_state.ballAttached = true;
    m_state.ballVel = glm::vec3(0.0f);
    m_state.ballPos = m_state.paddlePos + glm::vec3(
        0.0f, 0.0f,
        -(m_cfg.paddleSize.z * 0.5f + m_cfg.ballRadius + 0.15f)
    );

    // ----- BRICKS GRID -----
    m_state.bricks.clear();

    int cols = 10;
    int rows = 5;

    glm::vec3 brickSize(2.2f, 0.6f, 1.0f);
    float gapX = 0.35f;
    float gapZ = 0.35f;

    // área de bricks (perto do topo)
    float startZ = m_cfg.arenaMinZ + 2.5f;

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
            m_state.bricks.push_back(b);
        }
    }
}

void Game::update() {
    GLFWwindow* w = m_window.handle();
    float dt = m_time.delta();

    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(w, 1);

    int nowF11 = glfwGetKey(w, GLFW_KEY_F11);
    if (nowF11 == GLFW_PRESS && m_lastF11 == GLFW_RELEASE)
        m_window.toggleFullscreen();
    m_lastF11 = nowF11;

    // Paddle input
    float dir = 0.0f;
    if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_LEFT) == GLFW_PRESS) dir -= 1.0f;
    if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_RIGHT) == GLFW_PRESS) dir += 1.0f;

    m_state.paddlePos.x += dir * m_cfg.paddleSpeed * dt;

    float paddleHalfX = m_cfg.paddleSize.x * 0.5f;
    m_state.paddlePos.x = std::clamp(m_state.paddlePos.x, m_cfg.arenaMinX + paddleHalfX, m_cfg.arenaMaxX - paddleHalfX);

    // ball attached
    if (m_state.ballAttached) {
        m_state.ballPos = m_state.paddlePos + glm::vec3(
            0.0f, 0.0f,
            -(m_cfg.paddleSize.z * 0.5f + m_cfg.ballRadius + 0.15f)
        );

        if (glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS) {
            m_state.ballAttached = false;
            glm::vec3 d = glm::normalize(glm::vec3(0.30f, 0.0f, -1.0f));
            m_state.ballVel = d * m_cfg.ballSpeed;
        }
        return;
    }

    // move ball
    m_state.ballPos += m_state.ballVel * dt;

    // walls X
    if (m_state.ballPos.x - m_cfg.ballRadius < m_cfg.arenaMinX) {
        m_state.ballPos.x = m_cfg.arenaMinX + m_cfg.ballRadius;
        m_state.ballVel.x = std::abs(m_state.ballVel.x);
    }
    if (m_state.ballPos.x + m_cfg.ballRadius > m_cfg.arenaMaxX) {
        m_state.ballPos.x = m_cfg.arenaMaxX - m_cfg.ballRadius;
        m_state.ballVel.x = -std::abs(m_state.ballVel.x);
    }

    // top Z
    if (m_state.ballPos.z - m_cfg.ballRadius < m_cfg.arenaMinZ) {
        m_state.ballPos.z = m_cfg.arenaMinZ + m_cfg.ballRadius;
        m_state.ballVel.z = std::abs(m_state.ballVel.z);
    }

    // paddle collision
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
            m_state.ballPos.z = minZ - m_cfg.ballRadius - 0.001f;

            float t = (m_state.ballPos.x - m_state.paddlePos.x) / paddleHalfX;
            t = clampf(t, -1.0f, 1.0f);

            float ang = t * glm::radians(60.0f);
            m_state.ballVel.x = std::sin(ang) * m_cfg.ballSpeed;
            m_state.ballVel.z = -std::cos(ang) * m_cfg.ballSpeed;
        }
    }

    // ---- BRICK COLLISIONS ----
    // 1 hit por frame para não destruir vários bricks de uma vez
    for (auto& b : m_state.bricks) {
        if (!b.alive) continue;

        if (sphereAabbXZ(m_state.ballPos, m_cfg.ballRadius, b.pos, b.size)) {
            b.alive = false;

            // Decide se reflete em X ou Z baseado em "lado" do impacto (aproximação rápida)
            glm::vec3 diff = m_state.ballPos - b.pos;
            float ax = std::abs(diff.x) / (b.size.x * 0.5f);
            float az = std::abs(diff.z) / (b.size.z * 0.5f);

            if (ax > az) {
                m_state.ballVel.x = -m_state.ballVel.x;
            } else {
                m_state.ballVel.z = -m_state.ballVel.z;
            }
            break;
        }
    }

    // lost ball
    if (m_state.ballPos.z - m_cfg.ballRadius > m_cfg.arenaMaxZ) {
        m_state.ballAttached = true;
        m_state.ballVel = glm::vec3(0.0f);
    }
}

void Game::render() {
    int fbW, fbH;
    m_window.getFramebufferSize(fbW, fbH);
    m_renderer.beginFrame(fbW, fbH);

    float arenaW = (m_cfg.arenaMaxX - m_cfg.arenaMinX);
    float arenaD = (m_cfg.arenaMaxZ - m_cfg.arenaMinZ);
    float base = std::max(arenaW, arenaD);

    glm::vec3 camPos(0.0f, base * 0.55f, base * 0.85f);
    glm::vec3 camTarget(0.0f, 0.0f, 0.0f);
    glm::mat4 V = glm::lookAt(camPos, camTarget, glm::vec3(0,1,0));
    glm::mat4 P = glm::perspective(glm::radians(58.0f), (float)fbW/(float)fbH, 0.1f, 300.0f);
    m_renderer.setCamera(V, P, camPos);

    // walls
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

    // bricks
    glm::vec3 brickColor(0.90f, 0.60f, 0.15f);
    for (const auto& b : m_state.bricks) {
        if (!b.alive) continue;
        m_renderer.drawCube({}, b.pos, b.size, brickColor);
    }

    // paddle
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

    // ball
    glm::vec3 ballColor(0.95f, 0.35f, 0.35f);
    float d = m_cfg.ballRadius * 2.0f;
    m_renderer.drawCube({}, m_state.ballPos, glm::vec3(d, d, d), ballColor);

    m_window.swapBuffers();
}

} // namespace game
