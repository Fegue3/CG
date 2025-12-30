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

void Game::commitEndlessStreak(GameState& state) {
    if (state.endlessStreakPoints == 0) return;
    // Bank can be negative (penalties). Clamp score at 0.
    state.score = std::max(0, state.score + state.endlessStreakPoints);
    state.endlessStreakPoints = 0;
    state.endlessStreakPosPoints = 0;
    state.endlessStreakNegPoints = 0;
    state.endlessStreakIdleTimer = 0.0f;
    state.endlessStreakBanking = false;
    state.endlessStreakBankTimer = 0.0f;
}

void Game::maybeUpdateEndlessBest(GameState& state) {
    if (state.gameType != GameType::ENDLESS) return;
    if (state.score > state.endlessBestScore) {
        state.endlessBestScore = state.score;
        saveEndlessBestScore(state.endlessBestScore);
    }
}

} // namespace game


