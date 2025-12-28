#pragma once

#include "game/render/RenderContext.hpp"

namespace game {
struct GameAssets;
struct GameConfig;
struct GameState;
} // namespace game

namespace game::render {

void renderMenu(const RenderContext& ctx, const GameState& state, const GameAssets& assets);
void renderUI(const RenderContext& ctx, const GameState& state, const GameConfig& cfg, const GameAssets& assets);

} // namespace game::render


