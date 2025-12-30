# Audio

## Overview

Audio is implemented with **miniaudio** (single-header) and is driven by a central `game::AudioSystem` instance owned by `game::Game`.

- Header-only backend: `external/miniaudio.h`
- Integration: `include/game/AudioSystem.hpp`, `src/game/AudioSystem.cpp`

## Asset folders

- **SFX**: `assets/audio/sfx/`
- **Music loops**: `assets/audio/music/`
- **Musical stingers**: `assets/audio/stingers_music/`

## Naming & variations

- Files ending in `_01`, `_02`, `_03` are grouped and chosen **randomly** to avoid repetition.
  - Example: `hit_wall_01.wav`, `hit_wall_02.wav`, `hit_wall_03.wav` → group `hit_wall`

## Mix model (what the sliders do)

There are two layers:

- **Internal balancing (dB)**: used to keep music below SFX and stingers slightly above music.
- **User sliders (volume)**: `Options → Sound` controls **Master / SFX / Music / Stingers** as linear multipliers (0–100%).

Sliders update **live**, including for already-playing music and loop SFX.

## Music logic by mode

- **Menu**: `music_menu_loop`
- **Normal**: `music_gameplay_loop`
- **Endless**: ramps tiers based on `endlessRowsSpawned`
  - `music_endless_loop` → `music_endless_mid_loop` → `music_endless_high_loop`
- **Rogue**: acts by wave
  - `music_rogue_act1_loop`, `music_rogue_act2_loop`, `music_rogue_act3_loop`, `music_rogue_boss_loop`
- **Pause**: `music_pause_loop`
- **Results**: `music_results_loop`

Implementation: most “music director” logic lives in `src/game/update/GameUpdatePlaying.cpp`.


