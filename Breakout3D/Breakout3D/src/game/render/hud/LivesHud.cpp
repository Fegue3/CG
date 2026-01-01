/**
 * @file LivesHud.cpp
 * @brief HUD de vidas (corações 3D) + ícones do deck Rogue (cartas escolhidas).
 *
 * @details
 *  - Vidas: desenha uma fila de corações 3D no topo esquerdo.
 *    * Os corações até state.lives ficam vermelhos; os restantes ficam “apagados”.
 *    * Mostra pelo menos 3 corações para manter HUD estável (não mexer tudo).
 *
 *  - Rogue deck: se estiver em ROGUE e houver cartas escolhidas, desenha uma grelha
 *    de “mini-cards” por baixo dos corações:
 *     * fundo escuro
 *     * border com cor de acento da carta
 *     * label abreviado (ex: "EXP", "FIR", etc. vindo de RogueCards)
 */

 #include "game/render/ui/HudParts.hpp"

 #include "game/GameAssets.hpp"
 #include "game/GameState.hpp"
 #include "game/rogue/RogueCards.hpp"
 #include <algorithm>
 #include <string>
 
 #include <glm/glm.hpp>
 #include <glm/gtc/matrix_transform.hpp>
 
 namespace game::render::hud {
 
 void drawLivesHud(const RenderContext& ctx, const GameState& state, const GameAssets& assets) {
     // -------------------------------------------------------------------------
     // Layout base dos corações
     // -------------------------------------------------------------------------
     const float padX = 22.0f;   // margem esquerda
     const float padTop = 18.0f; // margem ao topo
 
     const float hs = 56.0f;  // “heart size” em píxeis (é usado como scale no mesh)
     const float gap = 12.0f; // espaço entre corações
 
     // Rotação leve para o coração parecer mais 3D no HUD (não ficar “plano”)
     const float rx = glm::radians(18.0f);
     const float ry = glm::radians(-12.0f);
 
     // -------------------------------------------------------------------------
     // Queremos sempre pelo menos 3 corações para manter consistência visual.
     // Se o jogador tiver mais vidas (powerups), desenhamos mais.
     // -------------------------------------------------------------------------
     int heartCount = std::max(3, state.lives);
 
     for (int i = 0; i < heartCount; ++i) {
         // Posição do “centro” do coração em pixels (UI coords)
         float xCenter = padX + hs * 0.5f + i * (hs + gap);
         float yCenter = (float)ctx.fbH - (padTop + hs * 0.5f);
 
         // ---------------------------------------------------------------------
         // Matriz model para desenhar o mesh 3D no contexto UI:
         // - translate para posição em píxeis
         // - rotate para “tilt” 3D
         // - scale para tamanho (hs)
         //
         // Nota: o renderer em UI usa ortho e o shader é o mesmo,
         // por isso meshes UI funcionam como “3D em overlay”.
         // ---------------------------------------------------------------------
         glm::mat4 M(1.0f);
         M = glm::translate(M, glm::vec3(xCenter, yCenter, 0.0f));
         M = glm::rotate(M, ry, glm::vec3(0,1,0));
         M = glm::rotate(M, rx, glm::vec3(1,0,0));
         M = glm::scale(M, glm::vec3(hs, hs, hs * 0.55f));
 
         // Cor do coração:
         // - se i < lives -> vermelho vivo
         // - senão -> “apagado” (cinza escuro)
         glm::vec3 col = (i < state.lives)
             ? glm::vec3(1.0f, 0.20f, 0.25f)
             : glm::vec3(0.20f, 0.20f, 0.22f);
 
         ctx.renderer.drawMesh(assets.heart, M, col);
     }
 
     // -------------------------------------------------------------------------
     // Rogue deck: ícones de cartas por baixo da fila de vidas.
     //
     // Ideia: mostrar “o que já escolheste” na run, sem ocupar muito espaço.
     // -------------------------------------------------------------------------
     if (state.gameType == GameType::ROGUE && !state.rogueChosen.empty()) {
         const int cols = 6;     // grelha: 6 colunas antes de saltar para a linha de baixo
         const float sz = 30.0f; // tamanho de cada mini-card (px)
         const float g = 8.0f;   // gap
 
         // Linha de corações vai de (fbH - padTop) para baixo durante hs.
         // Vamos pôr as cartas logo abaixo com uma margem.
         float yTopHearts = (float)ctx.fbH - padTop;
         float yBottomHearts = yTopHearts - hs;
 
         float baseY = yBottomHearts - 16.0f - sz; // 16px de espaço + altura do card
         float baseX = padX;
 
         for (size_t i = 0; i < state.rogueChosen.size(); ++i) {
             int col = (int)(i % cols);
             int row = (int)(i / cols);
 
             float x = baseX + col * (sz + g);
             float y = baseY - row * (sz + g);
 
             // cor “acento” depende da carta (para reconhecer rápido)
             glm::vec3 acc = game::rogue::cardAccent(state.rogueChosen[i]);
 
             // shadow (drop shadow) para dar separação do fundo
             ctx.renderer.drawUIQuad(x + 2.5f, y - 2.5f, sz, sz, glm::vec4(0, 0, 0, 0.45f));
 
             // corpo do card: azul escuro quase preto
             ctx.renderer.drawUIQuad(x, y, sz, sz, glm::vec4(0.08f, 0.08f, 0.14f, 0.98f));
 
             // border: 4 rectângulos finos para parecer moldura
             float bt = 2.0f;
             ctx.renderer.drawUIQuad(x - bt, y - bt, sz + 2*bt, bt, glm::vec4(acc, 1.0f)); // bottom
             ctx.renderer.drawUIQuad(x - bt, y + sz, sz + 2*bt, bt, glm::vec4(acc, 1.0f)); // top
             ctx.renderer.drawUIQuad(x - bt, y, bt, sz, glm::vec4(acc, 1.0f));              // left
             ctx.renderer.drawUIQuad(x + sz, y, bt, sz, glm::vec4(acc, 1.0f));              // right
 
             // label abreviado (texto)
             std::string lbl = game::rogue::cardAbbrev(state.rogueChosen[i]);
 
             // escala pequena para caber bem no quadrado
             float s = 0.62f;
 
             // centragem: medir texto e colocar no meio
             float tw = ctx.renderer.measureUITextWidth(lbl, s);
             float th = ctx.renderer.getUIFontLineHeight(s);
             float tx = x + (sz - tw) * 0.5f;
             float ty = y + (sz - th) * 0.5f;
 
             // sombra do texto para contraste (1px)
             ctx.renderer.drawUIText(tx + 1.0f, ty - 1.0f, lbl, s, glm::vec4(0, 0, 0, 0.55f));
             ctx.renderer.drawUIText(tx, ty, lbl, s, glm::vec4(1, 1, 1, 0.92f));
         }
     }
 }
 
 } // namespace game::render::hud
 