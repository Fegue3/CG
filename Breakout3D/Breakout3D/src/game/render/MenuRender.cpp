#include "game/render/UIRender.hpp"

#include "game/GameAssets.hpp"
#include "game/GameState.hpp"
#include "game/render/ui/UIHelpers.hpp"
#include "game/ui/OverlayLayout.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

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

    // Background panel (MAIN/OPTIONS/INSTRUCTIONS screens only). PLAY_MODES uses one card per mode instead.
    if (state.currentMenuScreen != MenuScreen::PLAY_MODES) {
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

    // Draw buttons based on current menu screen
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
        auto drawModeCard = [&](int /*idx*/,
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
            {"Draft upgrades after milestones.", "Synergies, trade-offs, and run variety.", "Coming soon."},
            glm::vec3(0.75f, 0.25f, 0.85f), false, false);

        if (hoveredIdx != 3) drawModeCard(3, L.levels, "LEVELS",
            "A curated set of stages with layouts, objectives, and difficulty ramps.",
            {"Hand-made stages and goals.", "Progression and challenge levels.", "Coming soon."},
            glm::vec3(0.20f, 0.75f, 0.85f), false, false);

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
            {"Draft upgrades after milestones.", "Synergies, trade-offs, and run variety.", "Coming soon."},
            glm::vec3(0.75f, 0.25f, 0.85f), false, true);

        if (hoveredIdx == 3) drawModeCard(3, L.levels, "LEVELS",
            "A curated set of stages with layouts, objectives, and difficulty ramps.",
            {"Hand-made stages and goals.", "Progression and challenge levels.", "Coming soon."},
            glm::vec3(0.20f, 0.75f, 0.85f), false, true);
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
        // Instructions screen: CONTROLS, POWERUPS, BACK (buttons open the detailed overlay panel)
        drawButton(0, btnX, btn1Y - 50.0f, btnW, btnH, "CONTROLS", glm::vec3(0.3f, 0.6f, 0.7f), "View");
        drawButton(1, btnX, btn2Y - 50.0f, btnW, btnH, "POWERUPS", glm::vec3(0.6f, 0.3f, 0.7f), "View");

        // BACK button
        float backW = 120.0f * uiS;
        float backH = 50.0f * uiS;
        float backX = panelX + 20.0f * uiS;
        float backY = panelY + 15.0f * uiS;
        drawButton(2, backX, backY, backW, backH, "< BACK", glm::vec3(0.5f, 0.5f, 0.5f), "");
    }

    // Small clickable "4" badge for the one-brick test mode (only on MAIN menu, bottom-right corner)
    if (state.currentMenuScreen == MenuScreen::MAIN) {
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
        // Instructions overlay (keep similar "menu panel" ratio; not fullscreen)
        float instrW = std::min(980.0f, (float)ctx.fbW * 0.75f);
        float instrH = std::min(690.0f, (float)ctx.fbH * 0.72f);
        float instrX = ((float)ctx.fbW - instrW) * 0.5f;
        float instrY = std::max(40.0f, ((float)ctx.fbH - instrH) * 0.5f - 60.0f);

        // Keep the overlay BELOW the big BREAKOUT title (match OverlayLayout.cpp spacing).
        const float titlePanelGap = 26.0f;
        float maxTopY = L.titleY - titlePanelGap; // y-up
        if (instrY + instrH > maxTopY) {
            instrY = maxTopY - instrH;
            instrY = std::max(40.0f, instrY);
        }

        ctx.renderer.drawUIQuad(instrX, instrY, instrW, instrH, glm::vec4(0.05f, 0.05f, 0.1f, 0.98f));

        // Instructions border — neon RGB (matching title style)
        float borderThickness = 3.0f;
        float tRgbInstr = ctx.time.now();
        float hueRgbInstr = std::fmod(0.56f + 0.08f * std::sin(tRgbInstr * 1.2f), 1.0f);
        glm::vec3 neonRgbInstr = ui::hsv2rgb(hueRgbInstr, 0.85f, 1.0f);
        glm::vec4 instrBorder(neonRgbInstr, 1.0f);
        ctx.renderer.drawUIQuad(instrX - borderThickness, instrY - borderThickness, instrW + 2*borderThickness, borderThickness, instrBorder);
        ctx.renderer.drawUIQuad(instrX - borderThickness, instrY + instrH, instrW + 2*borderThickness, borderThickness, instrBorder);
        ctx.renderer.drawUIQuad(instrX - borderThickness, instrY, borderThickness, instrH, instrBorder);
        ctx.renderer.drawUIQuad(instrX + instrW, instrY, borderThickness, instrH, instrBorder);

        // Title with glow + underline (tab-aware)
        std::string instrTitle = (state.instructionsTab == 0) ? "CONTROLS" : "POWERUPS";
        float instrTitleScale = 1.6f;
        float instrTitleW = ctx.renderer.measureUITextWidth(instrTitle, instrTitleScale);
        float instrTitleX = instrX + (instrW - instrTitleW) * 0.5f;
        float instrTitleY = instrY + instrH - 42.0f;
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
        float barH = 4.0f;
        float hueBar = std::fmod(0.56f + 0.08f * std::sin(ctx.time.now() * 1.2f), 1.0f);
        glm::vec3 neonBar = ui::hsv2rgb(hueBar, 0.85f, 1.0f);
        ctx.renderer.drawUIQuad(instrTitleX, instrTitleY - 6.0f, barW, barH, glm::vec4(neonBar, 1.0f));

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
                {"SPACE",   "Launch ball"},
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
            float textY = instrY + instrH - 100.0f;
            float textX = instrX + 24.0f;
            float lineGap = 28.0f;
            float sectionGap = 16.0f;

            // Power-ups section
            std::string puTitle = "Power-ups — Good";
            ctx.renderer.drawUIText(textX, textY, puTitle, 0.85f, glm::vec3(0.35f, 1.0f, 0.35f));
            ctx.renderer.drawUIQuad(textX, textY - 6.0f, 180.0f, 3.0f, glm::vec4(0.35f, 1.0f, 0.35f, 1.0f));
            textY -= lineGap;

            std::vector<std::string> goodPowerups = {
                "Green (EXPAND): Makes paddle wider for 7 seconds",
                "Blue (EXTRA BALL): Spawns 3 additional balls",
                "Red (EXTRA LIFE): Adds +1 life",
                "Orange (FIREBALL): Turn ball into one-shot fireball with AoE"
            };

            for (const auto& line : goodPowerups) {
                ctx.renderer.drawUIQuad(textX, textY + 4.0f, 8.0f, 8.0f, glm::vec4(0.35f, 1.0f, 0.35f, 1.0f));
                ctx.renderer.drawUIText(textX + 14.0f, textY, line, 0.62f, glm::vec3(0.8f, 1.0f, 0.85f));
                textY -= lineGap;
            }

            textY -= sectionGap;

            // Curses section
            std::string cursTitle = "Power-ups — Bad";
            ctx.renderer.drawUIText(textX, textY, cursTitle, 0.85f, glm::vec3(1.0f, 0.20f, 0.20f));
            ctx.renderer.drawUIQuad(textX, textY - 6.0f, 180.0f, 3.0f, glm::vec4(1.0f, 0.20f, 0.20f, 1.0f));
            textY -= lineGap;

            std::vector<std::string> curses = {
                "Brown (SLOW): Slows paddle movement for 7 seconds",
                "Magenta (REVERSE): Inverts left/right controls for ~4s",
                "Yellow (TINY): Shrinks paddle for 6 seconds",
                "Cyan (SHIELD): Barrier behind paddle saves balls for 6.5s"
            };

            for (const auto& line : curses) {
                ctx.renderer.drawUIQuad(textX, textY + 4.0f, 8.0f, 8.0f, glm::vec4(1.0f, 0.35f, 0.35f, 1.0f));
                ctx.renderer.drawUIText(textX + 14.0f, textY, line, 0.62f, glm::vec3(1.0f, 0.9f, 0.9f));
                textY -= lineGap;
            }
        }

        // BACK button (bottom-left; replaces the old X)
        {
            bool hovered = state.hoveredCloseButton;
            float backW = 150.0f;
            float backH = 56.0f;
            float backX = instrX + 22.0f;
            float backY = instrY + 16.0f;

            float s = hovered ? 1.06f : 1.0f;
            float cx = backX + backW * 0.5f;
            float cy = backY + backH * 0.5f;
            float w = backW * s;
            float h = backH * s;
            float x = cx - w * 0.5f;
            float y = cy - h * 0.5f;

            float sh = hovered ? 6.0f : 3.0f;
            ctx.renderer.drawUIQuad(x + sh, y - sh, w, h, glm::vec4(0,0,0, hovered ? 0.60f : 0.50f));
            glm::vec3 base(0.10f, 0.10f, 0.16f);
            glm::vec3 col = base * (hovered ? 1.25f : 1.0f);
            ctx.renderer.drawUIQuad(x, y, w, h, glm::vec4(col, 0.95f));

            float bt = 2.5f;
            float hue = std::fmod(0.56f + 0.08f * std::sin(ctx.time.now() * 1.2f), 1.0f);
            glm::vec3 neon = ui::hsv2rgb(hue, 0.85f, 1.0f);
            glm::vec4 bcol(neon, hovered ? 1.0f : 0.80f);
            ctx.renderer.drawUIQuad(x - bt, y - bt, w + 2*bt, bt, bcol);
            ctx.renderer.drawUIQuad(x - bt, y + h,  w + 2*bt, bt, bcol);
            ctx.renderer.drawUIQuad(x - bt, y, bt, h, bcol);
            ctx.renderer.drawUIQuad(x + w,  y, bt, h, bcol);

            std::string back = "< BACK";
            float scale = 0.86f;
            float tw = ctx.renderer.measureUITextWidth(back, scale);
            float th = ctx.renderer.getUIFontLineHeight(scale);
            float tx = x + (w - tw) * 0.5f - 6.0f;
            float ty = y + (h - th) * 0.5f;
            ctx.renderer.drawUIText(tx + 1.0f, ty - 1.0f, back, scale, glm::vec4(0,0,0,0.55f));
            ctx.renderer.drawUIText(tx, ty, back, scale, glm::vec4(1,1,1,1));
        }
    }

    ctx.renderer.endUI();
}

} // namespace game::render


