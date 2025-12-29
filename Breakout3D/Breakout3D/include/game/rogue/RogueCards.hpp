#pragma once

#include "game/GameConfig.hpp"
#include "game/GameState.hpp"
#include "game/rogue/RogueCardId.hpp"

#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace game::rogue {

struct RogueCardDef {
    RogueCardId id;
    const char* name;
    const char* shortDesc;
    bool isOp;
};

// --- Metadata/UI helpers ---
const RogueCardDef& cardDef(RogueCardId id);
glm::vec3 cardAccent(RogueCardId id);
std::string cardAbbrev(RogueCardId id);

// All cards (for UI browsing in the menu).
// Returned vector is stable for the program lifetime.
const std::vector<RogueCardId>& allCardIds();
bool isPowerupCard(RogueCardId id);

// --- Run lifecycle ---
void initRunPools(GameState& state);               // fills remaining pools, resets modifiers
void startInitialDraft(GameState& state);          // wave1: 3 rounds of 3-card choices

// --- Dealing rules ---
// Deals up to `count` unique cards into state.rogueOffer (removing them from remaining pools).
// If `opPack` is true, pull from OP pool; otherwise pull from normal pool.
void dealOffer(GameState& state, int count, bool opPack, int waveProgress);

// --- Applying a pick ---
// Applies effects, records pick (no repeats), and adds to deck if it contributes a drop.
void applyPickedCard(GameState& state, const GameConfig& cfg, RogueCardId picked);

// Effective drop chance for Rogue (cfg.powerUpChance * multiplier).
float effectiveDropChance(const GameState& state, const GameConfig& cfg);

// Effective paddle defaults (base scale & speed multipliers) applied in physics/render/collisions.
float basePaddleScaleX(const GameState& state);
float paddleSpeedMult(const GameState& state);

// Effective ball speed multiplier for newly launched balls (applied on launch and on spawn).
float ballSpeedMult(const GameState& state);

} // namespace game::rogue


