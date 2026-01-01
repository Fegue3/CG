/**
 * @file CollisionSystem.cpp
 * @brief Colisões principais do jogo (paredes, paddle, bricks).
 *
 * Resumo:
 *  - handleWorldCollisions(): bounces nas paredes (X e Z “top”).
 *  - handlePaddleCollision(): colisão bola-paddle em XZ, com bounce + ângulo.
 *    Inclui regra Rogue: "sticky paddle" (cola a bola ao bater na frente).
 *  - handleBrickCollisions(): colisões com bricks:
 *    - normal hit: dano (com bonus Rogue), score/bank, drops
 *    - fireball hit: explosão AoE, shards, ring FX, e mata a bola de fireball
 */
 #include "game/systems/CollisionSystem.hpp"
 #include "game/entities/Brick.hpp"
 #include "game/systems/PowerUpSystem.hpp"
 #include "game/rogue/RogueCards.hpp"
 
 #include <algorithm>
 #include <cmath>
 #include <glm/glm.hpp>
 #include <glm/gtc/constants.hpp>
 
 static float clampf(float v, float a, float b) {
     return std::max(a, std::min(v, b));
 }
 
 /**
  * @brief Teste de interseção esfera vs AABB no plano XZ.
  * - Ignora Y (o jogo é “2.5D” em termos de colisão para bola/bricks/paddle).
  */
 static bool sphereAabbXZ(const glm::vec3& c, float r,
                          const glm::vec3& bpos, const glm::vec3& bsize) {
     float hx = bsize.x * 0.5f;
     float hz = bsize.z * 0.5f;
 
     float minX = bpos.x - hx, maxX = bpos.x + hx;
     float minZ = bpos.z - hz, maxZ = bpos.z + hz;
 
     float cx = clampf(c.x, minX, maxX);
     float cz = clampf(c.z, minZ, maxZ);
 
     float dx = c.x - cx;
     float dz = c.z - cz;
     return (dx * dx + dz * dz) <= (r * r);
 }
 
 namespace game {
 
 bool CollisionSystem::handleWorldCollisions(Ball& ball, const GameConfig& cfg) {
     bool hit = false;
 
     // Left wall
     if (ball.pos.x - cfg.ballRadius < cfg.arenaMinX) {
         ball.pos.x = cfg.arenaMinX + cfg.ballRadius;
         ball.vel.x = std::abs(ball.vel.x);
         hit = true;
     }
     // Right wall
     if (ball.pos.x + cfg.ballRadius > cfg.arenaMaxX) {
         ball.pos.x = cfg.arenaMaxX - cfg.ballRadius;
         ball.vel.x = -std::abs(ball.vel.x);
         hit = true;
     }
     // Top wall (front wall)
     if (ball.pos.z - cfg.ballRadius < cfg.arenaMinZ) {
         ball.pos.z = cfg.arenaMinZ + cfg.ballRadius;
         ball.vel.z = std::abs(ball.vel.z);
         hit = true;
     }
 
     return hit;
 }
 
 bool CollisionSystem::handlePaddleCollision(Ball& ball, const GameState& state,
                                             const glm::vec3& paddlePos,
                                             const glm::vec3& paddleSize,
                                             const GameConfig& cfg) {
     // AABB do paddle (XZ)
     float halfZ = paddleSize.z * 0.5f;
     float paddleHalfX = paddleSize.x * 0.5f;
     float minX = paddlePos.x - paddleHalfX;
     float maxX = paddlePos.x + paddleHalfX;
     float minZ = paddlePos.z - halfZ;
     float maxZ = paddlePos.z + halfZ;
 
     // ponto mais próximo no AABB
     float cx = clampf(ball.pos.x, minX, maxX);
     float cz = clampf(ball.pos.z, minZ, maxZ);
 
     float dx = ball.pos.x - cx;
     float dz = ball.pos.z - cz;
 
     // hit sphere vs AABB
     if (dx*dx + dz*dz <= cfg.ballRadius*cfg.ballRadius) {
         // Resolve por eixo dominante (para evitar “stuck” em cantos)
         float ax = std::abs(dx);
         float az = std::abs(dz);
 
         float speed = glm::length(glm::vec2(ball.vel.x, ball.vel.z));
         if (speed < 1e-4f) speed = cfg.ballSpeed;
 
         // Tende a resolver em Z (mais “breakout feel”), salvo quando X é muito maior
         bool resolveX = (ax > az * 1.05f);
 
         if (resolveX) {
             // bateu num lado do paddle -> inverte X
             ball.vel.x = -ball.vel.x;
             float sideSign = (ball.pos.x < cx) ? -1.0f : 1.0f;
             ball.pos.x = cx + sideSign * (cfg.ballRadius + 0.002f);
         } else {
             // bateu na frente/atrás -> inverte Z + “angle control” pelo hitX
             bool frontSide = (ball.pos.z < paddlePos.z);
 
             // Rogue: sticky paddle (só na frente, só se a bola vem em direção ao paddle, e não em fireball)
             if (frontSide && ball.vel.z > 0.0f &&
                 state.gameType == GameType::ROGUE && state.rogueStickyPaddle && !ball.isFireball) {
                 ball.attached = true;
                 ball.vel = glm::vec3(0.0f);
                 ball.pos.z = minZ - cfg.ballRadius - 0.002f;
                 return true;
             }
 
             float t = (ball.pos.x - paddlePos.x) / std::max(0.001f, paddleHalfX);
             t = clampf(t, -1.0f, 1.0f);
             float ang = t * glm::radians(60.0f);
 
             ball.vel.x = std::sin(ang) * speed;
             ball.vel.z = (frontSide ? -std::cos(ang) : std::cos(ang)) * speed;
 
             ball.pos.z = (frontSide ? (minZ - cfg.ballRadius - 0.002f)
                                     : (maxZ + cfg.ballRadius + 0.002f));
         }
 
         return true;
     }
 
     return false;
 }
 
 CollisionSystem::BrickCollisionInfo CollisionSystem::handleBrickCollisions(Ball& ball, GameState& state, const GameConfig& cfg) {
     BrickCollisionInfo info;
 
     // pequeno cooldown para evitar multi-hits no mesmo frame/instante
     if (state.brickHitCooldown > 0.0f) {
         return info;
     }
 
     for (auto& br : state.bricks) {
         if (!br.alive) continue;
 
         if (sphereAabbXZ(ball.pos, cfg.ballRadius, br.pos, br.size)) {
             info.hit = true;
             const bool fireballActive = ball.isFireball;
             info.fireball = fireballActive;
             info.hpBefore = br.hp;
             info.maxHp = br.maxHp;
 
             // --- Helpers de scoring ---
             auto brickPoints = [&](int hp) -> int {
                 // hp 1..6 (endless pode ir até 6)
                 switch (hp) {
                     case 1: return 50;
                     case 2: return 120;
                     case 3: return 220;
                     case 4: return 350;
                     case 5: return 500;
                     default: return 700;
                 }
             };
 
             auto awardBrickPoints = [&](int maxHp, bool immediateScore) -> int {
                 int baseScore = brickPoints(maxHp);
                 int waveBonus = (state.gameType == GameType::ENDLESS || state.gameType == GameType::ROGUE)
                                 ? (state.wave * 25) : 0;
 
                 int pts = baseScore + waveBonus;
 
                 if (state.gameType == GameType::ROGUE) {
                     pts = (int)std::round((float)pts * state.rogueBrickPointsMult);
                 }
 
                 // Endless/Rogue: vai para streak bank (a não ser hits “imediatos” tipo fireball)
                 if (state.gameType == GameType::ENDLESS || state.gameType == GameType::ROGUE) {
                     if (immediateScore) {
                         state.score += pts;
                         return pts;
                     }
 
                     state.endlessStreakPoints += pts;
                     state.endlessStreakPosPoints += pts;
                     state.endlessStreakIdleTimer = 0.0f;
                     state.endlessStreakBanking = false;
                     state.endlessStreakBankTimer = 0.0f;
 
                     if (state.gameType == GameType::ENDLESS) {
                         // lógica de spawn por “quota” de bricks destruídos
                         state.bricksDestroyedThisWave++;
 
                         const float t = state.endlessElapsedTime;
                         float u = (t <= 120.0f) ? 0.0f : std::min(1.0f, (t - 120.0f) / 480.0f); // 2min->10min
                         int required = (int)std::round(22.0f - (7.0f * u)); // 22 -> 15
                         required = std::max(15, std::min(22, required));
 
                         if (state.bricksDestroyedThisWave >= required) {
                             state.pendingSpawnBricks += 12;
                             state.bricksDestroyedThisWave -= required;
                         }
                     }
                 } else {
                     // Normal/Levels: score direto
                     state.score += pts;
                 }
 
                 return pts;
             };
 
             // Mata brick + drop + progress + anchor de VFX
             auto killBrick = [&](Brick& b, bool allowPowerupDrop) -> int {
                 if (!b.alive) return 0;
                 b.alive = false;
                 info.bricksKilled += 1;
 
                 // âncora de VFX (último brick partido)
                 state.lastBrickDestroyedValid = true;
                 state.lastBrickDestroyedPos = b.pos;
 
                 // Rogue: quota por wave
                 if (state.gameType == GameType::ROGUE) {
                     state.rogueBricksBrokenThisWave++;
                 }
 
                 // Drops (com base Rogue diferente)
                 if (allowPowerupDrop) {
                     float chance = cfg.powerUpChance;
                     if (state.gameType == GameType::ROGUE) {
                         chance = game::rogue::effectiveDropChance(state, cfg);
                     }
                     PowerUpSystem::spawnPowerUp(state, b.pos, chance);
                 }
 
                 // fireball mete pontos imediatamente (para o popup e feedback)
                 return awardBrickPoints(b.maxHp, fireballActive);
             };
 
             // -------- FIREBALL PATH (AoE + mata a bola) --------
             if (fireballActive) {
                 int explosionPts = 0;
 
                 float r = cfg.fireballExplosionRadius;
                 if (state.gameType == GameType::ROGUE) {
                     r *= std::max(0.25f, state.rogueFireballRadiusMult);
                 }
                 float r2 = r * r;
 
                 // mata vizinhos dentro do raio
                 for (auto& other : state.bricks) {
                     if (!other.alive) continue;
                     float dx = other.pos.x - br.pos.x;
                     float dz = other.pos.z - br.pos.z;
                     if ((dx * dx + dz * dz) <= r2) {
                         // só o brick diretamente atingido pode dar drop (anti-flood)
                         bool allowDrop = (&other == &br);
                         int got = killBrick(other, allowDrop);
                         if (got > 0) explosionPts += got;
                     }
                 }
 
                 // Normal: se foi o último brick, guarda “snapshot” para o finisher
                 if (state.gameType == GameType::NORMAL && state.mode == GameMode::PLAYING) {
                     bool anyAlive = false;
                     for (const auto& b : state.bricks) {
                         if (b.alive) { anyAlive = true; break; }
                     }
                     if (!anyAlive) {
                         state.winFinisherHoldBrickValid = true;
                         state.winFinisherHoldBrickPos = br.pos;
                         state.winFinisherHoldBrickSize = br.size;
                         state.winFinisherHoldBrickMaxHp = br.maxHp;
                         state.winFinisherHoldBrickHp = 1;
                     }
                 }
 
                 // FX: ring + popup (endless)
                 state.fireballExplosions.push_back({ br.pos, 0.0f });
                 if (explosionPts > 0) {
                     if (state.gameType == GameType::ENDLESS) {
                         state.scorePopups.push_back({ explosionPts, 0.0f });
                     }
                 }
 
                 // FX: camera shake + shards
                 state.fireballShakeTimer = cfg.fireballShakeDuration;
                 state.fireballShakeAnchorPos = br.pos;
                 {
                     auto frand = []() -> float { return (float)rand() / (float)RAND_MAX; };
                     for (int i = 0; i < cfg.fireballShardCount; ++i) {
                         float a = frand() * glm::two_pi<float>();
                         float rr = 0.15f + frand() * 0.55f;
                         glm::vec3 p = br.pos + glm::vec3(std::cos(a) * rr,
                                                          0.12f + frand() * 0.18f,
                                                          std::sin(a) * rr);
 
                         float dirA = frand() * glm::two_pi<float>();
                         float sp = cfg.fireballShardSpeed * (0.65f + frand() * 0.55f);
                         glm::vec3 v(std::cos(dirA) * sp,
                                    cfg.fireballShardUp * (0.65f + frand() * 0.55f),
                                    std::sin(dirA) * sp);
 
                         GameState::FireballShard s;
                         s.pos = p;
                         s.vel = v;
                         s.t = 0.0f;
                         state.fireballShards.push_back(s);
                     }
                 }
 
                 // Fireball é one-shot: apaga a bola e sinaliza respawn
                 ball.alive = false;
                 ball.isFireball = false;
                 ball.vel = glm::vec3(0.0f);
                 state.pendingRespawnAfterFireball = true;
 
                 state.brickHitCooldown = 0.045f;
                 info.broke = (info.bricksKilled > 0);
                 info.hpAfter = 0;
                 return info;
             }
 
             // -------- NORMAL PATH (dano + bounce) --------
             {
                 int dmg = 1;
                 if (state.gameType == GameType::ROGUE) {
                     dmg += std::max(0, state.rogueBrickDamageBonus);
                 }
 
                 br.hp -= dmg;
 
                 if (br.hp <= 0) {
                     // Normal: se é o último brick, snapshot para finisher slow-down
                     if (state.gameType == GameType::NORMAL && state.mode == GameMode::PLAYING) {
                         bool anyOtherAlive = false;
                         for (const auto& other : state.bricks) {
                             if (&other == &br) continue;
                             if (other.alive) { anyOtherAlive = true; break; }
                         }
                         if (!anyOtherAlive) {
                             state.winFinisherHoldBrickValid = true;
                             state.winFinisherHoldBrickPos = br.pos;
                             state.winFinisherHoldBrickSize = br.size;
                             state.winFinisherHoldBrickMaxHp = br.maxHp;
                             state.winFinisherHoldBrickHp = 1;
                         }
                     }
 
                     (void)killBrick(br, true);
                     info.broke = true;
                 }
 
                 info.hpAfter = std::max(0, br.hp);
 
                 if (!info.broke) {
                     info.damaged = true;
                     if (info.hpBefore > 1 && info.hpAfter == 1) info.cracked = true;
                 }
             }
 
             // Bounce (eixo dominante vs o centro do brick)
             glm::vec3 diff = ball.pos - br.pos;
             float ax = std::abs(diff.x) / (br.size.x * 0.5f);
             float az = std::abs(diff.z) / (br.size.z * 0.5f);
 
             if (ax > az) {
                 ball.vel.x = -ball.vel.x;
                 float sign = (diff.x >= 0.0f) ? 1.0f : -1.0f;
                 ball.pos.x = br.pos.x + sign * (br.size.x * 0.5f + cfg.ballRadius + 0.002f);
             } else {
                 ball.vel.z = -ball.vel.z;
                 float sign = (diff.z >= 0.0f) ? 1.0f : -1.0f;
                 ball.pos.z = br.pos.z + sign * (br.size.z * 0.5f + cfg.ballRadius + 0.002f);
             }
 
             state.brickHitCooldown = 0.045f;
             return info;
         }
     }
 
     return info;
 }
 
 } // namespace game
 