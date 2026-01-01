/**
 * @file FireballExplosionFx.cpp
 * @brief FX de explosão da Fireball: anel laranja expansivo + flash de ecrã.
 *
 * @details
 *  Cada explosão em state.fireballExplosions tem:
 *   - pos (world)
 *   - t (tempo desde spawn)
 *
 *  Para cada FX:
 *   - projecta pos para coordenadas de UI (pixels) via P*V
 *   - desenha um anel (ring) aproximado por segmentos (triângulos)
 *   - alpha decresce com o tempo
 *
 *  Além disso:
 *   - calcula um flash global (overlay) com alpha max entre todas as explosões
 *     para dar “impact/break feel”.
 */

 #include "game/render/ui/HudParts.hpp"

 #include "game/GameConfig.hpp"
 #include "game/GameState.hpp"
 
 #include <algorithm>
 #include <cmath>
 
 #include <glm/glm.hpp>
 #include <glm/gtc/constants.hpp>
 
 namespace game::render::hud {
 
 void drawFireballExplosionFx(const RenderContext& ctx, const GameState& state, const GameConfig& cfg) {
     // Se não há explosões activas, não há nada a desenhar.
     if (state.fireballExplosions.empty()) return;
 
     // -------------------------------------------------------------------------
     // Helper: desenhar um anel (ring) centrado em 'center' com raio 'radius'
     // e espessura 'thickness', usando triângulos.
     //
     // Estratégia:
     //  - dividimos o círculo em 'segs' fatias
     //  - para cada fatia, temos 4 pontos (outer/inner) e fazemos 2 triângulos
     // -------------------------------------------------------------------------
     auto drawRing = [&](const glm::vec2& center, float radius, float thickness, const glm::vec4& col) {
         const int segs = 56;
 
         float r0 = std::max(0.0f, radius - thickness * 0.5f); // inner radius
         float r1 = radius + thickness * 0.5f;                 // outer radius
 
         for (int i = 0; i < segs; ++i) {
             float a0 = (float)i / (float)segs * glm::two_pi<float>();
             float a1 = (float)(i + 1) / (float)segs * glm::two_pi<float>();
 
             glm::vec2 d0(std::cos(a0), std::sin(a0));
             glm::vec2 d1(std::cos(a1), std::sin(a1));
 
             glm::vec2 p0o = center + d0 * r1; // outer at angle a0
             glm::vec2 p0i = center + d0 * r0; // inner at angle a0
             glm::vec2 p1o = center + d1 * r1; // outer at angle a1
             glm::vec2 p1i = center + d1 * r0; // inner at angle a1
 
             // quad (p0o, p0i, p1o, p1i) -> 2 triângulos
             ctx.renderer.drawUITriangle(p0o, p0i, p1o, col);
             ctx.renderer.drawUITriangle(p0i, p1o, p1i, col);
         }
     };
 
     // duração do FX (evitar divisão por zero)
     float dur = std::max(0.001f, cfg.fireballExplosionFxDuration);
 
     // alpha do flash global: escolhemos o maior flash entre as explosões activas
     float flashA = 0.0f;
 
     for (const auto& fx : state.fireballExplosions) {
         // u = progresso normalizado 0..1
         float u = fx.t / dur;
         if (u < 0.0f) u = 0.0f;
         if (u > 1.0f) u = 1.0f;
 
         // flash: no início é mais forte e vai caindo com o tempo
         flashA = std::max(flashA, (1.0f - u) * cfg.fireballFlashMaxAlpha);
 
         // ---------------------------------------------------------------------
         // Projectar a posição do mundo para ecrã (UI pixels):
         // clip = P * V * world
         // ndc = clip / w
         //
         // Notas:
         //  - se w ~ 0: projecção inválida
         //  - se w <= 0: atrás da câmara -> ignorar
         // ---------------------------------------------------------------------
         glm::vec4 clip = ctx.P * ctx.V * glm::vec4(fx.pos, 1.0f);
         if (std::abs(clip.w) < 1e-6f) continue;
         if (clip.w <= 0.0f) continue;
 
         float ndcX = clip.x / clip.w;
         float ndcY = clip.y / clip.w;
 
         glm::vec2 cPx(
             (ndcX * 0.5f + 0.5f) * (float)ctx.fbW,
             (ndcY * 0.5f + 0.5f) * (float)ctx.fbH
         );
 
         // anel cresce com u (tempo), e vai ficando mais fino e mais transparente
         float radiusPx = 26.0f + 170.0f * u;
         float thickPx  = 18.0f - 10.0f * u;
         float a        = (1.0f - u) * 0.85f;
 
         glm::vec4 col(1.0f, 0.55f, 0.10f, a);
 
         if (a > 0.01f) drawRing(cPx, radiusPx, thickPx, col);
     }
 
     // -------------------------------------------------------------------------
     // Flash overlay global: cobre o ecrã inteiro com laranja (muito transparente),
     // dá sensação de “impact” e reforça a explosão.
     // -------------------------------------------------------------------------
     if (flashA > 0.01f) {
         ctx.renderer.drawUIQuad(
             0.0f, 0.0f,
             (float)ctx.fbW, (float)ctx.fbH,
             glm::vec4(1.0f, 0.55f, 0.10f, flashA)
         );
     }
 }
 
 } // namespace game::render::hud
 