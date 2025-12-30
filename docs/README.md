# Breakout3D — Documentation Index

Source code lives under `Breakout3D/Breakout3D/`.

This folder is meant to be the **single source of truth** for:
- gameplay rules (Normal / Endless / Rogue)
- power-ups and Rogue cards
- engine/game architecture
- rendering/OpenGL pipeline
- thread usage / async asset loading

---

## Start here

- **`PROJECT_CONTEXT.md`**: what the project is, what’s implemented, and where to look in code.
- **`CONTROLS_AND_DEBUG.md`**: all player controls, modes, debug-only binds, and “how to test”.

---

## Quick reference (implementation docs)

These are short “how it works” docs that match the current codebase:

- **`BUILD.md`**: build + run instructions
- **`TROUBLESHOOTING.md`**: common problems (including “no sound”)
- **`ARCHITECTURE.md`**: high-level structure + update-module split
- **`AUDIO.md`**: audio implementation (miniaudio, mixing, music switching)
- **`SHADERS.md`**: shader files + uniforms (UI vs 3D usage)
- **`ASSETS_QUICK.md`**: asset folder layout + naming conventions

---

## Gameplay & rules (deep dives)

- **`ENDLESS_MODE.md`**: Endless rules, spawn logic, scoring/banking, danger UI, persistence.
- **`ROGUE_MODE.md`**: Rogue run loop, wave progression, drafting rules, scoring, persistence, difficulty curve.
- **`ROGUE_CARDS.md`**: complete Rogue card list (powerup cards, modifiers, OP cards) and what each does.

---

## Engineering docs

- **`Breakout3D_Architecture.md`**: engine/game split, systems, dependencies, ownership rules.
- **`RENDERING_OPENGL.md`**: coordinate system, render passes, shaders, UI rendering, GL constraints.
- **`THREADING.md`**: current threading usage (GIF decode workers + main-thread GL upload), synchronization, pitfalls.
- **`GIF_PREVIEWS.md`**: power-up animated GIF previews (implementation details + filenames).

---

## Design specs (not implemented)

- **`GUIA_AUDIO_BREAKOUT3D.md`**: audio plan/design spec (explicitly not implemented in code).


