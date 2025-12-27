# Menu Initial Implementation - Breakout3D

## Overview
Implemented a professional main menu for Breakout3D that launches before the game starts. The menu includes:
- **INSTRUCTIONS button** (Top): Shows/hides a help panel with game controls
- **PLAY button** (Middle): Starts a new game
- **EXIT button** (Bottom): Closes the application

---

## Changes Made

### 1. **main.cpp** (`src/main.cpp`)
#### Modified:
- Removed the initial `game.init()` call to allow the game to start in MENU state
- The game now initializes with `GameMode::MENU` as default
- `init()` is only called when the user clicks PLAY button

```cpp
// Before:
game::Game game(window, time, renderer, assets);
game.init();  // ❌ Removed

// After:
game::Game game(window, time, renderer, assets);
// Don't call init() here - let the game start at MENU state
```

---

### 2. **GameState.hpp** (`include/game/GameState.hpp`)
#### Added:
- New `MENU` state to `GameMode` enum:
  ```cpp
  enum class GameMode {
      MENU,           // New: Main menu state
      PLAYING,
      PAUSED,
      GAME_OVER,
      WIN
  };
  ```

- Menu tracking fields to `GameState` struct:
  ```cpp
  int selectedMenuOption = 0;      // Track selected button
  bool showInstructions = false;   // Toggle instructions panel visibility
  ```

- Changed default mode to `MENU` instead of `PLAYING`:
  ```cpp
  GameMode mode = GameMode::MENU;  // Changed from GameMode::PLAYING
  ```

---

### 3. **Game.cpp** - Update Logic (`src/game/Game.cpp`)
#### Menu State Handler:
```cpp
if (m_state.mode == GameMode::MENU) {
    // Button positions (from top to bottom)
    float btn2Y = panelY + 280.0f; // Instructions (top)
    float btn1Y = panelY + 170.0f; // Play (middle)
    float btn3Y = panelY + 60.0f;  // Exit (bottom)

    // If instructions panel is shown, only allow clicking to close it
    if (m_state.showInstructions) {
        // Click outside instructions panel = close it
        if (!pointInRectPx(px, py, instrX, instrY, instrW, instrH)) {
            m_state.showInstructions = false;
        }
        return;
    }

    // Handle button clicks
    if (click) {
        // PLAY button: Start game
        if (pointInRectPx(px, py, btnX, btn1Y, btnW, btnH)) {
            init();  // Sets mode to PLAYING
            return;
        }
        // INSTRUCTIONS button: Toggle help panel
        if (pointInRectPx(px, py, btnX, btn2Y, btnW, btnH)) {
            m_state.showInstructions = true;
        }
        // EXIT button: Close application
        if (pointInRectPx(px, py, btnX, btn3Y, btnW, btnH)) {
            m_window.requestClose();
            return;
        }
    }
    return;
}
```

**Key features:**
- Button detection using `pointInRectPx()` utility
- Clicking PLAY calls `init()` to start fresh game
- Clicking INSTRUCTIONS opens overlay (clicking outside closes it)
- Clicking EXIT closes the application
- Instructions panel blocks clicks to buttons behind it

#### Game Over / Win State Handler:
```cpp
if (m_state.mode == GameMode::GAME_OVER || m_state.mode == GameMode::WIN) {
    // Left button: RETRY - Start new game
    if (pointInRectPx(px, py, btnX_left, btnY, btnW, btnH)) {
        init();  // Sets mode to PLAYING
        return;
    }
    // Right button: MENU - Return to main menu
    if (pointInRectPx(px, py, btnX_right, btnY, btnW, btnH)) {
        m_state.mode = GameMode::MENU;
        m_state.showInstructions = false;
        return;
    }
}
```

**Changes:**
- Left button: **RETRY** (was RESTART) - Starts new game immediately
- Right button: **MENU** (was EXIT/SAIR) - Returns to main menu instead of closing app
- Both buttons now show clear intent without closing the application

---

### 4. **Game.cpp** - Render Logic (`src/game/Game.cpp`)
#### Menu Panel Rendering:
- Dark background overlay
- "BREAKOUT 3D" title with cyan color (scale 4.0)
- 500x400px centered panel with cyan borders
- Three buttons arranged vertically:
  1. **INSTRUCTIONS** (Blue) - Top position (btn2Y)
  2. **PLAY** (Green) - Middle position (btn1Y)
  3. **EXIT** (Red) - Bottom position (btn3Y)

#### Instructions Panel:
- 600x320px overlay with darker background
- "HOW TO PLAY" title
- Six lines of instructions:
  - "A/D or ARROW KEYS: Move paddle"
  - "SPACE: Launch ball"
  - "ESC: Pause/Resume game"
  - "1/2: Change camera view"
  - "Destroy all bricks to win!"
  - "Click here to go back"
