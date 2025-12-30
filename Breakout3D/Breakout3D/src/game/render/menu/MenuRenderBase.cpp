#include "game/render/menu/MenuRenderParts.hpp"

#include "game/GameAssets.hpp"
#include "game/render/ui/UIHelpers.hpp"

#include <algorithm>
#include <cmath>

namespace game::render::menu {

MenuCtx makeMenuCtx(const RenderContext& ctx, const GameState& state, const GameAssets& assets) {
    MenuCtx m{ctx, state, assets, state.menuLayout};
    m.uiS = m.L.uiScale;

    m.panelW = m.L.panelW;
    m.panelH = m.L.panelH;
    m.panelX = m.L.panelX;
    m.panelY = m.L.panelY;

    m.btnW = m.L.btn1.w;
    m.btnH = m.L.btn1.h;
    m.btnX = m.L.btn1.x;
    m.btn1Y = m.L.btn1.y;
    m.btn2Y = m.L.btn2.y;
    m.btn3Y = m.L.btn3.y;
    m.btn4Y = m.L.btn4.y;

    return m;
}

void drawRetroBackground(const MenuCtx& m) {
    // === RETRO ANIMATED BACKGROUND ===
    // Background texture with slow scrolling effect
    float scrollSpeed = 0.05f;
    float scrollY = std::fmod(m.ctx.time.now() * scrollSpeed * 100.0f, (float)m.ctx.fbH);

    // Draw background twice for seamless scrolling
    m.ctx.renderer.drawUIQuad(0, scrollY, (float)m.ctx.fbW, (float)m.ctx.fbH, glm::vec4(1,1,1,0.15f), m.assets.backgroundTexs[0].id);
    m.ctx.renderer.drawUIQuad(0, scrollY - m.ctx.fbH, (float)m.ctx.fbW, (float)m.ctx.fbH, glm::vec4(1,1,1,0.15f), m.assets.backgroundTexs[0].id);

    // Dark overlay to make buttons pop
    m.ctx.renderer.drawUIQuad(0, 0, (float)m.ctx.fbW, (float)m.ctx.fbH, glm::vec4(0.0f, 0.0f, 0.0f, 0.7f));

    // Retro scan lines effect (horizontal lines)
    float scanLineSpacing = 4.0f;
    float scanLineAlpha = 0.08f;
    for (float y = 0; y < m.ctx.fbH; y += scanLineSpacing * 2) {
        m.ctx.renderer.drawUIQuad(0, y, (float)m.ctx.fbW, scanLineSpacing, glm::vec4(0, 0, 0, scanLineAlpha));
    }

    // Vignette effect (darker edges)
    float vignetteSize = m.ctx.fbW * 0.3f;
    m.ctx.renderer.drawUIQuad(0, 0, (float)m.ctx.fbW, vignetteSize,
                           glm::vec4(0.0f, 0.0f, 0.0f, 0.5f)); // Top
    m.ctx.renderer.drawUIQuad(0, m.ctx.fbH - vignetteSize, (float)m.ctx.fbW, vignetteSize,
                           glm::vec4(0.0f, 0.0f, 0.0f, 0.5f)); // Bottom

    // Floating particles/stars for retro atmosphere
    int numParticles = 100;
    for (int i = 0; i < numParticles; i++) {
        float seed = (float)i * 123.456f;
        float x = std::fmod(seed * 0.314f, 1.0f) * m.ctx.fbW;
        float speed = 15.0f + std::fmod(seed * 0.271f, 1.0f) * 25.0f;
        float yOffset = std::fmod(m.ctx.time.now() * speed + seed * 100.0f, (float)m.ctx.fbH + 100.0f) - 50.0f;
        float y = m.ctx.fbH - yOffset;

        float size = 2.0f + std::fmod(seed * 0.421f, 1.0f) * 3.0f;
        float pulse = 0.5f + 0.5f * std::sin(m.ctx.time.now() * 2.0f + seed);
        float alpha = 0.3f + 0.4f * pulse;

        glm::vec3 starColor(0.2f + 0.8f * pulse, 0.6f + 0.4f * pulse, 1.0f);
        m.ctx.renderer.drawUIQuad(x, y, size, size, glm::vec4(starColor * alpha, alpha));
    }
}

void drawTitle(const MenuCtx& m) {
    std::string title = "BREAKOUT 3D";
    float titleScale = m.L.titleScale;
    float titleX = m.L.titleX;
    float titleY = m.L.titleY;

    // Enhanced title: glow + per-letter neon gradient + outline
    glm::vec3 glowColor(0.10f, 0.35f, 0.90f);
    for (float offset = 3.0f; offset >= 1.0f; offset -= 0.5f) {
        float alpha = 0.18f / offset;
        m.ctx.renderer.drawUIText(titleX - offset, titleY, title, titleScale, glowColor * alpha);
        m.ctx.renderer.drawUIText(titleX + offset, titleY, title, titleScale, glowColor * alpha);
        m.ctx.renderer.drawUIText(titleX, titleY - offset, title, titleScale, glowColor * alpha);
        m.ctx.renderer.drawUIText(titleX, titleY + offset, title, titleScale, glowColor * alpha);
    }

    // Per-letter gradient with subtle animation
    int n = (int)title.size();
    for (int i = 0; i < n; ++i) {
        float prefixW = m.ctx.renderer.measureUITextWidth(title.substr(0, i), titleScale);
        float cx = titleX + prefixW;
        float cy = titleY;
        std::string ch = title.substr(i, 1);

        // Outline (thin) for readability
        glm::vec3 outlineCol(0.02f, 0.02f, 0.06f);
        m.ctx.renderer.drawUIText(cx - 2.0f, cy, ch, titleScale, outlineCol);
        m.ctx.renderer.drawUIText(cx + 2.0f, cy, ch, titleScale, outlineCol);
        m.ctx.renderer.drawUIText(cx, cy - 2.0f, ch, titleScale, outlineCol);
        m.ctx.renderer.drawUIText(cx, cy + 2.0f, ch, titleScale, outlineCol);

        // Neon gradient color across the word with time pulse
        float t = m.ctx.time.now();
        float hue = std::fmod(0.56f + (float)i / (float)std::max(1, n-1) * 0.35f + 0.08f * std::sin(t * 1.2f + i * 0.3f), 1.0f);
        glm::vec3 col = ui::hsv2rgb(hue, 0.85f, 1.0f);
        m.ctx.renderer.drawUIText(cx, cy, ch, titleScale, col);
    }
}

void drawMainPanelIfNeeded(const MenuCtx& m) {
    // Background panel (MAIN/OPTIONS/INSTRUCTIONS screens only). PLAY_MODES and LEVEL_SELECT use custom panels.
    // If an Instructions overlay is open, we don't want the underlying screen panel to show through.
    if (m.state.showInstructions) return;
    if (m.state.currentMenuScreen == MenuScreen::PLAY_MODES) return;
    if (m.state.currentMenuScreen == MenuScreen::LEVEL_SELECT) return;

    float shadowOffset = 6.0f * m.uiS;
    m.ctx.renderer.drawUIQuad(m.panelX + shadowOffset, m.panelY - shadowOffset, m.panelW, m.panelH, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
    m.ctx.renderer.drawUIQuad(m.panelX, m.panelY, m.panelW, m.panelH, glm::vec4(0.08f, 0.08f, 0.14f, 0.98f));

    // Animated border with neon RGB (matching title style)
    float borderThickness = 3.0f * m.uiS;
    float tRgb = m.ctx.time.now();
    float hueRgb = std::fmod(0.56f + 0.08f * std::sin(tRgb * 1.2f), 1.0f);
    glm::vec3 neonRgb = ui::hsv2rgb(hueRgb, 0.85f, 1.0f);
    glm::vec4 borderColor(neonRgb, 1.0f);
    m.ctx.renderer.drawUIQuad(m.panelX - borderThickness, m.panelY - borderThickness, m.panelW + 2*borderThickness, borderThickness, borderColor);
    m.ctx.renderer.drawUIQuad(m.panelX - borderThickness, m.panelY + m.panelH, m.panelW + 2*borderThickness, borderThickness, borderColor);
    m.ctx.renderer.drawUIQuad(m.panelX - borderThickness, m.panelY, borderThickness, m.panelH, borderColor);
    m.ctx.renderer.drawUIQuad(m.panelX + m.panelW, m.panelY, borderThickness, m.panelH, borderColor);
}

void drawButton(
    const MenuCtx& m,
    int btnIndex,
    float bx, float by, float bw, float bh,
    const std::string& label,
    const glm::vec3& baseColor,
    const std::string& subtitle
) {
    bool hovered = (m.state.hoveredMenuButton == btnIndex);
    float hoverBrightness = hovered ? 1.3f : 1.0f;

    // Button shadow (larger on hover)
    float btnShadowOffset = (hovered ? 5.0f : 3.0f) * m.uiS;
    m.ctx.renderer.drawUIQuad(bx + btnShadowOffset, by - btnShadowOffset,
                           bw, bh, glm::vec4(0.0f, 0.0f, 0.0f, hovered ? 0.6f : 0.5f));

    // Button background (gradient effect with two quads)
    glm::vec3 color = baseColor * hoverBrightness;
    m.ctx.renderer.drawUIQuad(bx, by, bw, bh,
                           glm::vec4(color.r, color.g, color.b, 1.0f));
    m.ctx.renderer.drawUIQuad(bx, by + bh * 0.5f, bw, bh * 0.5f,
                           glm::vec4(color.r * 0.7f, color.g * 0.7f, color.b * 0.7f, 0.3f));

    // Button border (brighter and thicker on hover)
    float btnBorderThickness = (hovered ? 3.0f : 2.0f) * m.uiS;
    glm::vec4 btnBorder = hovered ? glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) : glm::vec4(0.0f, 0.0f, 0.0f, 0.4f);
    m.ctx.renderer.drawUIQuad(bx, by, bw, btnBorderThickness, btnBorder);
    m.ctx.renderer.drawUIQuad(bx, by + bh - btnBorderThickness, bw, btnBorderThickness, btnBorder);
    m.ctx.renderer.drawUIQuad(bx, by, btnBorderThickness, bh, btnBorder);
    m.ctx.renderer.drawUIQuad(bx + bw - btnBorderThickness, by, btnBorderThickness, bh, btnBorder);

    // Button label
    float labelScale = 1.0f * m.uiS;
    float labelW = m.ctx.renderer.measureUITextWidth(label, labelScale);
    float labelH = m.ctx.renderer.getUIFontLineHeight(labelScale);
    float labelX = bx + (bw - labelW) * 0.5f;
    float labelY = by + (bh - labelH) * 0.5f + (subtitle.empty() ? 0.0f : 8.0f * m.uiS);

    // Label shadow (stronger on hover)
    float shadowStrength = hovered ? 0.8f : 0.6f;
    m.ctx.renderer.drawUIText(labelX + 1.0f * m.uiS, labelY - 1.0f * m.uiS, label, labelScale, glm::vec3(0, 0, 0) * shadowStrength);
    m.ctx.renderer.drawUIText(labelX, labelY, label, labelScale, glm::vec3(1, 1, 1));

    // Subtitle (brighter on hover)
    if (!subtitle.empty()) {
        float subScale = 0.50f * m.uiS;
        float subW = m.ctx.renderer.measureUITextWidth(subtitle, subScale);
        float subX = bx + (bw - subW) * 0.5f;
        float subY = labelY - labelH * 0.5f - 4.0f * m.uiS;
        glm::vec3 subColor = hovered ? glm::vec3(1.0f, 1.0f, 1.0f) : glm::vec3(0.8f, 0.8f, 0.9f);
        m.ctx.renderer.drawUIText(subX, subY, subtitle, subScale, subColor);
    }
}

} // namespace game::render::menu


