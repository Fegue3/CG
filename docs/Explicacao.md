# Breakout3D — Short Explanation

This is a short, up-to-date explanation of the Breakout3D project in this repository.

> Source code lives under `Breakout3D/Breakout3D/`.

---

## What the project is

Breakout3D is an academic 3D version of classic Breakout, implemented in **C++17** with **OpenGL**.

- Gameplay is mostly on the **XZ plane** (Y is used for visuals/camera/UI placement).
- The project uses a modular split between an engine layer and a game layer.

---

## Current features (high level)

- Main menu (Normal / Endless / Instructions / Exit)
- Two camera modes (keys 1/2)
- Pause/resume (Esc)
- 3D bricks with HP (Endless can scale up to 6 HP)
- Power-ups (Extra Ball, Expand, Slow, Extra Life)
- Endless Mode with time-based + brick-destroy-based row spawning and best-score persistence

---

## Controls (current)

- **A / Left Arrow**: move paddle left
- **D / Right Arrow**: move paddle right
- **Space**: launch the ball (when attached)
- **Esc**: pause/resume
- **1 / 2**: change camera mode
- **Mouse**: click menu + click background selector (top right)

Note: fullscreen toggle is **not** bound to a key in the current code.

---

## Build & run

From repository root:

```bash
cd Breakout3D/Breakout3D
make -j
./breakout3d
```

Clean:

```bash
make clean
```

