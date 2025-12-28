#pragma once

#include "game/render/RenderContext.hpp"

namespace game {
struct GameAssets;
struct GameConfig;
struct GameState;
} // namespace game

namespace game::render::hud {

void drawWinFinisherOverlay(const RenderContext& ctx, const GameState& state);
void drawFireballExplosionFx(const RenderContext& ctx, const GameState& state, const GameConfig& cfg);
void drawLivesHud(const RenderContext& ctx, const GameState& state, const GameAssets& assets);
void drawDangerZoneHud(const RenderContext& ctx, const GameState& state);
void drawScoreAndWaveHud(const RenderContext& ctx, const GameState& state);
void drawBgSelectorHud(const RenderContext& ctx, const GameState& state);
void drawPauseOrEndOverlay(const RenderContext& ctx, const GameState& state);

} // namespace game::render::hud


