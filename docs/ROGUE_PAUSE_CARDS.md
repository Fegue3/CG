# Rogue Cards Display in Pause Menu

## Feature Overview
When playing in **ROGUE mode**, pressing ESC to pause displays **all** the cards you've chosen during your run with interactive hover tooltips and card names.

## Visual Design

### Expanded Pause Menu
- **Panel Size**: Automatically expands to 1000×640px (from 650×320px) in ROGUE mode
- **Title**: "YOUR CARDS" displayed prominently with spacing
- **Card Grid**: 2 rows × 10 columns (max 20 cards displayed)
- **Badge Style**: 50×50px colored badges with 10px spacing
- **Color Coding**: Each card uses its unique accent color

### Interactive Hover System
- **Badge Borders**: Thicken (2.0px → 3.5px) when hovered
- **Card Names**: Show below badges on hover (scale 0.85f with accent color)
- **Tooltip Display**: Shows on hover with:
  - Card full name (with colored glow effect)
  - Complete description (wrapped text)
  - Styled border matching the card's accent color
  - Positioned centrally at bottom with good spacing from buttons

### Card Display Format
```
PAUSED
YOUR CARDS

Row 1:  [XP] [EX] [HP] [FB] [SH] [SL] [RV] [TN] [MOD][MOD]
Row 2:  [MOD][MOD][MOD][MOD][MOD][MOD][MOD][MOD][MOD][MOD]
                              ▲
        (Labels appear on hover, e.g., "EXPAND")

        ┌─────────────────────────────────┐
        │       BONUS LIFE / TAX           │
        │                                 │
        │  +1 life now.                   │
        │  -Bank penalty.                 │
        └─────────────────────────────────┘
        (Tooltip appears at bottom center)
```

## Implementation Details

### Modified Files
1. **`include/engine/Input.hpp`**:
   - Added `Key::R` for debug card filling

2. **`include/game/GameState.hpp`**:
   - Already had `hoveredPauseRogueCard` state variable

3. **`include/game/ui/OverlayLayout.hpp`**:
   - Updated `pauseOverlayRogue()` to 1000×640px (was 750×580px)

4. **`src/engine/Input.cpp`**:
   - Added mapping for `Key::R` → `GLFW_KEY_R`

5. **`src/game/render/hud/PauseEndOverlays.cpp`**:
   - Grid-based card rendering (2 rows, 10 columns)
   - Badge size: 50×50px with 10px horizontal gaps
   - Row spacing: 80px (accommodates card name labels)
   - Title positioned 80px above badges
   - Tooltip positioned at `panelY2 + 150.0f` for spacing from buttons
   - Tooltip size: 500×180px

6. **`src/game/GameUpdate.cpp`**:
   - Updated hover detection for grid layout
   - Max 20 cards displayed (2×10 grid)
   - Debug command: Press R to fill rogueChosen with 20 sample cards

### Key Functions Used
- `game::rogue::cardDef(id)`: Gets card definition (name, description)
- `game::rogue::cardAccent(id)`: Gets card's unique color accent
- `game::rogue::cardAbbrev(id)`: Gets abbreviated card name for badge
- `state.rogueChosen`: Vector containing all picked cards (in order)
- `ui::drawWrappedText()`: Wraps description text in tooltip

### Grid Layout Calculations
```cpp
// 2 rows × 10 columns
float badgeStartY = titleY - 80.0f;       // Space below title
float badgeRowGap = 80.0f;                // Space between rows
float badgeGap = 10.0f;                   // Horizontal spacing
float badgesPerRow = 10;                  // Cards per row

// Grid centering
float totalWidth = badgesPerRow * badgeBaseW + (badgesPerRow - 1) * badgeGap;
float badgeStartX = panelX + (panelW - totalWidth) * 0.5f;

// Position calculation
float badgeX = badgeStartX + colIdx * (badgeBaseW + badgeGap);
float badgeY = badgeStartY - rowIdx * badgeRowGap;
```

### Tooltip Positioning
```cpp
float tooltipX = panelX + (panelW - tooltipW) * 0.5f;  // Centered horizontally
float tooltipY = panelY + 150.0f;                       // Well above buttons
```

## Debug Features

### Press R (Debug Mode Only)
- Only available in `breakout3d_debug` binary
- Fills `state.rogueChosen` with 20 sample cards for quick testing
- Cards are: All powerup cards (8) + first 12 modifier cards
- Useful for testing layout and hover interactions

## User Experience Benefits

1. **Complete Overview**: See ALL 20 cards in grid layout at once
2. **Visual Identification**: Color-coded badges make cards instantly recognizable
3. **Quick Reference**: Hover for card names and descriptions
4. **Better Spacing**: Rows separated for readability
5. **Polish**: Smooth hover effects and centered tooltips

## Technical Details

### Panel Size Selection
```cpp
const auto L = (state.gameType == GameType::ROGUE && !state.rogueChosen.empty()) 
    ? game::ui::pauseOverlayRogue(fbW, fbH)  // 1000×640px
    : game::ui::pauseOverlay(fbW, fbH);       // 650×320px
```

### Hover Detection
- Iterates through all cards up to 20
- Calculates badge position based on row/col index
- Hitbox: `badgeX` to `badgeX + badgeW`, `badgeY` to `badgeY + badgeH`
- Updates `state.hoveredPauseRogueCard` (-1 for none, 0+ for index)

### Rendering Pipeline
1. Draw badge background (shadow + colored body)
2. Draw badge border (thin normal, thick on hover)
3. Draw abbreviation text (fixed 1.2f scale)
4. Draw name label below (only on hover, 0.85f scale)
5. If hovered, draw tooltip with name and description

---

**Status**: ✅ Fully implemented and tested  
**Build**: Compiles without errors (both release and debug)  
**Compatibility**: Only activates in ROGUE mode  
**Latest Changes**: Grid layout (2×10), improved spacing, tooltip positioning
