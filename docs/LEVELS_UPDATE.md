# LEVELS/LEVEL SELECT Updates - 20 Level Expansion

## Recent Changes (20-Level System)

### Level Expansion (10 → 20)
- Expanded from 10 unique levels to 20 complete levels in `include/game/LevelLayouts.hpp`
- All 20 levels now properly defined with progressive difficulty tiers
- Level select screen updated to 4×5 grid (20 buttons total)

### Difficulty Tier Implementation
1. **Foundational Levels (1-10)**: Variable HP (1-4 hits)
   - Original base levels, mixed brick difficulties
   - Designed for learning and progression

2. **Intermediate Levels (11-15)**: Minimum HP = 2
   - No single-hit bricks allowed
   - Grid sizes: 10 columns × 7-9 rows
   - Increased challenge, more strategic play

3. **Master Levels (16-20)**: Minimum HP = 3
   - Only 3-4 hit bricks (no weaker variants)
   - **Expanded Grid Sizes**: 12 columns × 7-9 rows (was 10 columns)
   - Ultimate difficulty and visual complexity

### Grid Size Expansion
- **Levels 1-14**: 10 columns width (maintained)
- **Levels 15-20**: 12 columns width (expanded)
  - L15: 12×8 grid (Fortress pattern)
  - L16: 12×9 grid (Peak challenge)
  - L17: 12×8 grid (Fortress variation)
  - L18: 12×8 grid (Strategic pattern)
  - L19: 12×7 grid (Cross pattern)
  - L20: 12×8 grid (Ultimate boss level)

## Critical Implementation Details

### Level Select Screen (`MenuRender.cpp` + `InputSystem.cpp`)
- **Grid Layout**: 4 rows × 5 columns (20 buttons)
- **Panel Position**: fbH × 0.40f (synchronized across render and input)
- **Button Size**: 110×110 pixels
- **Button Spacing**: 25 pixels between buttons
- **Panel Dimensions**: 800×650 pixels

### Coordinate System Fix
- **Y-Axis Alignment**: Fixed inverted Y coordinate issue
- **Hit Detection Formula**: `screenY = fbH - y - btnSize`
- **Input Coordinate**: Uses `py_raw` (raw pixel Y from mouse)
- **Render Coordinate**: Positions buttons using OpenGL space (Y grows upward)
- Result: Hover detection now perfectly matches click detection for all 20 levels

### Brick HP Requirements in Code
Each level in `LevelLayouts.hpp` enforces minimum HP:
```cpp
// Levels 1-10: Variable HP (no restrictions)
// Levels 11-15: All bricks must have hp >= 2
// Levels 16-20: All bricks must have hp >= 3
```

## Files Modified

### Core Level System
- **`include/game/LevelLayouts.hpp`**: 
  - Levels 1-10: Maintained (L10 min hp=2)
  - Levels 11-20: Completely new with difficulty tiers
  - All 20 levels now with correct hp and grid dimensions

- **`src/game/render/MenuRender.cpp`**: 
  - Panel position: fbH × 0.40f
  - Grid: 4×5 layout (20 buttons)
  - Maintains hover effects and button rendering

- **`src/game/systems/InputSystem.cpp`**: 
  - Panel Y coordinate: fbH × 0.40f (synchronized with render)
  - Hit detection uses py_raw + screenY formula
  - All 20 level buttons properly mapped to detection grid

### Documentation
- **`docs/LEVELS_MODE.md`**: ✅ Updated with complete 20-level documentation, difficulty tiers, grid specifications
- **`docs/LEVELS_UPDATE.md`**: ✅ Updated with this expansion information

## Validation Checklist

- ✅ Build: `make clean && make` compiles without errors
- ✅ Level count: All 20 levels defined in LevelLayouts.hpp
- ✅ HP Requirements: 
  - Levels 1-10: Variable hp (1-4)
  - Levels 11-15: Minimum hp=2
  - Levels 16-20: Minimum hp=3
- ✅ Grid Expansion: Levels 15-20 use 12-column grids
- ✅ Level Select: 4×5 grid displays all 20 buttons
- ✅ Coordinate System: Hover and click detection synchronized at fbH × 0.40f
- ✅ UI Rendering: Panel and buttons positioned correctly without clipping

## Testing Recommendations

1. **Level Select Screen**:
   - Verify all 20 buttons appear in 4×5 grid
   - Hover over each button and confirm highlight
   - Click levels 1-10, 15, and 20 to verify selection works

2. **Gameplay Testing**:
   - Play Level 10: Should have at least one hp=2 brick
   - Play Level 15: Verify all bricks are hp≥3, grid is 12 columns
   - Play Level 20: Final boss, all hp≥3, 12×8 grid

3. **HP Verification**:
   - Level 10: Mix of hp=1,2,3 (verify min hp=2 by visual inspection)
   - Levels 11-15: No hp=1 bricks visible
   - Levels 16-20: Only hp=3,4 bricks (no single or double-hit visible)

4. **Secret Feature**:
   - Press "L" in main menu to unlock all 20 levels for testing

## Previous Implementation Notes

Original 10-level system used:
- 2×5 grid layout (2 rows × 5 columns)
- Variable difficulty progression
- Simpler brick patterns (10-column max)

New 20-level system provides:
- 4×5 grid layout (4 rows × 5 columns)
- Three distinct difficulty tiers
- Expanded grid sizes for higher levels
- Progressive challenge curve from foundational to master levels
