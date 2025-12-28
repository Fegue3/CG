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

} // namespace game::ui


