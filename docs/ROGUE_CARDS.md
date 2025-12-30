# Rogue Mode — Card System (Current)

This document is **code-accurate** for the current Rogue card system.

Source code:

- Card ids: `Breakout3D/Breakout3D/include/game/rogue/RogueCardId.hpp`
- Card metadata + effects + offer gating: `Breakout3D/Breakout3D/src/game/rogue/RogueCards.cpp`
- Offer timing (start-of-run + between waves): `Breakout3D/Breakout3D/src/game/GameUpdate.cpp`

---

## How drafting works (as implemented)

- **Offers**: each offer contains up to **3 unique cards**.
- **Picked cards are unique per run**:
  - the picked card is removed from its pool and cannot appear again
  - cards shown but not picked can appear again later
- **Run start**: Rogue begins with **3 draft rounds** before gameplay begins.
- **Between waves**:
  - cards are offered based on `GameState::rogueRewardEvery` (default: 1 → every cleared wave)
  - **OP pack schedule**: OP packs are offered at the **start of waves 3/6/9/...**

### Offer gating (anti-explosion)

Some cards are gated to later wave numbers:

- Normal packs:
  - `PU_EXTRA_BALL`: wave 4+
  - `PU_FIREBALL`: wave 6+
  - `MOD_BONUS_LIFE_TAX`: wave 2+
  - `MOD_RUSH_MODE`: wave 5+
  - `MOD_GLASS_CANNON`: wave 5+
  - `MOD_CURSE_ENGINE`: wave 3+
- OP packs:
  - only from wave 3+
  - `OP_THREE_BALL_START`: wave 9+

---

## Card categories

- **Powerup cards**: add a type to the Rogue **drop deck** and grant it once immediately.
- **Modifier cards**: permanent run modifiers (usually trade-offs).
- **OP cards**: strong modifiers with no downside (only shown in OP packs).

---

## Powerup cards (drop-deck + immediate grant)

All of these:

- add the power-up type to `GameState::rogueDropDeck` (if not already present)
- immediately call `PowerUpSystem::applyPowerUpEffect(...)`

- **EXPAND** (`PU_EXPAND`)
  - adds `EXPAND` to deck; grants EXPAND immediately
- **EXTRA BALL** (`PU_EXTRA_BALL`)
  - adds `EXTRA_BALL` to deck; grants EXTRA_BALL immediately (spawns 3 balls)
- **EXTRA LIFE** (`PU_EXTRA_LIFE`)
  - adds `EXTRA_LIFE` to deck; grants +1 life immediately
- **FIREBALL** (`PU_FIREBALL`)
  - adds `FIREBALL` to deck; grants FIREBALL immediately (one-shot fireball shot)
- **SHIELD** (`PU_SHIELD`)
  - adds `SHIELD` to deck; grants SHIELD immediately (duration scales with Rogue modifiers)
- **SLOW** (`PU_SLOW`) *(curse)*
  - adds `SLOW` to deck; grants SLOW immediately
- **REVERSE** (`PU_REVERSE`) *(curse)*
  - adds `REVERSE` to deck; grants REVERSE immediately
- **TINY** (`PU_TINY`) *(curse)*
  - adds `TINY` to deck; grants TINY immediately

---

## Normal modifier cards (trade-offs)

These are permanent modifiers applied immediately on pick.

- **WIDE PADDLE / SLOW** (`MOD_WIDE_PADDLE_SLOW`)
  - `rogueBasePaddleScaleX *= 1.35`
  - `roguePaddleSpeedMult *= 0.88`
- **WIDE PADDLE / LIGHT BALL** (`MOD_WIDE_PADDLE_LIGHT_BALL`)
  - `rogueBasePaddleScaleX *= 1.30`
  - `rogueBallSpeedMult *= 0.90`
- **FAST PADDLE / TINY PADDLE** (`MOD_FAST_PADDLE_TINY_PADDLE`)
  - `roguePaddleSpeedMult *= 1.25`
  - `rogueBasePaddleScaleX *= 0.82`
- **FAST BALL / SLOW PADDLE** (`MOD_FAST_BALL_SLOW_PADDLE`)
  - `rogueBallSpeedMult *= 1.18`
  - `roguePaddleSpeedMult *= 0.85`
- **LUCKY DROPS / BRITTLE** (`MOD_LUCKY_DROPS_BRITTLE`)
  - `rogueDropChanceMult *= 1.22`
  - immediately lose 1 life
- **LUCKY DROPS / SLOW BALL** (`MOD_LUCKY_DROPS_SLOW_BALL`)
  - `rogueDropChanceMult *= 1.25`
  - `rogueBallSpeedMult *= 0.88`
- **BONUS LIFE / TAX** (`MOD_BONUS_LIFE_TAX`)
  - gain +1 life immediately
  - apply a bank penalty: `endlessStreakPoints -= 300` (negative bank points)
- **GLASS CANNON** (`MOD_GLASS_CANNON`)
  - `rogueDropChanceMult *= 1.18`
  - `rogueBallSpeedMult *= 1.12`
  - harsher life loss: `rogueLifeLossPenaltyBonus += 140`
