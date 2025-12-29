#include "game/ui/InstructionsOverlayLayout.hpp"

#include <algorithm>

namespace game::ui {

InstructionsOverlayLayout instructionsOverlayLayout(const MenuLayout& menu, int fbW, int fbH, int instructionsTab) {
    InstructionsOverlayLayout L{};
    const bool isPowerups = (instructionsTab == 1);
    const float uiS = menu.uiScale;

    // Panel sizing/position MUST match menu render/input assumptions.
    // Powerups overlay should scale up more on fullscreen.
    float instrW = isPowerups ? std::min(1400.0f, (float)fbW * 0.92f)
                              : std::min(980.0f,  (float)fbW * 0.75f);
    float instrH = isPowerups ? std::min(860.0f,  (float)fbH * 0.78f)
                              : std::min(690.0f,  (float)fbH * 0.72f);
    float instrX = ((float)fbW - instrW) * 0.5f;
    float instrY = std::max(40.0f, ((float)fbH - instrH) * 0.5f - (isPowerups ? 80.0f : 60.0f));

    // Keep below the big title (same gap as OverlayLayout.cpp).
    const float titlePanelGap = 26.0f;
    float maxTopY = menu.titleY - titlePanelGap; // y-up
    if (instrY + instrH > maxTopY) {
        instrY = maxTopY - instrH;
        instrY = std::max(40.0f, instrY);
    }

    L.panel = Rect{instrX, instrY, instrW, instrH};

    // Back button should match PLAY screen position exactly.
    L.backBtn = menu.backBtn;

    // Content region inside the panel (avoid title/top and back/bottom).
    const float pad = (isPowerups ? 34.0f : 44.0f) * uiS;
    const float topReserve = (isPowerups ? 132.0f : 110.0f) * uiS; // title + underline breathing room
    float contentTop = instrY + instrH - topReserve;
    float contentBottom = std::max(instrY + pad, L.backBtn.y + L.backBtn.h + 18.0f * uiS);
    float contentH = std::max(1.0f, contentTop - contentBottom);

    float contentX = instrX + pad;
    float contentW = std::max(1.0f, instrW - pad * 2.0f);

    // Split into left model preview and right info panel.
    // Powerups: push the info widget further right while keeping it larger by letting the panel grow on fullscreen.
    const float gap = (isPowerups ? 34.0f : 26.0f) * uiS;
    float split = isPowerups ? 0.58f : 0.50f; // model left / info right
    float leftW = (contentW - gap) * split;
    float rightW = contentW - gap - leftW;

    L.modelRect = Rect{contentX, contentBottom, leftW, contentH};
    L.infoRect  = Rect{contentX + leftW + gap, contentBottom, rightW, contentH};

    // Navigation arrows (bottom of info rect)
    float navW = 64.0f * uiS;
    float navH = 56.0f * uiS;
    float navY = L.infoRect.y + 10.0f * uiS;
    L.navLeft  = Rect{L.infoRect.x + 12.0f * uiS, navY, navW, navH};
    L.navRight = Rect{L.infoRect.x + L.infoRect.w - navW - 12.0f * uiS, navY, navW, navH};

    // Video placeholder: 16:9 card above the nav arrows
    float vPadX = 12.0f * uiS;
    float vW = std::max(1.0f, L.infoRect.w - vPadX * 2.0f);
    float vH = vW * (9.0f / 16.0f);
    float maxVH = std::max(1.0f, L.infoRect.h * 0.46f);
    if (vH > maxVH) vH = maxVH;
    float vX = L.infoRect.x + vPadX;
    float vY = L.navLeft.y + L.navLeft.h + 18.0f * uiS;
    L.videoRect = Rect{vX, vY, vW, vH};

    return L;
}

} // namespace game::ui


