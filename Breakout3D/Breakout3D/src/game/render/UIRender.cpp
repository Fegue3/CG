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

static std::vector<std::string> wrapText(engine::Renderer& renderer, const std::string& text, float scale, float maxWidthPx) {
    std::vector<std::string> lines;
    if (maxWidthPx <= 1.0f || text.empty()) return lines;

    std::string cur;
    std::string word;

    auto flushWord = [&]() {
        if (word.empty()) return;
        if (cur.empty()) {
            cur = word;
        } else {
            std::string trial = cur + " " + word;
            if (renderer.measureUITextWidth(trial, scale) <= maxWidthPx) {
                cur = trial;
            } else {
                lines.push_back(cur);
                cur = word;
            }
        }
        word.clear();
    };

    for (char ch : text) {
        if (ch == '\n') {
            flushWord();
            if (!cur.empty()) lines.push_back(cur);
            cur.clear();
            continue;
        }
        if (ch == ' ' || ch == '\t') {
            flushWord();
            continue;
        }
        word.push_back(ch);
    }

    flushWord();
    if (!cur.empty()) lines.push_back(cur);
    return lines;
}

static void drawWrappedText(engine::Renderer& renderer,
                            float x, float yTop,
                            float maxWidthPx,
                            const std::string& text,
                            float scale,
                            const glm::vec4& color,
                            float lineGapPx) {
    auto lines = wrapText(renderer, text, scale, maxWidthPx);
    float lh = renderer.getUIFontLineHeight(scale);
    float y = yTop;
    for (const auto& ln : lines) {
        renderer.drawUIText(x, y, ln, scale, color);
        y -= (lh + lineGapPx);
    }
}

static glm::vec3 hsv2rgb(float h, float s, float v) {
    h = std::fmod(h, 1.0f); if (h < 0.0f) h += 1.0f;
    float c = v * s;
    float x = c * (1.0f - std::fabs(std::fmod(h * 6.0f, 2.0f) - 1.0f));
    float m = v - c;
    glm::vec3 rgb;
    if      (h < 1.0f/6.0f) rgb = {c, x, 0};
    else if (h < 2.0f/6.0f) rgb = {x, c, 0};
    else if (h < 3.0f/6.0f) rgb = {0, c, x};
    else if (h < 4.0f/6.0f) rgb = {0, x, c};
    else if (h < 5.0f/6.0f) rgb = {x, 0, c};
    else                    rgb = {c, 0, x};
    return rgb + glm::vec3(m);
}

// Helper to render a neon-styled key badge
static void drawKeyBadge(engine::Renderer& renderer, float x, float y, const std::string& label, float scale, float time) {
    float textW = renderer.measureUITextWidth(label, scale);
    float textH = renderer.getUIFontLineHeight(scale);
    float padX = 12.0f;
    float padY = 8.0f;
    float w = textW + padX * 2.0f;
    float h = textH + padY * 2.0f;

    renderer.drawUIQuad(x, y, w, h, glm::vec4(0.08f, 0.08f, 0.14f, 0.95f));

    float hue = std::fmod(0.56f + 0.08f * std::sin(time * 1.2f), 1.0f);
    glm::vec3 neon = hsv2rgb(hue, 0.85f, 1.0f);
    glm::vec4 border(neon, 1.0f);
    float bt = 2.0f;
    renderer.drawUIQuad(x, y, w, bt, border);
    renderer.drawUIQuad(x, y + h - bt, w, bt, border);
    renderer.drawUIQuad(x, y, bt, h, border);
    renderer.drawUIQuad(x + w - bt, y, bt, h, border);

    float tx = x + (w - textW) * 0.5f;
    float ty = y + (h - textH) * 0.5f;
    renderer.drawUIText(tx, ty, label, scale, glm::vec3(0.95f, 0.98f, 1.0f));
}

