#include "game/render/ui/HudParts.hpp"

#include "game/GameState.hpp"
#include "game/render/ui/UIHelpers.hpp"
#include "game/ui/OverlayLayout.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include <glm/glm.hpp>

namespace game::render::hud {

void drawPauseOrEndOverlay(const RenderContext& ctx, const GameState& state) {
    // Overlay (Pause / Game Over / Win) — draw LAST so it overlaps everything (danger, HUD, etc.)
    if (state.mode == GameMode::PLAYING) return;

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

        // Neon RGB border (matching title style)
        float bt2 = 3.0f;
        float hue2 = std::fmod(0.56f + 0.08f * std::sin(ctx.time.now() * 1.2f), 1.0f);
        glm::vec3 neon2 = ui::hsv2rgb(hue2, 0.85f, 1.0f);
        glm::vec4 border2(neon2, 1.0f);
        ctx.renderer.drawUIQuad(panelX2 - bt2, panelY2 - bt2, panelW2 + 2*bt2, bt2, border2);
        ctx.renderer.drawUIQuad(panelX2 - bt2, panelY2 + panelH2, panelW2 + 2*bt2, bt2, border2);
        ctx.renderer.drawUIQuad(panelX2 - bt2, panelY2, bt2, panelH2, border2);
        ctx.renderer.drawUIQuad(panelX2 + panelW2, panelY2, bt2, panelH2, border2);

        // PAUSED text at the top
        std::string msg = "PAUSED";
        float scale = ui::fitScaleToWidth(ctx.renderer, msg, 5.5f, panelW2 - 60.0f);
        float tw = ctx.renderer.measureUITextWidth(msg, scale);
        float th = ctx.renderer.getUIFontLineHeight(scale);

        float tx = panelX2 + (panelW2 - tw) * 0.5f;
        float ty = panelY2 + panelH2 - th - 55.0f;
        // Title glow + per-letter gradient
        glm::vec3 glowColP(0.10f, 0.35f, 0.90f);
        for (float o = 2.5f; o >= 1.0f; o -= 0.5f) {
            float a = 0.16f / o;
            ctx.renderer.drawUIText(tx - o, ty, msg, scale, glowColP * a);
            ctx.renderer.drawUIText(tx + o, ty, msg, scale, glowColP * a);
            ctx.renderer.drawUIText(tx, ty - o, msg, scale, glowColP * a);
            ctx.renderer.drawUIText(tx, ty + o, msg, scale, glowColP * a);
        }

        // Per-letter gradient
        int nP = (int)msg.size();
        for (int i = 0; i < nP; ++i) {
            float prefixWP = ctx.renderer.measureUITextWidth(msg.substr(0, i), scale);
            float cxP = tx + prefixWP;
            std::string chP = msg.substr(i, 1);

            // Outline
            glm::vec3 outlineP(0.02f, 0.02f, 0.06f);
            ctx.renderer.drawUIText(cxP - 2.0f, ty, chP, scale, outlineP);
            ctx.renderer.drawUIText(cxP + 2.0f, ty, chP, scale, outlineP);
            ctx.renderer.drawUIText(cxP, ty - 2.0f, chP, scale, outlineP);
            ctx.renderer.drawUIText(cxP, ty + 2.0f, chP, scale, outlineP);

            // Neon gradient
            float tP = ctx.time.now();
            float hueP = std::fmod(0.56f + (float)i / (float)std::max(1, nP-1) * 0.35f + 0.08f * std::sin(tP * 1.2f + i * 0.3f), 1.0f);
            glm::vec3 colP = ui::hsv2rgb(hueP, 0.85f, 1.0f);
            ctx.renderer.drawUIText(cxP, ty, chP, scale, colP);
        }

        // Buttons (Restart and Menu)
        float btnX_left = L.leftBtn.x;
        float btnY = L.leftBtn.y;
        float btnW = L.leftBtn.w;
        float btnH = L.leftBtn.h;
        float btnX_right = L.rightBtn.x;

        std::string leftLabel = "RESTART";
        std::string rightLabel = "MENU";

        ui::drawOverlayButton(ctx.renderer, btnX_left, btnY, btnW, btnH, leftLabel,
                              glm::vec3(0.8f, 0.2f, 0.2f), state.hoveredOverlayButton == 0);
        ui::drawOverlayButton(ctx.renderer, btnX_right, btnY, btnW, btnH, rightLabel,
                              glm::vec3(0.2f, 0.8f, 0.2f), state.hoveredOverlayButton == 1);
        return;
    }

