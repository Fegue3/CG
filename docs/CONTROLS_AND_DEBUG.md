# Breakout3D — Controls, Modes, Power-ups, and Debug

This document is a practical “how to play / how to test” reference for the current codebase:

- **Normal controls** (menu + gameplay)
- **Game modes / “kinds”** (Normal, Endless, and the hidden one-brick test)
- **Power-up kinds** (good + curses) and what they do
- **Debug-only binds** (only enabled in the debug build)

Source code lives under `Breakout3D/Breakout3D/`.

---

## Build variants (release vs debug)

From `Breakout3D/Breakout3D/`:

- **Release / normal build**
  - Build: `make`
  - Run: `./breakout3d`
  - Debug-only keybinds: **DISABLED**

- **Debug build**
  - Build: `make debug`
  - Run: `./breakout3d_debug`
  - Enables `BREAKOUT3D_DEBUG` (compile-time flag)
  - Debug-only keybinds: **ENABLED**

---

## Menu controls (non-debug)

In the main menu:

- **Mouse Left Click**
  - **NORMAL**: start Normal mode
  - **ENDLESS**: start Endless mode
  - **INSTRUCTIONS**: open the instructions panel
  - **EXIT**: quit the game

### Hidden “one brick” test level (non-debug)

There is a small clickable badge inside the **EXIT** button:

- Click the little **“4”** badge labeled “ONE BRICK”
  - Starts a tiny Normal-mode test level with a minimal brick setup (useful for quick checks).

Implementation reference: `game/systems/InputSystem.cpp` (menu hit-testing) and `game/render/UIRender.cpp` (menu rendering).

---

## In-game controls (non-debug)

### Paddle movement

- **A** or **Left Arrow**: move left
- **D** or **Right Arrow**: move right

Note: if the **REVERSE** curse is active, left/right are inverted.

### Ball launch

- **Space**: launch any ball that is currently attached to the paddle

### Pause

- **Esc**: toggle pause/resume

### Camera

- **1**: camera mode 1
- **2**: camera mode 2

### Background selector (HUD)

There is a small set of boxes in the top-right HUD:

- **Mouse Left Click** on a box changes the background
  - Includes a “none” option (shown as `/`).

Implementation reference: `game/systems/InputSystem.cpp` (BG selector input) and `game/render/UIRender.cpp` (BG selector UI).

---

## Overlay UI buttons (non-debug)

These are clickable buttons drawn as part of the UI overlay, and they work via mouse click hit-testing.

- **Paused overlay**
  - Buttons: **RESTART** and **MENU**
- **Game Over / Win overlay**
  - Buttons: **RETRY** and **MENU**

Important implementation detail:

- Layout is centralized in `game/ui/OverlayLayout.hpp` and used by both:
  - `game/render/UIRender.cpp` (drawing)
  - `game/Game.cpp` (click hit-testing)

---

## Game modes / “kinds”

### Normal mode

- Goal: destroy all bricks → shows **WIN** overlay.
- Lives: lose a life when all balls fall out; at 0 lives → **GAME OVER**.
- Note: score exists internally, but the **Normal-mode score HUD is intentionally hidden** in the current UI.

### Endless mode

- Infinite survival (no true “WIN” goal).
- Bricks are continuously inserted over time and based on bricks destroyed.
- Ends when bricks reach the paddle’s “death line” or when lives hit 0.
- Uses a **streak bank** scoring system and persists a best score on disk.

See `docs/ENDLESS_MODE.md` for full technical details.

---

## Power-ups / “kinds”

Power-ups are dropped on brick destruction with:

- `cfg.powerUpChance = 0.35` (35%)
- Drop speed: `cfg.powerUpDropSpeed = 4.5`

Implementation reference: `game/systems/PowerUpSystem.cpp`, config in `game/GameConfig.hpp`.

### Power-up list (current)

In `game/entities/PowerUp.hpp`:

- **Good**
  - `EXTRA_BALL`
  - `EXPAND`
  - `EXTRA_LIFE`
  - `FIREBALL`
  - `SHIELD`
- **Bad / curses**
  - `SLOW` (treated as a penalty in scoring)
  - `REVERSE`
  - `TINY`

### Drop distribution (current)

If a drop happens, the type distribution is (see `PowerUpSystem::spawnPowerUp`):

- `EXTRA_BALL`: 22%
- `EXPAND`: 14%
- `SLOW`: 12%
- `EXTRA_LIFE`: 10%
- `FIREBALL`: 14%
- `SHIELD`: 14%
- `REVERSE`: 7%
- `TINY`: 7%

### Effects (what each does)

- **EXTRA_LIFE**
  - Adds +1 life immediately.

- **EXTRA_BALL**
  - Spawns 3 additional balls with spread angles.

- **SLOW**
  - Applies `slowTimer = cfg.powerUpDuration` (default 7.0s).
  - Gameplay effect: slower paddle movement (factor `cfg.slowSpeedFactor = 0.5`).

- **EXPAND**
  - Applies `expandTimer = cfg.powerUpDuration` (default 7.0s).
  - Gameplay effect: wider paddle (factor `cfg.expandScaleFactor = 1.6`).

- **FIREBALL**
  - Turns the current play into a **one-shot fireball projectile** (loads onto the paddle immediately so you aim the shot).
  - Fireball brick hits cause an AoE explosion (see `GameConfig.hpp` fireball tuning).

- **SHIELD**
  - Enables a temporary barrier behind the paddle for `cfg.shieldDuration` (default 6.5s).

- **REVERSE** (curse)
  - Inverts left/right movement for `cfg.reverseDuration` (default 4.25s).

- **TINY** (curse)
  - Shrinks the paddle for `cfg.tinyDuration` (default 6.0s) using `cfg.tinyScaleFactor` (default 0.60).

---

## Debug-only keybinds (requires `make debug`)

These binds are compiled only when `BREAKOUT3D_DEBUG` is defined (the `make debug` target defines it).

In `PLAYING` mode:

- **3** (`K3`)
  - **Endless only**: forces a manual incremental brick row spawn (for testing pressure/lose-condition).

- **4** (`K4`)
  - Spawns a **TINY** power-up at the paddle.
- **5** (`K5`)
  - Spawns a **FIREBALL** power-up at the paddle.
- **6** (`K6`)
  - Spawns a **SHIELD** power-up at the paddle.
- **7** (`K7`)
  - Spawns a **REVERSE** power-up at the paddle.

Implementation reference: `game/Game.cpp` (debug section guarded by `#ifdef BREAKOUT3D_DEBUG`).


