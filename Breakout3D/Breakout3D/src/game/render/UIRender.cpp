#include "game/render/UIRender.hpp"
#include "game/GameAssets.hpp"
#include "game/GameConfig.hpp"
#include "game/GameState.hpp"
#include "game/effects/WinFinisher.hpp"
#include "game/ui/OverlayLayout.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace game::render {

static float fitScaleToWidth(engine::Renderer& renderer, const std::string& text, float desiredScale, float maxWidthPx) {
    float s = desiredScale;
    if (maxWidthPx <= 1.0f) return s;
    float w = renderer.measureUITextWidth(text, s);
    if (w > maxWidthPx && w > 1.0f) {
        s *= (maxWidthPx / w);
    }
    return s;
}

void renderMenu(const RenderContext& ctx, const GameState& state) {
    // Dark background
    ctx.renderer.beginUI(ctx.fbW, ctx.fbH);
    ctx.renderer.drawUIQuad(0, 0, (float)ctx.fbW, (float)ctx.fbH, glm::vec4(0.05f, 0.05f, 0.08f, 1.0f));

    // Menu layout: title on top, menu panel BELOW it (no overlap).
    float panelW = 500.0f;
    float panelH = 480.0f;
    float panelX = (ctx.fbW - panelW) * 0.5f;

    const float topMargin = 28.0f;
    const float bottomMargin = 24.0f;
    const float titlePanelGap = 26.0f;

    // Menu title (BIG, centered on screen)
    std::string title = "BREAKOUT 3D";
    float titleScale = fitScaleToWidth(ctx.renderer, title, 12.0f, (float)ctx.fbW * 0.92f);
    {
        // Ensure there's enough vertical room for: [title][gap][panel]
        float maxTitleH = (float)ctx.fbH - topMargin - bottomMargin - titlePanelGap - panelH;
        if (maxTitleH > 8.0f) {
            float titleH = ctx.renderer.getUIFontLineHeight(titleScale);
            if (titleH > maxTitleH && titleH > 1.0f) {
                titleScale *= (maxTitleH / titleH);
            }
        }
    }

    float titleW = ctx.renderer.measureUITextWidth(title, titleScale);
    float titleH = ctx.renderer.getUIFontLineHeight(titleScale);
    float titleX = ((float)ctx.fbW - titleW) * 0.5f;
    float titleY = (float)ctx.fbH - topMargin - titleH;

    // Panel sits below the title
    float panelY = titleY - titlePanelGap - panelH;
    if (panelY < bottomMargin) {
        panelY = bottomMargin;
        // If we had to clamp, keep title just above the panel.
        titleY = panelY + panelH + titlePanelGap;
        if (titleY + titleH > (float)ctx.fbH - topMargin) {
            titleY = (float)ctx.fbH - topMargin - titleH;
        }
    }

    ctx.renderer.drawUIText(titleX, titleY, title, titleScale, glm::vec3(0.2f, 0.8f, 1.0f));

    ctx.renderer.drawUIQuad(panelX, panelY, panelW, panelH, glm::vec4(0.08f, 0.08f, 0.12f, 0.95f));

    // Border
    float borderThickness = 3.0f;
    glm::vec4 borderColor(0.2f, 0.8f, 1.0f, 1.0f);
    ctx.renderer.drawUIQuad(panelX - borderThickness, panelY - borderThickness, panelW + 2*borderThickness, borderThickness, borderColor);
    ctx.renderer.drawUIQuad(panelX - borderThickness, panelY + panelH, panelW + 2*borderThickness, borderThickness, borderColor);
    ctx.renderer.drawUIQuad(panelX - borderThickness, panelY, borderThickness, panelH, borderColor);
    ctx.renderer.drawUIQuad(panelX + panelW, panelY, borderThickness, panelH, borderColor);

    // Buttons
    float btnW = 200.0f;
    float btnH = 70.0f;
    float btnX = panelX + (panelW - btnW) * 0.5f;

    // Button positions (from top to bottom)
    float btn1Y = panelY + 360.0f; // Normal Mode (top)
    float btn2Y = panelY + 250.0f; // Endless Mode
    float btn3Y = panelY + 140.0f; // Instructions
    float btn4Y = panelY + 30.0f;  // Exit (bottom)

    // Normal Mode button
    ctx.renderer.drawUIQuad(btnX, btn1Y, btnW, btnH, glm::vec4(0.2f, 0.8f, 0.2f, 1.0f));
    float normalScale = 1.0f;
    float normalLabelW = ctx.renderer.measureUITextWidth("NORMAL", normalScale);
    float normalH = ctx.renderer.getUIFontLineHeight(normalScale);
    ctx.renderer.drawUIText(btnX + (btnW - normalLabelW) * 0.5f, btn1Y + (btnH - normalH) * 0.5f, "NORMAL", normalScale, glm::vec3(1, 1, 1));

    // Endless Mode button
    ctx.renderer.drawUIQuad(btnX, btn2Y, btnW, btnH, glm::vec4(0.8f, 0.5f, 0.2f, 1.0f));
    float endlessScale = 1.0f;
    float endlessLabelW = ctx.renderer.measureUITextWidth("ENDLESS", endlessScale);
    float endlessH = ctx.renderer.getUIFontLineHeight(endlessScale);
    ctx.renderer.drawUIText(btnX + (btnW - endlessLabelW) * 0.5f, btn2Y + (btnH - endlessH) * 0.5f, "ENDLESS", endlessScale, glm::vec3(1, 1, 1));

    // Instructions button
    ctx.renderer.drawUIQuad(btnX, btn3Y, btnW, btnH, glm::vec4(0.3f, 0.5f, 0.8f, 1.0f));
    float instrScale = 0.85f;
    float instrLabelW = ctx.renderer.measureUITextWidth("INSTRUCTIONS", instrScale);
    float instrH = ctx.renderer.getUIFontLineHeight(instrScale);
    ctx.renderer.drawUIText(btnX + (btnW - instrLabelW) * 0.5f, btn3Y + (btnH - instrH) * 0.5f, "INSTRUCTIONS", instrScale, glm::vec3(1, 1, 1));

    // Exit button
    ctx.renderer.drawUIQuad(btnX, btn4Y, btnW, btnH, glm::vec4(0.8f, 0.2f, 0.2f, 1.0f));
    float exitScale = 1.2f;
    float exitLabelW = ctx.renderer.measureUITextWidth("EXIT", exitScale);
    float exitH = ctx.renderer.getUIFontLineHeight(exitScale);
    ctx.renderer.drawUIText(btnX + (btnW - exitLabelW) * 0.5f, btn4Y + (btnH - exitH) * 0.5f, "EXIT", exitScale, glm::vec3(1, 1, 1));

    // Small clickable "4" badge for the one-brick test mode.
    {
        float badgeW = 48.0f;
        float badgeH = btnH - 16.0f;
        float badgeX = btnX + btnW - 8.0f - badgeW;
        float badgeY = btn4Y + 8.0f;
        ctx.renderer.drawUIQuad(badgeX, badgeY, badgeW, badgeH, glm::vec4(0.2f, 0.8f, 1.0f, 1.0f));
        ctx.renderer.drawUIQuad(badgeX + 2.0f, badgeY + 2.0f, badgeW - 4.0f, badgeH - 4.0f, glm::vec4(0.06f, 0.06f, 0.10f, 1.0f));

        std::string k = "4";
        float kScale = 1.4f;
        float kW = ctx.renderer.measureUITextWidth(k, kScale);
        float kH = ctx.renderer.getUIFontLineHeight(kScale);
        ctx.renderer.drawUIText(badgeX + (badgeW - kW) * 0.5f, badgeY + (badgeH - kH) * 0.5f, k, kScale, glm::vec3(0.2f, 0.8f, 1.0f));

        // Hint text under the button
        std::string hint = "ONE BRICK";
        float hScale = 0.58f;
        float hW = ctx.renderer.measureUITextWidth(hint, hScale);
        ctx.renderer.drawUIText(btnX + (btnW - hW) * 0.5f, btn4Y - 20.0f, hint, hScale, glm::vec3(0.6f, 0.85f, 1.0f));
    }

    // Show instructions if toggled
    if (state.showInstructions) {
        // Instructions panel
        float instrW = 600.0f;
        float instrH = 320.0f;
        float instrX = (ctx.fbW - instrW) * 0.5f;
        float instrY = (ctx.fbH - instrH) * 0.5f;

        ctx.renderer.drawUIQuad(instrX, instrY, instrW, instrH, glm::vec4(0.05f, 0.05f, 0.1f, 0.98f));

        // Instructions border
        float borderThickness = 3.0f;
        glm::vec4 instrBorder(0.4f, 0.6f, 0.9f, 1.0f);
        ctx.renderer.drawUIQuad(instrX - borderThickness, instrY - borderThickness, instrW + 2*borderThickness, borderThickness, instrBorder);
        ctx.renderer.drawUIQuad(instrX - borderThickness, instrY + instrH, instrW + 2*borderThickness, borderThickness, instrBorder);
        ctx.renderer.drawUIQuad(instrX - borderThickness, instrY, borderThickness, instrH, instrBorder);
        ctx.renderer.drawUIQuad(instrX + instrW, instrY, borderThickness, instrH, instrBorder);

        // Instructions title
        std::string instrTitle = "HOW TO PLAY";
        float instrTitleScale = 1.5f;
        float instrTitleW = ctx.renderer.measureUITextWidth(instrTitle, instrTitleScale);
        ctx.renderer.drawUIText(instrX + (instrW - instrTitleW) * 0.5f, instrY + instrH - 50.0f, instrTitle, instrTitleScale, glm::vec3(0.4f, 0.8f, 1.0f));

        // Instructions text
        float textY = instrY + instrH - 100.0f;
        float textX = instrX + 20.0f;
        float lineGap = 28.0f;

        std::vector<std::string> instructions = {
            "A/D or ARROW KEYS: Move paddle",
            "SPACE: Launch ball",
            "ESC: Pause/Resume game",
            "1/2: Change camera view",
            "Destroy all bricks to win!",
            "Click here to go back"
        };

        for (const auto& line : instructions) {
            ctx.renderer.drawUIText(textX, textY, line, 0.65f, glm::vec3(0.8f, 0.9f, 1.0f));
            textY -= lineGap;
        }
    }

    ctx.renderer.endUI();
}

