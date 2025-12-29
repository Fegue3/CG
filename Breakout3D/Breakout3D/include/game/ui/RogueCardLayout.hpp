#pragma once

#include "game/ui/OverlayLayout.hpp" // for Rect

namespace game::ui {

struct RogueCardLayout {
    Rect cardA;
    Rect cardB;
    Rect cardC;
};

// Shared layout for Rogue "card pick" overlay.
// IMPORTANT: used by both rendering and click hit-testing.
inline RogueCardLayout rogueCardOverlay(int fbW, int fbH) {
    RogueCardLayout L;

    // Three tall "mode-card-like" cards, centered horizontally.
    // Keep them slightly lower so the top-center HUD (wave/score/choose) stays readable.
    float cardH = std::min(820.0f, (float)fbH * 0.68f);
    float cardW = std::min(420.0f, cardH * 0.56f); // tall + less width
    float gap = std::min(58.0f, cardW * 0.14f);

    float totalW = cardW * 3.0f + gap * 2.0f;
    float ax = ((float)fbW - totalW) * 0.5f;
    float bx = ax + cardW + gap;
    float cx = bx + cardW + gap;

    float cardY = ((float)fbH - cardH) * 0.34f; // slightly below center

    L.cardA = Rect{ax, cardY, cardW, cardH};
    L.cardB = Rect{bx, cardY, cardW, cardH};
    L.cardC = Rect{cx, cardY, cardW, cardH};
    return L;
}

} // namespace game::ui