// Helper function to draw a button with hover effects (used in overlays)
static void drawOverlayButton(engine::Renderer& renderer, float bx, float by, float bw, float bh,
                               const std::string& label, const glm::vec3& baseColor, bool hovered) {
    float hoverBrightness = hovered ? 1.3f : 1.0f;
    
    // Button shadow (larger on hover)
    float btnShadowOffset = hovered ? 5.0f : 3.0f;
    renderer.drawUIQuad(bx + btnShadowOffset, by - btnShadowOffset, 
                       bw, bh, glm::vec4(0.0f, 0.0f, 0.0f, hovered ? 0.6f : 0.5f));
    
    // Button background (gradient effect with two quads)
    glm::vec3 color = baseColor * hoverBrightness;
    renderer.drawUIQuad(bx, by, bw, bh, 
                       glm::vec4(color.r, color.g, color.b, 1.0f));
    renderer.drawUIQuad(bx, by + bh * 0.5f, bw, bh * 0.5f, 
                       glm::vec4(color.r * 0.7f, color.g * 0.7f, color.b * 0.7f, 0.3f));
    
    // Button border (brighter and thicker on hover)
    float btnBorderThickness = hovered ? 3.0f : 2.0f;
    glm::vec4 btnBorder = hovered ? glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) : glm::vec4(0.0f, 0.0f, 0.0f, 0.4f);
    renderer.drawUIQuad(bx, by, bw, btnBorderThickness, btnBorder);
    renderer.drawUIQuad(bx, by + bh - btnBorderThickness, bw, btnBorderThickness, btnBorder);
    renderer.drawUIQuad(bx, by, btnBorderThickness, bh, btnBorder);
    renderer.drawUIQuad(bx + bw - btnBorderThickness, by, btnBorderThickness, bh, btnBorder);
    
    // Button label
    float labelScale = 1.0f;
    float labelW = renderer.measureUITextWidth(label, labelScale);
    float labelH = renderer.getUIFontLineHeight(labelScale);
    float labelX = bx + (bw - labelW) * 0.5f;
    float labelY = by + (bh - labelH) * 0.5f;
    
    // Label shadow (stronger on hover)
    float shadowStrength = hovered ? 0.8f : 0.6f;
    renderer.drawUIText(labelX + 1.0f, labelY - 1.0f, label, labelScale, glm::vec3(0, 0, 0) * shadowStrength);
    renderer.drawUIText(labelX, labelY, label, labelScale, glm::vec3(1, 1, 1));
}

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
        glm::vec3 col = hsv2rgb(hue, 0.85f, 1.0f);
        ctx.renderer.drawUIText(cx, cy, ch, titleScale, col);
    }

    // Removed moving highlight bar to simplify title styling

    // Background panel (MAIN/OPTIONS screens only). PLAY_MODES uses one card per mode instead.
    if (state.currentMenuScreen != MenuScreen::PLAY_MODES) {
        float shadowOffset = 6.0f * uiS;
        ctx.renderer.drawUIQuad(panelX + shadowOffset, panelY - shadowOffset, panelW, panelH, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
        ctx.renderer.drawUIQuad(panelX, panelY, panelW, panelH, glm::vec4(0.08f, 0.08f, 0.14f, 0.98f));

        // Animated border with neon RGB (matching title style)
        float borderThickness = 3.0f * uiS;
        float tRgb = ctx.time.now();
        float hueRgb = std::fmod(0.56f + 0.08f * std::sin(tRgb * 1.2f), 1.0f);
        glm::vec3 neonRgb = hsv2rgb(hueRgb, 0.85f, 1.0f);
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
            glm::vec3 neonRgb = hsv2rgb(hueRgb, 0.85f, 1.0f);
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
            drawWrappedText(ctx.renderer, cardX + padX, dTop, maxW, desc, dScale, dCol, 7.0f * uiS);

            // Features (bullets)
            float lhDesc = ctx.renderer.getUIFontLineHeight(dScale);
            auto dLines = wrapText(ctx.renderer, desc, dScale, maxW);
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
            // (use scaled card space, but keep original play button position stable)
            const float minY = c.playBtn.y + c.playBtn.h + 22.0f * uiS;

            for (const auto& ft : features) {
                if (yCursor < minY) break;
                // dot (simple square bullet)
                ctx.renderer.drawUIQuad(cardX + padX, yCursor + lh * 0.35f, dot, dot, dotCol);
                float textX = cardX + padX + dot + bulletGap;
                float textW = maxW - (dot + bulletGap);
                drawWrappedText(ctx.renderer, textX, yCursor, textW, ft, fScale, fCol, lineGap);
                auto fl = wrapText(ctx.renderer, ft, fScale, textW);
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
        glm::vec3 neon = hsv2rgb(hue, 0.85f, 1.0f);
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

    // Show instructions if toggled
    if (state.showInstructions) {
        // Instructions panel (larger to fit powerup descriptions)
        float instrW = 820.0f;
        float instrH = 620.0f;
        float instrX = (ctx.fbW - instrW) * 0.5f;
        float instrY = (ctx.fbH - instrH) * 0.5f;

        ctx.renderer.drawUIQuad(instrX, instrY, instrW, instrH, glm::vec4(0.05f, 0.05f, 0.1f, 0.98f));

        // Instructions border — neon RGB (matching title style)
        float borderThickness = 3.0f;
        float tRgbInstr = ctx.time.now();
        float hueRgbInstr = std::fmod(0.56f + 0.08f * std::sin(tRgbInstr * 1.2f), 1.0f);
        glm::vec3 neonRgbInstr = hsv2rgb(hueRgbInstr, 0.85f, 1.0f);
        glm::vec4 instrBorder(neonRgbInstr, 1.0f);
        ctx.renderer.drawUIQuad(instrX - borderThickness, instrY - borderThickness, instrW + 2*borderThickness, borderThickness, instrBorder);
        ctx.renderer.drawUIQuad(instrX - borderThickness, instrY + instrH, instrW + 2*borderThickness, borderThickness, instrBorder);
        ctx.renderer.drawUIQuad(instrX - borderThickness, instrY, borderThickness, instrH, instrBorder);
        ctx.renderer.drawUIQuad(instrX + instrW, instrY, borderThickness, instrH, instrBorder);

        // Instructions title with glow + underline
        std::string instrTitle = "HOW TO PLAY";
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
        glm::vec3 neonBar = hsv2rgb(hueBar, 0.85f, 1.0f);
        ctx.renderer.drawUIQuad(instrTitleX, instrTitleY - 6.0f, barW, barH, glm::vec4(neonBar, 1.0f));

        // Basic Controls section
        float textY = instrY + instrH - 100.0f;
        float textX = instrX + 24.0f;
        float lineGap = 28.0f;
        float sectionGap = 16.0f;

        std::string ctrlTitle = "Controls";
        ctx.renderer.drawUIText(textX, textY, ctrlTitle, 0.85f, glm::vec3(1.0f, 0.95f, 0.70f));
        ctx.renderer.drawUIQuad(textX, textY - 6.0f, 140.0f, 3.0f, glm::vec4(1.0f, 0.95f, 0.70f, 1.0f));
        textY -= lineGap;

        struct CtrlItem { std::string key; std::string desc; };
        std::vector<CtrlItem> controls = {
            {"A / D", "Move paddle"},
            {"ARROWS", "Move paddle"},
            {"SPACE", "Launch ball"},
            {"ESC", "Pause / Resume"},
            {"1 / 2", "Change camera"}
        };
        for (const auto& item : controls) {
            float badgeScale = 0.65f;
            float badgeW = ctx.renderer.measureUITextWidth(item.key, badgeScale) + 24.0f;
            float badgeH = ctx.renderer.getUIFontLineHeight(badgeScale) + 16.0f;
            drawKeyBadge(ctx.renderer, textX, textY - badgeH * 0.3f, item.key, badgeScale, ctx.time.now());
            float descX = textX + badgeW + 18.0f;
            ctx.renderer.drawUIText(descX, textY, item.desc, 0.65f, glm::vec3(0.8f, 0.9f, 1.0f));
            textY -= lineGap;
        }

        textY -= sectionGap;

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

        // Close button (X) in top-right corner of panel
        float closeSize = 44.0f;
        float closeX = instrX + instrW - closeSize - 10.0f;
        float closeY = instrY + instrH - closeSize - 6.0f;
        
        // Apply hover effects
        bool hovered = state.hoveredCloseButton;
        float hoverBrightness = hovered ? 1.3f : 1.0f;
        glm::vec3 closeColor(0.8f, 0.2f, 0.2f);
        closeColor *= hoverBrightness;

        // Shadow (larger on hover)
        float shadowOffset = hovered ? 4.0f : 2.0f;
        ctx.renderer.drawUIQuad(closeX + shadowOffset, closeY - shadowOffset, closeSize, closeSize, 
                               glm::vec4(0.0f, 0.0f, 0.0f, hovered ? 0.6f : 0.5f));

        // Button background with gradient
        ctx.renderer.drawUIQuad(closeX, closeY, closeSize, closeSize, 
                               glm::vec4(closeColor.r, closeColor.g, closeColor.b, 0.95f));
        ctx.renderer.drawUIQuad(closeX, closeY + closeSize * 0.5f, closeSize, closeSize * 0.5f, 
                               glm::vec4(closeColor.r * 0.7f, closeColor.g * 0.7f, closeColor.b * 0.7f, 0.3f));

        // Border (brighter and thicker on hover)
        float borderThick = hovered ? 2.5f : 1.5f;
        glm::vec4 borderCol = hovered ? glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) : glm::vec4(0.0f, 0.0f, 0.0f, 0.3f);
        ctx.renderer.drawUIQuad(closeX, closeY, closeSize, borderThick, borderCol);
        ctx.renderer.drawUIQuad(closeX, closeY + closeSize - borderThick, closeSize, borderThick, borderCol);
        ctx.renderer.drawUIQuad(closeX, closeY, borderThick, closeSize, borderCol);
        ctx.renderer.drawUIQuad(closeX + closeSize - borderThick, closeY, borderThick, closeSize, borderCol);
        
        // Draw white X symbol
        std::string closeSymbol = "X";
        float closeSymScale = 1.8f;
        float closeSymW = ctx.renderer.measureUITextWidth(closeSymbol, closeSymScale);
        float closeSymH = ctx.renderer.getUIFontLineHeight(closeSymScale);
        float closeSymX = closeX + (closeSize - closeSymW) * 0.5f;
        float closeSymY = closeY + (closeSize - closeSymH) * 0.5f;

        // Shadow on text (stronger on hover)
        float textShadowStrength = hovered ? 0.8f : 0.6f;
        ctx.renderer.drawUIText(closeSymX + 1.0f, closeSymY - 1.0f, closeSymbol, closeSymScale, 
                               glm::vec3(0, 0, 0) * textShadowStrength);
        ctx.renderer.drawUIText(closeSymX, closeSymY, closeSymbol, closeSymScale, glm::vec3(1.0f, 1.0f, 1.0f));
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

            // Neon RGB border (matching title style)
            float bt2 = 3.0f;
            float hue2 = std::fmod(0.56f + 0.08f * std::sin(ctx.time.now() * 1.2f), 1.0f);
            glm::vec3 neon2 = hsv2rgb(hue2, 0.85f, 1.0f);
            glm::vec4 border2(neon2, 1.0f);
            ctx.renderer.drawUIQuad(panelX2 - bt2, panelY2 - bt2, panelW2 + 2*bt2, bt2, border2);
            ctx.renderer.drawUIQuad(panelX2 - bt2, panelY2 + panelH2, panelW2 + 2*bt2, bt2, border2);
            ctx.renderer.drawUIQuad(panelX2 - bt2, panelY2, bt2, panelH2, border2);
            ctx.renderer.drawUIQuad(panelX2 + panelW2, panelY2, bt2, panelH2, border2);

            // PAUSED text at the top
            std::string msg = "PAUSED";
            float scale = fitScaleToWidth(ctx.renderer, msg, 5.5f, panelW2 - 60.0f);
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
                glm::vec3 colP = hsv2rgb(hueP, 0.85f, 1.0f);
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

            drawOverlayButton(ctx.renderer, btnX_left, btnY, btnW, btnH, leftLabel, 
                            glm::vec3(0.8f, 0.2f, 0.2f), state.hoveredOverlayButton == 0);
            drawOverlayButton(ctx.renderer, btnX_right, btnY, btnW, btnH, rightLabel, 
                            glm::vec3(0.2f, 0.8f, 0.2f), state.hoveredOverlayButton == 1);
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

            // Neon RGB border (matching menu/instructions)
            float bt = 3.0f;
            float hue = std::fmod(0.56f + 0.08f * std::sin(ctx.time.now() * 1.2f), 1.0f);
            glm::vec3 neon = hsv2rgb(hue, 0.85f, 1.0f);
            glm::vec4 border(neon, 1.0f);
            ctx.renderer.drawUIQuad(panelX - bt, panelY - bt, panelW + 2*bt, bt, border);
            ctx.renderer.drawUIQuad(panelX - bt, panelY + panelH, panelW + 2*bt, bt, border);
            ctx.renderer.drawUIQuad(panelX - bt, panelY, bt, panelH, border);
            ctx.renderer.drawUIQuad(panelX + panelW, panelY, bt, panelH, border);

            std::string title = (state.mode == GameMode::GAME_OVER) ? "GAME OVER" : "WINNER!";
            float titleScale = fitScaleToWidth(ctx.renderer, title, 5.8f, panelW - 60.0f);
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
                glm::vec3 colE = hsv2rgb(hueE, 0.85f, 1.0f);
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

            drawOverlayButton(ctx.renderer, btnX_left, btnY, btnW, btnH, leftLabel, 
                            glm::vec3(0.8f, 0.2f, 0.2f), state.hoveredOverlayButton == 0);
            drawOverlayButton(ctx.renderer, btnX_right, btnY, btnW, btnH, rightLabel, 
                            glm::vec3(0.2f, 0.8f, 0.2f), state.hoveredOverlayButton == 1);
        }
    }

    ctx.renderer.endUI();
}

} // namespace game::render


