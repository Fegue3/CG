# Breakout3D — Project Context

This document gives a high-level and technically accurate overview of the **current** Breakout3D implementation in this repository.

> Source code lives under `Breakout3D/Breakout3D/` (engine + game).

---

## Overview

**Breakout3D** is a 3D version of classic Breakout for a Computer Graphics course:

- **Language**: C++17
- **Graphics**: OpenGL (GLEW + GLFW)
- **Math**: GLM
- **Gameplay plane**: mostly **XZ** (Y is used for visuals/camera/UI placement)
- **Default window**: `1280x900` (see `Breakout3D/Breakout3D/src/main.cpp`)

---

## Code architecture (as implemented)

The project is split into two layers:

### Engine layer (`Breakout3D/Breakout3D/include/engine`, `Breakout3D/Breakout3D/src/engine`)

- **`Window`**: creates the GLFW window, input/event pump, framebuffer sizing, swap.
- **`Input`**: key/mouse state, edge detection, mouse position in framebuffer pixels.
- **`Time`**: frame timing (`delta`).
- **`Shader`**: GLSL program compilation + uniform helpers.
- **`Texture`**: texture loading/binding.
- **`Mesh`**: OBJ/MTL loading and GPU buffers.
- **`Renderer`**:
  - 3D mesh drawing (basic lighting shader).
  - UI pass (ortho) with quads, triangles, and **TTF font rendering** using `stb_truetype` (baked atlas).

### Game layer (`Breakout3D/Breakout3D/include/game`, `Breakout3D/Breakout3D/src/game`)

- **`Game`**: orchestrates update + render; owns `GameState` and `GameConfig`.
- **`GameState`**: all mutable runtime state (balls, bricks, powerups, timers, UI state, mode).
- **`GameConfig`**: gameplay constants (arena bounds, speeds, power-up params).
- **Systems** (`game/systems`): logic split by responsibility:
  - `InitSystem`, `InputSystem`, `PhysicsSystem`, `CollisionSystem`, `PowerUpSystem`

---

## Gameplay (current behavior)

### Controls

- **A / Left Arrow**: move paddle left
- **D / Right Arrow**: move paddle right
- **Space**: launch ball (when attached)
- **Esc**: pause/resume
- **1 / 2**: camera mode
- **Mouse**: menu clicks + background selector

### Modes

- **Normal mode**:
  - Goal: destroy all bricks → `WIN`.
  - Lives: you lose a life when all balls fall out; game ends at 0 lives.
  - **Score is still tracked internally**, but the **normal-mode score HUD is intentionally hidden** (per project UI changes).

- **Endless mode**:
  - No `WIN` goal; intended as survival.
  - Bricks are continuously inserted over time and via brick-destroy thresholds.
  - Ends when bricks reach the paddle’s “death line” (and also if lives hit 0).
  - Uses a **streak bank** scoring model and persists a best score on disk.
  - Full details: see `docs/ENDLESS_MODE.md`.

### Power-ups (drop chance + distribution)

Power-ups are attempted on brick destruction with:

- `powerUpChance = 0.40` (40%), `powerUpDuration = 7s` (`Breakout3D/Breakout3D/include/game/GameConfig.hpp`)

If a power-up is spawned, type distribution (current code in `PowerUpSystem.cpp`):

- `EXTRA_BALL`: 45%
- `EXPAND`: 20%
- `SLOW`: 17%
- `EXTRA_LIFE`: 18%

Effects:

- **EXTRA_BALL**: spawns 3 new balls with spread angles.
- **EXPAND**: temporarily increases paddle width (`expandScaleFactor = 1.6`).
- **SLOW**: slows paddle speed (`slowSpeedFactor = 0.5`).
- **EXTRA_LIFE**: adds a life.

---

## Rendering & UI

Rendering is two-pass:

1. **3D pass**: walls, bricks, paddle, balls, power-ups.
2. **UI pass** (ortho):
   - hearts/lives HUD
   - menu UI (Normal/Endless/Instructions/Exit)
   - overlays (Pause / Game Over / Win)
   - background selector HUD
   - Endless danger band + “DANGER!” label

UI text uses a TTF baked atlas in `Renderer` (stb_truetype) with a bold Orbitron font by default.

---

## Build & run

From repository root:

```bash
cd Breakout3D/Breakout3D
make -j
./breakout3d
```

