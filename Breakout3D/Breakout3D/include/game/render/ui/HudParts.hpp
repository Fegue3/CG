// HudParts.hpp
#pragma once

#include "game/render/RenderContext.hpp"

namespace game {
struct GameAssets;
struct GameConfig;
struct GameState;
} // namespace game

namespace game::render::hud {

/**
 * @file HudParts.hpp
 * @brief Peças de HUD/overlays (UI 2D) desenhadas por cima do mundo.
 *
 * Cada função desenha um elemento específico do HUD/overlay, usando o `RenderContext`
 * (tamanhos, renderer, tempo, câmara) e o `GameState` (valores actuais do jogo).
 */

// Overlay “cinematográfico” de vitória (flash/ring/âncora no ecrã).
void drawWinFinisherOverlay(const RenderContext& ctx, const GameState& state);

// Efeito de explosão do fireball (normalmente ligado a um evento/estado no GameState).
void drawFireballExplosionFx(const RenderContext& ctx, const GameState& state, const GameConfig& cfg);

// HUD de vidas (icons/texto), depende também de assets.
void drawLivesHud(const RenderContext& ctx, const GameState& state, const GameAssets& assets);

// Indicador de “danger zone” (linha/aviso).
void drawDangerZoneHud(const RenderContext& ctx, const GameState& state);

// HUD de score e wave (endless/rogue), conforme valores no estado.
void drawScoreAndWaveHud(const RenderContext& ctx, const GameState& state);

// HUD do selector de background (quando aplicável).
void drawBgSelectorHud(const RenderContext& ctx, const GameState& state);

// Overlay de pausa ou fim de jogo (resume/retry/menu, etc.).
void drawPauseOrEndOverlay(const RenderContext& ctx, const GameState& state);

} // namespace game::render::hud
