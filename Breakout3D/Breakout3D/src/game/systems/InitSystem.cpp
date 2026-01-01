/**
 * @file InitSystem.cpp
 * @brief Inicialização do jogo e geração de bricks por modo.
 *
 * Responsabilidades:
 *  - initGame(): reset global de state (vidas, score, timers, powerups, finisher, etc)
 *  - resetBallAndPaddle(): coloca paddle e cria a bola inicial colada ao paddle
 *  - generateBricks(): geração procedural (Normal / Endless / Rogue)
 *  - spawnIncrementalBricks(): spawn incremental (Endless) + push das rows existentes
 *  - generateBricksFromLevel(): Levels mode -> converte layout grid para world coords
 */
 #include "game/systems/InitSystem.hpp"
 #include "game/entities/Brick.hpp"
 #include "game/entities/Ball.hpp"
 #include "game/systems/PhysicsSystem.hpp"
 #include "game/LevelLayouts.hpp"
 
 #include <cstdlib>
 #include <ctime>
 #include <algorithm>
 #include <fstream>
 #include <string>
 
 #include "game/systems/RogueSystem.hpp"
 
 namespace game {
 
 // Weighted base HP selection para bricks “random”
 static int pickWeightedBaseHp_1to4() {
     int r = rand() % 100;
     if (r < 30) return 1;
     if (r < 55) return 2;
     if (r < 80) return 3;
     return 4;
 }
 
 // path do best score do Endless
 static std::string endlessBestScorePath() {
     const char* home = std::getenv("HOME");
     if (home && *home) {
         return std::string(home) + "/.breakout3d_endless_best.txt";
     }
     return "breakout3d_endless_best.txt";
 }
 
 static int loadEndlessBestScore() {
     std::ifstream f(endlessBestScorePath());
     int best = 0;
     if (f >> best) return std::max(0, best);
     return 0;
 }
 
 bool InitSystem::anyBricksAlive(const GameState& state) {
     for (const auto& b : state.bricks) {
         if (b.alive) return true;
     }
     return false;
 }
 
 void InitSystem::resetBallAndPaddle(GameState& state, const GameConfig& cfg) {
     // paddle no fundo do campo (perto de arenaMaxZ)
     state.paddlePos = glm::vec3(
         0.0f, 0.0f,
         cfg.arenaMaxZ - (cfg.paddleSize.z * 0.5f) - 0.25f
     );
 
     // 1 bola inicial colada ao paddle
     state.balls.clear();
     Ball firstBall;
     PhysicsSystem::resetBallToPaddle(firstBall, state.paddlePos, cfg);
     state.balls.push_back(firstBall);
 }
 
 void InitSystem::initGame(GameState& state, const GameConfig& cfg) {
     // seed RNG
     srand(static_cast<unsigned int>(time(nullptr)));
 
     // base state
     state.mode = GameMode::PLAYING;
     state.lives = 3;
 
     // endless bookkeeping
     state.bricksDestroyedThisWave = 0;
     state.endlessRowsSpawned = 0;
     state.score = 0;
     state.endlessBestScore = (state.gameType == GameType::ENDLESS) ? loadEndlessBestScore() : 0;
     state.endlessStreakPoints = 0;
     state.endlessStreakPosPoints = 0;
     state.endlessStreakNegPoints = 0;
     state.endlessStreakIdleTimer = 0.0f;
     state.endlessStreakBanking = false;
     state.endlessStreakBankTimer = 0.0f;
     state.endlessDangerActive = false;
     state.endlessDangerTimer = 0.0f;
     state.endlessElapsedTime = 0.0f;
 
     // finisher/transientes
     state.winFinisherActive = false;
     state.winFinisherTimer = 0.0f;
     state.winFinisherRealTimer = 0.0f;
     state.lastBrickDestroyedValid = false;
     state.winFinisherAnchorValid = false;
     state.winFinisherHoldBrickValid = false;
 
     // wave: normal reset para 1 (endless/rogue pode vir pre-set do caller)
     if (state.gameType == GameType::NORMAL) {
         state.wave = 1;
     }
 
     resetBallAndPaddle(state, cfg);
 
     // powerups/timers
     state.powerups.clear();
     state.expandTimer = 0.0f;
     state.slowTimer = 0.0f;
     state.fireballTimer = 0.0f;
     state.shieldTimer = 0.0f;
     state.reverseTimer = 0.0f;
     state.tinyTimer = 0.0f;
 
     // fireball FX + score popups + cooldowns
     state.fireballExplosions.clear();
     state.fireballShards.clear();
     state.fireballShakeTimer = 0.0f;
     state.fireballShakeAnchorPos = glm::vec3(0.0f);
     state.scorePopups.clear();
     state.pendingRespawnAfterFireball = false;
     state.brickHitCooldown = 0.0f;
 
     // ---------- Generate bricks ----------
     if (state.gameType == GameType::LEVELS) {
         // Levels: layout fixo (1..20)
         generateBricksFromLevel(state, cfg, state.currentLevel);
     } else {
         // Procedural para outros modos
         int waveToGenerate = (state.gameType == GameType::ENDLESS || state.gameType == GameType::ROGUE) ? state.wave : 0;
 
         int rowsInitial = 9;
         if (state.gameType == GameType::ENDLESS) {
             rowsInitial = 9 + (state.wave / 2);
         }
 
         generateBricks(state, cfg, waveToGenerate);
 
         if (state.gameType == GameType::ENDLESS) {
             state.endlessRowsSpawned = rowsInitial;
         }
     }
 
     // Rogue: começa sempre com draft antes da wave 1
     if (state.gameType == GameType::ROGUE) {
         RogueSystem::startRun(state);
     }
 }
 
 void InitSystem::generateBricks(GameState& state, const GameConfig& cfg, int waveNumber) {
     state.bricks.clear();
 
     const int cols = 12;
     int rows = 9; // default
 
     // endless/rogue: cresce rows com wave
     if (waveNumber > 0) {
         rows = 9 + (waveNumber / 2);
     }
 
     // normal preset
     if (waveNumber == 0) {
         rows = 6;
     }
 
     glm::vec3 brickSize(2.95f, 0.7f, 1.30f);
     float gapX = 0.04f;
     float gapZ = 0.03f;
 
     float startZ = cfg.arenaMinZ + 0.85f;
     float totalW = cols * brickSize.x + (cols - 1) * gapX;
     float leftX  = -totalW * 0.5f + brickSize.x * 0.5f;
 
     // debug: 1 brick only
     if (state.testOneBrick) {
         Brick b;
         b.size = brickSize;
         b.alive = true;
         b.pos.x = 0.0f;
         b.pos.y = 0.0f;
         b.pos.z = startZ + 4.0f * (brickSize.z + gapZ);
         b.maxHp = b.hp = 1;
         state.bricks.push_back(b);
         return;
     }
 
     for (int r = 0; r < rows; ++r) {
         for (int c = 0; c < cols; ++c) {
             Brick b;
             b.size = brickSize;
             b.alive = true;
 
             b.pos.x = leftX + c * (brickSize.x + gapX);
             b.pos.y = 0.0f;
             b.pos.z = startZ + r * (brickSize.z + gapZ);
 
             // Normal preset: de longe->perto (farthest = purple, nearest = greens)
             if (waveNumber == 0) {
                 int hp = 1;
                 if (r >= 3) hp = 1;
                 else if (r == 2) hp = 2;
                 else if (r == 1) hp = 3;
                 else hp = 4;
 
                 b.maxHp = b.hp = hp;
                 state.bricks.push_back(b);
                 continue;
             }
 
             // front half (perto do paddle) vs back half
             bool isFrontHalf = (r >= rows / 2);
 
             if (state.gameType == GameType::ROGUE && waveNumber > 0) {
                 // Rogue: curva de HP vem do RogueSystem
                 int hp = RogueSystem::pickBrickHpForWave(waveNumber, isFrontHalf);
                 b.maxHp = b.hp = std::min(6, std::max(1, hp));
             } else {
                 if (isFrontHalf) {
                     // front: mais fácil
                     int r2 = rand() % 100;
                     if (r2 < 45)      b.maxHp = b.hp = 1;
                     else if (r2 < 85) b.maxHp = b.hp = 2;
                     else              b.maxHp = b.hp = 3;
                 } else {
                     // back: mais pesado e escala com wave
                     if (waveNumber == 0) {
                         b.maxHp = b.hp = pickWeightedBaseHp_1to4();
                     } else {
                         int hpBonus = waveNumber / 5;
                         int baseHp = pickWeightedBaseHp_1to4();
                         b.maxHp = b.hp = std::min(6, baseHp + hpBonus);
                     }
                 }
             }
 
             state.bricks.push_back(b);
         }
     }
 }
 
 void InitSystem::spawnIncrementalBricks(GameState& state, const GameConfig& cfg, int count, int waveNumber) {
     (void)waveNumber; // atualmente não usado aqui (fica para tuning futuro)
 
     glm::vec3 brickSize(2.95f, 0.7f, 1.30f);
     float gapX = 0.04f;
     float gapZ = 0.03f;
 
     const int cols = 12;
     float totalW = cols * brickSize.x + (cols - 1) * gapX;
     float leftX = -totalW * 0.5f + brickSize.x * 0.5f;
     float stepZ = brickSize.z + gapZ;
     float topZ = cfg.arenaMinZ + 0.85f;
 
     // quantas rows novas precisamos inserir
     int rowsToInsert = (count + cols - 1) / cols;
     float push = rowsToInsert * stepZ;
 
     // empurra bricks existentes “para baixo” (em direção ao paddle)
     for (auto& br : state.bricks) {
         if (!br.alive) continue;
         br.pos.z += push;
     }
 
     int totalRowsBefore = state.endlessRowsSpawned;
 
     // cria bricks na zona do topo
     for (int i = 0; i < count; ++i) {
         Brick b;
         b.size = brickSize;
         b.alive = true;
 
         int col = i % cols;
         int rowLocal = (i / cols);
         int rowGlobal = totalRowsBefore + rowLocal;
 
         b.pos.x = leftX + col * (brickSize.x + gapX);
         b.pos.y = 0.0f;
         b.pos.z = topZ + rowLocal * stepZ;
 
         int baseHp = pickWeightedBaseHp_1to4();
         int difficultyBonus = (rowGlobal / 20);
         b.maxHp = b.hp = std::min(6, baseHp + difficultyBonus);
 
         state.bricks.push_back(b);
     }
 
     state.endlessRowsSpawned = totalRowsBefore + rowsToInsert;
 }
 
 void InitSystem::generateBricksFromLevel(GameState& state, const GameConfig& cfg, int levelNumber) {
     state.bricks.clear();
 
     auto levels = getAllLevels();
     if (levelNumber < 1 || levelNumber > (int)levels.size()) {
         levelNumber = 1;
     }
 
     const LevelLayout& layout = levels[levelNumber - 1];
 
     // Conversão de grid->world
     float brickW = 2.2f;
     float brickH = 1.0f;
     float gapX = 0.15f;
     float gapZ = 0.15f;
 
     float totalGridWidth = 12 * (brickW + gapX);
     float startX = -totalGridWidth * 0.5f;
     float startZ = cfg.arenaMinZ + 2.0f;
 
     for (const auto& data : layout.bricks) {
         Brick b;
         b.pos.x = startX + data.gridX * (brickW + gapX);
         b.pos.y = 0.0f;
         b.pos.z = startZ + data.gridZ * (brickH + gapZ);
         b.size = glm::vec3(brickW, 0.6f, brickH);
         b.hp = b.maxHp = data.hp;
         b.alive = true;
         state.bricks.push_back(b);
     }
 }
 
 } // namespace game
 