- **STREAK GREED** (`MOD_STREAK_GREED`)
  - `rogueBrickPointsMult *= 1.18`
  - riskier banking: `rogueBankIdleMult *= 0.75` (bank commits faster)
- **SAFE BANKER** (`MOD_SAFE_BANKER`)
  - `rogueBrickPointsMult *= 0.88`
  - safer banking: `rogueBankIdleMult *= 1.35`
- **SHIELD LONG / SLOW** (`MOD_SHIELD_LONG_SLOW`)
  - `rogueShieldDurationMult *= 1.35`
  - `roguePaddleSpeedMult *= 0.90`
- **FIREBALL WIDE / SLOW** (`MOD_FIREBALL_WIDE_SLOW`)
  - `rogueFireballRadiusMult *= 1.35`
  - `roguePaddleSpeedMult *= 0.90`

---

## Environment / creative modifier cards

- **WIND: CHAOTIC** (`MOD_WIND_RANDOM`)
  - `rogueRandomWindActive = true`
  - balls are pushed left or right **randomly every 0.3 seconds**
  - `rogueDropChanceMult *= 1.08`
  - `roguePaddleSpeedMult *= 0.92`
- **CENTERED ARENA** (`MOD_CENTERED_ARENA`)
  - `roguePaddleClampMarginX += 3.25`
  - `rogueBrickPointsMult *= 1.15`
  - `rogueDropChanceMult *= 0.85`
- **STICKY PADDLE** (`MOD_STICKY_PADDLE`)
  - `rogueStickyPaddle = true`
  - `rogueBallSpeedMult *= 0.88`
- **SCORE FARM** (`MOD_SCORE_FARM`)
  - `rogueBrickPointsMult *= 1.28`
  - `rogueDropChanceMult *= 0.78`
- **CURSE ENGINE** (`MOD_CURSE_ENGINE`)
  - `rogueDropChanceMult *= 1.20`
  - adds curses into deck: `SLOW`, `REVERSE`, `TINY`
- **SHOCK ABSORB** (`MOD_SHOCK_ABSORB`)
  - `rogueBankIdleMult *= 1.55`
  - `roguePaddleSpeedMult *= 0.92`
- **RUSH MODE** (`MOD_RUSH_MODE`)
  - `rogueBallSpeedMult *= 1.18`
  - `rogueDropChanceMult *= 1.10`
  - harsher life loss: `rogueLifeLossPenaltyBonus += 220`
- **FRAIL PADDLE** (`MOD_FRAIL_PADDLE`)
  - `rogueBasePaddleScaleX *= 0.80`
  - `rogueDropChanceMult *= 1.18`
  - `roguePaddleSpeedMult *= 1.12`
- **ROW CONTROL** (`MOD_ROW_BARGAIN`) *(shown as OP in UI; implemented as this id)*
  - `rogueRowsPerWaveDelta -= 1`

---

## OP cards (no downsides; shown in OP packs)

- **FIREBALL MASTERY** (`OP_FIREBALL_MASTERY`)
  - adds `FIREBALL` to deck
  - `rogueFireballRadiusMult *= 1.55`
  - `rogueDropChanceMult *= 1.08`
- **SHIELD GENERATOR** (`OP_SHIELD_GENERATOR`)
  - adds `SHIELD` to deck
  - `rogueShieldDurationMult *= 1.55`
  - `rogueDropChanceMult *= 1.06`
- **LUCK ENGINE** (`OP_LUCK_ENGINE`)
  - `rogueDropChanceMult *= 1.35`
- **TITAN PADDLE** (`OP_TITAN_PADDLE`)
  - `rogueBasePaddleScaleX *= 1.70`
- **OVERDRIVE** (`OP_OVERDRIVE`)
  - `roguePaddleSpeedMult *= 1.35`
  - `rogueBallSpeedMult *= 1.25`
- **THREE BALL START** (`OP_THREE_BALL_START`)
  - adds `EXTRA_BALL` to deck
  - grants `EXTRA_BALL` immediately (spawns 3 balls)
- **HEART STOCKPILE** (`OP_HEART_STOCKPILE`)
  - gain +2 lives immediately
- **BANKER'S UPSIDE** (`OP_BANKER_S_UPSIDE`)
  - `rogueBrickPointsMult *= 1.20`
  - `rogueBankIdleMult *= 1.45`
- **PIERCE TRAINING** (`OP_PIERCE_TRAINING`)
  - `rogueBrickDamageBonus += 1`
- **ROW CONTROL** (`OP_ROW_CONTROL`)
  - `rogueRowsPerWaveDelta -= 1`

---

## How drops work in Rogue (important)

Rogue uses a **drop deck**:

- if `rogueDropDeck` is empty, **no brick drops can spawn**
- otherwise, brick drops pick a random type from that deck

Drop chance in Rogue uses `rogue::effectiveDropChance(state, cfg)` (baseline reduction + multipliers + cap).

See `docs/ROGUE_MODE.md` for the drop formula and caps.
