#include "game/render/UIRender.hpp"

#include "game/GameAssets.hpp"
#include "game/GameState.hpp"
#include "game/rogue/RogueCards.hpp"
#include "game/render/ui/UIHelpers.hpp"
#include "game/ui/InstructionsOverlayLayout.hpp"
#include "game/ui/OverlayLayout.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace game::render {

void renderMenu(const RenderContext& ctx, const GameState& state, const GameAssets& assets) {
    ctx.renderer.beginUI(ctx.fbW, ctx.fbH);

    // === RETRO ANIMATED BACKGROUND ===
    // Background texture with slow scrolling effect
    float scrollSpeed = 0.05f;
    float scrollY = std::fmod(ctx.time.now() * scrollSpeed * 100.0f, (float)ctx.fbH);

    // Draw background twice for seamless scrolling
    ctx.renderer.drawUIQuad(0, scrollY, (float)ctx.fbW, (float)ctx.fbH, glm::vec4(1,1,1,0.15f), assets.backgroundTexs[0].id);
    ctx.renderer.drawUIQuad(0, scrollY - ctx.fbH, (float)ctx.fbW, (float)ctx.fbH, glm::vec4(1,1,1,0.15f), assets.backgroundTexs[0].id);

    // Dark overlay to make buttons pop
    ctx.renderer.drawUIQuad(0, 0, (float)ctx.fbW, (float)ctx.fbH, glm::vec4(0.0f, 0.0f, 0.0f, 0.7f));

    // Retro scan lines effect (horizontal lines)
    float scanLineSpacing = 4.0f;
    float scanLineAlpha = 0.08f;
    for (float y = 0; y < ctx.fbH; y += scanLineSpacing * 2) {
        ctx.renderer.drawUIQuad(0, y, (float)ctx.fbW, scanLineSpacing, glm::vec4(0, 0, 0, scanLineAlpha));
    }

    // Vignette effect (darker edges)
    float vignetteSize = ctx.fbW * 0.3f;
    ctx.renderer.drawUIQuad(0, 0, (float)ctx.fbW, vignetteSize,
                           glm::vec4(0.0f, 0.0f, 0.0f, 0.5f)); // Top
    ctx.renderer.drawUIQuad(0, ctx.fbH - vignetteSize, (float)ctx.fbW, vignetteSize,
                           glm::vec4(0.0f, 0.0f, 0.0f, 0.5f)); // Bottom

    // Floating particles/stars for retro atmosphere
    int numParticles = 100;
    for (int i = 0; i < numParticles; i++) {
        float seed = (float)i * 123.456f;
        float x = std::fmod(seed * 0.314f, 1.0f) * ctx.fbW;
        float speed = 15.0f + std::fmod(seed * 0.271f, 1.0f) * 25.0f;
        float yOffset = std::fmod(ctx.time.now() * speed + seed * 100.0f, (float)ctx.fbH + 100.0f) - 50.0f;
        float y = ctx.fbH - yOffset;

        float size = 2.0f + std::fmod(seed * 0.421f, 1.0f) * 3.0f;
        float pulse = 0.5f + 0.5f * std::sin(ctx.time.now() * 2.0f + seed);
        float alpha = 0.3f + 0.4f * pulse;

        glm::vec3 starColor(0.2f + 0.8f * pulse, 0.6f + 0.4f * pulse, 1.0f);
        ctx.renderer.drawUIQuad(x, y, size, size, glm::vec4(starColor * alpha, alpha));
    }

    // Menu layout is computed (with real font metrics) in Game::update/render and cached in state.
    const game::ui::MenuLayout& L = state.menuLayout;
    const float uiS = L.uiScale;

    // Menu title (BIG, centered on screen with glow effect)
    std::string title = "BREAKOUT 3D";
    float titleScale = L.titleScale;
    float titleX = L.titleX;
    float titleY = L.titleY;

    float panelW = L.panelW;
    float panelH = L.panelH;
    float panelX = L.panelX;
    float panelY = L.panelY;

    // Enhanced title: glow + per-letter neon gradient + outline
    glm::vec3 glowColor(0.10f, 0.35f, 0.90f);
    for (float offset = 3.0f; offset >= 1.0f; offset -= 0.5f) {
        float alpha = 0.18f / offset;
        ctx.renderer.drawUIText(titleX - offset, titleY, title, titleScale, glowColor * alpha);
        ctx.renderer.drawUIText(titleX + offset, titleY, title, titleScale, glowColor * alpha);
        ctx.renderer.drawUIText(titleX, titleY - offset, title, titleScale, glowColor * alpha);
        ctx.renderer.drawUIText(titleX, titleY + offset, title, titleScale, glowColor * alpha);
    }

    // Per-letter gradient with subtle animation
    int n = (int)title.size();
    for (int i = 0; i < n; ++i) {
        float prefixW = ctx.renderer.measureUITextWidth(title.substr(0, i), titleScale);
        float cx = titleX + prefixW;
        float cy = titleY;
        std::string ch = title.substr(i, 1);

        // Outline (thin) for readability
        glm::vec3 outlineCol(0.02f, 0.02f, 0.06f);
        ctx.renderer.drawUIText(cx - 2.0f, cy, ch, titleScale, outlineCol);
        ctx.renderer.drawUIText(cx + 2.0f, cy, ch, titleScale, outlineCol);
        ctx.renderer.drawUIText(cx, cy - 2.0f, ch, titleScale, outlineCol);
        ctx.renderer.drawUIText(cx, cy + 2.0f, ch, titleScale, outlineCol);

        // Neon gradient color across the word with time pulse
        float t = ctx.time.now();
        float hue = std::fmod(0.56f + (float)i / (float)std::max(1, n-1) * 0.35f + 0.08f * std::sin(t * 1.2f + i * 0.3f), 1.0f);
        glm::vec3 col = ui::hsv2rgb(hue, 0.85f, 1.0f);
        ctx.renderer.drawUIText(cx, cy, ch, titleScale, col);
    }

    // Background panel (MAIN/OPTIONS/INSTRUCTIONS screens only). PLAY_MODES and LEVEL_SELECT use custom panels.
    // If an Instructions overlay is open, we don't want the underlying screen panel to show through.
    if (!state.showInstructions && state.currentMenuScreen != MenuScreen::PLAY_MODES && state.currentMenuScreen != MenuScreen::LEVEL_SELECT) {
        float shadowOffset = 6.0f * uiS;
        ctx.renderer.drawUIQuad(panelX + shadowOffset, panelY - shadowOffset, panelW, panelH, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
        ctx.renderer.drawUIQuad(panelX, panelY, panelW, panelH, glm::vec4(0.08f, 0.08f, 0.14f, 0.98f));

        // Animated border with neon RGB (matching title style)
        float borderThickness = 3.0f * uiS;
        float tRgb = ctx.time.now();
        float hueRgb = std::fmod(0.56f + 0.08f * std::sin(tRgb * 1.2f), 1.0f);
        glm::vec3 neonRgb = ui::hsv2rgb(hueRgb, 0.85f, 1.0f);
        glm::vec4 borderColor(neonRgb, 1.0f);
        ctx.renderer.drawUIQuad(panelX - borderThickness, panelY - borderThickness, panelW + 2*borderThickness, borderThickness, borderColor);
        ctx.renderer.drawUIQuad(panelX - borderThickness, panelY + panelH, panelW + 2*borderThickness, borderThickness, borderColor);
        ctx.renderer.drawUIQuad(panelX - borderThickness, panelY, borderThickness, panelH, borderColor);
        ctx.renderer.drawUIQuad(panelX + panelW, panelY, borderThickness, panelH, borderColor);
    }

    // Buttons (positions must match InputSystem.cpp)
    float btnW = L.btn1.w;
    float btnH = L.btn1.h;
    float btnX = L.btn1.x;
    float btn1Y = L.btn1.y;
    float btn2Y = L.btn2.y;
    float btn3Y = L.btn3.y;
    float btn4Y = L.btn4.y;

    auto drawButton = [&](int btnIndex, float bx, float by, float bw, float bh,
                          const std::string& label, const glm::vec3& baseColor,
                          const std::string& subtitle = "") {
        bool hovered = (state.hoveredMenuButton == btnIndex);
        float hoverBrightness = hovered ? 1.3f : 1.0f;

        // Button shadow (larger on hover)
        float btnShadowOffset = (hovered ? 5.0f : 3.0f) * uiS;
        ctx.renderer.drawUIQuad(bx + btnShadowOffset, by - btnShadowOffset,
                               bw, bh, glm::vec4(0.0f, 0.0f, 0.0f, hovered ? 0.6f : 0.5f));

        // Button background (gradient effect with two quads)
        glm::vec3 color = baseColor * hoverBrightness;
        ctx.renderer.drawUIQuad(bx, by, bw, bh,
                               glm::vec4(color.r, color.g, color.b, 1.0f));
        ctx.renderer.drawUIQuad(bx, by + bh * 0.5f, bw, bh * 0.5f,
                               glm::vec4(color.r * 0.7f, color.g * 0.7f, color.b * 0.7f, 0.3f));

        // Button border (brighter and thicker on hover)
        float btnBorderThickness = (hovered ? 3.0f : 2.0f) * uiS;
        glm::vec4 btnBorder = hovered ? glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) : glm::vec4(0.0f, 0.0f, 0.0f, 0.4f);
        ctx.renderer.drawUIQuad(bx, by, bw, btnBorderThickness, btnBorder);
        ctx.renderer.drawUIQuad(bx, by + bh - btnBorderThickness, bw, btnBorderThickness, btnBorder);
        ctx.renderer.drawUIQuad(bx, by, btnBorderThickness, bh, btnBorder);
        ctx.renderer.drawUIQuad(bx + bw - btnBorderThickness, by, btnBorderThickness, bh, btnBorder);

        // Button label
        float labelScale = 1.0f * uiS;
        float labelW = ctx.renderer.measureUITextWidth(label, labelScale);
        float labelH = ctx.renderer.getUIFontLineHeight(labelScale);
        float labelX = bx + (bw - labelW) * 0.5f;
        float labelY = by + (bh - labelH) * 0.5f + (subtitle.empty() ? 0.0f : 8.0f * uiS);

        // Label shadow (stronger on hover)
        float shadowStrength = hovered ? 0.8f : 0.6f;
        ctx.renderer.drawUIText(labelX + 1.0f * uiS, labelY - 1.0f * uiS, label, labelScale, glm::vec3(0, 0, 0) * shadowStrength);
        ctx.renderer.drawUIText(labelX, labelY, label, labelScale, glm::vec3(1, 1, 1));

        // Subtitle (brighter on hover)
        if (!subtitle.empty()) {
            float subScale = 0.50f * uiS;
            float subW = ctx.renderer.measureUITextWidth(subtitle, subScale);
            float subX = bx + (bw - subW) * 0.5f;
            float subY = labelY - labelH * 0.5f - 4.0f * uiS;
            glm::vec3 subColor = hovered ? glm::vec3(1.0f, 1.0f, 1.0f) : glm::vec3(0.8f, 0.8f, 0.9f);
            ctx.renderer.drawUIText(subX, subY, subtitle, subScale, subColor);
        }
    };

    // Draw buttons based on current menu screen.
    // If the Instructions overlay is open, don't draw the underlying screen UI (so it doesn't "bleed through"
    // when parts of the overlay are intentionally transparent).
    if (!state.showInstructions) {
        if (state.currentMenuScreen == MenuScreen::MAIN) {
            // Main menu: PLAY, INSTRUCTIONS, OPTIONS, EXIT
            drawButton(0, btnX, btn1Y, btnW, btnH, "PLAY", glm::vec3(0.2f, 0.7f, 0.2f), "Game Modes");
            drawButton(1, btnX, btn2Y, btnW, btnH, "INSTRUCTIONS", glm::vec3(0.3f, 0.5f, 0.8f), "How to Play");
            drawButton(2, btnX, btn3Y, btnW, btnH, "OPTIONS", glm::vec3(0.7f, 0.5f, 0.2f), "Settings");
            drawButton(3, btnX, btn4Y, btnW, btnH, "EXIT", glm::vec3(0.7f, 0.2f, 0.2f), "Quit Game");
        } else if (state.currentMenuScreen == MenuScreen::PLAY_MODES) {
        // Play modes screen: one card per mode (aligned grid), each with title + description + PLAY button.
        // Hover should scale the ACTUAL card (no extra "invisible backplate" layer), and hovered card should draw last
        // so it can overlap the footer BACK button.
        auto drawModeCard = [&](int idx,
                                const game::ui::MenuLayout::ModeCard& c,
                                const std::string& modeTitle,
                                const std::string& desc,
                                const std::vector<std::string>& features,
                                const glm::vec3& accent,
                                bool enabled,
                                bool hovered) {

            // Scale the card itself (visual only).
            float pop = hovered ? 1.10f : 1.0f;
            float cx = c.card.x + c.card.w * 0.5f;
            float cy = c.card.y + c.card.h * 0.5f;
            float w = c.card.w * pop;
            float h = c.card.h * pop;
            float cardX = cx - w * 0.5f;
            float cardY = cy - h * 0.5f;

            // RGB outline for hovered card (single outline), accent outline otherwise.
            float hueRgb = std::fmod(0.56f + 0.08f * std::sin(ctx.time.now() * 1.2f), 1.0f);
            glm::vec3 neonRgb = ui::hsv2rgb(hueRgb, 0.85f, 1.0f);
            glm::vec4 borderCol = hovered ? glm::vec4(neonRgb, 1.0f) : glm::vec4(accent, 1.0f);

            // Card shadow
            float sh = (hovered ? 20.0f : 8.0f) * uiS;
            ctx.renderer.drawUIQuad(cardX + sh, cardY - sh, w, h, glm::vec4(0, 0, 0, hovered ? 0.66f : 0.55f));

            // Card body
            glm::vec4 body(0.08f, 0.08f, 0.14f, 0.98f);
            ctx.renderer.drawUIQuad(cardX, cardY, w, h, body);

            // Single outline (no extra layers)
            float bt = (hovered ? 6.0f : 3.0f) * uiS;
            ctx.renderer.drawUIQuad(cardX - bt, cardY - bt, w + 2*bt, bt, borderCol);
            ctx.renderer.drawUIQuad(cardX - bt, cardY + h,  w + 2*bt, bt, borderCol);
            ctx.renderer.drawUIQuad(cardX - bt, cardY, bt, h, borderCol);
            ctx.renderer.drawUIQuad(cardX + w,  cardY, bt, h, borderCol);

            // Title
            // Bigger, easier-to-read type for tall cards
            float tScale = 1.75f * uiS;
            float tW = ctx.renderer.measureUITextWidth(modeTitle, tScale);
            float tH = ctx.renderer.getUIFontLineHeight(tScale);
            float tX = cardX + (w - tW) * 0.5f;
            float tY = cardY + h - tH - 22.0f * uiS;
            ctx.renderer.drawUIText(tX + 1.0f * uiS, tY - 1.0f * uiS, modeTitle, tScale, glm::vec4(0,0,0,0.65f));
            ctx.renderer.drawUIText(tX, tY, modeTitle, tScale, glm::vec4(1,1,1,1));

            // Description
            // More left/right padding so text breathes inside tall cards.
            float padX = 34.0f * uiS;
            float maxW = w - padX * 2.0f;

            float dScale = 0.86f * uiS;
            glm::vec4 dCol = enabled ? glm::vec4(0.82f, 0.92f, 1.0f, 0.90f) : glm::vec4(0.80f, 0.80f, 0.84f, 0.85f);
            // BIG gap between title and description (user request).
            float dTop = tY - 120.0f * uiS;
            ui::drawWrappedText(ctx.renderer, cardX + padX, dTop, maxW, desc, dScale, dCol, 7.0f * uiS);

            // Features (bullets)
            float lhDesc = ctx.renderer.getUIFontLineHeight(dScale);
            auto dLines = ui::wrapText(ctx.renderer, desc, dScale, maxW);
            // Clear separation between description and features (more pleasing hierarchy).
            float yCursor = dTop - (float)dLines.size() * (lhDesc + 7.0f * uiS) - 18.0f * uiS;

            // Small "FEATURES" label (helps the block feel intentional).
            {
                std::string hdr = "FEATURES";
                float hs = 0.62f * uiS;
                glm::vec4 hcol(accent, enabled ? 0.95f : 0.70f);
                ctx.renderer.drawUIText(cardX + padX, yCursor, hdr, hs, hcol);
                float hh = ctx.renderer.getUIFontLineHeight(hs);
                yCursor -= hh + 10.0f * uiS;
            }

            float fScale = 0.80f * uiS;
            float lh = ctx.renderer.getUIFontLineHeight(fScale);
            float lineGap = 6.0f * uiS;

            glm::vec4 fCol = enabled ? glm::vec4(0.92f, 0.96f, 1.0f, 0.88f) : glm::vec4(0.86f, 0.86f, 0.90f, 0.80f);
            glm::vec4 dotCol(accent, enabled ? 0.95f : 0.70f);
            float dot = 6.0f * uiS;
            float bulletGap = 18.0f * uiS;

            // Keep text above the PLAY/SOON button.
            const float minY = c.playBtn.y + c.playBtn.h + 22.0f * uiS;

            for (const auto& ft : features) {
                if (yCursor < minY) break;
                // dot (simple square bullet)
                ctx.renderer.drawUIQuad(cardX + padX, yCursor + lh * 0.35f, dot, dot, dotCol);
                float textX = cardX + padX + dot + bulletGap;
                float textW = maxW - (dot + bulletGap);
                ui::drawWrappedText(ctx.renderer, textX, yCursor, textW, ft, fScale, fCol, lineGap);
                auto fl = ui::wrapText(ctx.renderer, ft, fScale, textW);
                yCursor -= (float)fl.size() * (lh + lineGap) + 12.0f * uiS;
            }

            // Play button
            glm::vec3 base = enabled ? accent : glm::vec3(0.38f, 0.38f, 0.40f);
            glm::vec3 btnCol = base * (hovered ? 1.25f : 1.0f);
            float btnShadow = hovered ? 5.0f * uiS : 3.0f * uiS;
            ctx.renderer.drawUIQuad(c.playBtn.x + btnShadow, c.playBtn.y - btnShadow, c.playBtn.w, c.playBtn.h,
                                    glm::vec4(0,0,0, hovered ? 0.55f : 0.45f));
            ctx.renderer.drawUIQuad(c.playBtn.x, c.playBtn.y, c.playBtn.w, c.playBtn.h, glm::vec4(btnCol, 1.0f));

            // Neon RGB border on PLAY button when the button itself is hovered (matches Rogue PICK hover)
            bool playHovered = (state.hoveredPlayModeButton == idx);
            if (playHovered) {
                float bt2 = 3.0f * uiS;
                float hueRgb2 = std::fmod(0.56f + 0.08f * std::sin(ctx.time.now() * 1.2f), 1.0f);
                glm::vec3 neonRgb2 = ui::hsv2rgb(hueRgb2, 0.85f, 1.0f);
                glm::vec4 borderCol2(neonRgb2, 1.0f);
                ctx.renderer.drawUIQuad(c.playBtn.x - bt2, c.playBtn.y - bt2, c.playBtn.w + 2 * bt2, bt2, borderCol2);
                ctx.renderer.drawUIQuad(c.playBtn.x - bt2, c.playBtn.y + c.playBtn.h, c.playBtn.w + 2 * bt2, bt2, borderCol2);
                ctx.renderer.drawUIQuad(c.playBtn.x - bt2, c.playBtn.y, bt2, c.playBtn.h, borderCol2);
                ctx.renderer.drawUIQuad(c.playBtn.x + c.playBtn.w, c.playBtn.y, bt2, c.playBtn.h, borderCol2);
            }

            // Button label
            std::string lbl = enabled ? "PLAY" : "SOON";
            float pScale = 1.28f * uiS;
            float pW = ctx.renderer.measureUITextWidth(lbl, pScale);
            float pH = ctx.renderer.getUIFontLineHeight(pScale);
            float pX = c.playBtn.x + (c.playBtn.w - pW) * 0.5f;
            float pY = c.playBtn.y + (c.playBtn.h - pH) * 0.5f;
            ctx.renderer.drawUIText(pX + 1.0f * uiS, pY - 1.0f * uiS, lbl, pScale, glm::vec4(0,0,0,0.55f));
            ctx.renderer.drawUIText(pX, pY, lbl, pScale, glm::vec4(1,1,1,1));
        };

        const int hoveredIdx = (state.hoveredMenuButton >= 0 && state.hoveredMenuButton <= 3) ? state.hoveredMenuButton : -1;

        // Draw non-hovered cards first
        if (hoveredIdx != 0) drawModeCard(0, L.normal, "NORMAL",
            "A classic run: clear the wall, keep your lives, and learn the feel of the paddle + ball.",
            {"Win by destroying every brick.", "Balanced power-ups and curses.", "Great for quick matches and learning."},
            glm::vec3(0.25f, 0.85f, 0.25f), true, false);

        if (hoveredIdx != 1) drawModeCard(1, L.endless, "ENDLESS",
            "No finish line. Survive the pressure as new rows spawn and your score keeps climbing.",
            {"Infinite waves with rising intensity.", "Score streak banking (risk vs. safety).", "Perfect for high-score chasing."},
            glm::vec3(0.95f, 0.55f, 0.15f), true, false);

        if (hoveredIdx != 2) drawModeCard(2, L.rogue, "ROGUE",
            "Build a run by choosing upgrades and taking risks — every decision shapes the next round.",
            {"Draft reward cards after wave milestones.", "No random drops — choices define your run.", "Wave-based progression (prototype)."},
            glm::vec3(0.75f, 0.25f, 0.85f), true, false);

        if (hoveredIdx != 3) drawModeCard(3, L.levels, "LEVELS",
            "A curated set of 10 stages with unique brick layouts.",
            {"Hand-crafted brick patterns.", "10 progressive levels.", "Win by completing all levels."},
            glm::vec3(0.20f, 0.75f, 0.85f), true, false);

        // BACK button (footer, bottom-left) — draws before hovered card so hovered can overlap it.
        bool backHover = (state.hoveredMenuButton == 4);
        float bx = L.backBtn.x, by = L.backBtn.y, bw = L.backBtn.w, bh = L.backBtn.h;
        float sh = (backHover ? 6.0f : 3.0f) * uiS;
        ctx.renderer.drawUIQuad(bx + sh, by - sh, bw, bh, glm::vec4(0,0,0, backHover ? 0.55f : 0.45f));
        glm::vec3 base(0.10f, 0.10f, 0.16f);
        glm::vec3 col = base * (backHover ? 1.25f : 1.0f);
        ctx.renderer.drawUIQuad(bx, by, bw, bh, glm::vec4(col, 0.95f));

        // Neon border (subtle, matches menu vibe)
        float bt = 2.5f * uiS;
        float hue = std::fmod(0.56f + 0.08f * std::sin(ctx.time.now() * 1.2f), 1.0f);
        glm::vec3 neon = ui::hsv2rgb(hue, 0.85f, 1.0f);
        glm::vec4 bcol(neon, backHover ? 1.0f : 0.75f);
        ctx.renderer.drawUIQuad(bx - bt, by - bt, bw + 2*bt, bt, bcol);
        ctx.renderer.drawUIQuad(bx - bt, by + bh, bw + 2*bt, bt, bcol);
        ctx.renderer.drawUIQuad(bx - bt, by, bt, bh, bcol);
        ctx.renderer.drawUIQuad(bx + bw, by, bt, bh, bcol);
        std::string back = "< BACK";
        // Scale label from actual button height for consistency.
        float bs = (bh / 56.0f) * (0.78f * uiS);
        float tw = ctx.renderer.measureUITextWidth(back, bs);
        float th = ctx.renderer.getUIFontLineHeight(bs);
        // Slight left bias feels more "navigation" than "action".
        float tx = bx + (bw - tw) * 0.5f - 6.0f * uiS;
        float ty = by + (bh - th) * 0.5f;
        ctx.renderer.drawUIText(tx + 1.0f * uiS, ty - 1.0f * uiS, back, bs, glm::vec4(0,0,0,0.55f));
        ctx.renderer.drawUIText(tx, ty, back, bs, glm::vec4(1,1,1,1));

        // Draw hovered card last (overlaps footer/back as requested)
        if (hoveredIdx == 0) drawModeCard(0, L.normal, "NORMAL",
            "A classic run: clear the wall, keep your lives, and learn the feel of the paddle + ball.",
            {"Win by destroying every brick.", "Balanced power-ups and curses.", "Great for quick matches and learning."},
            glm::vec3(0.25f, 0.85f, 0.25f), true, true);

        if (hoveredIdx == 1) drawModeCard(1, L.endless, "ENDLESS",
            "No finish line. Survive the pressure as new rows spawn and your score keeps climbing.",
            {"Infinite waves with rising intensity.", "Score streak banking (risk vs. safety).", "Perfect for high-score chasing."},
            glm::vec3(0.95f, 0.55f, 0.15f), true, true);

        if (hoveredIdx == 2) drawModeCard(2, L.rogue, "ROGUE",
            "Build a run by choosing upgrades and taking risks — every decision shapes the next round.",
            {"Draft reward cards after wave milestones.", "No random drops — choices define your run.", "Wave-based progression (prototype)."},
            glm::vec3(0.75f, 0.25f, 0.85f), true, true);

        if (hoveredIdx == 3) drawModeCard(3, L.levels, "LEVELS",
            "A curated set of 10 stages with unique brick layouts.",
            {"Hand-crafted brick patterns.", "10 progressive levels.", "Win by completing all levels."},
            glm::vec3(0.20f, 0.75f, 0.85f), true, true);
        } else if (state.currentMenuScreen == MenuScreen::OPTIONS) {
        // Options submenu: SOUND (placeholder), GRAPHICS (placeholder), BACK
        drawButton(0, btnX, btn1Y - 50.0f, btnW, btnH, "SOUND", glm::vec3(0.3f, 0.6f, 0.7f), "Coming Soon");
        drawButton(1, btnX, btn2Y - 50.0f, btnW, btnH, "GRAPHICS", glm::vec3(0.6f, 0.3f, 0.7f), "Coming Soon");

        // BACK button
        float backW = 120.0f * uiS;
        float backH = 50.0f * uiS;
        float backX = panelX + 20.0f * uiS;
        float backY = panelY + 15.0f * uiS;
        drawButton(2, backX, backY, backW, backH, "< BACK", glm::vec3(0.5f, 0.5f, 0.5f), "");
        } else if (state.currentMenuScreen == MenuScreen::INSTRUCTIONS) {
        // Instructions screen: CONTROLS, POWERUPS, ROGUE CARDS, BACK (buttons open the detailed overlay panel)
        drawButton(0, btnX, btn1Y - 50.0f, btnW, btnH, "CONTROLS", glm::vec3(0.3f, 0.6f, 0.7f), "View");
        drawButton(1, btnX, btn2Y - 50.0f, btnW, btnH, "POWERUPS", glm::vec3(0.6f, 0.3f, 0.7f), "View");
        drawButton(2, btnX, btn3Y - 50.0f, btnW, btnH, "ROGUE CARDS", glm::vec3(0.85f, 0.55f, 0.15f), "Browse");

        // BACK button
        float backW = 120.0f * uiS;
        float backH = 50.0f * uiS;
        float backX = panelX + 20.0f * uiS;
        float backY = panelY + 15.0f * uiS;
        drawButton(3, backX, backY, backW, backH, "< BACK", glm::vec3(0.5f, 0.5f, 0.5f), "");
        } else if (state.currentMenuScreen == MenuScreen::LEVEL_SELECT) {
        // LEVEL SELECT screen: custom larger panel for level grid
        float levelPanelW = 720.0f * uiS;
        float levelPanelH = 420.0f * uiS;  // Smaller height than before
        float levelPanelX = ((float)ctx.fbW - levelPanelW) * 0.5f;
        float levelPanelY = ((float)ctx.fbH * 0.45f) - levelPanelH * 0.5f;
        
        // Draw custom panel for level select
        float shadowOffset = 6.0f * uiS;
        ctx.renderer.drawUIQuad(levelPanelX + shadowOffset, levelPanelY - shadowOffset, levelPanelW, levelPanelH, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
        ctx.renderer.drawUIQuad(levelPanelX, levelPanelY, levelPanelW, levelPanelH, glm::vec4(0.08f, 0.08f, 0.14f, 0.98f));
        
        // Animated border
        float borderThickness = 3.0f * uiS;
        float tRgb = ctx.time.now();
        float hueRgb = std::fmod(0.56f + 0.08f * std::sin(tRgb * 1.2f), 1.0f);
        glm::vec3 neonRgb = ui::hsv2rgb(hueRgb, 0.85f, 1.0f);
        glm::vec4 borderColor(neonRgb, 1.0f);
        ctx.renderer.drawUIQuad(levelPanelX - borderThickness, levelPanelY - borderThickness, levelPanelW + 2*borderThickness, borderThickness, borderColor);
        ctx.renderer.drawUIQuad(levelPanelX - borderThickness, levelPanelY + levelPanelH, levelPanelW + 2*borderThickness, borderThickness, borderColor);
        ctx.renderer.drawUIQuad(levelPanelX - borderThickness, levelPanelY, borderThickness, levelPanelH, borderColor);
        ctx.renderer.drawUIQuad(levelPanelX + levelPanelW, levelPanelY, borderThickness, levelPanelH, borderColor);
        
        // LEVEL SELECT screen: 2 rows x 5 columns grid of level buttons (10 total)
        const int totalLevels = 10;
        const int cols = 5;
        const int rows = 2;
        
        // Grid layout
        float btnSize = 110.0f * uiS;
        float gapX = 25.0f * uiS;
        float gapY = 25.0f * uiS;
        float gridW = cols * btnSize + (cols - 1) * gapX;
        float gridH = rows * btnSize + (rows - 1) * gapY;
        float startX = levelPanelX + (levelPanelW - gridW) * 0.5f;
        float startY = levelPanelY + 80.0f * uiS;
        
        // Title: "SELECT LEVEL"
        {
            std::string title = "SELECT LEVEL";
            float titleScale = 2.0f * uiS;
            float titleW = ctx.renderer.measureUITextWidth(title, titleScale);
            float titleX = levelPanelX + (levelPanelW - titleW) * 0.5f;
            float titleY = startY + gridH + 50.0f * uiS;
            
            // Cyan glow
            glm::vec3 titleColor(0.20f, 0.75f, 0.85f);
            ctx.renderer.drawUIText(titleX - 2.0f * uiS, titleY, title, titleScale, glm::vec4(0,0,0,0.6f));
            ctx.renderer.drawUIText(titleX + 2.0f * uiS, titleY, title, titleScale, glm::vec4(0,0,0,0.6f));
            ctx.renderer.drawUIText(titleX, titleY - 2.0f * uiS, title, titleScale, glm::vec4(0,0,0,0.6f));
            ctx.renderer.drawUIText(titleX, titleY + 2.0f * uiS, title, titleScale, glm::vec4(0,0,0,0.6f));
            ctx.renderer.drawUIText(titleX, titleY, title, titleScale, glm::vec4(titleColor, 1.0f));
        }
        
        // Draw level buttons
        for (int i = 0; i < totalLevels; i++) {
            int row = i / cols;
            int col = i % cols;
            float x = startX + col * (btnSize + gapX);
            float y = startY + (rows - 1 - row) * (btnSize + gapY);
            
            bool unlocked = (i < state.levelsBestLevel);
            bool hovered = (state.hoveredLevelButton == i);
            
            // Button background
            float shadow = (hovered ? 5.0f : 3.0f) * uiS;
            ctx.renderer.drawUIQuad(x + shadow, y - shadow, btnSize, btnSize, glm::vec4(0,0,0, hovered ? 0.6f : 0.5f));
            
            glm::vec3 bgColor = unlocked 
                ? (hovered ? glm::vec3(0.25f, 0.90f, 1.0f) : glm::vec3(0.15f, 0.50f, 0.65f))
                : glm::vec3(0.12f, 0.12f, 0.15f);
            float alpha = unlocked ? 0.95f : 0.50f;
            ctx.renderer.drawUIQuad(x, y, btnSize, btnSize, glm::vec4(bgColor, alpha));
            
            // Border
            float borderThick = (hovered ? 3.5f : 2.5f) * uiS;
            glm::vec3 borderColor = unlocked ? glm::vec3(0.20f, 0.75f, 0.85f) : glm::vec3(0.25f, 0.25f, 0.30f);
            ctx.renderer.drawUIQuad(x - borderThick, y - borderThick, btnSize + 2*borderThick, borderThick, glm::vec4(borderColor, 1));
            ctx.renderer.drawUIQuad(x - borderThick, y + btnSize, btnSize + 2*borderThick, borderThick, glm::vec4(borderColor, 1));
            ctx.renderer.drawUIQuad(x - borderThick, y, borderThick, btnSize, glm::vec4(borderColor, 1));
            ctx.renderer.drawUIQuad(x + btnSize, y, borderThick, btnSize, glm::vec4(borderColor, 1));
            
            // Level number
            std::string numStr = std::to_string(i + 1);
            float numScale = 2.5f * uiS;
            float numW = ctx.renderer.measureUITextWidth(numStr, numScale);
            float numX = x + (btnSize - numW) * 0.5f;
            float numY = y + (btnSize - ctx.renderer.getUIFontLineHeight(numScale)) * 0.5f;
            
            glm::vec4 numColor = unlocked ? glm::vec4(1,1,1,1) : glm::vec4(0.4f, 0.4f, 0.45f, 0.8f);
            ctx.renderer.drawUIText(numX, numY, numStr, numScale, numColor);
        }
        
        // BACK button
        float backW = 120.0f * uiS;
        float backH = 50.0f * uiS;
        float backX = levelPanelX + 20.0f * uiS;
        float backY = levelPanelY + 15.0f * uiS;
        drawButton(10, backX, backY, backW, backH, "< BACK", glm::vec3(0.5f, 0.5f, 0.5f), "");
        }
    }

    // Small clickable "4" badge for the one-brick test mode (only on MAIN menu, bottom-right corner)
    if (!state.showInstructions && state.currentMenuScreen == MenuScreen::MAIN) {
        float badgeW = L.testBadge.w;
        float badgeH = L.testBadge.h;
        float badgeX = L.testBadge.x;
        float badgeY = L.testBadge.y;

        bool hovered = state.hoveredTestBadge;
        float hoverScale = hovered ? 1.15f : 1.0f;
        float hoverBrightness = hovered ? 1.4f : 1.0f;

        // Shadow (larger on hover)
        float shadowOffset = (hovered ? 4.0f : 2.0f) * uiS;
        ctx.renderer.drawUIQuad(badgeX + shadowOffset, badgeY - shadowOffset,
                               badgeW, badgeH, glm::vec4(0.0f, 0.0f, 0.0f, hovered ? 0.6f : 0.4f));

        // Badge background with hover brightness
        glm::vec3 badgeColor = glm::vec3(0.2f, 0.8f, 1.0f) * hoverBrightness;
        ctx.renderer.drawUIQuad(badgeX, badgeY, badgeW, badgeH, glm::vec4(badgeColor, 1.0f));
        ctx.renderer.drawUIQuad(badgeX + 2.0f, badgeY + 2.0f, badgeW - 4.0f, badgeH - 4.0f, glm::vec4(0.06f, 0.06f, 0.10f, 1.0f));

        // "4" text with hover scale and glow
        std::string k = "4";
        float kScale = 1.4f * hoverScale * uiS;
        float kW = ctx.renderer.measureUITextWidth(k, kScale);
        float kH = ctx.renderer.getUIFontLineHeight(kScale);
        float kX = badgeX + (badgeW - kW) * 0.5f;
        float kY = badgeY + (badgeH - kH) * 0.5f;

        // Glow effect on hover
        if (hovered) {
            ctx.renderer.drawUIText(kX + 1.0f, kY, k, kScale, glm::vec3(0.5f, 1.0f, 1.0f) * 0.5f);
            ctx.renderer.drawUIText(kX - 1.0f, kY, k, kScale, glm::vec3(0.5f, 1.0f, 1.0f) * 0.5f);
        }
        ctx.renderer.drawUIText(kX, kY, k, kScale, glm::vec3(0.2f, 0.8f, 1.0f) * hoverBrightness);

        // Hint text above the badge (brighter on hover)
        std::string hint = "ONE BRICK";
        float hScale = 0.48f * uiS;
        float hW = ctx.renderer.measureUITextWidth(hint, hScale);
        glm::vec3 hintColor = hovered ? glm::vec3(0.8f, 1.0f, 1.0f) : glm::vec3(0.6f, 0.85f, 1.0f);
        ctx.renderer.drawUIText(badgeX + (badgeW - hW) * 0.5f, badgeY + badgeH + 8.0f * uiS, hint, hScale, hintColor);
    }

    // Show instructions overlay if toggled
    if (state.showInstructions) {
        const auto OL = game::ui::instructionsOverlayLayout(L, ctx.fbW, ctx.fbH, state.instructionsTab);
        const float instrX = OL.panel.x;
        const float instrY = OL.panel.y;
        const float instrW = OL.panel.w;
        const float instrH = OL.panel.h;

        // Overlay fill:
        // - Controls: solid panel for readability.
        // - Powerups/Rogue Cards: NO big background panel (content floats on background).
        if (state.instructionsTab == 0) {
            ctx.renderer.drawUIQuad(instrX, instrY, instrW, instrH, glm::vec4(0.05f, 0.05f, 0.1f, 0.98f));
        }

        // Instructions border — keep for Controls, but for Powerups we avoid a big "panel box"
        // so the left model area feels truly on the background.
        float borderThickness = 3.0f;
        float tRgbInstr = ctx.time.now();
        float hueRgbInstr = std::fmod(0.56f + 0.08f * std::sin(tRgbInstr * 1.2f), 1.0f);
        glm::vec3 neonRgbInstr = ui::hsv2rgb(hueRgbInstr, 0.85f, 1.0f);
        glm::vec4 instrBorder(neonRgbInstr, 1.0f);
        if (state.instructionsTab == 0) {
            ctx.renderer.drawUIQuad(instrX - borderThickness, instrY - borderThickness, instrW + 2*borderThickness, borderThickness, instrBorder);
            ctx.renderer.drawUIQuad(instrX - borderThickness, instrY + instrH, instrW + 2*borderThickness, borderThickness, instrBorder);
            ctx.renderer.drawUIQuad(instrX - borderThickness, instrY, borderThickness, instrH, instrBorder);
            ctx.renderer.drawUIQuad(instrX + instrW, instrY, borderThickness, instrH, instrBorder);
        }

        // Title with glow + underline (tab-aware)
        std::string instrTitle =
            (state.instructionsTab == 0) ? "CONTROLS" :
            (state.instructionsTab == 1) ? "POWERUPS" :
            "ROGUE CARDS";
        float instrTitleScale = 1.6f * uiS;
        float instrTitleW = ctx.renderer.measureUITextWidth(instrTitle, instrTitleScale);
        float instrTitleX = instrX + (instrW - instrTitleW) * 0.5f;
        // Powerups/RogueCards title should sit lower so it doesn't crowd the big "BREAKOUT 3D" header.
        float titleTopPad = (state.instructionsTab == 1 || state.instructionsTab == 2) ? (96.0f * uiS) : (42.0f * uiS);
        float instrTitleY = instrY + instrH - titleTopPad;
        // In Powerups/Rogue Cards (no panel fill), add a tiny backdrop behind the title only (keeps it readable).
        if (state.instructionsTab == 1 || state.instructionsTab == 2) {
            float padX = 26.0f * uiS;
            float padY = 12.0f * uiS;
            float th = ctx.renderer.getUIFontLineHeight(instrTitleScale);
            ctx.renderer.drawUIQuad(instrTitleX - padX, instrTitleY - padY, instrTitleW + 2.0f * padX, th + 2.0f * padY,
                                   glm::vec4(0.05f, 0.05f, 0.1f, 0.40f));
        }
        glm::vec3 glowCol(0.10f, 0.35f, 0.90f);
        for (float o = 2.5f; o >= 1.0f; o -= 0.5f) {
            float a = 0.16f / o;
            ctx.renderer.drawUIText(instrTitleX - o, instrTitleY, instrTitle, instrTitleScale, glowCol * a);
            ctx.renderer.drawUIText(instrTitleX + o, instrTitleY, instrTitle, instrTitleScale, glowCol * a);
            ctx.renderer.drawUIText(instrTitleX, instrTitleY - o, instrTitle, instrTitleScale, glowCol * a);
            ctx.renderer.drawUIText(instrTitleX, instrTitleY + o, instrTitle, instrTitleScale, glowCol * a);
        }
        ctx.renderer.drawUIText(instrTitleX, instrTitleY, instrTitle, instrTitleScale, glm::vec3(0.2f, 0.85f, 1.0f));
        // Underline neon bar
        float barW = instrTitleW;
        float barH = 4.0f * uiS;
        float hueBar = std::fmod(0.56f + 0.08f * std::sin(ctx.time.now() * 1.2f), 1.0f);
        glm::vec3 neonBar = ui::hsv2rgb(hueBar, 0.85f, 1.0f);
        ctx.renderer.drawUIQuad(instrTitleX, instrTitleY - 6.0f * uiS, barW, barH, glm::vec4(neonBar, 1.0f));

        if (state.instructionsTab == 0) {
            // Controls list (single panel; no inner "card", no duplicate title, no tip)
            float pad = 44.0f;
            float x = instrX + pad;
            float y = instrY + instrH - 132.0f;

            float lineGap = 68.0f;
            float badgeScale = 1.20f;
            float descScale = 1.10f;
            struct CtrlItem { std::string key; std::string desc; };
            std::vector<CtrlItem> controls = {
                {"A / D",   "Move paddle"},
                {"ARROWS",  "Move paddle"},
                {"SPACE",   "Launch ball (towards mouse)"},
                {"ESC",     "Pause / Resume"},
                {"1 / 2",   "Change camera"}
            };

            // Align descriptions (consistent key column width)
            float maxKeyW = 0.0f;
            for (const auto& it : controls) {
                maxKeyW = std::max(maxKeyW, ctx.renderer.measureUITextWidth(it.key, badgeScale));
            }
            float badgeW = maxKeyW + 38.0f;
            float descX = x + badgeW + 28.0f;

            for (const auto& it : controls) {
                ui::drawKeyBadge(ctx.renderer, x, y - 18.0f, it.key, badgeScale, ctx.time.now());
                ctx.renderer.drawUIText(descX, y, it.desc, descScale, glm::vec3(0.86f, 0.94f, 1.0f));
                y -= lineGap;
            }
        }

        if (state.instructionsTab == 1) {
            // --- POWERUP INSPECTOR (LET HIM COOK) ---
            struct Entry {
                const char* name;
                const char* desc;
                const engine::Mesh* mesh;
                glm::vec3 tint;
                float scale;
                int videoIdx; // index into GameAssets::powerupVideoFrames
            };

            std::vector<Entry> entries = {
                {"EXPAND",      "Makes the paddle wider for a short time.",              &assets.expand,    glm::vec3(0.35f, 1.0f, 0.35f), 1.05f, 0},
                {"EXTRA BALL",  "Spawns 3 additional balls.",                             &assets.extraBall, glm::vec3(0.35f, 0.85f, 1.0f),  1.00f, 1},
                {"EXTRA LIFE",  "Adds +1 life.",                                          &assets.extraLife, glm::vec3(1.0f, 0.25f, 0.25f),  1.05f, 2},
                {"FIREBALL",    "Turns the ball into a one-shot fireball with AoE.",     &assets.fireball,  glm::vec3(1.0f, 0.55f, 0.10f),  1.05f, 3},
                {"SLOW",        "Slows paddle movement for a short time.",               &assets.slow,      glm::vec3(0.75f, 0.55f, 0.20f), 1.05f, 4},
                {"SHIELD",      "A barrier behind the paddle saves balls temporarily.",  &assets.shield,    glm::vec3(0.25f, 1.0f, 1.0f),  1.10f, 5},
                // Curses (no SKULL powerup in the actual game; SKULL mesh is used as the REVERSE icon)
                {"REVERSE",     "Inverts left/right controls temporarily.",              &assets.skull,     glm::vec3(1.0f, 0.20f, 0.90f), 1.05f, 6},
                {"TINY",        "Shrinks the paddle for a short time.",                  &assets.minus,     glm::vec3(1.0f, 0.95f, 0.25f), 1.00f, 7},
            };

            int idx = state.powerupInspectIndex;
            if (entries.empty()) idx = 0;
            else {
                if (idx < 0) idx = 0;
                if (idx >= (int)entries.size()) idx = (int)entries.size() - 1;
            }
            const Entry& e = entries[idx];

            // Left: model viewport
            {
                const auto& R = OL.modelRect;
                // Model (slow spin + user rotation) — rendered "on the background" (no inner box),
                // clipped to the left half so it feels like the PLAY screen card style.
                float cx = R.x + R.w * 0.5f;
                float cy = R.y + R.h * 0.60f;
                // Make preview models much bigger so they occupy the left half.
                float s = std::min(R.w, R.h) * 0.42f * e.scale;

                float spin = ctx.time.now() * 0.35f;
                float yaw = state.powerupInspectYaw + spin;
                float pitch = state.powerupInspectPitch;

                glm::mat4 M(1.0f);
                M = glm::translate(M, glm::vec3(cx, cy, 0.0f));
                M = glm::rotate(M, yaw, glm::vec3(0, 1, 0));
                M = glm::rotate(M, pitch, glm::vec3(1, 0, 0));
                M = glm::scale(M, glm::vec3(s, s, s));

                // Match in-game "TINY looks like a plank": sculpt minus into a bar (see `WorldRender.cpp`).
                if (std::string(e.name) == "TINY") {
                    M = glm::scale(M, glm::vec3(2.35f, 0.22f, 0.60f));
                }

                // Important: UI pass disables depth test, which makes complex meshes look "inside-out".
                // Enable depth + clip to the model rect just for this draw.
                ctx.renderer.uiSetScissor(true, R.x, R.y, R.w, R.h);
                ctx.renderer.uiSetDepthTest(true, true);
                ctx.renderer.drawMesh(*e.mesh, M, e.tint);
                ctx.renderer.uiSetDepthTest(false, false);
                ctx.renderer.uiSetScissor(false);
            }

            // Right: info widget (name, description, video placeholder, arrows)
            {
                const auto& R = OL.infoRect;
                // Keep this as the "second widget" (readable card).
                ctx.renderer.drawUIQuad(R.x, R.y, R.w, R.h, glm::vec4(0.06f, 0.06f, 0.10f, 0.92f));

                // Header
                float x = R.x + 16.0f * uiS;
                float y = R.y + R.h - 58.0f * uiS;
                float titleScale = 1.55f * uiS;
                ctx.renderer.drawUIText(x, y, e.name, titleScale, glm::vec3(1.0f, 1.0f, 1.0f));
                ctx.renderer.drawUIQuad(x, y - 10.0f * uiS, std::min(R.w - 28.0f * uiS, 340.0f * uiS), 3.0f * uiS, glm::vec4(e.tint, 1.0f));

                // Description (wrapped)
                float descScale = 1.10f * uiS;
                float descY = y - 62.0f * uiS;
                ui::drawWrappedText(ctx.renderer, x, descY, R.w - 28.0f * uiS, e.desc, descScale, glm::vec4(0.86f, 0.94f, 1.0f, 0.92f), 6.0f * uiS);

                // Video placeholder
                const auto& V = OL.videoRect;
                // "Video" preview: if extracted frame textures exist, play them; otherwise show placeholder.
                ctx.renderer.drawUIQuad(V.x, V.y, V.w, V.h, glm::vec4(0.02f, 0.02f, 0.04f, 0.90f));
                float bt = 2.0f;
                ctx.renderer.drawUIQuad(V.x - bt, V.y - bt, V.w + 2*bt, bt, glm::vec4(1,1,1,0.12f));
                ctx.renderer.drawUIQuad(V.x - bt, V.y + V.h, V.w + 2*bt, bt, glm::vec4(1,1,1,0.12f));
                ctx.renderer.drawUIQuad(V.x - bt, V.y, bt, V.h, glm::vec4(1,1,1,0.12f));
                ctx.renderer.drawUIQuad(V.x + V.w, V.y, bt, V.h, glm::vec4(1,1,1,0.12f));

                const auto& gif = assets.powerupVideo(e.videoIdx);
                if (!gif.empty()) {
                    const auto& fr = gif.frameAtTime(ctx.time.now());
                    ctx.renderer.drawUIQuad(V.x, V.y, V.w, V.h, glm::vec4(1,1,1,1), fr.id);
                } else {
                    std::string ph = assets.powerupVideoPaths[e.videoIdx].empty()
                        ? "VIDEO (gif not set)"
                        : "VIDEO (loading...)";
                    float phS = 0.72f * uiS;
                    float phW = ctx.renderer.measureUITextWidth(ph, phS);
                    float phH = ctx.renderer.getUIFontLineHeight(phS);
                    ctx.renderer.drawUIText(V.x + (V.w - phW) * 0.5f, V.y + (V.h - phH) * 0.5f, ph, phS, glm::vec4(1,1,1,0.55f));
                    std::string hint = "assets/video/*.gif";
                    float hs = 0.55f * uiS;
                    float hw = ctx.renderer.measureUITextWidth(hint, hs);
                    ctx.renderer.drawUIText(V.x + (V.w - hw) * 0.5f, V.y + 16.0f * uiS, hint, hs, glm::vec4(1,1,1,0.45f));
                }

                // RGB border on the right widget (requested)
                {
                    float bt2 = 3.0f * uiS;
                    float hue2 = std::fmod(0.56f + 0.08f * std::sin(ctx.time.now() * 1.2f), 1.0f);
                    glm::vec3 neon2 = ui::hsv2rgb(hue2, 0.85f, 1.0f);
                    glm::vec4 b2(neon2, 0.95f);
                    ctx.renderer.drawUIQuad(R.x - bt2, R.y - bt2, R.w + 2*bt2, bt2, b2);
                    ctx.renderer.drawUIQuad(R.x - bt2, R.y + R.h, R.w + 2*bt2, bt2, b2);
                    ctx.renderer.drawUIQuad(R.x - bt2, R.y, bt2, R.h, b2);
                    ctx.renderer.drawUIQuad(R.x + R.w, R.y, bt2, R.h, b2);
                }

                // Nav arrows
                auto drawNav = [&](const game::ui::Rect& B, const std::string& label, bool hovered) {
                    glm::vec3 base(0.10f, 0.10f, 0.16f);
                    glm::vec3 col = base * (hovered ? 1.25f : 1.0f);
                    float sh = hovered ? 6.0f : 3.0f;
                    ctx.renderer.drawUIQuad(B.x + sh, B.y - sh, B.w, B.h, glm::vec4(0,0,0, hovered ? 0.55f : 0.45f));
                    ctx.renderer.drawUIQuad(B.x, B.y, B.w, B.h, glm::vec4(col, 0.95f));
                    float tS = 1.15f * uiS;
                    float tw = ctx.renderer.measureUITextWidth(label, tS);
                    float th = ctx.renderer.getUIFontLineHeight(tS);
                    ctx.renderer.drawUIText(B.x + (B.w - tw) * 0.5f, B.y + (B.h - th) * 0.5f, label, tS, glm::vec4(1,1,1,1));
                };

                drawNav(OL.navLeft, "<", state.hoveredPowerupNav == 0);
                drawNav(OL.navRight, ">", state.hoveredPowerupNav == 1);

                // Index indicator
                std::string ind = std::to_string(idx + 1) + " / " + std::to_string((int)entries.size());
                float indS = 0.70f * uiS;
                float indW = ctx.renderer.measureUITextWidth(ind, indS);
                ctx.renderer.drawUIText(R.x + (R.w - indW) * 0.5f, OL.navLeft.y + 16.0f * uiS, ind, indS, glm::vec4(1,1,1,0.70f));
            }
        }

        if (state.instructionsTab == 2) {
            // --- ROGUE CARDS BROWSER ---
            // Group all cards into Powerups / Modifiers / OP.
            std::vector<game::rogue::RogueCardId> powerups;
            std::vector<game::rogue::RogueCardId> modifiers;
            std::vector<game::rogue::RogueCardId> ops;
            for (auto id : game::rogue::allCardIds()) {
                const auto& def = game::rogue::cardDef(id);
                if (def.isOp) ops.push_back(id);
                else if (game::rogue::isPowerupCard(id)) powerups.push_back(id);
                else modifiers.push_back(id);
            }

            // Content region = union of model+info rects.
            float contentX = OL.modelRect.x;
            float contentY = OL.modelRect.y;
            float contentW = (OL.infoRect.x + OL.infoRect.w) - contentX;
            float contentH = OL.modelRect.h;

            float gap = 22.0f * uiS;
            float colW = (contentW - 2.0f * gap) / 3.0f;
            float colH = contentH;
            float colX0 = contentX;
            float colX1 = contentX + colW + gap;
            float colX2 = contentX + (colW + gap) * 2.0f;

            auto drawColHeader = [&](float x, const char* title, glm::vec3 tint) {
                float s = 1.05f * uiS;
                float tw = ctx.renderer.measureUITextWidth(title, s);
                ctx.renderer.drawUIText(x + (colW - tw) * 0.5f, contentY + colH - 34.0f * uiS, title, s, tint);
            };

            drawColHeader(colX0, "POWERUPS", glm::vec3(0.35f, 0.85f, 1.0f));
            drawColHeader(colX1, "MODIFIERS", glm::vec3(0.85f, 0.35f, 1.0f));
            drawColHeader(colX2, "OP", glm::vec3(1.0f, 0.80f, 0.15f));

            int base0 = 0;
            int base1 = base0 + (int)powerups.size();
            int base2 = base1 + (int)modifiers.size();
            // Scrollable columns: clip each column list and apply independent scroll offsets.
            // IMPORTANT: scissor must include border thickness, otherwise vertical borders get clipped.
            auto drawListScrollable = [&](float x, float scrollPx,
                                         const std::vector<game::rogue::RogueCardId>& ids, int baseIndex) {
                float padX = 10.0f * uiS;
                float itemH = 54.0f * uiS;
                float itemGap = 10.0f * uiS;
                float step = itemH + itemGap;
                float viewTopY = contentY + colH - 80.0f * uiS;
                float viewBottomY = contentY + 30.0f * uiS;
                float viewH = std::max(1.0f, viewTopY - viewBottomY);
                float totalH = std::max(0.0f, (float)ids.size() * step - itemGap);
                float maxScroll = std::max(0.0f, totalH - viewH);
                if (scrollPx < 0.0f) scrollPx = 0.0f;
                if (scrollPx > maxScroll) scrollPx = maxScroll;

                float bt = 3.0f * uiS;
                // Clip to column viewport (expanded by border thickness).
                ctx.renderer.uiSetScissor(true, x - bt, viewBottomY - bt, colW + 2.0f * bt, viewH + 2.0f * bt);

                float y = viewTopY + scrollPx;
                for (size_t i = 0; i < ids.size(); ++i) {
                    // Skip items above the viewport quickly.
                    if ((y - itemH) > (viewTopY + itemH)) { y -= step; continue; }
                    if (y < viewBottomY) break;

                    int idx = baseIndex + (int)i;
                    bool hovered = (state.hoveredRogueCardsItem == idx) && !state.rogueCardsInspectOpen;
                    bool selected = (state.rogueCardsSelected == ids[i]);
                    // Outline colors should match the card itself, but feel "neon".
                    glm::vec3 acc = game::rogue::cardAccent(ids[i]);
                    glm::vec3 neonAcc = glm::clamp(acc * 1.55f + glm::vec3(0.08f), glm::vec3(0.0f), glm::vec3(1.0f));

                    glm::vec4 bg(0.06f, 0.06f, 0.10f, hovered ? 0.96f : 0.88f);
                    ctx.renderer.drawUIQuad(x, y - itemH, colW, itemH, bg);

                    float hue = std::fmod(0.56f + 0.08f * std::sin(ctx.time.now() * 1.2f), 1.0f);
                    glm::vec3 neon = ui::hsv2rgb(hue, 0.85f, 1.0f);
                    // Default outline: subtle accent-colored border (matches the "mode cards" vibe).
                    // Hover/selected: neon RGB highlight.
                    glm::vec4 border = (hovered || selected)
                        ? glm::vec4(neon, 1.0f)
                        : glm::vec4(neonAcc, 0.92f);
                    ctx.renderer.drawUIQuad(x - bt, y - itemH - bt, colW + 2 * bt, bt, border);
                    ctx.renderer.drawUIQuad(x - bt, y, colW + 2 * bt, bt, border);
                    ctx.renderer.drawUIQuad(x - bt, y - itemH, bt, itemH, border);
                    ctx.renderer.drawUIQuad(x + colW, y - itemH, bt, itemH, border);

                    const auto& def = game::rogue::cardDef(ids[i]);
                    std::string nm = def.name;
                    float nameScale = ui::fitScaleToWidth(ctx.renderer, nm, 0.90f * uiS, colW - 2.0f * padX);
                    float th = ctx.renderer.getUIFontLineHeight(nameScale);
                    ctx.renderer.drawUIText(x + padX, (y - itemH) + (itemH - th) * 0.5f, nm, nameScale, glm::vec3(1.0f));

                    y -= step;
                }

                ctx.renderer.uiSetScissor(false);
            };

            drawListScrollable(colX0, state.rogueCardsScrollPowerups, powerups, base0);
            drawListScrollable(colX1, state.rogueCardsScrollModifiers, modifiers, base1);
            drawListScrollable(colX2, state.rogueCardsScrollOp, ops, base2);

            // Modal inspect overlay: dim the world and show the selected card big in the center.
            if (state.rogueCardsInspectOpen) {
                ctx.renderer.drawUIQuad(0, 0, (float)ctx.fbW, (float)ctx.fbH, glm::vec4(0, 0, 0, 0.82f));

                // Big centered card (menu inspector)
                float cardWBase = 420.0f;
                float cardHBase = 900.0f;
                float s = std::min((float)ctx.fbW / (cardWBase + 120.0f), (float)ctx.fbH / (cardHBase + 160.0f));
                s = std::max(0.65f, std::min(1.45f, s));
                float cardW = cardWBase * s;
                float cardH = cardHBase * s;
                float cx = ((float)ctx.fbW - cardW) * 0.5f;
                float cy = ((float)ctx.fbH - cardH) * 0.5f;

                game::ui::Rect r{cx, cy, cardW, cardH};
                auto id = state.rogueCardsSelected;
                const auto& def = game::rogue::cardDef(id);
                glm::vec3 acc = game::rogue::cardAccent(id);

                ctx.renderer.drawUIQuad(r.x + 14.0f, r.y - 14.0f, r.w, r.h, glm::vec4(0, 0, 0, 0.66f));
                ctx.renderer.drawUIQuad(r.x, r.y, r.w, r.h, glm::vec4(0.08f, 0.08f, 0.14f, 0.98f));
                float bt = 5.0f;
                ctx.renderer.drawUIQuad(r.x - bt, r.y - bt, r.w + 2 * bt, bt, glm::vec4(acc, 1.0f));
                ctx.renderer.drawUIQuad(r.x - bt, r.y + r.h, r.w + 2 * bt, bt, glm::vec4(acc, 1.0f));
                ctx.renderer.drawUIQuad(r.x - bt, r.y, bt, r.h, glm::vec4(acc, 1.0f));
                ctx.renderer.drawUIQuad(r.x + r.w, r.y, bt, r.h, glm::vec4(acc, 1.0f));

                std::string nm = def.name;
                float nameScale = ui::fitScaleToWidth(ctx.renderer, nm, 1.85f, r.w - 70.0f);
                float nw = ctx.renderer.measureUITextWidth(nm, nameScale);
                float nh = ctx.renderer.getUIFontLineHeight(nameScale);
                float nx = r.x + (r.w - nw) * 0.5f;
                float ny = r.y + r.h - nh - 34.0f;
                ctx.renderer.drawUIText(nx, ny, nm, nameScale, glm::vec3(1, 1, 1));

                std::string desc = def.shortDesc;
                float pad = 40.0f;
                float dScale = 1.25f;
                ui::drawWrappedText(ctx.renderer, r.x + pad, ny - 150.0f, r.w - 2.0f * pad, desc, dScale,
                                    glm::vec4(0.93f, 0.97f, 1.0f, 0.95f), 12.0f);

                std::string hint = "CLICK OUTSIDE TO CLOSE";
                float hS = 0.62f;
                float hw = ctx.renderer.measureUITextWidth(hint, hS);
                ctx.renderer.drawUIText(r.x + (r.w - hw) * 0.5f, r.y + 26.0f, hint, hS, glm::vec3(0.8f, 0.9f, 1.0f));
            }
        }

        // BACK button (bottom-left; replaces the old X)
        {
            // Match the PLAY_MODES back button style.
            bool backHover = state.hoveredCloseButton;
            float uiS = L.uiScale;
            float bx = OL.backBtn.x, by = OL.backBtn.y, bw = OL.backBtn.w, bh = OL.backBtn.h;

            float sh = (backHover ? 6.0f : 3.0f) * uiS;
            ctx.renderer.drawUIQuad(bx + sh, by - sh, bw, bh, glm::vec4(0,0,0, backHover ? 0.55f : 0.45f));
            glm::vec3 base(0.10f, 0.10f, 0.16f);
            glm::vec3 col = base * (backHover ? 1.25f : 1.0f);
            ctx.renderer.drawUIQuad(bx, by, bw, bh, glm::vec4(col, 0.95f));

            // Neon border (subtle, matches menu vibe)
            float bt = 2.5f * uiS;
            float hue = std::fmod(0.56f + 0.08f * std::sin(ctx.time.now() * 1.2f), 1.0f);
            glm::vec3 neon = ui::hsv2rgb(hue, 0.85f, 1.0f);
            glm::vec4 bcol(neon, backHover ? 1.0f : 0.75f);
            ctx.renderer.drawUIQuad(bx - bt, by - bt, bw + 2*bt, bt, bcol);
            ctx.renderer.drawUIQuad(bx - bt, by + bh, bw + 2*bt, bt, bcol);
            ctx.renderer.drawUIQuad(bx - bt, by, bt, bh, bcol);
            ctx.renderer.drawUIQuad(bx + bw, by, bt, bh, bcol);

            std::string back = "< BACK";
            float bs = (bh / 56.0f) * (0.78f * uiS);
            float tw = ctx.renderer.measureUITextWidth(back, bs);
            float th = ctx.renderer.getUIFontLineHeight(bs);
            float tx = bx + (bw - tw) * 0.5f - 6.0f * uiS;
            float ty = by + (bh - th) * 0.5f;
            ctx.renderer.drawUIText(tx + 1.0f * uiS, ty - 1.0f * uiS, back, bs, glm::vec4(0,0,0,0.55f));
            ctx.renderer.drawUIText(tx, ty, back, bs, glm::vec4(1,1,1,1));
        }
    }

    ctx.renderer.endUI();
}

} // namespace game::render


