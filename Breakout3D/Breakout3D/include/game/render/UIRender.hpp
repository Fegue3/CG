// UIRender.hpp
#pragma once

#include "game/render/RenderContext.hpp"

namespace game {
struct GameAssets;
struct GameConfig;
struct GameState;
} // namespace game

namespace game::render {

/**
 * @file UIRender.hpp
 * @brief Entradas de render para UI (menu + HUD) por frame.
 */

// Render do menu (ecrãs principais/overlays do menu).
void renderMenu(const RenderContext& ctx, const GameState& state, const GameAssets& assets);

// Render do HUD/overlays em jogo (vidas, score, pause, etc.).
void renderUI(const RenderContext& ctx, const GameState& state, const GameConfig& cfg, const GameAssets& assets);

} // namespace game::render
