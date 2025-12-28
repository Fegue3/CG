#pragma once

#include "game/render/RenderContext.hpp"

namespace game {
struct GameAssets;
struct GameConfig;
struct GameState;
} // namespace game

namespace game::render {

void renderWorld(const RenderContext& ctx, const GameState& state, const GameConfig& cfg, const GameAssets& assets);

} // namespace game::render


