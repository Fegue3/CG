#pragma once

#include "game/render/RenderContext.hpp"
#include "game/GameState.hpp"

#include <string>

#include <glm/glm.hpp>

namespace game {
struct GameAssets;
} // namespace game

namespace game::render::menu {

// Internal helper context for menu rendering.
// This is intentionally a small "bundle" so the menu implementation can be split
// across multiple .cpp files without threading 10+ parameters everywhere.
struct MenuCtx {
    const RenderContext& ctx;
    const GameState& state;
    const GameAssets& assets;
    const game::ui::MenuLayout& L;

    float uiS = 1.0f;

    // Main menu panel geometry (cached from L).
    float panelW = 0.0f;
    float panelH = 0.0f;
    float panelX = 0.0f;
    float panelY = 0.0f;

    // Main menu button geometry (cached from L).
    float btnW = 0.0f;
    float btnH = 0.0f;
    float btnX = 0.0f;
    float btn1Y = 0.0f;
    float btn2Y = 0.0f;
    float btn3Y = 0.0f;
    float btn4Y = 0.0f;
};

MenuCtx makeMenuCtx(const RenderContext& ctx, const GameState& state, const GameAssets& assets);

// Base layers (background/title/panel).
void drawRetroBackground(const MenuCtx& m);
void drawTitle(const MenuCtx& m);
void drawMainPanelIfNeeded(const MenuCtx& m);

// Shared button drawing helper (used by several screens).
void drawButton(
    const MenuCtx& m,
    int btnIndex,
    float bx, float by, float bw, float bh,
    const std::string& label,
    const glm::vec3& baseColor,
    const std::string& subtitle = ""
);

// Top-level screens (not overlays).
void drawMainScreen(const MenuCtx& m);
void drawOptionsScreen(const MenuCtx& m);
void drawInstructionsScreen(const MenuCtx& m);
void drawPlayModesScreen(const MenuCtx& m);
void drawLevelSelectScreen(const MenuCtx& m);

// Small extra UI.
void drawOneBrickTestBadge(const MenuCtx& m);

// Instructions overlay (tabs: controls / powerups / rogue cards).
void drawInstructionsOverlay(const MenuCtx& m);

} // namespace game::render::menu


