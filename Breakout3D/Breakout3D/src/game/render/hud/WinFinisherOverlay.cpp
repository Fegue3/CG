/**
 * @file WinFinisherOverlay.cpp
 * @brief Overlay de “finisher” de vitória: flash branco + anel de choque expansivo (shockwave).
 *
 * @details
 *  Este efeito é activado quando state.winFinisherActive está true.
 *  O overlay consiste em:
 *   1) Um flash branco fullscreen (alpha decai com o tempo)
 *   2) Um anel (ring) centrado num ponto projectado (âncora), que expande e afina ao longo do tempo.
 *
 *  A geometria do anel é desenhada em UI (triângulos) para evitar texturas e manter estilo “vector crisp”.
 *  A posição do centro é calculada por game::effects::computeWinFinisherOverlay(), que:
 *   - Projecta a âncora do mundo para coordenadas de ecrã (px)
 *   - Gera parâmetros temporais (radius/thickness/alpha) com easing suave
 *
 *  Nota:
 *   - UI aqui está em coordenadas bottom-left (y para cima).
 *   - O ring é aproximado por segmentos (segs). Mais segs = mais suave, mais caro.
 */

 #include "game/render/ui/HudParts.hpp"

 #include "game/GameState.hpp"
 #include "game/effects/WinFinisher.hpp"
 
 #include <algorithm>
 
 #include <glm/glm.hpp>
 #include <glm/gtc/constants.hpp>
 
 namespace game::render::hud {
 
 /**
  * @brief Desenha o overlay do finisher de vitória (flash + shockwave ring).
  *
  * @param ctx   Contexto de render (contém fbW/fbH, matrizes V/P e renderer).
  * @param state Estado do jogo (timers e âncora do finisher).
  *
  * @note
  *  Deve ser desenhado *depois* da cena 3D (porque é UI) e geralmente perto do fim,
  *  mas *antes* de overlays que queiras por cima (se existirem).
  */
 void drawWinFinisherOverlay(const RenderContext& ctx, const GameState& state) {
     // -------------------------------------------------------------------------
     // Guard: só desenha se o efeito estiver activo.
     // -------------------------------------------------------------------------
     if (!state.winFinisherActive) return;
 
     // -------------------------------------------------------------------------
     // Calcula todos os parâmetros do overlay (centro, radius, thickness, alphas).
     // Usa V/P para projectar âncora do mundo para o ecrã.
     // -------------------------------------------------------------------------
     auto o = game::effects::computeWinFinisherOverlay(
         ctx.fbW, ctx.fbH, ctx.V, ctx.P,
         state.winFinisherAnchorPos,
         state.winFinisherTimer,
         state.winFinisherAnchorValid
     );
 
     // -------------------------------------------------------------------------
     // (1) Flash branco fullscreen (rápido, dá “impact”).
     // -------------------------------------------------------------------------
     float flashA = o.flashAlpha;
     if (flashA > 0.01f) {
         ctx.renderer.drawUIQuad(
             0.0f, 0.0f,
             (float)ctx.fbW, (float)ctx.fbH,
             glm::vec4(1.0f, 1.0f, 1.0f, flashA)
         );
     }
 
     // -------------------------------------------------------------------------
     // (2) Shockwave ring: desenhado por triângulos (segs fatias).
     // -------------------------------------------------------------------------
     glm::vec2 c = o.centerPx;
     float r = o.ringRadiusPx;
     float thick = o.ringThicknessPx;
 
     float ringA = o.ringAlpha;
     glm::vec4 ringCol(0.85f, 0.95f, 1.0f, ringA);
 
     /**
      * @brief Desenha um anel 2D (UI) aproximado por triângulos.
      *
      * @param center    Centro do anel (px).
      * @param radius    Raio médio do anel (px).
      * @param thickness Espessura do anel (px).
      * @param col       Cor (RGBA).
      *
      * @details
      *  O anel é a região entre dois círculos:
      *   - r0 = radius - thickness/2  (raio interior)
      *   - r1 = radius + thickness/2  (raio exterior)
      *  Para cada segmento i, desenhamos 2 triângulos que formam um “quad” curvo.
      */
     auto drawRing = [&](const glm::vec2& center, float radius, float thickness, const glm::vec4& col) {
         const int segs = 72;
         float r0 = std::max(0.0f, radius - thickness * 0.5f);
         float r1 = radius + thickness * 0.5f;
 
         for (int i = 0; i < segs; ++i) {
             float a0 = (float)i / (float)segs * glm::two_pi<float>();
             float a1 = (float)(i + 1) / (float)segs * glm::two_pi<float>();
 
             glm::vec2 d0(std::cos(a0), std::sin(a0));
             glm::vec2 d1(std::cos(a1), std::sin(a1));
 
             // 4 pontos do “quad” (outer/inner, segment start/end)
             glm::vec2 p0o = center + d0 * r1;
             glm::vec2 p0i = center + d0 * r0;
             glm::vec2 p1o = center + d1 * r1;
             glm::vec2 p1i = center + d1 * r0;
 
             // dois triângulos para preencher a fatia
             ctx.renderer.drawUITriangle(p0o, p0i, p1o, col);
             ctx.renderer.drawUITriangle(p0i, p1o, p1i, col);
         }
     };
 
     if (ringA > 0.01f) {
         drawRing(c, r, thick, ringCol);
     }
 }
 
 } // namespace game::render::hud
 