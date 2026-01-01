/**
 * @file BgSelectorHud.cpp
 * @brief HUD pequeno para escolher o background activo (paleta de 5 opções).
 *
 * @details
 *  - Desenha quadrados no canto superior direito.
 *  - A opção seleccionada tem “border” branco; as restantes ficam ligeiramente mais escuras.
 *  - O primeiro quadrado (índice -1) representa “sem background” e mostra um '/'.
 */

 #include "game/render/ui/HudParts.hpp"

 #include "game/GameState.hpp"
 
 #include <glm/glm.hpp>
 
 namespace game::render::hud {
 
 void drawBgSelectorHud(const RenderContext& ctx, const GameState& state) {
     // --- BG Selector HUD ---
     float boxSize = 30.0f;
     float gap = 10.0f;
     float startX = (float)ctx.fbW - (boxSize + gap) * 5.0f - 20.0f;
     float startY = (float)ctx.fbH - 20.0f - boxSize;
 
     glm::vec3 colors[] = {
         {0.15f, 0.15f, 0.2f}, // None (Dark Blueish)
         {0.2f, 0.4f, 0.9f}, // Blueish
         {0.6f, 0.2f, 0.9f}, // Purplish
         {0.2f, 0.8f, 0.4f}, // Greenish
         {0.9f, 0.2f, 0.3f}  // Redish
     };
 
     for (int i = -1; i < 4; i++) {
         float bx = startX + (i + 1) * (boxSize + gap);
         glm::vec3 col = colors[i + 1];
 
         // Draw border if selected
         if (state.currentBg == i) {
             float border = 2.0f;
             ctx.renderer.drawUIQuad(bx - border, startY - border, boxSize + border * 2.0f, boxSize + border * 2.0f, glm::vec4(1, 1, 1, 1.0f));
         } else {
             col *= 0.6f;
         }
 
         ctx.renderer.drawUIQuad(bx, startY, boxSize, boxSize, glm::vec4(col, 1.0f));
 
         if (i == -1) {
             // Draw "/" inside the first box
             float tx = bx + (boxSize - 10.0f) * 0.5f;
             float ty = startY + (boxSize - 16.0f) * 0.5f;
             ctx.renderer.drawUIText(tx, ty, "/", 0.8f, glm::vec3(1, 1, 1));
         }
     }
 }
 
 } // namespace game::render::hud
 