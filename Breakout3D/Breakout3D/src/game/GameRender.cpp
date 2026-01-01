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

/*
    Render pipeline:
    - Se estiver em MENU: desenha só UI do menu e sai.
    - Caso contrário:
      1) Background (se activo)
      2) Pass 3D (mundo) com câmara configurada
      3) Pass UI (HUD, overlays, etc.)
*/
void Game::render() {
    auto [fbW, fbH] = m_window.getFramebufferSize();
    m_renderer.beginFrame(fbW, fbH);

    // MENU: render dedicado
    if (m_state.mode == GameMode::MENU) {
        // Defesa: se por alguma razão render vier antes de update, layout existe na mesma.
        m_state.menuLayout = ui::calculateMenuLayout(m_renderer, fbW, fbH);
        game::render::RenderContext ctx{fbW, fbH, m_time, m_renderer};
        game::render::renderMenu(ctx, m_state, m_assets);
        m_window.swapBuffers();
        return;
    }

    // Background (opcional)
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
        // Mode 1: Top-down (um pouco mais afastado)
        camPos    = glm::vec3(0.0f, base * 1.03f, zMid + 0.5f);
        camTarget = glm::vec3(0.0f, 0.0f, zMid);
        fov       = 45.0f;
    } else {
        // Mode 2: Angled (mais "apertado")
        camPos    = glm::vec3(0.0f, base * 0.62f, base * 0.82f);
        camTarget = glm::vec3(0.0f, 0.0f, 0.8f);
        fov       = 45.0f;
    }

    // WIN FINISHER: câmara cinematográfica (matemática em game/effects)
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

    // Fireball shake: jitter subtil na câmara ao impacto
    if (m_state.fireballShakeTimer > 0.0f && m_cfg.fireballShakeDuration > 1e-4f) {
        float u = m_state.fireballShakeTimer / m_cfg.fireballShakeDuration; // 1..0
        u = std::max(0.0f, std::min(1.0f, u));

        float e = u * u; // ease-out
        float amp = m_cfg.fireballShakeStrength * e;
        float t = m_time.now();

        float ox = std::sin(t * 44.0f) * amp;
        float oy = std::sin(t * 57.0f + 1.7f) * amp * 0.65f;

        camPos += glm::vec3(ox, oy, 0.0f);
        camTarget += glm::vec3(ox * 0.55f, oy * 0.55f, 0.0f);
    }

    glm::mat4 V = glm::lookAt(camPos, camTarget, glm::vec3(0, 1, 0));
    if (rollRad != 0.0f) {
        V = glm::rotate(glm::mat4(1.0f), rollRad, glm::vec3(0, 0, 1)) * V;
    }

    glm::mat4 P = glm::perspective(glm::radians(fov), (float)fbW / (float)fbH, 0.1f, 300.0f);
    m_renderer.setCamera(V, P, camPos);

    // Danger line (ENDLESS/ROGUE): projecta a linha para Y de ecrã
    // para a UI poder desenhar a banda/efeito apenas abaixo desse Y.
    float dangerLineScreenY = -1.0f; // UI pixels, origem em baixo
    if ((m_state.gameType == GameType::ENDLESS || m_state.gameType == GameType::ROGUE) && m_state.endlessDangerActive) {
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
