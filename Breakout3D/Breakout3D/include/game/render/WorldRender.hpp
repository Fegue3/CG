// WorldRender.hpp
#pragma once

#include "game/render/RenderContext.hpp"

namespace game {
struct GameAssets;
struct GameConfig;
struct GameState;
} // namespace game

namespace game::render {

/**
 * @file WorldRender.hpp
 * @brief Entrada de render do mundo 3D por frame (scene/gameplay).
 */
void renderWorld(const RenderContext& ctx, const GameState& state, const GameConfig& cfg, const GameAssets& assets);

} // namespace game::render
