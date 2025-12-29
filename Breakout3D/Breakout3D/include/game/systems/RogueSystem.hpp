#pragma once

#include "game/GameState.hpp"
#include "game/GameConfig.hpp"

namespace game {

// Minimal Rogue-mode helpers.
// For now, Rogue mode is: wave rooms + reward cards every N cleared waves (and at start).
class RogueSystem {
public:
    static void startRun(GameState& state);
    static void setCardOptions(GameState& state);
    static void onCardPicked(GameState& state, PowerUpType chosen);
    static bool shouldOfferCardsAfterClearingWave(const GameState& state, int clearedWave);
    static bool shouldWinAfterClearingWave(const GameState& state, int clearedWave);

    // Brick difficulty curve for Rogue waves.
    // Wave 1: only green/yellow. Later waves progressively introduce tougher bricks.
    static int pickBrickHpForWave(int wave, bool isFrontHalf);

    // Wave progression: advance by quota (bricks broken) and/or time.
    static int bricksRequiredForWave(int wave);
    static float minTimeForWave(int wave);
    static float timeLimitForWave(int wave);

    static void onWaveAdvanced(GameState& state);
    static void spawnWaveRows(GameState& state, const GameConfig& cfg, int rowsToInsert);
    static int rowsToInsertForWave(const GameState& state, int wave);

    // Rogue best score persistence (separate from Endless)
    static int loadRogueBestScore();
    static void maybeUpdateRogueBest(GameState& state);
};

} // namespace game


