#include "game/Game.hpp"

#include "game/GameAssets.hpp"
#include "game/effects/WinFinisher.hpp"
#include "game/render/RenderContext.hpp"
#include "game/render/UIRender.hpp"
#include "game/render/WorldRender.hpp"
#include "game/ui/OverlayLayout.hpp"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

namespace game {

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
    if ((m_state.gameType == GameType::ENDLESS || m_state.gameType == GameType::ROGUE) && m_state.endlessDangerActive) {
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


