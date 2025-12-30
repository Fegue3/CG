# Breakout3D

3D Breakout with multiple game modes (Normal / Endless / Rogue / Levels), a full SFX + music system, and an in-game Sound Settings screen.

## Quick start

### Build (Linux/WSL)

Install deps (Ubuntu/Debian):

```bash
sudo apt update
sudo apt install -y g++ make \
  libglfw3-dev libglew-dev libglu1-mesa-dev mesa-common-dev
```

Build and run:

```bash
cd Breakout3D/Breakout3D
make -j
./breakout3d
```

Debug build:

```bash
make debug -j
./breakout3d_debug
```

### Build (macOS)

You’ll need a working toolchain + `glew` + `glfw` (e.g. via MacPorts/Homebrew). Then:

```bash
cd Breakout3D/Breakout3D
make -j
./breakout3d
```

## Controls

- **Move paddle**: `A/D` or `Left/Right`
- **Launch ball**: hold `Space` (aim uses the mouse position)
- **Pause**: `Esc`
- **Menus**: mouse hover + click

## Audio

- **Backend**: [`miniaudio`](external/miniaudio.h) (single-header)
- **Assets**:
  - `assets/audio/sfx/` (one-shots + loops)
  - `assets/audio/music/` (looping music)
  - `assets/audio/stingers_music/` (musical stingers)
- **Variations**: files with suffix `_01/_02/_03` are grouped and chosen randomly.
- **Settings**: `Options → Sound` has **Master / SFX / Music / Stingers** sliders (0–100%) and updates live.

If you’re on WSL: audio works best via **WSLg/PulseAudio**.

## Docs

Docs live at the repo root:

- Start here: [`../../docs/README.md`](../../docs/README.md)


