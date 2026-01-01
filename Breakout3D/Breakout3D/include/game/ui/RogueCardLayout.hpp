// RogueCardLayout.hpp
#pragma once

#include "game/ui/OverlayLayout.hpp" // para Rect
#include <algorithm>                 // std::min

namespace game::ui {

/**
 * @file RogueCardLayout.hpp
 * @brief Layout do overlay de escolha de cartas no Rogue (3 cartas lado a lado).
 *
 * Este layout é partilhado entre:
 * - render (para desenhar as cartas nos sítios certos)
 * - input (hit-testing do clique nas cartas)
 */
struct RogueCardLayout {
    Rect cardA;
    Rect cardB;
    Rect cardC;
};

/**
 * @brief Calcula a geometria do overlay de cartas (3 cartões altos centrados).
 *
 * @note
 * Mantém as cartas ligeiramente abaixo do centro para não tapar o HUD de topo
 * (wave/score/instruções).
 */
inline RogueCardLayout rogueCardOverlay(int fbW, int fbH) {
    RogueCardLayout L;

    float cardH = std::min(820.0f, (float)fbH * 0.68f);
    float cardW = std::min(420.0f, cardH * 0.56f);
    float gap = std::min(58.0f, cardW * 0.14f);

    float totalW = cardW * 3.0f + gap * 2.0f;
    float ax = ((float)fbW - totalW) * 0.5f;
    float bx = ax + cardW + gap;
    float cx = bx + cardW + gap;

    float cardY = ((float)fbH - cardH) * 0.34f;

    L.cardA = Rect{ax, cardY, cardW, cardH};
    L.cardB = Rect{bx, cardY, cardW, cardH};
    L.cardC = Rect{cx, cardY, cardW, cardH};
    return L;
}

} // namespace game::ui
