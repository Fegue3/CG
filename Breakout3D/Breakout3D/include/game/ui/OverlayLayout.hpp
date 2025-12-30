#pragma once

namespace engine { class Renderer; }

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

// Larger pause overlay for ROGUE mode (to fit card list)
inline OverlayLayout pauseOverlayRogue(int fbW, int fbH) {
    OverlayLayout L;
    L.panel.w = 920.0f;  // Increased from 920
    L.panel.h = 700.0f;
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
    // Computed to match `game/render/UIRender.cpp` exactly.
    float uiScale = 1.0f;
    float panelX = 0.0f;
    float panelY = 0.0f;
    float panelW = 0.0f;
    float panelH = 0.0f;

    // Title placement (so render + input always agree across resolutions).
    float titleX = 0.0f;
    float titleY = 0.0f;
    float titleW = 0.0f;
    float titleH = 0.0f;
    float titleScale = 1.0f;

    Rect btn1; // Normal
    Rect btn2; // Endless
    Rect btn3; // Instructions
    Rect btn4; // Exit
    Rect testBadge; // One brick test

    // PLAY_MODES layout: one card per mode (aligned grid), each with its own PLAY button.
    struct ModeCard {
        Rect card;
        Rect playBtn;
    };
    ModeCard normal;
    ModeCard endless;
    ModeCard rogue;
    ModeCard levels;
    Rect backBtn;
};

// Must match visuals in `game/render/UIRender.cpp`.
// NOTE: implemented in `src/game/ui/OverlayLayout.cpp` to avoid pulling the full Renderer into headers.
MenuLayout calculateMenuLayout(engine::Renderer& renderer, int fbW, int fbH);

} // namespace game::ui


