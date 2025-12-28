#include "game/render/ui/HudParts.hpp"

#include "game/GameState.hpp"
#include "game/effects/WinFinisher.hpp"

#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace game::render::hud {

void drawWinFinisherOverlay(const RenderContext& ctx, const GameState& state) {
    // --- WIN FINISHER OVERLAY: flash + expanding shockwave ("blown air") ---
    if (!state.winFinisherActive) return;

    auto o = game::effects::computeWinFinisherOverlay(
        ctx.fbW, ctx.fbH, ctx.V, ctx.P,
        state.winFinisherAnchorPos,
        state.winFinisherTimer,
        state.winFinisherAnchorValid
    );

    // White flash
    float flashA = o.flashAlpha;
    if (flashA > 0.01f) {
        ctx.renderer.drawUIQuad(0.0f, 0.0f, (float)ctx.fbW, (float)ctx.fbH, glm::vec4(1.0f, 1.0f, 1.0f, flashA));
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

            ctx.renderer.drawUITriangle(p0o, p0i, p1o, col);
            ctx.renderer.drawUITriangle(p0i, p1o, p1i, col);
        }
    };

    if (ringA > 0.01f) {
        drawRing(c, r, thick, ringCol);
    }
}

} // namespace game::render::hud


