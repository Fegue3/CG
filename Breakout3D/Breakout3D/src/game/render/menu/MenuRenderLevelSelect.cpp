#include "game/render/menu/MenuRenderParts.hpp"

#include "game/render/ui/UIHelpers.hpp"

#include <algorithm>
#include <string>

namespace game::render::menu {

void drawLevelSelectScreen(const MenuCtx& m) {
    // LEVEL SELECT screen: custom larger panel for level grid
    float levelPanelW = 800.0f * m.uiS;
    float levelPanelH = 650.0f * m.uiS;  // Increased for 4x5 grid
    float levelPanelX = ((float)m.ctx.fbW - levelPanelW) * 0.5f;
    float levelPanelY = ((float)m.ctx.fbH * 0.40f) - levelPanelH * 0.5f;

    // Draw custom panel for level select
    float shadowOffset = 6.0f * m.uiS;
    m.ctx.renderer.drawUIQuad(levelPanelX + shadowOffset, levelPanelY - shadowOffset, levelPanelW, levelPanelH, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
    m.ctx.renderer.drawUIQuad(levelPanelX, levelPanelY, levelPanelW, levelPanelH, glm::vec4(0.08f, 0.08f, 0.14f, 0.98f));

    // Animated border
    float borderThickness = 3.0f * m.uiS;
    float tRgb = m.ctx.time.now();
    float hueRgb = std::fmod(0.56f + 0.08f * std::sin(tRgb * 1.2f), 1.0f);
    glm::vec3 neonRgb = ui::hsv2rgb(hueRgb, 0.85f, 1.0f);
    glm::vec4 borderColor(neonRgb, 1.0f);
    m.ctx.renderer.drawUIQuad(levelPanelX - borderThickness, levelPanelY - borderThickness, levelPanelW + 2*borderThickness, borderThickness, borderColor);
    m.ctx.renderer.drawUIQuad(levelPanelX - borderThickness, levelPanelY + levelPanelH, levelPanelW + 2*borderThickness, borderThickness, borderColor);
    m.ctx.renderer.drawUIQuad(levelPanelX - borderThickness, levelPanelY, borderThickness, levelPanelH, borderColor);
    m.ctx.renderer.drawUIQuad(levelPanelX + levelPanelW, levelPanelY, borderThickness, levelPanelH, borderColor);

    // LEVEL SELECT screen: 4 rows x 5 columns grid of level buttons (20 total)
    const int totalLevels = 20;
    const int cols = 5;
    const int rows = 4;

    // Grid layout
    float btnSize = 110.0f * m.uiS;
    float gapX = 25.0f * m.uiS;
    float gapY = 25.0f * m.uiS;
    float gridW = cols * btnSize + (cols - 1) * gapX;
    float startX = levelPanelX + (levelPanelW - gridW) * 0.5f;
    float startY = levelPanelY + 80.0f * m.uiS;

    // Title: "SELECT LEVEL"
    {
        std::string title = "SELECT LEVEL";
        float titleScale = 2.0f * m.uiS;
        float titleW = m.ctx.renderer.measureUITextWidth(title, titleScale);
        float titleX = levelPanelX + (levelPanelW - titleW) * 0.5f;
        float titleY = levelPanelY + 25.0f * m.uiS;

        // Cyan glow
        glm::vec3 titleColor(0.20f, 0.75f, 0.85f);
        m.ctx.renderer.drawUIText(titleX - 2.0f * m.uiS, titleY, title, titleScale, glm::vec4(0,0,0,0.6f));
        m.ctx.renderer.drawUIText(titleX + 2.0f * m.uiS, titleY, title, titleScale, glm::vec4(0,0,0,0.6f));
        m.ctx.renderer.drawUIText(titleX, titleY - 2.0f * m.uiS, title, titleScale, glm::vec4(0,0,0,0.6f));
        m.ctx.renderer.drawUIText(titleX, titleY + 2.0f * m.uiS, title, titleScale, glm::vec4(0,0,0,0.6f));
        m.ctx.renderer.drawUIText(titleX, titleY, title, titleScale, glm::vec4(titleColor, 1.0f));
    }

    // Draw level buttons
    for (int i = 0; i < totalLevels; i++) {
        int row = i / cols;
        int col = i % cols;
        float x = startX + col * (btnSize + gapX);
        float y = startY + (rows - 1 - row) * (btnSize + gapY);

        bool unlocked = (i < m.state.levelsBestLevel);
        bool hovered = (m.state.hoveredLevelButton == i);

        // Button background
        float shadow = (hovered ? 5.0f : 3.0f) * m.uiS;
        m.ctx.renderer.drawUIQuad(x + shadow, y - shadow, btnSize, btnSize, glm::vec4(0,0,0, hovered ? 0.6f : 0.5f));

        glm::vec3 bgColor = unlocked
            ? (hovered ? glm::vec3(0.25f, 0.90f, 1.0f) : glm::vec3(0.15f, 0.50f, 0.65f))
            : glm::vec3(0.12f, 0.12f, 0.15f);
        float alpha = unlocked ? 0.95f : 0.50f;
        m.ctx.renderer.drawUIQuad(x, y, btnSize, btnSize, glm::vec4(bgColor, alpha));

        // Border
        float borderThick = (hovered ? 3.5f : 2.5f) * m.uiS;
        glm::vec3 borderColor = unlocked ? glm::vec3(0.20f, 0.75f, 0.85f) : glm::vec3(0.25f, 0.25f, 0.30f);
        m.ctx.renderer.drawUIQuad(x - borderThick, y - borderThick, btnSize + 2*borderThick, borderThick, glm::vec4(borderColor, 1));
        m.ctx.renderer.drawUIQuad(x - borderThick, y + btnSize, btnSize + 2*borderThick, borderThick, glm::vec4(borderColor, 1));
        m.ctx.renderer.drawUIQuad(x - borderThick, y, borderThick, btnSize, glm::vec4(borderColor, 1));
        m.ctx.renderer.drawUIQuad(x + btnSize, y, borderThick, btnSize, glm::vec4(borderColor, 1));

        // Level number
        std::string numStr = std::to_string(i + 1);
        float numScale = 2.5f * m.uiS;
        float numW = m.ctx.renderer.measureUITextWidth(numStr, numScale);
        float numX = x + (btnSize - numW) * 0.5f;
        float numY = y + (btnSize - m.ctx.renderer.getUIFontLineHeight(numScale)) * 0.5f;

        glm::vec4 numColor = unlocked ? glm::vec4(1,1,1,1) : glm::vec4(0.4f, 0.4f, 0.45f, 0.8f);
        m.ctx.renderer.drawUIText(numX, numY, numStr, numScale, numColor);
    }

    // BACK button
    float backW = 120.0f * m.uiS;
    float backH = 50.0f * m.uiS;
    float backX = levelPanelX + 20.0f * m.uiS;
    float backY = levelPanelY + 15.0f * m.uiS;
    drawButton(m, 10, backX, backY, backW, backH, "< BACK", glm::vec3(0.5f, 0.5f, 0.5f), "");
}

} // namespace game::render::menu


