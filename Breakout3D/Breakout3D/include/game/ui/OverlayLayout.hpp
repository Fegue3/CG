#pragma once

namespace game::ui {

struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    bool contains(float px, float py) const {
        return px >= x && px <= (x + w) && py >= y && py <= (y + h);
    }
};

struct OverlayLayout {
    Rect panel;
    Rect leftBtn;
    Rect rightBtn;
};

// Must match visuals in `game/render/UIRender.cpp`.
inline OverlayLayout pauseOverlay(int fbW, int fbH) {
    OverlayLayout L;
    L.panel.w = 650.0f;
    L.panel.h = 320.0f; // Taller to fit bigger PAUSED text + buttons
    L.panel.x = (fbW - L.panel.w) * 0.5f;
    L.panel.y = (fbH - L.panel.h) * 0.5f;

    const float btnW = 140.0f;
    const float btnH = 60.0f;
    const float btnGap = 50.0f;
    const float btnXLeft = L.panel.x + (L.panel.w - 2.0f * btnW - btnGap) * 0.5f;
    const float btnXRight = btnXLeft + btnW + btnGap;
    const float btnY = L.panel.y + 40.0f;

    L.leftBtn = Rect{btnXLeft, btnY, btnW, btnH};
    L.rightBtn = Rect{btnXRight, btnY, btnW, btnH};
    return L;
}

// Must match visuals in `game/render/UIRender.cpp`.
inline OverlayLayout endOverlay(int fbW, int fbH) {
    OverlayLayout L;
    L.panel.w = 650.0f;
    L.panel.h = 260.0f;
    L.panel.x = (fbW - L.panel.w) * 0.5f;
    L.panel.y = (fbH - L.panel.h) * 0.5f;

    const float btnW = 140.0f;
    const float btnH = 60.0f;
    const float btnGap = 50.0f;
    const float btnXLeft = L.panel.x + (L.panel.w - 2.0f * btnW - btnGap) * 0.5f;
    const float btnXRight = btnXLeft + btnW + btnGap;
    const float btnY = L.panel.y + 40.0f;

    L.leftBtn = Rect{btnXLeft, btnY, btnW, btnH};
    L.rightBtn = Rect{btnXRight, btnY, btnW, btnH};
    return L;
}

struct MenuLayout {
    float panelY = 0.0f;
    Rect btn1; // Normal
    Rect btn2; // Endless
    Rect btn3; // Instructions
    Rect btn4; // Exit
    Rect testBadge; // One brick test
};

// Calculate menu panel Y position (must match UIRender.cpp logic)
inline MenuLayout calculateMenuLayout(int fbW, int fbH) {
    MenuLayout L;
    
    const float panelW = 500.0f;
    const float panelH = 480.0f;
    const float panelX = (fbW - panelW) * 0.5f;
    
    const float topMargin = 28.0f;
    const float bottomMargin = 24.0f;
    const float titlePanelGap = 26.0f;
    
    // Approximate title height (measured from actual rendering at 1280x900)
    // This should match the actual titleH calculated in UIRender.cpp
    const float titleH = 175.0f; // Real value from renderer
    float titleY = (float)fbH - topMargin - titleH;
    
    // Panel sits below the title
    float panelY = titleY - titlePanelGap - panelH;
    if (panelY < bottomMargin) {
        panelY = bottomMargin;
    }
    
    L.panelY = panelY;
    
    // Button dimensions
    const float btnW = 200.0f;
    const float btnH = 70.0f;
    const float btnX = panelX + (panelW - btnW) * 0.5f;
    
    // Button positions (from top to bottom)
    L.btn1 = Rect{btnX, panelY + 360.0f, btnW, btnH}; // Normal
    L.btn2 = Rect{btnX, panelY + 250.0f, btnW, btnH}; // Endless
    L.btn3 = Rect{btnX, panelY + 140.0f, btnW, btnH}; // Instructions
    L.btn4 = Rect{btnX, panelY + 30.0f, btnW, btnH};  // Exit
    
    // Test badge inside Exit button
    const float testW = 48.0f;
    const float testH = btnH - 16.0f;
    L.testBadge = Rect{btnX + btnW - 8.0f - testW, panelY + 30.0f + 8.0f, testW, testH};
    
    return L;
}

} // namespace game::ui


