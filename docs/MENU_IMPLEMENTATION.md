# Menu / UI Implementation (Current)

This document describes the **current** menu system, instructions overlay, and in-game overlays.

Source code lives under `Breakout3D/Breakout3D/`.

---

## Game entry / menu state machine

The game starts in `GameMode::MENU`.

Inside the menu, the UI uses a screen state:

- `MenuScreen::MAIN`
- `MenuScreen::PLAY_MODES`
- `MenuScreen::OPTIONS`
- `MenuScreen::INSTRUCTIONS`

See:

- state: `include/game/GameState.hpp`
- rendering: `src/game/render/MenuRender.cpp`
- input/hit-testing: `src/game/systems/InputSystem.cpp`

---

## Main menu (`MenuScreen::MAIN`)

Primary actions:

- **PLAY**: opens `PLAY_MODES`
- **INSTRUCTIONS**: opens `INSTRUCTIONS`
- **OPTIONS**: opens `OPTIONS`
- **EXIT**: closes the window

---

## Play modes (`MenuScreen::PLAY_MODES`)

Play modes include:

- **NORMAL**
- **ENDLESS**
- **ROGUE**
- (optional placeholders may exist visually depending on build)

Selecting a mode sets `GameState::gameType` and transitions to `GameMode::PLAYING` (or Rogue’s draft overlay flow).

---

## Instructions (`MenuScreen::INSTRUCTIONS`)

Instructions is a tabbed UI:

- **Controls**
- **Powerups** (includes a powerup inspector and animated GIF previews)
- **Rogue Cards** (browse all Rogue cards and open a card inspection modal)
- **Back**

Notes:

- The powerup inspector uses lazy-loaded GIF previews (see `docs/GIF_PREVIEWS.md`).
- The Rogue Cards browser is driven by `game::rogue::allCardIds()` + `cardDef(...)` metadata.

---

## In-game overlays

### Pause overlay (`GameMode::PAUSED`)

- Toggle with **Esc** (handled by `InputSystem::handleGameInput`)
- Buttons:
  - **RESTART**
  - **MENU**

### Game Over / Win overlay

- **GAME OVER**: loss condition met
- **WIN**: Normal clears all bricks (after a short “win finisher” cinematic)
- Buttons:
  - **RETRY**
  - **MENU**

Overlay layout is centralized so input hit-testing matches render:

- `include/game/ui/OverlayLayout.hpp`

---

## Other UI elements

### Background selector (HUD)

Clickable boxes in the HUD set `GameState::currentBg`.

---

## Window size note

Default window is `1280x900` in `src/main.cpp` to ensure the menu and instructions fit comfortably.

