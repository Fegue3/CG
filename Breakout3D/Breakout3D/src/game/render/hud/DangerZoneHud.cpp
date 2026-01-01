/**
 * @file DangerZoneHud.cpp
 * @brief HUD de aviso “DANGER ZONE” (overlay vermelho + texto + sinais) para Endless/Rogue.
 *
 * @details
 *  Este HUD aparece quando o jogador está em risco (zona de morte a aproximar-se).
 *  O efeito é desenhado *antes* de overlays como Pause/GameOver para estes ficarem por cima.
 *
 *  Regras de activação:
 *   - Apenas em ENDLESS ou ROGUE
 *   - Apenas quando endlessDangerActive está activo
 *   - Apenas em PLAYING ou PAUSED
 *   - Só mostra se:
 *       a) timer de perigo < 10s (período de aviso “recente”), OU
 *       b) o paddle está muito perto da linha de morte (veryCloseToDeath)
 *
 *  Componentes visuais:
 *   - “filtro” vermelho que tapa a parte inferior do ecrã até à linha de perigo
 *   - animação de pulso suave (triangular + smoothstep + pow)
 *   - texto "DANGER!" centrado e 2 sinais triangulares simétricos
 */

 #include "game/render/ui/HudParts.hpp"

 #include "game/GameState.hpp"
 #include "game/render/ui/UIHelpers.hpp"
 
 #include <algorithm>
 #include <cmath>
 #include <string>
 
 #include <glm/glm.hpp>
 
 namespace game::render::hud {
 
 void drawDangerZoneHud(const RenderContext& ctx, const GameState& state) {
     // -------------------------------------------------------------------------
     // 1) Early-out: se não estamos em condições de perigo, não desenhar nada.
     //
     // Nota: isto é desenhado antes de overlays (pause/gameover). O render pass
     // chama isto cedo para que depois os overlays (menus) fiquem por cima.
     // -------------------------------------------------------------------------
     if (!((state.gameType == GameType::ENDLESS || state.gameType == GameType::ROGUE) &&
           state.endlessDangerActive &&
           (state.mode == GameMode::PLAYING || state.mode == GameMode::PAUSED) &&
           (
               // durante os primeiros segundos de "perigo", mostramos sempre o aviso
               state.endlessDangerTimer < 10.0f ||
               // ou então mostramos se o paddle está fisicamente muito perto
               ((state.paddlePos.z - 0.5f) - state.endlessDangerMaxZ) <= (1.33f * 2.0f)
           ))) {
         return;
     }
 
     // -------------------------------------------------------------------------
     // 2) Linha de perigo: o ctx pode trazer a linha já projectada em píxeis.
     // Se não vier (valor negativo), usamos um fallback (30% da altura).
     // Depois clamp para manter tudo dentro do framebuffer.
     // -------------------------------------------------------------------------
     float screenY = (ctx.dangerLineScreenY >= 0.0f) ? ctx.dangerLineScreenY : (float)ctx.fbH * 0.30f;
     screenY = std::max(0.0f, std::min((float)ctx.fbH, screenY));
 
     // -------------------------------------------------------------------------
     // 3) Pulso do aviso (alpha animado) — queremos “flash” suave, sem strobe agressivo.
     //
     // - period define a cadência do pulso
     // - ph é a fase do timer dentro do período
     // - tri: triangle wave 0..1..0
     // - smoothstep: suaviza transições para não parecer “liga/desliga”
     // - pow(pulse, 1.15): dá mais presença no pico, sem aumentar strobe
     // -------------------------------------------------------------------------
     const float period = 0.95f; // seconds
     float ph = std::fmod(state.endlessDangerTimer, period);
     if (ph < 0.0f) ph += period;
     float x = ph / period; // 0..1
 
     // triangle wave 0..1..0
     float tri = 1.0f - std::abs(2.0f * x - 1.0f);
 
     // smoothstep (0..1) = t^2(3-2t)
     float pulse = tri * tri * (3.0f - 2.0f * tri);
 
     // ligeira ênfase no topo
     pulse = std::pow(pulse, 1.15f);
 
     // -------------------------------------------------------------------------
     // 4) Filtro vermelho: cobre a área de “risco” (parte inferior até screenY).
     // A alpha base é baixa, e aumenta com o pulso.
     // -------------------------------------------------------------------------
     float baseA = 0.06f + 0.30f * pulse; // fades in/out with the warning
     if (baseA > 0.01f) {
         ctx.renderer.drawUIQuad(0.0f, 0.0f, (float)ctx.fbW, screenY, glm::vec4(1.0f, 0.0f, 0.0f, baseA));
     }
 
     // -------------------------------------------------------------------------
     // 5) Conteúdo “forte” (texto + sinais) só aparece quando:
     //    - estamos ainda no período “recente” do perigo (<10s), OU
     //    - o paddle está muito perto da zona de morte
     //
     // Isto impede ruído visual constante — deixa o filtro leve sempre que relevante,
     // e mete “DANGER!” só quando faz mesmo sentido.
     // -------------------------------------------------------------------------
     bool veryCloseToDeath = (((state.paddlePos.z - 0.5f) - state.endlessDangerMaxZ) <= (1.33f * 2.0f));
     if (!(state.endlessDangerTimer < 10.0f || veryCloseToDeath)) return;
 
     // alpha para texto/sinais: muito mais agressivo do que o filtro, mas sempre animado pelo pulso
     float triAlpha = 0.10f + 0.90f * pulse;
     if (triAlpha < 0.02f) return;
 
     // -------------------------------------------------------------------------
     // 6) Texto "DANGER!" centrado, com scale auto-ajustado para caber em ~92% do ecrã.
     // - fitScaleToWidth: mede o texto e baixa o scale se estiver a estourar
     // - yPos tenta centrar verticalmente dentro da zona (0..screenY)
     // -------------------------------------------------------------------------
     std::string dMsg = "DANGER!";
     float dScale = ui::fitScaleToWidth(ctx.renderer, dMsg, 4.0f, (float)ctx.fbW * 0.92f);
     float dW = ctx.renderer.measureUITextWidth(dMsg, dScale);
     float xPos = (float)ctx.fbW * 0.5f - dW * 0.5f;
 
     float th = ctx.renderer.getUIFontLineHeight(dScale);
     float yPos = (screenY - th) * 0.5f;
 
     // clamps para não “bater” em margens estranhas quando screenY é pequeno
     yPos = std::max(12.0f, yPos);
     if (yPos + th > screenY - 8.0f) yPos = std::max(12.0f, screenY - th - 8.0f);
 
     // ponto médio do texto para alinhar sinais triangulares
     float yMid = yPos + th * 0.5f;
 
     // -------------------------------------------------------------------------
     // 7) “Thickness pass”: desenha o mesmo texto deslocado ligeiramente
     // (quase como stroke/sombra) para ficar mais “pesado” e legível.
     // -------------------------------------------------------------------------
     {
         const float o = 1.25f;            // offset px
         const float a = triAlpha * 0.16f; // alpha baixa para não ficar borrado
         glm::vec4 thick(1.0f, 0.0f, 0.0f, a);
         ctx.renderer.drawUIText(xPos - o, yPos, dMsg, dScale, thick);
         ctx.renderer.drawUIText(xPos + o, yPos, dMsg, dScale, thick);
         ctx.renderer.drawUIText(xPos, yPos - o, dMsg, dScale, thick);
         ctx.renderer.drawUIText(xPos, yPos + o, dMsg, dScale, thick);
     }
 
     // texto principal (alpha moderada; o filtro já dá “peso” ao vermelho)
     ctx.renderer.drawUIText(xPos, yPos, dMsg, dScale, glm::vec4(1.0f, 0.0f, 0.0f, triAlpha * 0.30f));
 
     // -------------------------------------------------------------------------
     // 8) Sinais triangulares (tipo warning sign) à esquerda e direita.
     //
     // Implementação: desenhar 3 “edges” grossos com triângulos (duas faces por edge).
     // Depois desenhar um "!" simples com 2 rectângulos.
     //
     // Nota: drawUITriangle desenha triângulos em coords de UI (px), por isso
     // conseguimos construir “geometria 2D” sem texturas.
     // -------------------------------------------------------------------------
     auto drawTriangleSign = [&](float xCenter) {
         float triW = 100.0f;
         float triH = 85.0f;
         float yTri = yMid - triH * 0.5f;
 
         glm::vec2 pTop(xCenter, yTri + triH);
         glm::vec2 pLeft(xCenter - triW*0.5f, yTri);
         glm::vec2 pRight(xCenter + triW*0.5f, yTri);
 
         // desenha uma aresta grossa entre dois pontos:
         // calcula a perpendicular à direcção e cria um “quad” em 2 triângulos
         auto drawEdge = [&](const glm::vec2& a, const glm::vec2& b, float t) {
             glm::vec2 d = b - a;
             float len = glm::length(d);
             if (len < 0.01f) return;
 
             glm::vec2 dir = d / len;
             glm::vec2 perp(-dir.y, dir.x);
 
             glm::vec2 c1 = a + perp * (t * 0.5f);
             glm::vec2 c2 = a - perp * (t * 0.5f);
             glm::vec2 c3 = b + perp * (t * 0.5f);
             glm::vec2 c4 = b - perp * (t * 0.5f);
 
             ctx.renderer.drawUITriangle(c1, c2, c3, glm::vec4(1.0f, 0.0f, 0.0f, triAlpha));
             ctx.renderer.drawUITriangle(c2, c3, c4, glm::vec4(1.0f, 0.0f, 0.0f, triAlpha));
         };
 
         float edgeT = 6.0f;
         drawEdge(pLeft, pRight, edgeT);
         drawEdge(pLeft, pTop, edgeT);
         drawEdge(pRight, pTop, edgeT);
 
         // "!" (barra + ponto) no centro do triângulo
         ctx.renderer.drawUIQuad(xCenter - 3, yTri + 30, 6, 40, glm::vec4(1.0f, 0.0f, 0.0f, triAlpha));
         ctx.renderer.drawUIQuad(xCenter - 3, yTri + 16, 6, 6,  glm::vec4(1.0f, 0.0f, 0.0f, triAlpha));
     };
 
     // Composição simétrica: 22% e 78% do ecrã
     drawTriangleSign((float)ctx.fbW * 0.22f);
     drawTriangleSign((float)ctx.fbW * 0.78f);
 }
 
 } // namespace game::render::hud
 