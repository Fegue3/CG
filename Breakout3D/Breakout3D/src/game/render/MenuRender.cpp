/**
 * @file MenuRender.cpp
 * @brief Entry-point do render UI do menu.
 *
 * Camadas base (sempre):
 * - fundo retro animado
 * - título "BREAKOUT 3D"
 * - painel principal (quando aplicável)
 *
 * Depois:
 * - desenha o ecrã atual (MAIN/PLAY_MODES/OPTIONS/SOUND/INSTRUCTIONS/LEVEL_SELECT)
 * - badge do "ONE BRICK" (só no MAIN)
 * - overlay de Instruções (modal-ish)
 *
 * Nota importante:
 * - Quando `state.showInstructions` está ativo, escondemos os ecrãs por baixo
 *   porque alguns tabs (Powerups/Rogue Cards) usam layout “sem painel opaco”
 *   e ver UI por trás ficava feio/confuso.
 */
 #include "game/render/UIRender.hpp"

 #include "game/GameAssets.hpp"
 #include "game/GameState.hpp"
 #include "game/render/menu/MenuRenderParts.hpp"
 
 namespace game::render {
 
 void renderMenu(const RenderContext& ctx, const GameState& state, const GameAssets& assets) {
     ctx.renderer.beginUI(ctx.fbW, ctx.fbH);
 
     // Contexto com layout e shortcuts (posições, escalas, assets).
     const auto m = game::render::menu::makeMenuCtx(ctx, state, assets);
 
     // Base layers (sempre)
     game::render::menu::drawRetroBackground(m);
     game::render::menu::drawTitle(m);
     game::render::menu::drawMainPanelIfNeeded(m);
 
     // Ecrãs “de baixo” ficam desligados quando há overlay de Instruções.
     if (!state.showInstructions) {
         switch (state.currentMenuScreen) {
             case MenuScreen::MAIN:
                 game::render::menu::drawMainScreen(m);
                 break;
             case MenuScreen::PLAY_MODES:
                 game::render::menu::drawPlayModesScreen(m);
                 break;
             case MenuScreen::OPTIONS:
                 game::render::menu::drawOptionsScreen(m);
                 break;
             case MenuScreen::SOUND:
                 game::render::menu::drawSoundScreen(m);
                 break;
             case MenuScreen::INSTRUCTIONS:
                 game::render::menu::drawInstructionsScreen(m);
                 break;
             case MenuScreen::LEVEL_SELECT:
                 game::render::menu::drawLevelSelectScreen(m);
                 break;
         }
     }
 
     // Badge “4” (one-brick test) — desenha por cima do screen normal, mas abaixo do overlay.
     game::render::menu::drawOneBrickTestBadge(m);
 
     // Overlay de Instruções (modal-ish)
     if (state.showInstructions) {
         game::render::menu::drawInstructionsOverlay(m);
     }
 
     ctx.renderer.endUI();
 }
 
 } // namespace game::render
 