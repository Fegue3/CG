#include "game/render/menu/MenuRenderParts.hpp"

#include "game/GameAssets.hpp"

namespace game::render::menu {

void drawMainScreen(const MenuCtx& m) {
    // Main menu: PLAY, INSTRUCTIONS, OPTIONS, EXIT
    drawButton(m, 0, m.btnX, m.btn1Y, m.btnW, m.btnH, "PLAY", glm::vec3(0.2f, 0.7f, 0.2f), "Game Modes");
    drawButton(m, 1, m.btnX, m.btn2Y, m.btnW, m.btnH, "INSTRUCTIONS", glm::vec3(0.3f, 0.5f, 0.8f), "How to Play");
    drawButton(m, 2, m.btnX, m.btn3Y, m.btnW, m.btnH, "OPTIONS", glm::vec3(0.7f, 0.5f, 0.2f), "Settings");
    drawButton(m, 3, m.btnX, m.btn4Y, m.btnW, m.btnH, "EXIT", glm::vec3(0.7f, 0.2f, 0.2f), "Quit Game");
}

void drawOptionsScreen(const MenuCtx& m) {
    // Options submenu: SOUND, GRAPHICS, BACK
    drawButton(m, 0, m.btnX, m.btn1Y - 50.0f, m.btnW, m.btnH, "SOUND", glm::vec3(0.3f, 0.6f, 0.7f), "Mix");
    drawButton(m, 1, m.btnX, m.btn2Y - 50.0f, m.btnW, m.btnH, "GRAPHICS", glm::vec3(0.6f, 0.3f, 0.7f), "");

    float backW = 120.0f * m.uiS;
    float backH = 50.0f * m.uiS;
    float backX = m.panelX + 20.0f * m.uiS;
    float backY = m.panelY + 15.0f * m.uiS;
    drawButton(m, 2, backX, backY, backW, backH, "< BACK", glm::vec3(0.5f, 0.5f, 0.5f), "");
}

void drawInstructionsScreen(const MenuCtx& m) {
    // Instructions screen: CONTROLS, POWERUPS, ROGUE CARDS, BACK (buttons open the detailed overlay panel)
    drawButton(m, 0, m.btnX, m.btn1Y - 50.0f, m.btnW, m.btnH, "CONTROLS", glm::vec3(0.3f, 0.6f, 0.7f), "View");
    drawButton(m, 1, m.btnX, m.btn2Y - 50.0f, m.btnW, m.btnH, "POWERUPS", glm::vec3(0.6f, 0.3f, 0.7f), "View");
    drawButton(m, 2, m.btnX, m.btn3Y - 50.0f, m.btnW, m.btnH, "ROGUE CARDS", glm::vec3(0.85f, 0.55f, 0.15f), "Browse");

    float backW = 120.0f * m.uiS;
    float backH = 50.0f * m.uiS;
    float backX = m.panelX + 20.0f * m.uiS;
    float backY = m.panelY + 15.0f * m.uiS;
    drawButton(m, 3, backX, backY, backW, backH, "< BACK", glm::vec3(0.5f, 0.5f, 0.5f), "");
}

void drawOneBrickTestBadge(const MenuCtx& m) {
    // Small clickable "4" badge for the one-brick test mode (only on MAIN menu, bottom-right corner)
    if (m.state.showInstructions) return;
    if (m.state.currentMenuScreen != MenuScreen::MAIN) return;

    float badgeW = m.L.testBadge.w;
    float badgeH = m.L.testBadge.h;
    float badgeX = m.L.testBadge.x;
    float badgeY = m.L.testBadge.y;

    bool hovered = m.state.hoveredTestBadge;
    float hoverScale = hovered ? 1.15f : 1.0f;
    float hoverBrightness = hovered ? 1.4f : 1.0f;

    // Shadow (larger on hover)
    float shadowOffset = (hovered ? 4.0f : 2.0f) * m.uiS;
    m.ctx.renderer.drawUIQuad(badgeX + shadowOffset, badgeY - shadowOffset,
                           badgeW, badgeH, glm::vec4(0.0f, 0.0f, 0.0f, hovered ? 0.6f : 0.4f));

    // Badge background with hover brightness
    glm::vec3 badgeColor = glm::vec3(0.2f, 0.8f, 1.0f) * hoverBrightness;
    m.ctx.renderer.drawUIQuad(badgeX, badgeY, badgeW, badgeH, glm::vec4(badgeColor, 1.0f));
    m.ctx.renderer.drawUIQuad(badgeX + 2.0f, badgeY + 2.0f, badgeW - 4.0f, badgeH - 4.0f, glm::vec4(0.06f, 0.06f, 0.10f, 1.0f));

    // "4" text with hover scale and glow
    std::string k = "4";
    float kScale = 1.4f * hoverScale * m.uiS;
    float kW = m.ctx.renderer.measureUITextWidth(k, kScale);
    float kH = m.ctx.renderer.getUIFontLineHeight(kScale);
    float kX = badgeX + (badgeW - kW) * 0.5f;
    float kY = badgeY + (badgeH - kH) * 0.5f;

    // Glow effect on hover
    if (hovered) {
        m.ctx.renderer.drawUIText(kX + 1.0f, kY, k, kScale, glm::vec3(0.5f, 1.0f, 1.0f) * 0.5f);
        m.ctx.renderer.drawUIText(kX - 1.0f, kY, k, kScale, glm::vec3(0.5f, 1.0f, 1.0f) * 0.5f);
    }
    m.ctx.renderer.drawUIText(kX, kY, k, kScale, glm::vec3(0.2f, 0.8f, 1.0f) * hoverBrightness);

    // Hint text above the badge (brighter on hover)
    std::string hint = "ONE BRICK";
    float hScale = 0.48f * m.uiS;
    float hW = m.ctx.renderer.measureUITextWidth(hint, hScale);
    glm::vec3 hintColor = hovered ? glm::vec3(0.8f, 1.0f, 1.0f) : glm::vec3(0.6f, 0.85f, 1.0f);
    m.ctx.renderer.drawUIText(badgeX + (badgeW - hW) * 0.5f, badgeY + badgeH + 8.0f * m.uiS, hint, hScale, hintColor);
}

} // namespace game::render::menu


