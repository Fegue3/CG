/**
 * @file RogueSystem.cpp
 * @brief Sistema de run no Rogue: start, progressão de waves, spawn de rows, best score.
 *
 * Conceitos:
 *  - Wave tem quota de bricks (bricksRequiredForWave) + temporizadores (minTime/timeLimit).
 *  - A cada avanço de wave, insere rows novas no “back” (arenaMinZ), empurrando as existentes.
 *  - Dificuldade de HP por wave: pickBrickHpForWave (front half tende a ser mais fácil).
 *  - Cartas/Offers: delega para módulo RogueCards (packs normais vs OP).
 *  - Best score: guarda em ~/.breakout3d_rogue_best.txt
 */
 #include "game/systems/RogueSystem.hpp"
 #include "game/entities/Brick.hpp"
 #include "game/rogue/RogueCards.hpp"
 
 #include <fstream>
 #include <string>
 
 #include <algorithm>
 
 namespace game {
 
 static std::string rogueBestScorePath() {
     const char* home = std::getenv("HOME");
     if (home && *home) {
         return std::string(home) + "/.breakout3d_rogue_best.txt";
     }
     return "breakout3d_rogue_best.txt";
 }
 
 static void saveRogueBestScore(int best) {
     std::ofstream f(rogueBestScorePath(), std::ios::trunc);
     if (!f) return;
     f << best;
 }
 
 void RogueSystem::startRun(GameState& state) {
     state.wave = std::max(1, state.wave);
 
     // reset de estado de run/wave
     state.hoveredRogueCard = -1;
     state.hoveredRogueCardPickButton = -1;
     state.rogueBricksBrokenThisWave = 0;
     state.rogueWaveTimer = 0.0f;
     state.rogueWaveCooldown = 0.0f;
     state.rogueRowsSpawned = 0;
     state.roguePendingRowsToSpawn = 0;
     state.rogueRowSpawnTimer = 0.0f;
 
     // best score persistente
     state.rogueBestScore = loadRogueBestScore();
 
     // Init pools/modifiers + start draft inicial (3 rondas)
     game::rogue::initRunPools(state);
     game::rogue::startInitialDraft(state);
 }
 
 void RogueSystem::setCardOptions(GameState& state) {
     // Mantido por consistência (Game.cpp é que decide na prática).
     int nextWave = std::max(1, state.wave);
     bool opPack = (nextWave % 3) == 0 && nextWave >= 3;
     game::rogue::dealOffer(state, 3, opPack, nextWave);
 }
 
 void RogueSystem::onCardPicked(GameState& state, PowerUpType chosen) {
     // Legacy (não usado)
     (void)state; (void)chosen;
 }
 
 bool RogueSystem::shouldOfferCardsAfterClearingWave(const GameState& state, int clearedWave) {
     if (state.rogueRewardEvery <= 0) return false;
     return (clearedWave % state.rogueRewardEvery) == 0;
 }
 
 bool RogueSystem::shouldWinAfterClearingWave(const GameState& state, int clearedWave) {
     return (state.rogueMaxWaves > 0) && (clearedWave >= state.rogueMaxWaves);
 }
 
 int RogueSystem::pickBrickHpForWave(int wave, bool isFrontHalf) {
     wave = std::max(1, wave);
 
     // Wave 1: 1-2 HP
     if (wave == 1) {
         int r = rand() % 100;
         if (isFrontHalf) return (r < 70) ? 1 : 2;
         return (r < 45) ? 1 : 2;
     }
 
     // Wave 2-3: adiciona 3 HP às vezes
     if (wave <= 3) {
         int r = rand() % 100;
         if (isFrontHalf) {
             if (r < 55) return 1;
             if (r < 90) return 2;
             return 3;
         } else {
             if (r < 35) return 1;
             if (r < 80) return 2;
             return 3;
         }
     }
 
     // Wave 4-6: introduz 4 HP
     if (wave <= 6) {
         int r = rand() % 100;
         if (isFrontHalf) {
             if (r < 35) return 1;
             if (r < 75) return 2;
             if (r < 95) return 3;
             return 4;
         } else {
             if (r < 20) return 1;
             if (r < 55) return 2;
             if (r < 85) return 3;
             return 4;
         }
     }
 
     // Wave 7+: maioritariamente 3-4, raros 5, e depois soft-scaling
     int r = rand() % 100;
     int hp = 3;
 
     if (wave <= 8) {
         if (r < 55) hp = 3;
         else if (r < 92) hp = 4;
         else hp = 5;
     } else {
         if (r < 25) hp = 3;
         else if (r < 95) hp = 4;
         else hp = 5;
     }
 
     // front half puxa para 3
     if (isFrontHalf && hp > 3) {
         int rr = rand() % 100;
         if (rr < 55) hp = 3;
     }
 
     // beyond wave 10: introduz 5/6 muito lentamente
     if (wave >= 11) {
         int bonus = (wave - 11) / 3;
         hp = std::min(6, hp + bonus);
     }
     return std::min(6, std::max(1, hp));
 }
 
 int RogueSystem::bricksRequiredForWave(int wave) {
     wave = std::max(1, wave);
     if (wave <= 2) return 22;
     if (wave <= 5) return 26;
     if (wave <= 8) return 30;
     return 34;
 }
 
 float RogueSystem::minTimeForWave(int wave) {
     wave = std::max(1, wave);
     if (wave <= 2) return 12.0f;
     if (wave <= 5) return 13.0f;
     if (wave <= 8) return 14.0f;
     return 15.0f;
 }
 
 float RogueSystem::timeLimitForWave(int wave) {
     wave = std::max(1, wave);
     if (wave <= 2) return 54.0f;
     if (wave <= 5) return 52.0f;
     if (wave <= 8) return 50.0f;
     return 48.0f;
 }
 
 void RogueSystem::onWaveAdvanced(GameState& state) {
     state.rogueBricksBrokenThisWave = 0;
     state.rogueWaveTimer = 0.0f;
     state.rogueWaveCooldown = 0.50f;
 }
 
 int RogueSystem::rowsToInsertForWave(const GameState& state, int wave) {
     wave = std::max(1, wave);
 
     int rows = 3;
     if (wave <= 2) rows = 2;
     else if (wave <= 10) rows = 3;
     else rows = 4;
 
     rows += state.rogueRowsPerWaveDelta;
     if (rows < 1) rows = 1;
     return rows;
 }
 
 void RogueSystem::spawnWaveRows(GameState& state, const GameConfig& cfg, int rowsToInsert) {
     if (rowsToInsert <= 0) return;
 
     glm::vec3 brickSize(2.95f, 0.7f, 1.30f);
     float gapX = 0.04f;
     float gapZ = 0.03f;
 
     const int cols = 12;
     float totalW = cols * brickSize.x + (cols - 1) * gapX;
     float leftX = -totalW * 0.5f + brickSize.x * 0.5f;
     float stepZ = brickSize.z + gapZ;
     float topZ = cfg.arenaMinZ + 0.85f;
 
     // Empurra bricks existentes para abrir espaço no "back"
     float push = (float)rowsToInsert * stepZ;
     for (auto& br : state.bricks) {
         if (!br.alive) continue;
         br.pos.z += push;
     }
 
     // Spawna novas rows no back (tratadas como mais difíceis)
     int count = rowsToInsert * cols;
     for (int i = 0; i < count; ++i) {
         Brick b;
         b.size = brickSize;
         b.alive = true;
 
         int col = i % cols;
         int rowLocal = (i / cols);
 
         b.pos.x = leftX + col * (brickSize.x + gapX);
         b.pos.y = 0.0f;
         b.pos.z = topZ + rowLocal * stepZ;
 
         int hp = RogueSystem::pickBrickHpForWave(state.wave, /*isFrontHalf=*/false);
         b.maxHp = b.hp = std::min(6, std::max(1, hp));
 
         state.bricks.push_back(b);
     }
 
     state.rogueRowsSpawned += rowsToInsert;
 }
 
 int RogueSystem::loadRogueBestScore() {
     std::ifstream f(rogueBestScorePath());
     int best = 0;
     if (f >> best) return std::max(0, best);
     return 0;
 }
 
 void RogueSystem::maybeUpdateRogueBest(GameState& state) {
     if (state.gameType != GameType::ROGUE) return;
     if (state.score > state.rogueBestScore) {
         state.rogueBestScore = state.score;
         saveRogueBestScore(state.rogueBestScore);
     }
 }
 
 } // namespace game
 