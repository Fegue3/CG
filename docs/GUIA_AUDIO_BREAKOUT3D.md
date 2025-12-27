# Audio Guide — Breakout3D (SFX + Music) — Design Spec (Not Implemented)

This document is an **audio design specification** for Breakout3D.

Important: **audio is not implemented in the current codebase** (no audio manager and no runtime playback). Treat this as a plan for a future integration.

> Source code lives under `Breakout3D/Breakout3D/`.

---

## Goals

- **Fast feedback**: short SFX for impacts and UI.
- **Low fatigue**: variations for high-frequency events and conservative levels.
- **Clear mix**: gameplay impacts > UI > music.

---

## Suggested asset folders (future)

```text
Breakout3D/Breakout3D/assets/audio/
  music/
  sfx/
    ui/
    game/
    bricks/
    powerups/
    endless/
    stingers/
```

---

## Formats and naming

- **SFX**: WAV (low latency)
- **Music**: OGG preferred (size), WAV acceptable
- **Naming**: `category_action_variant.ext`
  - Examples: `hit_brick_02.wav`, `ui_select_01.wav`

---

## Proposed SFX events

### UI (`assets/audio/sfx/ui/`)

- `ui_move_01.wav`, `ui_move_02.wav` — move selection / hover
- `ui_select_01.wav` — confirm / click
- `ui_back_01.wav` — cancel / back
- `ui_pause_in.wav`, `ui_pause_out.wav` — pause / resume

### Gameplay core (`assets/audio/sfx/game/`)

- `ball_launch_01.wav`
- `hit_paddle_01.wav` (+ variations)
- `hit_wall_01.wav` (+ variations)
- `life_lost_01.wav` (ball lost)
- `life_gain_01.wav` (extra life pickup)

### Bricks (`assets/audio/sfx/bricks/`)

- `hit_brick_01.wav` (+ variations)
- `brick_crack_01.wav` (+ variations)
- `brick_break_01.wav` (+ variations)

### Power-ups (`assets/audio/sfx/powerups/`)

- `powerup_spawn_01.wav` (spawn)
- `powerup_pickup_01.wav` (+ variations)
- `powerup_expand_on.wav`, `powerup_expand_off.wav`
- `powerup_slow_on.wav`, `powerup_slow_off.wav`
- `powerup_multiball_spawn.wav`

### Endless (`assets/audio/sfx/endless/`)

- `endless_row_spawn_01.wav` — when a new row is inserted
- `endless_warning_loop.wav` — when the danger UI is active (loop with fade)
- `endless_warning_end.wav` — exit warning
- `endless_fail_01.wav` — Endless game over

### Stingers (`assets/audio/sfx/stingers/`)

- `stinger_win.wav`
- `stinger_game_over.wav`

---

## Proposed music loops (future)

- `music_menu_loop.ogg`
- `music_gameplay_loop.ogg` (Normal)
- `music_endless_loop.ogg` (Endless)
- `music_pause_loop.ogg` (optional)

Recommended transitions:

- crossfade ~0.3–0.6s between loops
- duck music briefly under major stingers

---

## Integration sketch (future)

Minimal API (example):

- `playSfx(name, volume=1.0f, pitch=1.0f)`
- `playMusic(name, loop=true, fade=0.5f)`
- `stopMusic(fade=0.5f)`
- bus volumes: `MASTER`, `MUSIC`, `SFX`, `UI`


