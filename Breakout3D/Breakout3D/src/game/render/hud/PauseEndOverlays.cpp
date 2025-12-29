#include "game/render/ui/HudParts.hpp"

#include "game/GameState.hpp"
#include "game/render/ui/UIHelpers.hpp"
#include "game/ui/OverlayLayout.hpp"
#include "game/ui/RogueCardLayout.hpp"
#include "game/rogue/RogueCards.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include <glm/glm.hpp>

namespace game::render::hud {

static void drawRgbTitle(engine::Renderer& r,
                         const std::string& title,
                         float x, float y,
                         float scale,
                         float timeNow) {
    // Glow
    glm::vec3 glow(0.10f, 0.65f, 1.00f);
    for (float o = 3.0f; o >= 1.0f; o -= 0.5f) {
        float a = 0.18f / o;
        r.drawUIText(x - o, y, title, scale, glow * a);
        r.drawUIText(x + o, y, title, scale, glow * a);
        r.drawUIText(x, y - o, title, scale, glow * a);
        r.drawUIText(x, y + o, title, scale, glow * a);
    }

    int n = (int)title.size();
    for (int i = 0; i < n; ++i) {
        float prefixW = r.measureUITextWidth(title.substr(0, i), scale);
        float cx = x + prefixW;
        std::string ch = title.substr(i, 1);

        // Outline
        glm::vec3 outline(0.02f, 0.02f, 0.06f);
        r.drawUIText(cx - 2.0f, y, ch, scale, outline);
        r.drawUIText(cx + 2.0f, y, ch, scale, outline);
        r.drawUIText(cx, y - 2.0f, ch, scale, outline);
        r.drawUIText(cx, y + 2.0f, ch, scale, outline);

        // RGB
        float hue = std::fmod(0.56f + (float)i / (float)std::max(1, n - 1) * 0.35f + 0.08f * std::sin(timeNow * 1.2f + i * 0.3f), 1.0f);
        glm::vec3 col = ui::hsv2rgb(hue, 0.85f, 1.0f);
        r.drawUIText(cx, y, ch, scale, col);
    }
}

void drawPauseOrEndOverlay(const RenderContext& ctx, const GameState& state) {
    // Overlay (Pause / Game Over / Win) — draw LAST so it overlaps everything (danger, HUD, etc.)
    if (state.mode == GameMode::PLAYING) return;

    // Rogue card pick overlay
    if (state.mode == GameMode::ROGUE_CARDS) {
        const auto L = game::ui::rogueCardOverlay(ctx.fbW, ctx.fbH);

        // No big center panel (like Instructions->Powerups vibe). Just a subtle fullscreen dim.
        ctx.renderer.drawUIQuad(0, 0, (float)ctx.fbW, (float)ctx.fbH, glm::vec4(0, 0, 0, 0.62f));

        auto prettifyDesc = [](const std::string& raw) -> std::string {
            // Convert "+Thing" / "-Thing" to clearer "GAIN:" / "COST:" lines.
            std::string out;
            out.reserve(raw.size() + 32);
            std::string line;
            for (size_t i = 0; i <= raw.size(); ++i) {
                char ch = (i < raw.size() ? raw[i] : '\n');
                if (ch != '\n') {
                    line.push_back(ch);
                    continue;
                }

                // trim left spaces
                size_t p = 0;
                while (p < line.size() && (line[p] == ' ' || line[p] == '\t')) p++;

                if (p < line.size() && (line[p] == '+' || line[p] == '-')) {
                    bool gain = (line[p] == '+');
                    // trim after +/- too
                    p++;
                    while (p < line.size() && (line[p] == ' ' || line[p] == '\t')) p++;
                    out += (gain ? "GAIN: " : "COST: ");
                    out.append(line.substr(p));
                } else {
                    out.append(line);
                }
                out.push_back('\n');
                line.clear();
            }

            // drop trailing newline if present
            if (!out.empty() && out.back() == '\n') out.pop_back();
            return out;
        };

        auto drawCard = [&](const game::ui::Rect& r, game::rogue::RogueCardId id, bool hovered) {
            const auto& def = game::rogue::cardDef(id);
            glm::vec3 acc = game::rogue::cardAccent(id);
            float uiS = 1.0f;
            float sh = hovered ? 20.0f * uiS : 10.0f * uiS;
            ctx.renderer.drawUIQuad(r.x + sh, r.y - sh, r.w, r.h, glm::vec4(0, 0, 0, hovered ? 0.66f : 0.55f));

            // Card body (match menu vibe)
            glm::vec4 body(0.08f, 0.08f, 0.14f, 0.98f);
            ctx.renderer.drawUIQuad(r.x, r.y, r.w, r.h, body);

            // Outline: hovered = animated RGB, otherwise accent
            float bt = (hovered ? 6.0f : 3.0f) * uiS;
            float hueRgb = std::fmod(0.56f + 0.08f * std::sin(ctx.time.now() * 1.2f), 1.0f);
            glm::vec3 neonRgb = ui::hsv2rgb(hueRgb, 0.85f, 1.0f);
            glm::vec4 borderCol = hovered ? glm::vec4(neonRgb, 1.0f) : glm::vec4(acc, 1.0f);
            ctx.renderer.drawUIQuad(r.x - bt, r.y - bt, r.w + 2 * bt, bt, borderCol);
            ctx.renderer.drawUIQuad(r.x - bt, r.y + r.h, r.w + 2 * bt, bt, borderCol);
            ctx.renderer.drawUIQuad(r.x - bt, r.y, bt, r.h, borderCol);
            ctx.renderer.drawUIQuad(r.x + r.w, r.y, bt, r.h, borderCol);

            // Card label
            std::string nm = def.name;
            float nameScale = ui::fitScaleToWidth(ctx.renderer, nm, 1.75f, r.w - 60.0f);
            float nw = ctx.renderer.measureUITextWidth(nm, nameScale);
            float nh = ctx.renderer.getUIFontLineHeight(nameScale);
            float nx = r.x + (r.w - nw) * 0.5f;
            float ny = r.y + r.h - nh - 28.0f * uiS;
            ctx.renderer.drawUIText(nx + 1.0f * uiS, ny - 1.0f * uiS, nm, nameScale, glm::vec4(0, 0, 0, 0.55f));
            ctx.renderer.drawUIText(nx, ny, nm, nameScale, glm::vec4(1, 1, 1, 1));

            // Description (tighter column like mode cards)
            std::string desc = prettifyDesc(def.shortDesc);
            float padX = 34.0f * uiS;
            float dScale = 1.18f * uiS; // bigger + easier to read
            glm::vec4 dCol(0.93f, 0.97f, 1.0f, 0.95f);
            float dTop = ny - 140.0f * uiS;
            ui::drawWrappedText(ctx.renderer, r.x + padX, dTop, r.w - 2.0f * padX, desc, dScale, dCol, 12.0f * uiS);

            // "PLAY button" style PICK button (like Play Modes cards)
            float pbW = r.w * 0.56f;
            float pbH = 76.0f * uiS;
            float pbX = r.x + (r.w - pbW) * 0.5f;
            float pbY = r.y + 26.0f * uiS;

            glm::vec3 base = acc;
            glm::vec3 btnCol = base * (hovered ? 1.25f : 1.0f);
            float btnShadow = hovered ? 6.0f * uiS : 3.0f * uiS;
            ctx.renderer.drawUIQuad(pbX + btnShadow, pbY - btnShadow, pbW, pbH, glm::vec4(0,0,0, hovered ? 0.55f : 0.45f));
            ctx.renderer.drawUIQuad(pbX, pbY, pbW, pbH, glm::vec4(btnCol, 1.0f));

            // Animated RGB border on PICK button when hovered specifically over the button
            if (state.hoveredRogueCardPickButton >= 0 && state.hoveredRogueCardPickButton < state.rogueOfferCount && 
                state.rogueOffer[state.hoveredRogueCardPickButton] == id) {
                float bt = 3.0f * uiS;
                float hueRgb = std::fmod(0.56f + 0.08f * std::sin(ctx.time.now() * 1.2f), 1.0f);
                glm::vec3 neonRgb = ui::hsv2rgb(hueRgb, 0.85f, 1.0f);
                glm::vec4 borderCol(neonRgb, 1.0f);
                ctx.renderer.drawUIQuad(pbX - bt, pbY - bt, pbW + 2 * bt, bt, borderCol);
                ctx.renderer.drawUIQuad(pbX - bt, pbY + pbH, pbW + 2 * bt, bt, borderCol);
                ctx.renderer.drawUIQuad(pbX - bt, pbY, bt, pbH, borderCol);
                ctx.renderer.drawUIQuad(pbX + pbW, pbY, bt, pbH, borderCol);
            }

            std::string lbl = "PICK";
            float pScale = 1.28f * uiS;
            float pW = ctx.renderer.measureUITextWidth(lbl, pScale);
            float pH = ctx.renderer.getUIFontLineHeight(pScale);
            float pX = pbX + (pbW - pW) * 0.5f;
            float pY = pbY + (pbH - pH) * 0.5f;
            ctx.renderer.drawUIText(pX + 1.0f * uiS, pY - 1.0f * uiS, lbl, pScale, glm::vec4(0,0,0,0.55f));
            ctx.renderer.drawUIText(pX, pY, lbl, pScale, glm::vec4(1,1,1,1));
        };

        if (state.rogueOfferCount > 0) drawCard(L.cardA, state.rogueOffer[0], state.hoveredRogueCard == 0);
        if (state.rogueOfferCount > 1) drawCard(L.cardB, state.rogueOffer[1], state.hoveredRogueCard == 1);
        if (state.rogueOfferCount > 2) drawCard(L.cardC, state.rogueOffer[2], state.hoveredRogueCard == 2);

        // Title OVER the cards, in the same top-center slot as Rogue's Wave/Score HUD.
        // (Wave/score are hidden during ROGUE_CARDS.)
        {
            std::string title = "CHOOSE A CARD";
            float scale = ui::fitScaleToWidth(ctx.renderer, title, 2.35f, (float)ctx.fbW * 0.92f);
            float th = ctx.renderer.getUIFontLineHeight(scale);
            float tw = ctx.renderer.measureUITextWidth(title, scale);
            float x = (float)ctx.fbW * 0.5f - tw * 0.5f;
            float y = (float)ctx.fbH - 18.0f - th; // matches top HUD margin

            drawRgbTitle(ctx.renderer, title, x, y, scale, ctx.time.now());
        }
        return;
    }

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


