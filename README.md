# Breakout3D

3D Breakout game implemented in **C++17** with **OpenGL** (GLFW + GLEW) and a full **SFX/Music** system (miniaudio).

## Features

- **Game modes**: Normal, Endless, Rogue, and Levels
- **3D gameplay + UI overlays**: menus, pause, results, instructions, rogue card pick
- **Audio system**:
  - one-shot SFX + looped SFX
  - dynamic music per mode (menu/gameplay/pause/results) with Rogue/Endless progression
  - musical stingers
  - in-game **Sound Settings** (Master / SFX / Music / Stingers sliders)

## Project layout

- **Source code**: `Breakout3D/Breakout3D/`
- **Design + engineering docs**: `docs/` (repo root)

## Installation / Build

### 1) Clone

```bash
git clone <your-repo-url>
cd CG
```

### 2) Linux / WSL (Ubuntu/Debian)

Install dependencies:

```bash
sudo apt update
sudo apt install -y g++ make \
  libglfw3-dev libglew-dev libglu1-mesa-dev mesa-common-dev
```

Build:

```bash
cd Breakout3D/Breakout3D
make -j
```

Run:

```bash
./breakout3d
```

Debug build:

```bash
make debug -j
./breakout3d_debug
```

### 3) macOS

You need a working toolchain plus `glfw` and `glew` (e.g. via Homebrew/MacPorts).

Build + run:

```bash
cd Breakout3D/Breakout3D
make -j
./breakout3d
```

## Controls

- **Move paddle**: `A/D` or `Left/Right`
- **Launch ball**: hold `Space` (mouse position controls aim)
- **Pause**: `Esc`
- **Menus/UI**: mouse hover + click

For full controls (including debug-only binds), see [`docs/CONTROLS_AND_DEBUG.md`](./docs/CONTROLS_AND_DEBUG.md).

## Audio notes

- **Assets**: `Breakout3D/Breakout3D/assets/audio/`
- **WSL**: audio works best via **WSLg/PulseAudio**.

## Documentation

- **[docs/README.md](./docs/README.md)** - Documentation index (start here)
- **[docs/PROJECT_CONTEXT.md](./docs/PROJECT_CONTEXT.md)** - Current project overview (code-accurate)
- **[docs/CONTROLS_AND_DEBUG.md](./docs/CONTROLS_AND_DEBUG.md)** - Controls, modes, power-ups, and debug-only keybinds
- **[docs/Breakout3D_Architecture.md](./docs/Breakout3D_Architecture.md)** - Architecture (engine/game split and systems)
- **[docs/RENDERING_OPENGL.md](./docs/RENDERING_OPENGL.md)** - Rendering/OpenGL notes
- **[docs/THREADING.md](./docs/THREADING.md)** - Thread usage (GIF preview decoding + main-thread GL upload)
- **[docs/ENDLESS_MODE.md](./docs/ENDLESS_MODE.md)** - Endless mode deep dive
- **[docs/ROGUE_MODE.md](./docs/ROGUE_MODE.md)** - Rogue mode deep dive
- **[docs/ROGUE_CARDS.md](./docs/ROGUE_CARDS.md)** - Rogue cards list and effects

## Troubleshooting

- If you get build errors, re-check the dependency list above.
- If you get **no sound** on WSL, ensure WSLg audio is working and that you didn’t move the `assets/` folder.
