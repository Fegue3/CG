#pragma once

#include "game/ui/OverlayLayout.hpp"

namespace game::ui {

// Layout helper for the Instructions overlay panel shown on top of the menu.
// Centralizes geometry so input hitboxes always match what is rendered.
struct InstructionsOverlayLayout {
    Rect panel;
    Rect backBtn;
    Rect modelRect;   // Left half (3D preview)
    Rect infoRect;    // Right half (text + video placeholder)
    Rect navLeft;     // Previous powerup
    Rect navRight;    // Next powerup
    Rect videoRect;   // Video placeholder inside infoRect
};

InstructionsOverlayLayout instructionsOverlayLayout(const MenuLayout& menu, int fbW, int fbH, int instructionsTab);

} // namespace game::ui