void renderUI(const RenderContext& ctx, const GameState& state, const GameConfig& cfg, const GameAssets& assets) {
    ctx.renderer.beginUI(ctx.fbW, ctx.fbH);

    // --- WIN FINISHER OVERLAY: flash + expanding shockwave ("blown air") ---
    if (state.winFinisherActive) {
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

    // --- FIREBALL EXPLOSION FX (orange expanding ring at impact point) ---
    if (!state.fireballExplosions.empty()) {
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

    const float padX = 22.0f;
    const float padTop = 18.0f;

    const float hs = 56.0f;
    const float gap = 12.0f;

    const float rx = glm::radians(18.0f);
    const float ry = glm::radians(-12.0f);

    int heartCount = std::max(3, state.lives);
    for (int i = 0; i < heartCount; ++i) {
        float xCenter = padX + hs * 0.5f + i * (hs + gap);
        float yCenter = (float)ctx.fbH - (padTop + hs * 0.5f);

        glm::mat4 M(1.0f);
        M = glm::translate(M, glm::vec3(xCenter, yCenter, 0.0f));
        M = glm::rotate(M, ry, glm::vec3(0,1,0));
        M = glm::rotate(M, rx, glm::vec3(1,0,0));
        M = glm::scale(M, glm::vec3(hs, hs, hs * 0.55f));

        glm::vec3 col = (i < state.lives)
            ? glm::vec3(1.0f, 0.20f, 0.25f)
            : glm::vec3(0.20f, 0.20f, 0.22f);

        ctx.renderer.drawMesh(assets.heart, M, col);
    }

    // --- DANGER ZONE FILTER (draw BEFORE pause/gameover overlay so overlays stay on top) ---
    if (state.gameType == GameType::ENDLESS &&
        state.endlessDangerActive &&
        (state.mode == GameMode::PLAYING || state.mode == GameMode::PAUSED) &&
        (
            // Keep the "early warning" capped (so it doesn't flash forever),
            // but ALWAYS show again when bricks are very close to the death line (1-2 rows away).
            state.endlessDangerTimer < 10.0f ||
            ((state.paddlePos.z - 0.5f) - state.endlessDangerMaxZ) <= (1.33f * 2.0f)
        )) {

        float screenY = (ctx.dangerLineScreenY >= 0.0f) ? ctx.dangerLineScreenY : (float)ctx.fbH * 0.30f;
        screenY = std::max(0.0f, std::min((float)ctx.fbH, screenY));

        // Clean "real warning" flash: smooth fade in/out (no harsh on/off, no double-strobe).
        // Keep in sync with text/signs by using the same intensity curve.
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
    }

    // (Overlay is drawn at the end of UI so it truly sits on top of everything.)

    // --- SCORE AND WAVE HUD ---
    {
        if (state.gameType == GameType::ENDLESS) {
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
            // - gains: white "+XXX" on the RIGHT (same as original)
            // - penalties: red "-XXX" on the LEFT (same animation, mirrored)
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

                // Mirror the white popup's placement:
                // white: addX = scoreX + scoreTotalW - w*0.25
                // red:   negX = scoreX - w*0.75
                float negX = scoreX - negMaxW * 0.75f;
                float posX = scoreX + (scoreTotalW - posMaxW) * 0.5f;

                // Match the white streak popup height baseline exactly.
                float yBase = scoreY - scoreH * 0.65f;
                int negIdx = 0;
                int posIdx = 0;

                for (const auto& sp : state.scorePopups) {
                    std::string s = (sp.pts >= 0) ? ("+" + std::to_string(sp.pts)) : std::to_string(sp.pts);
                    if (sp.pts < 0) {
                        // User request: same float/fade feel as the white streak popup.
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

            if (state.endlessDangerActive && (state.mode == GameMode::PLAYING || state.mode == GameMode::PAUSED)) {
                bool veryCloseToDeath = (((state.paddlePos.z - 0.5f) - state.endlessDangerMaxZ) <= (1.33f * 2.0f));
                if (state.endlessDangerTimer < 10.0f || veryCloseToDeath) {
                    // Same clean fade in/out as the filter above (keeps everything synced).
                    const float period = 0.95f;
                    float ph = std::fmod(state.endlessDangerTimer, period);
                    if (ph < 0.0f) ph += period;
                    float x = ph / period; // 0..1
                    float tri = 1.0f - std::abs(2.0f * x - 1.0f);
                    float pulse = tri * tri * (3.0f - 2.0f * tri);
                    pulse = std::pow(pulse, 1.15f);

                    float triAlpha = 0.10f + 0.90f * pulse;
                    if (triAlpha < 0.02f) {
                        // effectively off (clean gap)
                    } else {
                        // 2. Signs (Red, Big & Transparent) — centered within the red danger band
                        std::string dMsg = "DANGER!";
                        float dScale = fitScaleToWidth(ctx.renderer, dMsg, 4.0f, (float)ctx.fbW * 0.92f);
                        float dW = ctx.renderer.measureUITextWidth(dMsg, dScale);
                        float xPos = (float)ctx.fbW * 0.5f - dW * 0.5f;

                        // Center vertically inside the danger band (y in UI coords is bottom-up)
                        float screenY = (ctx.dangerLineScreenY >= 0.0f) ? ctx.dangerLineScreenY : (float)ctx.fbH * 0.30f;
                        screenY = std::max(0.0f, std::min((float)ctx.fbH, screenY));
                        float th = ctx.renderer.getUIFontLineHeight(dScale);
                        float yPos = (screenY - th) * 0.5f;
                        yPos = std::max(12.0f, yPos);
                        if (yPos + th > screenY - 8.0f) yPos = std::max(12.0f, screenY - th - 8.0f);
                        float yMid = yPos + th * 0.5f;

                        // Make it read "bold" without overpowering: a tiny thickness pass + main text.
                        {
                            const float o = 1.25f;
                            const float a = triAlpha * 0.16f;
                            glm::vec4 thick(1.0f, 0.0f, 0.0f, a);
                            ctx.renderer.drawUIText(xPos - o, yPos, dMsg, dScale, thick);
                            ctx.renderer.drawUIText(xPos + o, yPos, dMsg, dScale, thick);
                            ctx.renderer.drawUIText(xPos, yPos - o, dMsg, dScale, thick);
                            ctx.renderer.drawUIText(xPos, yPos + o, dMsg, dScale, thick);
                        }

                        // Main crisp text (kept subtle)
                        ctx.renderer.drawUIText(xPos, yPos, dMsg, dScale, glm::vec4(1.0f, 0.0f, 0.0f, triAlpha * 0.30f));

                        auto drawTriangleSign = [&](float x) {
                            float triW = 100.0f;
                            float triH = 85.0f;
                            float yTri = yMid - triH * 0.5f;

                            glm::vec2 pTop(x, yTri + triH);
                            glm::vec2 pLeft(x - triW*0.5f, yTri);
                            glm::vec2 pRight(x + triW*0.5f, yTri);

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

                            ctx.renderer.drawUIQuad(x - 3, yTri + 30, 6, 40, glm::vec4(1.0f, 0.0f, 0.0f, triAlpha));
                            ctx.renderer.drawUIQuad(x - 3, yTri + 16, 6, 6, glm::vec4(1.0f, 0.0f, 0.0f, triAlpha));
                        };

                        // Symmetric composition around center
                        drawTriangleSign((float)ctx.fbW * 0.22f);
                        drawTriangleSign((float)ctx.fbW * 0.78f);
                    }
                }
            }
        } else {
            // Normal mode: score HUD removed (per request).
            // Normal mode: don't show score popups (green or red).
        }
    }

    // --- BG Selector HUD ---
    {
        float boxSize = 30.0f;
        float gap = 10.0f;
        float startX = (float)ctx.fbW - (boxSize + gap) * 5.0f - 20.0f;
        float startY = (float)ctx.fbH - 20.0f - boxSize;

        glm::vec3 colors[] = {
            {0.15f, 0.15f, 0.2f}, // None (Dark Blueish)
            {0.2f, 0.4f, 0.9f}, // Blueish
            {0.6f, 0.2f, 0.9f}, // Purplish
            {0.2f, 0.8f, 0.4f}, // Greenish (Swapped)
            {0.9f, 0.2f, 0.3f}  // Redish (Swapped)
        };

        for (int i = -1; i < 4; i++) {
            float bx = startX + (i + 1) * (boxSize + gap);
            glm::vec3 col = colors[i + 1];

            // Draw border if selected
            if (state.currentBg == i) {
                float border = 2.0f;
                ctx.renderer.drawUIQuad(bx - border, startY - border, boxSize + border * 2.0f, boxSize + border * 2.0f, glm::vec4(1, 1, 1, 1.0f));
            } else {
                col *= 0.6f;
            }

            ctx.renderer.drawUIQuad(bx, startY, boxSize, boxSize, glm::vec4(col, 1.0f));

            if (i == -1) {
                // Draw "/" inside the first box
                float tx = bx + (boxSize - 10.0f) * 0.5f;
                float ty = startY + (boxSize - 16.0f) * 0.5f;
                ctx.renderer.drawUIText(tx, ty, "/", 0.8f, glm::vec3(1, 1, 1));
            }
        }
    }

    // Overlay (Pause / Game Over / Win) — draw LAST so it overlaps everything (danger, HUD, etc.)
    if (state.mode != GameMode::PLAYING) {
        if (state.mode == GameMode::PAUSED) {
            const auto L = game::ui::pauseOverlay(ctx.fbW, ctx.fbH);
            float panelW2 = L.panel.w;
            float panelH2 = L.panel.h;
            float panelX2 = L.panel.x;
            float panelY2 = L.panel.y;

            // Full screen masked overlay (dimmed outside, clear inside)
            glm::vec2 maskMin(panelX2, panelY2);
            glm::vec2 maskMax(panelX2 + panelW2, panelY2 + panelH2);
            ctx.renderer.drawUIQuad(0, 0, (float)ctx.fbW, (float)ctx.fbH, glm::vec4(0, 0, 0, 0.82f), true, maskMin, maskMax);

            // Panel background
            ctx.renderer.drawUIQuad(panelX2, panelY2, panelW2, panelH2, glm::vec4(0.06f, 0.06f, 0.06f, 1.0f));

            // PAUSED text at the top
            std::string msg = "PAUSED";
            float scale = fitScaleToWidth(ctx.renderer, msg, 5.5f, panelW2 - 60.0f);
            float tw = ctx.renderer.measureUITextWidth(msg, scale);
            float th = ctx.renderer.getUIFontLineHeight(scale);

            float tx = panelX2 + (panelW2 - tw) * 0.5f;
            float ty = panelY2 + panelH2 - th - 55.0f;
            ctx.renderer.drawUIText(tx, ty, msg, scale, glm::vec3(1, 1, 1));

            // Buttons (Restart and Menu)
            float btnX_left = L.leftBtn.x;
            float btnY = L.leftBtn.y;
            float btnW = L.leftBtn.w;
            float btnH = L.leftBtn.h;
            float btnX_right = L.rightBtn.x;

            ctx.renderer.drawUIQuad(btnX_left, btnY, btnW, btnH, glm::vec4(0.8f, 0.2f, 0.2f, 1.0f));
            ctx.renderer.drawUIQuad(btnX_right, btnY, btnW, btnH, glm::vec4(0.2f, 0.8f, 0.2f, 1.0f));

            std::string leftLabel = "RESTART";
            std::string rightLabel = "MENU";

            float btnLabelScale = 1.0f;
            float ltw = ctx.renderer.measureUITextWidth(leftLabel, btnLabelScale);
            float rtw = ctx.renderer.measureUITextWidth(rightLabel, btnLabelScale);
            float lth = ctx.renderer.getUIFontLineHeight(btnLabelScale);

            ctx.renderer.drawUIText(btnX_left + (btnW - ltw) * 0.5f,  btnY + (btnH - lth) * 0.5f, leftLabel, btnLabelScale, glm::vec3(1, 1, 1));
            ctx.renderer.drawUIText(btnX_right + (btnW - rtw) * 0.5f, btnY + (btnH - lth) * 0.5f, rightLabel, btnLabelScale, glm::vec3(1, 1, 1));
        } else {
            const auto L = game::ui::endOverlay(ctx.fbW, ctx.fbH);
            float panelW = L.panel.w;
            float panelH = L.panel.h;
            float panelX = L.panel.x;
            float panelY = L.panel.y;

            // Full screen masked overlay for Game Over / Win
            glm::vec2 maskMin(panelX, panelY);
            glm::vec2 maskMax(panelX + panelW, panelY + panelH);
            ctx.renderer.drawUIQuad(0, 0, (float)ctx.fbW, (float)ctx.fbH, glm::vec4(0, 0, 0, 0.86f), true, maskMin, maskMax);

            // Panel for Game Over / Win - Fully opaque
            ctx.renderer.drawUIQuad(panelX, panelY, panelW, panelH, glm::vec4(0.08f, 0.08f, 0.08f, 1.0f));

            std::string title = (state.mode == GameMode::GAME_OVER) ? "GAME OVER" : "WINNER!";
            float titleScale = fitScaleToWidth(ctx.renderer, title, 5.8f, panelW - 60.0f);
            float tw = ctx.renderer.measureUITextWidth(title, titleScale);
            float th = ctx.renderer.getUIFontLineHeight(titleScale);
            float tx = panelX + (panelW - tw) * 0.5f;
            float ty = panelY + panelH - th - 45.0f;
            ctx.renderer.drawUIText(tx, ty, title, titleScale, glm::vec3(1, 1, 1));

            // Buttons
            float btnX_left = L.leftBtn.x;
            float btnY = L.leftBtn.y;
            float btnW = L.leftBtn.w;
            float btnH = L.leftBtn.h;
            float btnX_right = L.rightBtn.x;

            ctx.renderer.drawUIQuad(btnX_left, btnY, btnW, btnH, glm::vec4(0.8f, 0.2f, 0.2f, 1.0f));
            ctx.renderer.drawUIQuad(btnX_right, btnY, btnW, btnH, glm::vec4(0.2f, 0.8f, 0.2f, 1.0f));

            std::string leftLabel = "RETRY";
            std::string rightLabel = "MENU";

            float btnLabelScale = 1.0f;
            float ltw = ctx.renderer.measureUITextWidth(leftLabel, btnLabelScale);
            float rtw = ctx.renderer.measureUITextWidth(rightLabel, btnLabelScale);
            float lth = ctx.renderer.getUIFontLineHeight(btnLabelScale);

            ctx.renderer.drawUIText(btnX_left + (btnW - ltw) * 0.5f,  btnY + (btnH - lth) * 0.5f, leftLabel, btnLabelScale, glm::vec3(1, 1, 1));
            ctx.renderer.drawUIText(btnX_right + (btnW - rtw) * 0.5f, btnY + (btnH - lth) * 0.5f, rightLabel, btnLabelScale, glm::vec3(1, 1, 1));
        }
    }

    ctx.renderer.endUI();
}

} // namespace game::render


