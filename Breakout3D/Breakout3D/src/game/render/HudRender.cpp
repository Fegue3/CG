/**
 * @file HudRender.cpp
 * @brief Entry-point do render UI durante o jogo (HUD + overlays).
 *
 * Pipeline:
 * - beginUI/endUI define o pass 2D por cima do mundo 3D.
 * - A ordem dos draws importa: efeitos/elementos "por cima" devem ser desenhados no fim.
 *
 * Ordem escolhida:
 *  1) Win finisher (flash + shockwave)
 *  2) Fireball explosion FX (UI)
 *  3) Lives HUD
 *  4) Danger zone HUD
 *  5) Score/Wave HUD
 *  6) Background selector HUD
 *  7) Pause / End overlay (desenha por cima de tudo)
 */
 #include "game/render/UIRender.hpp"

 #include "game/GameAssets.hpp"
 #include "game/GameConfig.hpp"
 #include "game/GameState.hpp"
 #include "game/render/ui/HudParts.hpp"
 
 namespace game::render {
 
 void renderUI(const RenderContext& ctx, const GameState& state, const GameConfig& cfg, const GameAssets& assets) {
     ctx.renderer.beginUI(ctx.fbW, ctx.fbH);
 
     // FX e HUD "normais"
     hud::drawWinFinisherOverlay(ctx, state);
     hud::drawFireballExplosionFx(ctx, state, cfg);
 
     hud::drawLivesHud(ctx, state, assets);
     hud::drawDangerZoneHud(ctx, state);
 
     hud::drawScoreAndWaveHud(ctx, state);
     hud::drawBgSelectorHud(ctx, state);
 
     // Overlay modal (PAUSED / GAME OVER / WIN / ROGUE_CARDS) — tem de ficar por cima.
     hud::drawPauseOrEndOverlay(ctx, state);
 
     ctx.renderer.endUI();
 }
 
 } // namespace game::render
 