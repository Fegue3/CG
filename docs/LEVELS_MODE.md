# LEVELS Mode

## Overview
LEVELS mode is a curated progression experience featuring 10 unique, hand-crafted brick layouts. Each level presents a distinct challenge with its own brick pattern and difficulty.

## Gameplay

### Objective
Complete all 10 levels by destroying every brick in each level. Progress through increasingly challenging layouts.

### Level Structure
- **Total Levels:** 10
- **Lives:** 3 (shared across all levels)
- **Progression:** Linear - must complete each level to advance to the next
- **Victory:** Complete level 10 to win the game

### Level Progression
- When all bricks in a level are destroyed, you automatically advance to the next level
- Your paddle, ball, and lives carry over between levels
- Active powerups are cleared when transitioning to the next level
- Level progress is tracked (best level reached)

### Level Layouts

Each level features a unique brick pattern:

1. **Level 1 - Simple Rows**: 4 rows of bricks with varying HP (1-4)
2. **Level 2 - Diamond**: Diamond-shaped formation with 2-3 HP bricks
3. **Level 3 - Blocks**: Scattered block pattern with mixed HP
4. **Level 4 - Pyramid**: Classic pyramid formation (1-2 HP)
5. **Level 5 - Checkerboard**: Alternating pattern with 2-3 HP
6. **Level 6 - X Pattern**: Diagonal X formation (3-4 HP)
7. **Level 7 - Maze**: Complex maze-like structure (2-4 HP)
8. **Level 8 - Vertical Stripes**: Column-based layout (3 HP)
9. **Level 9 - Circle**: Circular formation (3-4 HP)
10. **Level 10 - Boss**: Dense, challenging final pattern (2-4 HP)

## HUD Display

The top-center HUD shows:
```
LEVEL X / 10
```
This displays your current level number out of 10 total levels.

## Features

### Mouse-Directed Launch
- When launching the ball, aim with your mouse cursor
- The ball will launch toward the direction of your mouse pointer
- Allows strategic positioning for optimal brick destruction

### Win Condition
- Complete all 10 levels to trigger the WIN screen
- Win finisher animation plays after destroying the last brick

### Game Over
- Lose all 3 lives → GAME OVER (can restart from level 1)
- Lives persist across level transitions

## Controls

All standard Breakout controls apply:
- **Mouse**: Move paddle (X-axis) + aim ball launch direction
- **Left Click**: Launch ball (aims toward mouse)
- **P**: Pause
- **ESC**: Return to menu

## Technical Details

### Implementation
- Levels are defined in `include/game/LevelLayouts.hpp`
- Each level uses `BrickData` structures: `{gridX, gridZ, hp}`
- Grid coordinates are converted to world space during level initialization
- Brick spacing and centering are automatic

### State Tracking
- `currentLevel`: Current level number (1-10)
- `levelsBestLevel`: Highest level reached (for future persistence)

## Differences from Other Modes

| Feature | NORMAL | ENDLESS | ROGUE | LEVELS |
|---------|--------|---------|-------|--------|
| Brick Layout | Procedural | Procedural waves | Procedural waves | Hand-crafted |
| Progression | Single stage | Infinite | Wave-based | 10 fixed levels |
| Win Condition | Clear all bricks | None (survival) | None (prototype) | Complete level 10 |
| Difficulty | Fixed | Scales with waves | Scales + cards | Designed per level |
| Powerups | Random drops | Random drops | Card selection | Random drops |

## Future Enhancements (Potential)

- **Persistence**: Save best level reached across sessions
- **Level Goals**: Time challenges, score targets, or special objectives per level
- **Level Select**: Unlock and replay completed levels
- **Difficulty Tiers**: Easy/Medium/Hard variants of each layout
- **Custom Levels**: Level editor or community-created patterns

---

**Status**: ✅ Fully implemented and playable

