# Menu Implementation (Current)

This document describes the **current** menu and overlay UI implementation in this repository.

> Source code lives under `Breakout3D/Breakout3D/`.

---

## Main menu (startup)

The game starts in `GameMode::MENU` (see `Breakout3D/Breakout3D/include/game/GameState.hpp` and `Breakout3D/Breakout3D/src/main.cpp`).

### Buttons (top → bottom)

- **NORMAL**: starts a Normal run
- **ENDLESS**: starts an Endless run
- **INSTRUCTIONS**: opens the modal instructions panel
- **EXIT**: closes the window

Implementation:

- Click handling: `Breakout3D/Breakout3D/src/game/systems/InputSystem.cpp` (`handleMenuInput`)
- Rendering: `Breakout3D/Breakout3D/src/game/Game.cpp` (MENU render path)

### Instructions modal

When `showInstructions == true`, a centered panel is displayed. Mouse clicks are handled as:

- clicking **outside** the panel closes it
- clicks inside do nothing (panel is modal)

---

## In-game overlays

### Pause overlay (`GameMode::PAUSED`)

- Toggle with **Esc** (handled by `InputSystem::handleGameInput`)
- On pause, an overlay panel is rendered with:
  - title **PAUSED**
  - buttons: **RESTART** and **MENU**

### Game Over / Win overlay

When the game ends:

- **GAME OVER**: loss condition met
- **WINNER!**: all bricks cleared in Normal mode

Overlay panel shows:

- title: `GAME OVER` or `WINNER!`
- buttons: **RETRY** and **MENU**

Implementation:

- Render: `Breakout3D/Breakout3D/src/game/Game.cpp` (overlay section at end of UI pass)
- Click logic: `Breakout3D/Breakout3D/src/game/Game.cpp` (GAME_OVER/WIN click handling in `update()`)

---

## Other UI elements

### Background selector (top-right HUD)

Clickable color boxes that set `GameState::currentBg`. Implemented in:

- Input: `Breakout3D/Breakout3D/src/game/systems/InputSystem.cpp`
- Render: `Breakout3D/Breakout3D/src/game/Game.cpp`

---

## Window size notes

Default window is `1280x900` in `Breakout3D/Breakout3D/src/main.cpp` to ensure the large title and menu panel fit without overlap.

