# Breakout3D — Rogue Mode (Technical Documentation)

Source code lives under `Breakout3D/Breakout3D/`.

Rogue Mode is a **wave-based run** where you draft **Rogue cards** between waves (and at run start). Cards permanently modify the run and (optionally) expand a **drop deck** that controls which power-ups can appear from bricks.

Key files:

- `include/game/GameState.hpp` (Rogue state fields)
- `src/game/GameUpdate.cpp` (wave progression loop + card offer scheduling + row insertion cadence)
- `src/game/systems/RogueSystem.cpp` (wave rules, brick difficulty curve, persistence)
- `src/game/rogue/RogueCards.cpp` (card pools, gating rules, and card effects)
- `src/game/systems/PowerUpSystem.cpp` (Rogue drop-deck behavior)

---

## Run loop overview

### Start of a Rogue run

When a Rogue run starts, the game immediately begins a **special starting draft**:

- **3 rounds** of draft picks happen before normal gameplay resumes.
- During drafting, the game switches to `GameMode::ROGUE_CARDS` (gameplay paused).

Implementation:

- `RogueSystem::startRun(...)` calls `game::rogue::initRunPools(...)` + `game::rogue::startInitialDraft(...)`.

### Between waves

Waves advance by **quota and/or time**, not by clearing all bricks:

- A wave can advance when:
  - enough bricks were broken this wave (quota), and a minimum time has passed, OR
  - a time limit is reached (fallback so waves always progress)

Implementation: `GameUpdate.cpp` calls:

- `RogueSystem::bricksRequiredForWave(wave)`
- `RogueSystem::minTimeForWave(wave)`
- `RogueSystem::timeLimitForWave(wave)`

### Rewards / card offers

After a wave advances, Rogue can offer a 3-card pack:

- `GameState::rogueRewardEvery` controls frequency:
  - default is `1` → offer cards after every cleared wave
- **OP packs**:
  - offered at the **start of waves 3/6/9/...** (i.e., after clearing 2/5/8/...)

Implementation: `GameUpdate.cpp` + `RogueCards::dealOffer(...)`.

---

## Difficulty and wave pacing

### Quota requirements (bricks broken)

Current tuning in `RogueSystem.cpp`:

- wave 1–2: 22 bricks
- wave 3–5: 26 bricks
- wave 6–8: 30 bricks
- wave 9+: 34 bricks

### Time limits

Current tuning in `RogueSystem.cpp`:

- minimum time before quota can advance:
  - 12s → 15s (slightly increases with wave)
- hard time limit (fallback advance):
  - ~54s → ~48s (slightly decreases with wave)

### Brick HP distribution

Rogue uses a wave-based HP distribution (not the Endless HP scaling rule):

- wave 1: only HP 1–2
- wave 2–3: introduces some HP 3
- wave 4–6: introduces HP 4 (back half tougher)
- wave 7+: mostly HP 3–4, rare 5, with very late soft scaling toward 6

Implementation: `RogueSystem::pickBrickHpForWave(...)`.

---

## Row insertion (Rogue pressure)

When a wave advances, Rogue queues a number of rows to insert:

- base:
  - wave 1–2: 2 rows
  - wave 3–10: 3 rows
  - wave 11+: 4 rows
- modifiers can reduce rows per wave (Row Control cards)
- clamped to at least 1 row

Important pacing detail:

- Rows are **not spawned instantly**.
- They spawn **after the player picks a card**, gradually (1 row per `rogueRowSpawnInterval`).

Defaults in `GameState`:

- `rogueRowSpawnInterval = 0.55s`

Implementation:

- queue size: `RogueSystem::rowsToInsertForWave(...)`
- insertion: `RogueSystem::spawnWaveRows(...)`
- cadence: driven in `GameUpdate.cpp`

---

## Drops and the Rogue “drop deck”

Rogue does **not** use the normal weighted random pool for power-ups.

Instead:

- Only power-up types present in `GameState::rogueDropDeck` can drop from bricks.
- The deck starts empty and is filled by **powerup cards** (and some modifier/OP cards).

Implementation: `PowerUpSystem::spawnPowerUp(...)`.

### Effective drop chance in Rogue

When a brick is destroyed, Rogue uses:

- `chance = rogue::effectiveDropChance(state, cfg)`

Current formula (see `RogueCards.cpp`):

- baseline multiplier `rogueBase = 0.62`
- `chance = cfg.powerUpChance * rogueBase * state.rogueDropChanceMult`
- clamped to `<= 0.68`

This keeps Rogue from becoming a “powerup fountain” as the deck grows.

---

## Scoring and persistence

Rogue uses the same **streak bank** logic as Endless:

- brick points accumulate into `endlessStreakPoints`
- after a short idle delay, the bank is committed to `score`

Rogue-specific modifiers can change:

- brick points multiplier
- bank idle time multiplier (safer/riskier)

Rogue also persists a best score to disk:

- `${HOME}/.breakout3d_rogue_best.txt` (or `breakout3d_rogue_best.txt` if `HOME` is missing)

Implementation: `RogueSystem::loadRogueBestScore()` and `RogueSystem::maybeUpdateRogueBest()`.

---

## Card list

See `docs/ROGUE_CARDS.md` for the full card list and exact effects.


