#include "game/render/ui/HudParts.hpp"

#include "game/GameState.hpp"
#include "game/render/ui/UIHelpers.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include <glm/glm.hpp>

namespace game::render::hud {

void drawDangerZoneHud(const RenderContext& ctx, const GameState& state) {
    // --- DANGER ZONE FILTER (draw BEFORE pause/gameover overlay so overlays stay on top) ---
    if (!((state.gameType == GameType::ENDLESS || state.gameType == GameType::ROGUE) &&
          state.endlessDangerActive &&
          (state.mode == GameMode::PLAYING || state.mode == GameMode::PAUSED) &&
          (
              state.endlessDangerTimer < 10.0f ||
              ((state.paddlePos.z - 0.5f) - state.endlessDangerMaxZ) <= (1.33f * 2.0f)
          ))) {
        return;
    }

    float screenY = (ctx.dangerLineScreenY >= 0.0f) ? ctx.dangerLineScreenY : (float)ctx.fbH * 0.30f;
    screenY = std::max(0.0f, std::min((float)ctx.fbH, screenY));

    // Clean "real warning" flash: smooth fade in/out (no harsh on/off, no double-strobe).
    const float period = 0.95f; // seconds
    float ph = std::fmod(state.endlessDangerTimer, period);
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
        ctx.renderer.drawUIQuad(0.0f, 0.0f, (float)ctx.fbW, screenY, glm::vec4(1.0f, 0.0f, 0.0f, baseA));
    }

    // Danger signs + text (Endless/Rogue, show while warning is active)
    bool veryCloseToDeath = (((state.paddlePos.z - 0.5f) - state.endlessDangerMaxZ) <= (1.33f * 2.0f));
    if (!(state.endlessDangerTimer < 10.0f || veryCloseToDeath)) return;

    float triAlpha = 0.10f + 0.90f * pulse;
    if (triAlpha < 0.02f) return;

    std::string dMsg = "DANGER!";
    float dScale = ui::fitScaleToWidth(ctx.renderer, dMsg, 4.0f, (float)ctx.fbW * 0.92f);
    float dW = ctx.renderer.measureUITextWidth(dMsg, dScale);
    float xPos = (float)ctx.fbW * 0.5f - dW * 0.5f;

    float th = ctx.renderer.getUIFontLineHeight(dScale);
    float yPos = (screenY - th) * 0.5f;
    yPos = std::max(12.0f, yPos);
    if (yPos + th > screenY - 8.0f) yPos = std::max(12.0f, screenY - th - 8.0f);
    float yMid = yPos + th * 0.5f;

    // Slight thickness pass
    {
        const float o = 1.25f;
        const float a = triAlpha * 0.16f;
        glm::vec4 thick(1.0f, 0.0f, 0.0f, a);
        ctx.renderer.drawUIText(xPos - o, yPos, dMsg, dScale, thick);
        ctx.renderer.drawUIText(xPos + o, yPos, dMsg, dScale, thick);
        ctx.renderer.drawUIText(xPos, yPos - o, dMsg, dScale, thick);
        ctx.renderer.drawUIText(xPos, yPos + o, dMsg, dScale, thick);
    }
    ctx.renderer.drawUIText(xPos, yPos, dMsg, dScale, glm::vec4(1.0f, 0.0f, 0.0f, triAlpha * 0.30f));

    auto drawTriangleSign = [&](float xCenter) {
        float triW = 100.0f;
        float triH = 85.0f;
        float yTri = yMid - triH * 0.5f;

        glm::vec2 pTop(xCenter, yTri + triH);
        glm::vec2 pLeft(xCenter - triW*0.5f, yTri);
        glm::vec2 pRight(xCenter + triW*0.5f, yTri);

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
            ctx.renderer.drawUITriangle(c1, c2, c3, glm::vec4(1.0f, 0.0f, 0.0f, triAlpha));
            ctx.renderer.drawUITriangle(c2, c3, c4, glm::vec4(1.0f, 0.0f, 0.0f, triAlpha));
        };

        float edgeT = 6.0f;
        drawEdge(pLeft, pRight, edgeT);
        drawEdge(pLeft, pTop, edgeT);
        drawEdge(pRight, pTop, edgeT);

        ctx.renderer.drawUIQuad(xCenter - 3, yTri + 30, 6, 40, glm::vec4(1.0f, 0.0f, 0.0f, triAlpha));
        ctx.renderer.drawUIQuad(xCenter - 3, yTri + 16, 6, 6, glm::vec4(1.0f, 0.0f, 0.0f, triAlpha));
    };

    // Symmetric composition around center
    drawTriangleSign((float)ctx.fbW * 0.22f);
    drawTriangleSign((float)ctx.fbW * 0.78f);
}

} // namespace game::render::hud


