# LEVELS Mode

## Overview
LEVELS mode is a curated progression experience featuring 20 unique, hand-crafted brick layouts. Each level presents a distinct challenge with its own brick pattern and escalating difficulty. Levels 15-20 feature larger grids with minimum HP requirements to increase challenge.

## Gameplay

### Objective
Complete all 20 levels by destroying every brick in each level. Progress through increasingly challenging layouts.

### Level Structure
- **Total Levels:** 20
- **Lives:** 3 (shared across all levels)
- **Progression:** Linear - must complete each level to advance to the next
- **Victory:** Complete level 20 to win the game

### Level Progression
- When all bricks in a level are destroyed, you automatically advance to the next level
- Your paddle, ball, and lives carry over between levels
- Active powerups are cleared when transitioning to the next level
- Level progress is tracked (best level reached)
- Debug: Press **L** to unlock all levels (**debug build only**: `make debug`)

### Level Layouts

**Levels 1-10** - Foundational Challenges (Variable HP):

1. **Level 1 - Simple Rows**: 4 rows of bricks with varying HP (1-4)
2. **Level 2 - Diamond**: Diamond-shaped formation with 2-3 HP bricks
3. **Level 3 - Blocks**: Scattered block pattern with mixed HP
4. **Level 4 - Pyramid**: Classic pyramid formation (1-2 HP)
5. **Level 5 - Checkerboard**: Alternating pattern with 2-3 HP
6. **Level 6 - X Pattern**: Diagonal X formation (3-4 HP)
7. **Level 7 - Maze**: Complex maze-like structure (2-4 HP)
8. **Level 8 - Vertical Stripes**: Column-based layout (3 HP)
9. **Level 9 - Circle**: Circular formation (3-4 HP)
10. **Level 10 - Boss**: Dense, challenging final pattern (min HP: 2)

**Levels 11-15** - Intermediate Challenge (Min HP: 2, Larger Grids):

11. **Level 11 - Full Grid Dense**: Complete 10×7 grid with hp≥2
12. **Level 12 - Spiral Pattern**: Nested spirals (10×7 grid, hp≥2)
13. **Level 13 - Double Pyramid**: Top and inverted bottom pyramids (10×9 grid, hp≥2)
14. **Level 14 - Wave Pattern**: Wave-like formations (10×7 grid, hp≥2)
15. **Level 15 - Fortress**: Defensive walls with inner rooms (12×8 grid, hp≥3)

**Levels 16-20** - Master Challenge (Min HP: 3, Expanded Grids 12×8+):

16. **Level 16 - Star/Cross Pattern**: Vertical and horizontal star formation (12×9 grid, hp≥3)
17. **Level 17 - Honeycomb**: Hexagonal pattern (12×8 grid, hp≥3)
18. **Level 18 - Labyrinth**: Complex maze with multiple paths (12×8 grid, hp≥3)
19. **Level 19 - Gradient Wall**: Increasing HP from edges to center (12×7 grid, hp≥3)
20. **Level 20 - Ultimate Boss**: Complete 12×8 grid with all hp≥3

## HUD Display

The top-center HUD shows:
```
LEVEL X / 20
```
This displays your current level number out of 20 total levels.

## Level Select Screen

- **Grid Layout:** 4×5 (20 total level buttons)
- **Panel Size:** 800×650
- **Features:**
  - Visual feedback on hover with neon blue border
  - Locked levels (grayed out) until reached
  - Click anywhere on button to select
  - "SELECT LEVEL" title at top of panel
  - BACK button to return to mode selection

## Features

### Mouse-Directed Launch (aim)
- When launching the ball (Space), aim with your mouse cursor
- The ball launches toward the direction of your mouse pointer
- This affects both the normal ball and the one-shot fireball projectile

### Win Condition
- Complete all 20 levels to trigger the WIN screen
- Win finisher animation plays after destroying the last brick

### Game Over
- Lose all 3 lives → GAME OVER (can restart from level 1)
- Lives persist across level transitions

## Controls

All standard Breakout controls apply:
- **A / D** (or **Left / Right Arrow**): move paddle
- **Mouse**: aim launch direction
- **Space**: launch ball (aims toward mouse)
- **Esc**: pause / resume
- **L**: unlock all levels (**debug build only**: `make debug`)

## Technical Details

### Implementation
- Levels are defined in `include/game/LevelLayouts.hpp`
- Each level uses `BrickData` structures: `{gridX, gridZ, hp}`
- Grid coordinates are converted to world space during level initialization
- Brick spacing and centering are automatic
- Levels 15-20 use expanded grids (12 columns instead of 10)

### Difficulty Scaling
- **Levels 1-10:** Variable difficulty with mixed HP values (1-4)
- **Levels 11-15:** Minimum HP of 2 (no single-hit bricks)
- **Levels 16-20:** Minimum HP of 3 (only 3-4 hit bricks, larger grids)

### State Tracking
- `currentLevel`: Current level number (1-20)
- `levelsBestLevel`: Highest level reached (for persistence)
- `levelsCompletedStars[20]`: Track completion status

## Differences from Other Modes

| Feature | NORMAL | ENDLESS | ROGUE | LEVELS |
|---------|--------|---------|-------|--------|
| Brick Layout | Procedural | Procedural waves | Procedural waves | Hand-crafted (20) |
| Progression | Single stage | Infinite | Wave-based | 20 fixed levels |
| Win Condition | Clear all bricks | None (survival) | None (prototype) | Complete level 20 |
| Difficulty | Fixed | Scales with waves | Scales + cards | Designed per level |
| Grid Size | 10×6 | 10×6 | Variable | 10×6 (1-14), 12×8+ (15-20) |
| HP Scaling | Variable | Increases | Dynamic | 1-4 (1-10), min 2 (11-15), min 3 (16-20) |
| Powerups | Random drops | Random drops | Card selection | Random drops |

## Level Select Screen Features

- **Unlock System**: Levels unlock progressively as you complete them
- **Level Buttons**: 20 buttons in 4×5 grid (4 rows, 5 columns)
- **Hover Effect**: Neon blue border animation on button hover
- **Visual States:**
  - Unlocked levels: Bright cyan with visible number
  - Locked levels: Dark gray with faded number
  - Hovered levels: Brighter border with enhanced visibility

## Future Enhancements (Potential)

- **Persistence**: Save best level reached across sessions
- **Level Goals**: Time challenges, score targets, or special objectives per level
- **Level Replays**: Choose and replay completed levels
- **Difficulty Tiers**: Additional hard variants of existing layouts
- **Leaderboards**: Track fastest completion times per level
- **Custom Levels**: Level editor or community-created patterns

---

**Status**: ✅ Fully implemented and playable

