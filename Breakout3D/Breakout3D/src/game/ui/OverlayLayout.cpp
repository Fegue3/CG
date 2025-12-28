#include "game/ui/OverlayLayout.hpp"
#include "engine/Renderer.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace game::ui {

static float fitScaleToWidth(engine::Renderer& renderer, const std::string& text, float desiredScale, float maxWidthPx) {
    float s = desiredScale;
    if (maxWidthPx <= 1.0f) return s;
    float w = renderer.measureUITextWidth(text, s);
    if (w > maxWidthPx && w > 1.0f) {
        s *= (maxWidthPx / w);
    }
    return s;
}

MenuLayout calculateMenuLayout(engine::Renderer& renderer, int fbW, int fbH) {
    MenuLayout L;

    const float topMargin = 28.0f;
    const float bottomMargin = 24.0f;
    const float titlePanelGap = 26.0f;

    const std::string title = "BREAKOUT 3D";
    L.titleScale = fitScaleToWidth(renderer, title, 12.0f, (float)fbW * 0.92f);

    // Ensure vertical room for: [title][gap][panel]
    float maxTitleH = (float)fbH - topMargin - bottomMargin - titlePanelGap - L.panelH;
    if (maxTitleH > 8.0f) {
        float th = renderer.getUIFontLineHeight(L.titleScale);
        if (th > maxTitleH && th > 1.0f) {
            L.titleScale *= (maxTitleH / th);
        }
    }

    L.titleW = renderer.measureUITextWidth(title, L.titleScale);
    L.titleH = renderer.getUIFontLineHeight(L.titleScale);
    L.titleX = ((float)fbW - L.titleW) * 0.5f;
    L.titleY = (float)fbH - topMargin - L.titleH;

    // UI scaling: make the menu panel/buttons grow on bigger screens (like the title).
    // Reference picked from the project's previous tuning at ~1280x900.
    const float refW = 1280.0f;
    const float refH = 900.0f;
    float s = std::min((float)fbW / refW, (float)fbH / refH);
    s = std::max(0.85f, std::min(1.60f, s)); // clamp

    // Also ensure we fit between margins and below the title (if possible).
    float maxY = L.titleY - titlePanelGap;                       // y-up
    float availH = std::max(1.0f, maxY - bottomMargin);          // bottomMargin..maxY

    // --- MAIN MENU panel sizing (scaled) ---
    const float basePanelW = 500.0f;
    const float basePanelH = 480.0f;
    float sPanel = std::min(s, availH / basePanelH);
    sPanel = std::max(0.70f, std::min(1.60f, sPanel));
    L.uiScale = sPanel;

    L.panelW = basePanelW * sPanel;
    L.panelH = basePanelH * sPanel;
    L.panelX = ((float)fbW - L.panelW) * 0.5f;

    // Put the panel slightly BELOW center (composition, under the big title).
    float desiredCenterY = (float)fbH * 0.42f; // < 0.5 => lower on screen (y-up)
    L.panelY = desiredCenterY - L.panelH * 0.5f;

    // Clamp to margins.
    L.panelY = std::max(bottomMargin, std::min(L.panelY, (float)fbH - bottomMargin - L.panelH));

    // Avoid overlapping the title: keep panel top below (titleY - gap).
    if (L.panelY + L.panelH > maxY) {
        L.panelY = maxY - L.panelH;
        L.panelY = std::max(bottomMargin, L.panelY);
    }

    // Buttons with hover effects
    const float btnW = 200.0f * sPanel;
    const float btnH = 70.0f * sPanel;
    const float btnX = L.panelX + (L.panelW - btnW) * 0.5f;

    // Button positions (from top to bottom)
    const float btn1Y = L.panelY + 360.0f * sPanel;
    const float btn2Y = L.panelY + 250.0f * sPanel;
    const float btn3Y = L.panelY + 140.0f * sPanel;
    const float btn4Y = L.panelY + 30.0f  * sPanel;

    L.btn1 = Rect{btnX, btn1Y, btnW, btnH};
    L.btn2 = Rect{btnX, btn2Y, btnW, btnH};
    L.btn3 = Rect{btnX, btn3Y, btnW, btnH};
    L.btn4 = Rect{btnX, btn4Y, btnW, btnH};

    // Small clickable "4" badge for the one-brick test mode (bottom-right corner inside panel)
    const float badgeW = 48.0f * sPanel;
    const float badgeH = 48.0f * sPanel;
    const float badgeX = L.panelX + L.panelW - badgeW - 15.0f * sPanel;
    const float badgeY = L.panelY + 15.0f * sPanel;
    L.testBadge = Rect{badgeX, badgeY, badgeW, badgeH};

    // --- PLAY MODES: aligned card grid (one rectangle per mode) ---
    // User request: 4 columns (single row): NORMAL | ENDLESS | ROGUE | LEVELS
    // Make cards bigger and richer; scale to fit width/height when needed.
    const float baseCardW = 380.0f;
    // WAY taller cards (user request) so the mode pages read like “mini brochures”.
    const float baseCardH = 820.0f;
    // We'll spend extra width on BOTH bigger cards and bigger gaps (fullscreen aesthetics),
    // but we still need a minimum for fit calculations.
    const float baseGapXMin  = 44.0f;
    const float baseGridH = baseCardH;

    // More breathing room on the sides (especially in fullscreen).
    const float sideMargin = 84.0f;
    float availW = std::max(1.0f, (float)fbW - 2.0f * sideMargin);
    float baseGridW = baseCardW * 4.0f + baseGapXMin * 3.0f;

    float sGrid = s;
    // Reserve footer space for the BACK button so it never overlaps the cards.
    const float baseBackH = 72.0f;
    const float baseBackGap = 18.0f;
    float availHGrid = std::max(1.0f, availH - (baseBackH + baseBackGap));

    sGrid = std::min(sGrid, availHGrid / baseGridH);
    sGrid = std::min(sGrid, availW / baseGridW);
    sGrid = std::max(0.60f, std::min(1.70f, sGrid));

    float cardWBase = baseCardW * sGrid;
    float cardH = baseCardH * sGrid;

    // Start with a minimum gap, then distribute remaining width:
    // - most goes to card width (bigger columns)
    // - the rest goes to spacing (bigger gaps)
    float minGapX = baseGapXMin * sGrid;
    float remaining = availW - (cardWBase * 4.0f + minGapX * 3.0f);
    if (remaining < 0.0f) remaining = 0.0f;

    float extraCardPer = std::min((remaining * 0.65f) / 4.0f, 140.0f * sGrid);
    float cardW = cardWBase + extraCardPer;

    float remainingAfterCards = std::max(0.0f, availW - (cardW * 4.0f + minGapX * 3.0f));
    float gapX = minGapX + (remainingAfterCards / 3.0f);
    gapX = std::min(gapX, 220.0f * sGrid);

    float gridW = cardW * 4.0f + gapX * 3.0f;
    float gridH = cardH;
    float gridX = ((float)fbW - gridW) * 0.5f;

    // Position row below the title, slightly lower than center for nicer composition.
    float centerY = (bottomMargin + maxY) * 0.5f;
    centerY -= 24.0f * sGrid;
    float gridY = centerY - gridH * 0.5f;
    float footerY = bottomMargin + (baseBackH + baseBackGap) * sGrid;
    gridY = std::max(footerY, std::min(gridY, maxY - gridH));

    auto makeCard = [&](float x, float y) -> MenuLayout::ModeCard {
        MenuLayout::ModeCard c;
        c.card = Rect{x, y, cardW, cardH};
        float pbW = cardW * 0.46f;
        float pbH = 72.0f * sGrid;
        float pbX = x + (cardW - pbW) * 0.5f;
        float pbY = y + 26.0f * sGrid;
        c.playBtn = Rect{pbX, pbY, pbW, pbH};
        return c;
    };

    // 4-column row (y-up)
    float y = gridY;
    float x0 = gridX;
    float x1 = gridX + (cardW + gapX) * 1.0f;
    float x2 = gridX + (cardW + gapX) * 2.0f;
    float x3 = gridX + (cardW + gapX) * 3.0f;

    L.normal  = makeCard(x0, y);
    L.endless = makeCard(x1, y);
    L.rogue   = makeCard(x2, y);
    L.levels  = makeCard(x3, y);

    // Back button: anchored bottom-left (not at the corner), like a footer control.
    float backW = 260.0f * sGrid;
    float backH = baseBackH * sGrid;
    float backX = sideMargin;
    float backY = bottomMargin;
    L.backBtn = Rect{backX, backY, backW, backH};

    return L;
}

} // namespace game::ui


