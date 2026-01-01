// GameUpdateHelpers.cpp
/**
 * @file GameUpdateHelpers.cpp
 * @brief Helpers do Game para Endlessness (best score + commit do streak bank).
 *
 * Conceitos:
 *  - Endless/Rogue usam “streak bank” (endlessStreakPoints):
 *      - ganha pontos -> acumula no bank
 *      - penalidades -> podem tornar o bank negativo
 *      - commit -> soma bank ao score real (com clamp >= 0), e limpa estado do bank
 *
 * Persistência:
 *  - Best score é guardado num ficheiro no $HOME (se existir) ou fallback local.
 */
 #include "game/Game.hpp"

 #include <algorithm>
 #include <cstdlib>
 #include <fstream>
 
 namespace game {
 
 std::string Game::endlessBestScorePath() {
     const char* home = std::getenv("HOME");
     if (home && *home) {
         return std::string(home) + "/.breakout3d_endless_best.txt";
     }
     return "breakout3d_endless_best.txt";
 }
 
 void Game::saveEndlessBestScore(int best) {
     std::ofstream f(endlessBestScorePath(), std::ios::trunc);
     if (!f) return;
     f << best;
 }
 
 /**
  * @brief Converte o bank (positivo ou negativo) em score real, e limpa o estado do bank.
  *
  * Regras:
  *  - Se bank == 0, não faz nada (evita trabalho inútil).
  *  - score nunca fica negativo (clamp a 0).
  *  - limpa também contadores auxiliares (pos/neg) e timers do sistema de banking.
  */
 void Game::commitEndlessStreak(GameState& state) {
     if (state.endlessStreakPoints == 0) return;
 
     // Bank pode ser negativo devido a penalidades.
     state.score = std::max(0, state.score + state.endlessStreakPoints);
 
     // Reset total e breakdown (pos/neg)
     state.endlessStreakPoints = 0;
     state.endlessStreakPosPoints = 0;
     state.endlessStreakNegPoints = 0;
 
     // Reset timers/flags do “banking”
     state.endlessStreakIdleTimer = 0.0f;
     state.endlessStreakBanking = false;
     state.endlessStreakBankTimer = 0.0f;
 }
 
 /**
  * @brief Atualiza best score do Endless (se aplicável).
  *
  * Nota:
  *  - Só atualiza no modo ENDLESS.
  *  - Assumimos que state.endlessBestScore já foi carregado no init().
  */
 void Game::maybeUpdateEndlessBest(GameState& state) {
     if (state.gameType != GameType::ENDLESS) return;
 
     if (state.score > state.endlessBestScore) {
         state.endlessBestScore = state.score;
         saveEndlessBestScore(state.endlessBestScore);
     }
 }
 
 } // namespace game
 