- Blue borders and light cyan text
- Click outside to dismiss

#### Game Over / Win Panel:
- Two buttons at the bottom:
  - **RETRY** (Red button): Restart game immediately
  - **MENU** (Green button): Return to main menu
- Clear visual feedback with proper button positioning
- Title changes between "GAME OVER" and "WINNER!"

---

## Game Flow Diagram

```
┌─────────────────────────────────────────┐
│         MAIN MENU (startup)             │
├─────────────────────────────────────────┤
│  ┌─────────────────────────────────┐    │
│  │  BREAKOUT 3D (Title)            │    │
│  │  ┌───────────────────────────┐  │    │
│  │  │   INSTRUCTIONS (Blue)     │  │    │
│  │  ├───────────────────────────┤  │    │
│  │  │   PLAY (Green) ──────────┐│  │    │
│  │  ├───────────────────────────┤│  │    │
│  │  │   EXIT (Red)             ││  │    │
│  │  └───────────────────────────┘│  │    │
│  └─────────────────────────────────┘    │
└──────────────┬──────────────┬────────────┘
               │              │
         ┌─────▼──┐    ┌──────▼─────┐
         │  GAME  │    │  INSTRUCTIONS
         │ PLAYING│    │    PANEL
         └────┬───┘    └──────┬──────┘
              │                │
         ┌────▼─────────────────┘
         │  Click outside = Close
         │
    ┌────▼────────────────────────┐
    │  GAME OVER / WIN SCREEN      │
    ├──────────────────────────────┤
    │  ┌──────────────────────┐    │
    │  │ RETRY   │   MENU    │    │
    │  └──────────────────────┘    │
    └────┬──────────────────┬──────┘
         │                  │
    ┌────▼────┐        ┌────▼──────┐
    │ NEW GAME │        │ BACK MENU  │
    │ (Playing)│        │(Menu State)│
    └──────────┘        └────────────┘
```

---

## User Controls

### Main Menu:
| Action | Result |
|--------|--------|
| Click **INSTRUCTIONS** | Show help panel |
| Click **PLAY** | Start new game (→ PLAYING) |
| Click **EXIT** | Close application |
| Click outside INSTRUCTIONS | Close help panel |

### During Game:
| Key | Action |
|-----|--------|
| **ESC** | Pause/Resume |
| **A/D or Arrows** | Move paddle |
| **SPACE** | Launch ball |
| **1/2** | Change camera |

### Game Over / Win:
| Button | Action |
|--------|--------|
| **RETRY** | Start new game immediately |
| **MENU** | Return to main menu |

---

## Compilation & Testing

✅ **Compiles without errors or warnings**
```bash
make clean && make
make run  # Launches with menu
```

✅ **Game starts with menu displayed**
✅ **All buttons are clickable and functional**
✅ **Instructions panel opens/closes correctly**
✅ **Menu transitions to game when PLAY is clicked**
✅ **Game Over returns to menu (not closed)**
✅ **RETRY starts new game immediately**

---

## Architecture Highlights

### State Machine
The menu is fully integrated into the existing state machine:
- Maintains separation of concerns
- Each game mode handles its own logic
- Seamless transitions between states
- No breaking changes to existing code

### Click Handling
- Consistent use of `pointInRectPx()` for button detection
- Proper coordinate conversion (framebuffer to screen space)
- Modal dialogs (instructions panel blocks background buttons)

### Rendering
- Early return in render function for menu mode
- Efficient panel-based UI system
- Proper depth/layering for overlays
- Clean font rendering with scale support

---

## Files Modified
- `src/main.cpp` - Removed initial init() call
- `include/game/GameState.hpp` - Added MENU mode and state variables
- `src/game/Game.cpp` - Added complete menu update and render logic

---

## Summary of Changes from v1 → v2

| Feature | v1 | v2 |
|---------|----|----|
| **Menu Buttons** | Calculated wrong Y positions | Hardcoded correct positions |
| **Instructions Panel** | Toggle behavior | Click outside = close |
| **Panel Blocking** | No blocking | Blocks clicks to buttons |
| **Game Over** | Close app | Return to menu |
| **Game Over Buttons** | RESTART / SAIR | RETRY / MENU |
| **Initial State** | Called init() in main | Starts in MENU |
| **User Flow** | Direct to game | Menu → Game → Game Over → Menu |

---

## No Breaking Changes
All existing game functionality remains intact. The menu wraps the existing game loop naturally and can be extended with additional features in the future.
