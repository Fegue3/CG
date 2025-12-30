# Breakout3D — Recent Updates (Rogue UI & Cards)

This document summarizes the recent improvements made to the Rogue Mode UI and card system.

---

## Changes Made

### 1. Wind Card Refactor: Directional → Chaotic

**Before:**
- `MOD_WIND_RIGHT`: constant wind pushing balls right
- `MOD_WIND_LEFT`: constant wind pushing balls left

**After:**
- `MOD_WIND_RANDOM` (renamed `MOD_WIND_CHAOTIC` in UI): randomized wind that pushes balls left **or** right every 0.3 seconds

**Implementation:**
- Added `rogueRandomWindActive` and `rogueRandomWindTimer` to `GameState`
- Wind force randomizes between 2.8-4.4 magnitude
- Direction randomizes (left or right)
- Ball speed magnitude is preserved (only direction changes)
- See: [GameState.hpp](../Breakout3D/Breakout3D/include/game/GameState.hpp) + [PhysicsSystem.cpp](../Breakout3D/Breakout3D/src/game/systems/PhysicsSystem.cpp)

**Files Modified:**
- `include/game/rogue/RogueCardId.hpp`
- `src/game/rogue/RogueCards.cpp`
- `include/game/GameState.hpp`
- `src/game/systems/PhysicsSystem.cpp`
- `docs/ROGUE_CARDS.md`

---

### 2. Powerup Card Colors: Unified Blue

**Before:**
- Each powerup had a different color (EXTRA_LIFE: red, EXTRA_BALL: green, EXPAND: blue, FIREBALL: orange, SHIELD: light blue, SLOW: gray, REVERSE: purple, TINY: yellow)

**After:**
- All powerup cards now display in **blue** (`0.25f, 0.75f, 0.95f`)
- Matches the title color of the Rogue Cards menu (consistent visual language)
- Modifier cards: purple
- OP cards: gold

**Implementation:**
- Simplified `cardAccent()` function to use `isPowerupCard()` check
- All powerups return the same blue color

**Files Modified:**
- `src/game/rogue/RogueCards.cpp`

---

### 3. Rogue Card Pick Button Animation

**Before:**
- No hover feedback on the PICK button
- Card outline animated RGB when hovering card

**After:**
- Card outline: animates RGB neon when hovering **anywhere on the card**
- PICK button: animates RGB neon border **only when hovering the button itself**
- Two independent animations for better UX

**Implementation:**
- Added `hoveredRogueCardPickButton` to `GameState` to track button-specific hover
- Implemented precise hit-testing for the button rectangle in `GameUpdate.cpp`
- Button animation uses same RGB neon formula as card: `hue = fmod(0.56f + 0.08f * sin(time * 1.2f), 1.0f)`
- Animation only renders when mouse is directly over the button

**Files Modified:**
- `include/game/GameState.hpp`
- `src/game/GameUpdate.cpp`
- `src/game/rogue/RogueCards.cpp`
- `src/game/systems/RogueSystem.cpp`
- `src/game/render/hud/PauseEndOverlays.cpp`

---

## Visual Improvements

✅ **Cleaner card browser**: Powerup cards are now visually grouped by color  
✅ **Better tactile feedback**: Button animation only on hover, not on card hover  
✅ **More dynamic gameplay**: Wind card adds unpredictability without being unfair  
✅ **Consistent neon aesthetic**: RGB animations match the Rogue menu's visual language

---

## Testing

All changes have been compiled and tested:
- Rogue mode card pick UI works correctly
- Wind chaotic card applies randomized pushes properly
- Button animations appear only on precise hover
- No errors or warnings in compilation

**Build command:**
```bash
cd Breakout3D/Breakout3D
make clean && make -j4
./breakout3d
```

---

## Related Documentation

- [ROGUE_MODE.md](./ROGUE_MODE.md) - Wave progression and difficulty
- [ROGUE_CARDS.md](./ROGUE_CARDS.md) - Complete card list (updated with WIND_RANDOM)
- [Breakout3D_Architecture.md](./Breakout3D_Architecture.md) - System architecture
