# Breakout3D — Project Context (Current)

This document is a **high-level, code-accurate** description of what is currently implemented in this repository.

Source code lives under `Breakout3D/Breakout3D/`.

---

## What the project is

**Breakout3D** is a 3D Breakout for a Computer Graphics course:

- **Language**: C++17
- **Graphics**: OpenGL (GLEW + GLFW)
- **Math**: GLM
- **Gameplay plane**: mostly **XZ** (Y is used for visuals/camera/UI placement)
- **Default window**: `1280x900` (see `src/main.cpp`)

---

## What’s implemented (feature checklist)

- **3D gameplay**: paddle/ball/bricks/walls, 3D meshes, lighting shader
- **UI**: menu, overlays (pause/game over/win), instructions with tabs
- **Modes**:
  - **Normal**: clear all bricks to win (plays a short “win finisher” before the WIN overlay)
  - **Endless**: survival with continuous row insertion and streak-bank scoring + best score persistence
  - **Rogue**: wave-based run with **Rogue cards** (drafting between waves + at run start), best score persistence
- **Power-ups / curses**:
  - Good: `EXPAND`, `EXTRA_BALL`, `EXTRA_LIFE`, `FIREBALL`, `SHIELD`
  - Curses: `SLOW`, `REVERSE`, `TINY`
- **Animated GIF previews** in Instructions → Powerups:
  - CPU decode on worker threads, GPU upload on the main thread (OpenGL context owner)

---

## Code architecture (as implemented)

The project is split into two layers:

### Engine layer (`include/engine`, `src/engine`)

- **`Window`**: GLFW window + OpenGL context, framebuffer sizing, event pump, swap.
- **`Input`**: key/mouse state, edge detection, mouse position in framebuffer pixels.
- **`Time`**: frame timing (`delta`).
- **`Shader`**: GLSL program compilation + uniform helpers.
- **`Texture`**: texture loading/binding (+ `stb_image` decode).
- **`Mesh`**: OBJ/MTL loading and GPU buffers.
- **`Renderer`**:
  - 3D mesh drawing
  - UI pass (ortho) with quads/triangles + **TTF font rendering** via `stb_truetype` (baked atlas)

### Game layer (`include/game`, `src/game`)

- **`Game`**: orchestrates update + render; owns `GameState`, `GameConfig`, `GameAssets`.
- **`GameState`**: all mutable runtime state (entities, timers, UI state, mode/type, Rogue state).
- **`GameConfig`**: gameplay constants (arena bounds, speeds, durations, VFX tuning).
- **Systems** (`game/systems`): focused logic modules
  - `InitSystem`, `InputSystem`, `PhysicsSystem`, `CollisionSystem`, `PowerUpSystem`
  - `RogueSystem` (Rogue wave rules + persistence + row insertion for Rogue)

For deeper architecture notes, see `docs/Breakout3D_Architecture.md`.

---

## Gameplay summary (accurate defaults)

### Controls

See `docs/CONTROLS_AND_DEBUG.md` for the full list.

### Power-ups / curses (baseline tuning)

Config lives in `include/game/GameConfig.hpp`:

- **Drop chance**: `cfg.powerUpChance = 0.35` (35%)
- **Default duration**: `cfg.powerUpDuration = 7.0s`
- **Specific durations**:
  - `REVERSE`: `4.25s`
  - `TINY`: `6.0s`
  - `SHIELD`: `6.5s`

### Rogue cards

Rogue cards are implemented under:

- `include/game/rogue/*` + `src/game/rogue/*` (card definitions, pools, effects)
- `src/game/systems/RogueSystem.cpp` (wave rules)

Full details:

- `docs/ROGUE_MODE.md`
- `docs/ROGUE_CARDS.md`

---

## Rendering & UI (overview)

Rendering is two-pass:

1. **3D pass**: walls, bricks, paddle, balls, power-ups (meshes + lighting shader).
2. **UI pass** (ortho):
   - hearts/lives HUD
   - menu + instructions overlay (Controls / Powerups / Rogue Cards)
   - overlays (Pause / Game Over / Win)
   - background selector HUD
   - Endless/Rogue danger band + “DANGER!” label

More details: `docs/RENDERING_OPENGL.md`.

---

## Build & run

From repository root:

```bash
cd Breakout3D/Breakout3D
make -j
./breakout3d
```

