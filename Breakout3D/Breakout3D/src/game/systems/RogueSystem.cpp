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
    state.hoveredRogueCard = -1;
    state.hoveredRogueCardPickButton = -1;
    state.rogueBricksBrokenThisWave = 0;
    state.rogueWaveTimer = 0.0f;
    state.rogueWaveCooldown = 0.0f;
    state.rogueRowsSpawned = 0;
    state.roguePendingRowsToSpawn = 0;
    state.rogueRowSpawnTimer = 0.0f;
    state.rogueBestScore = loadRogueBestScore();

    // Initialize run card pools/modifiers and start the special wave-1 triple draft.
    game::rogue::initRunPools(state);
    game::rogue::startInitialDraft(state);
}

void RogueSystem::setCardOptions(GameState& state) {
    // Defer to RogueCards module (3-card offers, no repeats).
    // OP pack every 3 waves (3, 6, 9, ...) else normal pack.
    // NOTE: This function is currently unused (Game.cpp drives offers), but keep it consistent.
    int nextWave = std::max(1, state.wave);
    bool opPack = (nextWave % 3) == 0 && nextWave >= 3;
    game::rogue::dealOffer(state, 3, opPack, nextWave);
}

void RogueSystem::onCardPicked(GameState& state, PowerUpType chosen) {
    // Legacy path (no longer used): powerup-only cards.
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

    // Wave 1: only green/yellow (1-2 HP)
    if (wave == 1) {
        // front half easier (more green)
        int r = rand() % 100;
        if (isFrontHalf) return (r < 70) ? 1 : 2;
        return (r < 45) ? 1 : 2;
    }

    // Wave 2-3: introduce some blue (3 HP), mostly 1-2
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

    // Wave 4-6: introduce purple (4 HP), back half can be tougher
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

    // Wave 7+: ramp hard bricks faster so late game is mostly blue/purple.
    // Target: wave 10 -> mostly purple (4) with some blue (3).
    int r = rand() % 100;
    int hp = 3;

    if (wave <= 8) {
        // Mostly blue/purple, rare 5.
        if (r < 55) hp = 3;
        else if (r < 92) hp = 4;
        else hp = 5;
    } else {
        // Wave 9+ : mostly purple, some blue, rare 5.
        if (r < 25) hp = 3;
        else if (r < 95) hp = 4;
        else hp = 5;
    }

    // Front half slightly easier: bias toward blue instead of purple.
    if (isFrontHalf && hp > 3) {
        int rr = rand() % 100;
        if (rr < 55) hp = 3;
    }

    // Soft scaling beyond wave 10: slowly introduce 5/6 (but keep it rare).
    if (wave >= 11) {
        int bonus = (wave - 11) / 3; // 0,0,0,1,...
        hp = std::min(6, hp + bonus);
    }
    return std::min(6, std::max(1, hp));
}

int RogueSystem::bricksRequiredForWave(int wave) {
    wave = std::max(1, wave);
    // Start a bit lenient, then ramp slightly so later waves aren't instantly skipped.
    // (Tune freely.)
    // User request: "dont make it that fast" => increase quota a bit.
    if (wave <= 2) return 22;
    if (wave <= 5) return 26;
    if (wave <= 8) return 30;
    return 34;
}

float RogueSystem::minTimeForWave(int wave) {
    wave = std::max(1, wave);
    // Prevent chain-advancing in a few seconds when the ball melts a cluster.
    if (wave <= 2) return 12.0f;
    if (wave <= 5) return 13.0f;
    if (wave <= 8) return 14.0f;
    return 15.0f;
}

float RogueSystem::timeLimitForWave(int wave) {
    wave = std::max(1, wave);
    // Time-based fallback so waves always progress even if the player plays safe.
    // Slightly decreases over time to keep pacing up.
    // User request: bigger time between waves => longer limits.
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
    // More waves => more rows. Baseline is ~3, with a gentle ramp.
    // User request: reduce mid/late-game row incrementation (still ramps, but slower).
    int rows = 3;
    if (wave <= 2) rows = 2;   // early game fairness
    else if (wave <= 10) rows = 3;
    else rows = 4;

    // Card modifier: Row Control reduces rows per wave (clamped to >=1).
    rows += state.rogueRowsPerWaveDelta;
    if (rows < 1) rows = 1;
    return rows;
}

void RogueSystem::spawnWaveRows(GameState& state, const GameConfig& cfg, int rowsToInsert) {
    if (rowsToInsert <= 0) return;

    // Match InitSystem brick sizing/spacing so rows align.
    glm::vec3 brickSize(2.95f, 0.7f, 1.30f);
    float gapX = 0.04f;
    float gapZ = 0.03f;

    const int cols = 12;
    float totalW = cols * brickSize.x + (cols - 1) * gapX;
    float leftX = -totalW * 0.5f + brickSize.x * 0.5f;
    float stepZ = brickSize.z + gapZ;
    float topZ = cfg.arenaMinZ + 0.85f;

    float push = (float)rowsToInsert * stepZ;
    // Push all existing bricks forward to make room at the back (near arenaMinZ).
    for (auto& br : state.bricks) {
        if (!br.alive) continue;
        br.pos.z += push;
    }

    // Spawn new rows at the back; treat them as "back half" for difficulty.
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


