# Breakout3D — Structure Analysis (Current)

This document describes the **current** code structure and highlights the main technical strengths and remaining tech debt areas.

> Source code lives under `Breakout3D/Breakout3D/`.

---

## Executive summary

- The repo uses a clean **engine/game split** and a **system-based** game layer.
- Core systems already exist (`InitSystem`, `InputSystem`, `PhysicsSystem`, `CollisionSystem`, `PowerUpSystem`).
- Remaining improvements are mostly about **maintainability** (deduplication, naming, docs) and **performance** (UI text rendering).

---

## What’s good

### Engine/game separation

- `engine/` has no dependency on `game/`.
- `game/` uses engine as a service layer (renderer/window/input/time).

### Systems are separated by responsibility

Game logic is decomposed into focused systems:

- Init, input, physics, collisions, power-ups

This makes feature changes and debugging significantly easier.

### Data model is explicit

`GameState` centralizes all runtime state in one place, which is practical for a small game:

- timers, lists of entities, mode/type, menu state, Endless state

---

## Current tech debt / improvement opportunities

### 1) Endless “wave” is not incremented

`GameState::wave` exists and is used for some bonuses, but is not updated during gameplay. If you want wave-based progression, add a single progression rule (e.g., every N rows or every N minutes).

### 2) Duplicate best-score path helper

The best-score file path logic is implemented in both:

- `Breakout3D/Breakout3D/src/game/Game.cpp`
- `Breakout3D/Breakout3D/src/game/systems/InitSystem.cpp`

These can be deduplicated into a shared helper if desired.

### 3) UI text rendering performance

`Renderer::drawUIText()` currently creates VAO/VBO/EBO per glyph. This is fine for a class project, but could be optimized by:

- batching glyph quads into one dynamic buffer per string/frame
- caching VAO/VBO and reusing

### 4) “Audio” docs are a design spec only

There is no audio playback code currently. Any audio documentation should be treated as a future integration plan, not as implemented behavior.

---

## Recommended next steps (optional)

- Add a simple Endless wave progression rule (time-based or rows-based).
- Deduplicate persistent-score helpers.
- Batch UI text rendering if performance becomes a problem.


