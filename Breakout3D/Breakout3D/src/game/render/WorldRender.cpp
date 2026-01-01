/**
 * @file WorldRender.cpp
 * @brief Render do mundo 3D (arena, bricks, paddle, balls, powerups, etc).
 *
 * Convenções:
 * - O caller já setou a câmara (view/projection) antes de entrar aqui.
 * - "tint" base fica a branco para deixar texturas falarem.
 *
 * Destaques:
 * - Rails laterais estendidos apenas no sentido da câmara (para dar "pista").
 * - Bricks escolhem mesh consoante HP atual (assets *hit variants).
 * - Fireball tem trail barato sem histórico (offsets contra a velocidade).
 * - Powerups são meshes próprias, com tilt para a câmara + spin + bob.
 * - Alguns meshes recebem “correções” em render-space (ex: TINY virar barra).
 */
 #include "game/render/WorldRender.hpp"
 #include "game/GameAssets.hpp"
 #include "game/GameConfig.hpp"
 #include "game/GameState.hpp"
 #include "game/rogue/RogueCards.hpp"
 
 #include <algorithm>
 #include <cmath>
 #include <glm/glm.hpp>
 #include <glm/gtc/matrix_transform.hpp>
 
 namespace game::render {
 
 void renderWorld(const RenderContext& ctx, const GameState& state, const GameConfig& cfg, const GameAssets& assets) {
     // -------- 3D PASS content (camera já foi definida pelo caller) --------
     float arenaW = (cfg.arenaMaxX - cfg.arenaMinX);
 
     // Branco: não “pinta” a cena, só multiplica textura por 1.
     glm::vec3 tint(1.0f);
 
     // ---- Walls / Arena rails (extendidos na direção da câmara) ----
     float sideThickness = 1.2f;
     float topThickness  = 1.2f;
     float wallHeight    = 1.0f;
     float railLen       = 50.0f;
 
     float railZStart    = cfg.arenaMinZ - topThickness;
     float railZCenter   = railZStart + railLen * 0.5f;
 
     // Left Rail
     ctx.renderer.drawMesh(assets.brick01,
         glm::vec3(cfg.arenaMinX - sideThickness*0.5f, 0.0f, railZCenter),
         glm::vec3(sideThickness, wallHeight, railLen),
         tint
     );
 
     // Right Rail
     ctx.renderer.drawMesh(assets.brick01,
         glm::vec3(cfg.arenaMaxX + sideThickness*0.5f, 0.0f, railZCenter),
         glm::vec3(sideThickness, wallHeight, railLen),
         tint
     );
 
     // Top Border
     ctx.renderer.drawMesh(assets.brick01,
         glm::vec3(0.0f, 0.0f, cfg.arenaMinZ - topThickness*0.5f),
         glm::vec3(arenaW + sideThickness*2.0f, wallHeight, topThickness),
         tint
     );
 
     // ---- Bricks: escolher mesh consoante hp/maxHp (hit variants) ----
     auto pickBrickMesh = [&](int maxHp, int hp) -> const engine::Mesh* {
         const engine::Mesh* m = &assets.brick01;
 
         if (maxHp == 4) {
             if (hp == 4)      m = &assets.brick04;
             else if (hp == 3) m = &assets.brick04_1hit;
             else if (hp == 2) m = &assets.brick04_2hit;
             else              m = &assets.brick04_3hit;
         } else if (maxHp == 3) {
             if (hp == 3)      m = &assets.brick03;
             else if (hp == 2) m = &assets.brick03_1hit;
             else              m = &assets.brick03_2hit;
         } else if (maxHp == 2) {
             if (hp == 2)      m = &assets.brick02;
             else              m = &assets.brick02_1hit;
         } else {
             m = &assets.brick01;
         }
         return m;
     };
 
     for (const auto& b : state.bricks) {
         if (!b.alive) continue;
         const engine::Mesh* m = pickBrickMesh(b.maxHp, b.hp);
         ctx.renderer.drawMesh(*m, b.pos, b.size, tint);
     }
 
     // ---- Fireball debris shards (visual feel de "break") ----
     if (!state.fireballShards.empty()) {
         for (const auto& s : state.fireballShards) {
             float u = (cfg.fireballShardLife > 1e-4f)
                 ? std::min(1.0f, std::max(0.0f, s.t / cfg.fireballShardLife))
                 : 1.0f;
             float k = 1.0f - u;
 
             glm::vec3 col = glm::mix(glm::vec3(1.0f, 0.55f, 0.15f), glm::vec3(0.15f, 0.08f, 0.03f), u);
             glm::vec3 sz(0.30f * k, 0.18f * k, 0.22f * k);
 
             ctx.renderer.drawMesh(assets.brick01, s.pos, sz, col);
         }
     }
 
     // ---- Paddle (com escalas de Rogue/Expand/Tiny) ----
     glm::vec3 currentPaddleSize = cfg.paddleSize;
     if (state.gameType == GameType::ROGUE) currentPaddleSize.x *= game::rogue::basePaddleScaleX(state);
     if (state.expandTimer > 0.0f) currentPaddleSize.x *= cfg.expandScaleFactor;
     if (state.tinyTimer > 0.0f) currentPaddleSize.x *= cfg.tinyScaleFactor;
 
     ctx.renderer.drawMesh(assets.paddle, state.paddlePos, currentPaddleSize, tint);
 
     // ---- Shield barrier (atrás do paddle) ----
     if (state.shieldTimer > 0.0f) {
         float barrierZ = state.paddlePos.z + cfg.shieldOffsetZ;
         barrierZ = std::min(barrierZ, 19.0f);
 
         glm::vec3 barrierPos(0.0f, 0.0f, barrierZ);
         glm::vec3 barrierSize((cfg.arenaMaxX - cfg.arenaMinX) * 1.10f, 1.0f, 0.30f);
 
         ctx.renderer.drawMesh(assets.shield, barrierPos, barrierSize, glm::vec3(0.25f, 0.90f, 1.00f));
     }
 
     // ---- Balls (fireball vs normal) ----
     float ballD = cfg.ballRadius * 2.0f;
 
     for (const auto& b : state.balls) {
         if (b.isFireball) {
             glm::vec3 fireTint(1.00f, 0.55f, 0.15f);
             ctx.renderer.drawMesh(assets.fireball, b.pos, glm::vec3(ballD), fireTint);
 
             // Trail barato: N cópias no sentido oposto da velocidade
             float speed2 = b.vel.x * b.vel.x + b.vel.z * b.vel.z;
             if (!b.attached && speed2 > 1e-6f) {
                 glm::vec3 dir = glm::normalize(glm::vec3(b.vel.x, 0.0f, b.vel.z));
                 const int trailCount = 6;
                 const float spacing = 0.55f;
 
                 for (int i = 1; i <= trailCount; ++i) {
                     float u = (float)i / (float)(trailCount + 1);
                     glm::vec3 tp = b.pos - dir * (spacing * (float)i);
                     float s = ballD * (0.90f - 0.55f * u);
                     glm::vec3 tcol(1.0f, 0.55f + 0.25f * (1.0f - u), 0.20f);
                     ctx.renderer.drawMesh(assets.fireball, tp, glm::vec3(s), tcol);
                 }
             }
         } else {
             ctx.renderer.drawMesh(assets.ball, b.pos, glm::vec3(ballD), tint);
         }
     }
 
     // ---- Powerups: mesh + tilt para a câmara + spin + bob ----
     float powerUpSpin = ctx.time.now() * 2.5f;
     float powerUpTilt = glm::radians(123.0f);                // “standing + tilt” para ler melhor
     float powerUpBob  = std::sin(ctx.time.now() * 4.0f) * 0.25f;
 
     for (const auto& p : state.powerups) {
         const engine::Mesh* m = &assets.ball;
 
         if (p.type == PowerUpType::EXPAND)           m = &assets.expand;
         else if (p.type == PowerUpType::EXTRA_BALL)  m = &assets.extraBall;
         else if (p.type == PowerUpType::SLOW)        m = &assets.slow;
         else if (p.type == PowerUpType::EXTRA_LIFE)  m = &assets.extraLife;
         else if (p.type == PowerUpType::FIREBALL)    m = &assets.fireball;
         else if (p.type == PowerUpType::SHIELD)      m = &assets.shield;
         else if (p.type == PowerUpType::REVERSE)     m = &assets.skull;
         else if (p.type == PowerUpType::TINY)        m = &assets.minus;
 
         glm::mat4 M(1.0f);
         M = glm::translate(M, p.pos + glm::vec3(0, 0.5f + powerUpBob, 0));
         M = glm::rotate(M, powerUpTilt, glm::vec3(1, 0, 0));
 
         // Snail: roda mais 90º no Y para mostrar perfil.
         float rotation = powerUpSpin;
         if (p.type == PowerUpType::SLOW) rotation += glm::radians(90.0f);
 
         M = glm::rotate(M, rotation, glm::vec3(0, 1, 0));
 
         // Heart: flip 180º no X
         if (p.type == PowerUpType::EXTRA_LIFE) {
             M = glm::rotate(M, glm::radians(180.0f), glm::vec3(1, 0, 0));
         }
 
         M = glm::scale(M, glm::vec3(cfg.powerUpVisualScale));
 
         // Minus: “escultura” para virar barra e não cubo.
         if (p.type == PowerUpType::TINY) {
             M = glm::scale(M, glm::vec3(2.35f, 0.22f, 0.60f));
         }
 
         // Cores por tipo (boa leitura e linguagem visual)
         glm::vec3 col = tint;
 
         // Good
         if (p.type == PowerUpType::EXPAND)      col = glm::vec3(0.25f, 0.95f, 0.25f);
         if (p.type == PowerUpType::EXTRA_BALL)  col = glm::vec3(0.15f, 0.70f, 1.00f);
         if (p.type == PowerUpType::EXTRA_LIFE)  col = glm::vec3(1.00f, 0.18f, 0.22f);
         if (p.type == PowerUpType::FIREBALL)    col = glm::vec3(1.00f, 0.55f, 0.15f);
         if (p.type == PowerUpType::SHIELD)      col = glm::vec3(0.20f, 0.95f, 1.00f);
 
         // Bad / curses
         if (p.type == PowerUpType::SLOW)        col = glm::vec3(0.55f, 0.38f, 0.22f);
         if (p.type == PowerUpType::REVERSE)     col = glm::vec3(0.95f, 0.20f, 0.90f);
         if (p.type == PowerUpType::TINY)        col = glm::vec3(1.00f, 0.85f, 0.10f);
 
         ctx.renderer.drawMesh(*m, M, col);
     }
 }
 
 } // namespace game::render
 