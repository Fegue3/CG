#include "game/render/ui/HudParts.hpp"

#include "game/GameState.hpp"
#include "game/render/ui/UIHelpers.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include <glm/glm.hpp>

namespace game::render::hud {

void drawScoreAndWaveHud(const RenderContext& ctx, const GameState& state) {
    // --- SCORE AND WAVE HUD ---
    if (state.gameType == GameType::ROGUE) {
        // During the card-pick overlay, the top-center slot is reserved for the overlay title.
        if (state.mode == GameMode::ROGUE_CARDS) return;

        // Rogue: wave + score with the same streak/bank UI as Endless, but NO best-score display.
        std::string waveStr = "WAVE " + std::to_string(std::max(1, state.wave));
        std::string scoreStr = std::to_string(state.score);

        float waveScale = 1.30f;
        float scoreScale = 2.15f;

        float waveH = ctx.renderer.getUIFontLineHeight(waveScale);
        float scoreH = ctx.renderer.getUIFontLineHeight(scoreScale);

        float waveW = ctx.renderer.measureUITextWidth(waveStr, waveScale);
        float scoreW = ctx.renderer.measureUITextWidth(scoreStr, scoreScale);

        float cx = (float)ctx.fbW * 0.5f;
        float waveX = cx - waveW * 0.5f;
        float scoreX = cx - scoreW * 0.5f;

        const float topMargin = 18.0f;
        const float lineGap = 8.0f;
        // Keep wave/score in their NORMAL positions always (do not shift when picking).
        float yTop = (float)ctx.fbH - topMargin;
        float waveY = yTop - waveH;
        float scoreY = waveY - lineGap - scoreH;

        // Wave should be plain white (no RGB).
        {
            glm::vec3 outline(0.02f, 0.02f, 0.06f);
            ctx.renderer.drawUIText(waveX - 2.0f, waveY, waveStr, waveScale, outline);
            ctx.renderer.drawUIText(waveX + 2.0f, waveY, waveStr, waveScale, outline);
            ctx.renderer.drawUIText(waveX, waveY - 2.0f, waveStr, waveScale, outline);
            ctx.renderer.drawUIText(waveX, waveY + 2.0f, waveStr, waveScale, outline);
            ctx.renderer.drawUIText(waveX, waveY, waveStr, waveScale, glm::vec4(1, 1, 1, 0.90f));
        }
        ctx.renderer.drawUIText(scoreX, scoreY, scoreStr, scoreScale, glm::vec4(1, 1, 1, 1.0f));

        // Split bank display (same as Endless)
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
                float x = scoreX + scoreW - w * 0.25f;
                ctx.renderer.drawUIText(x, baseY, s, addScale, glm::vec4(1, 1, 1, alpha));
            }
            if (state.endlessStreakNegPoints > 0) {
                std::string s = "-" + std::to_string(state.endlessStreakNegPoints);
                float w = ctx.renderer.measureUITextWidth(s, addScale);
                float x = scoreX - w * 0.75f;
                ctx.renderer.drawUIText(x, baseY, s, addScale, glm::vec4(1.0f, 0.15f, 0.15f, alpha));
            }
        }

        // Score popups (same as Endless)
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
            float posX = scoreX + (scoreW - posMaxW) * 0.5f;

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
                    float a = 1.0f - 0.75f * ease;
                    float y = (scoreY - scoreH * 1.25f) + ease * 26.0f + (float)posIdx * 20.0f;
                    ctx.renderer.drawUIText(posX, y, s, scale, glm::vec4(0.35f, 1.0f, 0.35f, a));
                    posIdx++;
                }
            }
        }
        return;
    }
    if (state.gameType == GameType::LEVELS) {
        // LEVELS mode: show current level number
        std::string levelStr = "LEVEL " + std::to_string(state.currentLevel) + " / 20";
        float levelScale = 1.60f;
        float levelH = ctx.renderer.getUIFontLineHeight(levelScale);
        float levelW = ctx.renderer.measureUITextWidth(levelStr, levelScale);
        float cx = (float)ctx.fbW * 0.5f;
        float levelX = cx - levelW * 0.5f;
        const float topMargin = 18.0f;
        float levelY = (float)ctx.fbH - topMargin - levelH;

        // Outline for readability
        glm::vec3 outline(0.02f, 0.02f, 0.06f);
        ctx.renderer.drawUIText(levelX - 2.0f, levelY, levelStr, levelScale, outline);
        ctx.renderer.drawUIText(levelX + 2.0f, levelY, levelStr, levelScale, outline);
        ctx.renderer.drawUIText(levelX, levelY - 2.0f, levelStr, levelScale, outline);
        ctx.renderer.drawUIText(levelX, levelY + 2.0f, levelStr, levelScale, outline);
        ctx.renderer.drawUIText(levelX, levelY, levelStr, levelScale, glm::vec4(0.20f, 0.75f, 0.85f, 1.0f));
        return;
    }
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


