#include "game/render/ui/HudParts.hpp"

#include "game/GameState.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include <glm/glm.hpp>

namespace game::render::hud {

void drawScoreAndWaveHud(const RenderContext& ctx, const GameState& state) {
    // --- SCORE AND WAVE HUD ---
    if (state.gameType != GameType::ENDLESS) {
        // Normal mode: score HUD removed (per request).
        return;
    }

    // Endless: HUD at the top-center (white).
    std::string bestStr = std::to_string(state.endlessBestScore);
    std::string scoreStr = std::to_string(state.score);

    // Bigger, more readable score HUD
    float bestScale = 1.30f;
    float scoreScale = 2.35f;

    float bestH = ctx.renderer.getUIFontLineHeight(bestScale);
    float scoreH = ctx.renderer.getUIFontLineHeight(scoreScale);

    float bestTotalW = ctx.renderer.measureUITextWidth(bestStr, bestScale);
    float scoreTotalW = ctx.renderer.measureUITextWidth(scoreStr, scoreScale);

    float bestX = (float)ctx.fbW * 0.5f - bestTotalW * 0.5f;
    float scoreX = (float)ctx.fbW * 0.5f - scoreTotalW * 0.5f;

    const float topMargin = 18.0f;
    const float lineGap = 8.0f;
    float bestY = (float)ctx.fbH - topMargin - bestH;
    float scoreY = bestY - lineGap - scoreH;

    ctx.renderer.drawUIText(bestX, bestY, bestStr, bestScale, glm::vec4(1.0f, 1.0f, 1.0f, 0.75f));
    ctx.renderer.drawUIText(scoreX, scoreY, scoreStr, scoreScale, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // Split bank display:
    // - gains: white "+XXX" on the RIGHT
    // - penalties: red "-XXX" on the LEFT
    {
        const float addScale = 1.45f;
        const float bankAnim = 0.55f;
        float t = state.endlessStreakBanking
            ? std::min(1.0f, std::max(0.0f, state.endlessStreakBankTimer / bankAnim))
            : 0.0f;
        float ease = t * t * (3.0f - 2.0f * t);
        float yOffset = ease * 22.0f;
        float alpha = 1.0f - 0.65f * ease;
        float baseY = scoreY - scoreH * 0.65f + yOffset;

        if (state.endlessStreakPosPoints > 0) {
            std::string s = "+" + std::to_string(state.endlessStreakPosPoints);
            float w = ctx.renderer.measureUITextWidth(s, addScale);
            float x = scoreX + scoreTotalW - w * 0.25f; // close-right
            ctx.renderer.drawUIText(x, baseY, s, addScale, glm::vec4(1.0f, 1.0f, 1.0f, alpha));
        }
        if (state.endlessStreakNegPoints > 0) {
            std::string s = "-" + std::to_string(state.endlessStreakNegPoints);
            float w = ctx.renderer.measureUITextWidth(s, addScale);
            float x = scoreX - w * 0.75f; // mirrored to left
            ctx.renderer.drawUIText(x, baseY, s, addScale, glm::vec4(1.0f, 0.15f, 0.15f, alpha));
        }
    }

    // Score popups:
    // - negatives: red, to the LEFT of the score
    // - positives: green, under/near the score (used e.g. by Fireball explosion)
    if (!state.scorePopups.empty()) {
        const float popupDur = 2.10f;
        float scale = 1.35f;
        float negMaxW = 0.0f;
        float posMaxW = 0.0f;
        for (const auto& sp : state.scorePopups) {
            std::string s = (sp.pts >= 0) ? ("+" + std::to_string(sp.pts)) : std::to_string(sp.pts);
            float w = ctx.renderer.measureUITextWidth(s, scale);
            if (sp.pts < 0) negMaxW = std::max(negMaxW, w);
            else            posMaxW = std::max(posMaxW, w);
        }

        float negX = scoreX - negMaxW * 0.75f;
        float posX = scoreX + (scoreTotalW - posMaxW) * 0.5f;

        float yBase = scoreY - scoreH * 0.65f;
        int negIdx = 0;
        int posIdx = 0;

        for (const auto& sp : state.scorePopups) {
            std::string s = (sp.pts >= 0) ? ("+" + std::to_string(sp.pts)) : std::to_string(sp.pts);
            if (sp.pts < 0) {
                const float bankAnim = 0.55f;
                float t = std::min(1.0f, std::max(0.0f, sp.t / bankAnim));
                float ease = t * t * (3.0f - 2.0f * t);
                float yOffset = ease * 22.0f;
                float a = 1.0f - 0.65f * ease;
                float y = yBase + yOffset + (float)negIdx * 20.0f;
                ctx.renderer.drawUIText(negX, y, s, scale, glm::vec4(1.0f, 0.15f, 0.15f, a));
                negIdx++;
            } else if (sp.pts > 0) {
                float u = std::min(1.0f, std::max(0.0f, sp.t / popupDur));
                float ease = u * u * (3.0f - 2.0f * u);
                float a = 1.0f - 0.75f * ease; // green can stay "snappier"
                float y = (scoreY - scoreH * 1.25f) + ease * 26.0f + (float)posIdx * 20.0f;
                ctx.renderer.drawUIText(posX, y, s, scale, glm::vec4(0.35f, 1.0f, 0.35f, a));
                posIdx++;
            }
        }
    }
}

} // namespace game::render::hud