    // GAME_OVER / WIN overlays
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

    // Neon RGB border (matching menu/instructions)
    float bt = 3.0f;
    float hue = std::fmod(0.56f + 0.08f * std::sin(ctx.time.now() * 1.2f), 1.0f);
    glm::vec3 neon = ui::hsv2rgb(hue, 0.85f, 1.0f);
    glm::vec4 border(neon, 1.0f);
    ctx.renderer.drawUIQuad(panelX - bt, panelY - bt, panelW + 2*bt, bt, border);
    ctx.renderer.drawUIQuad(panelX - bt, panelY + panelH, panelW + 2*bt, bt, border);
    ctx.renderer.drawUIQuad(panelX - bt, panelY, bt, panelH, border);
    ctx.renderer.drawUIQuad(panelX + panelW, panelY, bt, panelH, border);

    std::string title = (state.mode == GameMode::GAME_OVER) ? "GAME OVER" : "WINNER!";
    float titleScale = ui::fitScaleToWidth(ctx.renderer, title, 5.8f, panelW - 60.0f);
    float tw = ctx.renderer.measureUITextWidth(title, titleScale);
    float th = ctx.renderer.getUIFontLineHeight(titleScale);
    float tx = panelX + (panelW - tw) * 0.5f;
    float ty = panelY + panelH - th - 45.0f;
    // Title glow + per-letter gradient
    glm::vec3 glowColE(0.10f, 0.35f, 0.90f);
    for (float o = 2.5f; o >= 1.0f; o -= 0.5f) {
        float a = 0.16f / o;
        ctx.renderer.drawUIText(tx - o, ty, title, titleScale, glowColE * a);
        ctx.renderer.drawUIText(tx + o, ty, title, titleScale, glowColE * a);
        ctx.renderer.drawUIText(tx, ty - o, title, titleScale, glowColE * a);
        ctx.renderer.drawUIText(tx, ty + o, title, titleScale, glowColE * a);
    }

    // Per-letter gradient
    int nE = (int)title.size();
    for (int i = 0; i < nE; ++i) {
        float prefixWE = ctx.renderer.measureUITextWidth(title.substr(0, i), titleScale);
        float cxE = tx + prefixWE;
        std::string chE = title.substr(i, 1);

        // Outline
        glm::vec3 outlineE(0.02f, 0.02f, 0.06f);
        ctx.renderer.drawUIText(cxE - 2.0f, ty, chE, titleScale, outlineE);
        ctx.renderer.drawUIText(cxE + 2.0f, ty, chE, titleScale, outlineE);
        ctx.renderer.drawUIText(cxE, ty - 2.0f, chE, titleScale, outlineE);
        ctx.renderer.drawUIText(cxE, ty + 2.0f, chE, titleScale, outlineE);

        // Neon gradient
        float tE = ctx.time.now();
        float hueE = std::fmod(0.56f + (float)i / (float)std::max(1, nE-1) * 0.35f + 0.08f * std::sin(tE * 1.2f + i * 0.3f), 1.0f);
        glm::vec3 colE = ui::hsv2rgb(hueE, 0.85f, 1.0f);
        ctx.renderer.drawUIText(cxE, ty, chE, titleScale, colE);
    }

    // Buttons
    float btnX_left = L.leftBtn.x;
    float btnY = L.leftBtn.y;
    float btnW = L.leftBtn.w;
    float btnH = L.leftBtn.h;
    float btnX_right = L.rightBtn.x;

    std::string leftLabel = "RETRY";
    std::string rightLabel = "MENU";

    ui::drawOverlayButton(ctx.renderer, btnX_left, btnY, btnW, btnH, leftLabel,
                          glm::vec3(0.8f, 0.2f, 0.2f), state.hoveredOverlayButton == 0);
    ui::drawOverlayButton(ctx.renderer, btnX_right, btnY, btnW, btnH, rightLabel,
                          glm::vec3(0.2f, 0.8f, 0.2f), state.hoveredOverlayButton == 1);
}

} // namespace game::render::hud


