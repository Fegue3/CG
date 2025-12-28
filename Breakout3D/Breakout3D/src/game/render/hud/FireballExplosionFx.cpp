#include "game/render/ui/HudParts.hpp"

#include "game/GameConfig.hpp"
#include "game/GameState.hpp"

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace game::render::hud {

void drawFireballExplosionFx(const RenderContext& ctx, const GameState& state, const GameConfig& cfg) {
    // --- FIREBALL EXPLOSION FX (orange expanding ring at impact point) ---
    if (state.fireballExplosions.empty()) return;

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

            ctx.renderer.drawUITriangle(p0o, p0i, p1o, col);
            ctx.renderer.drawUITriangle(p0i, p1o, p1i, col);
        }
    };

    float dur = std::max(0.001f, cfg.fireballExplosionFxDuration);
    float flashA = 0.0f;
    for (const auto& fx : state.fireballExplosions) {
        float u = fx.t / dur;
        if (u < 0.0f) u = 0.0f;
        if (u > 1.0f) u = 1.0f;

        flashA = std::max(flashA, (1.0f - u) * cfg.fireballFlashMaxAlpha);

        // Project explosion center to UI pixels
        glm::vec4 clip = ctx.P * ctx.V * glm::vec4(fx.pos, 1.0f);
        if (std::abs(clip.w) < 1e-6f) continue;
        float ndcX = clip.x / clip.w;
        float ndcY = clip.y / clip.w;
        if (clip.w <= 0.0f) continue;

        glm::vec2 cPx(
            (ndcX * 0.5f + 0.5f) * (float)ctx.fbW,
            (ndcY * 0.5f + 0.5f) * (float)ctx.fbH
        );

        float radiusPx = 26.0f + 170.0f * u;
        float thickPx = 18.0f - 10.0f * u;
        float a = (1.0f - u) * 0.85f;
        glm::vec4 col(1.0f, 0.55f, 0.10f, a);
        if (a > 0.01f) drawRing(cPx, radiusPx, thickPx, col);
    }

    // Flash overlay (break feel)
    if (flashA > 0.01f) {
        ctx.renderer.drawUIQuad(0.0f, 0.0f, (float)ctx.fbW, (float)ctx.fbH, glm::vec4(1.0f, 0.55f, 0.10f, flashA));
    }
}

} // namespace game::render::hud


