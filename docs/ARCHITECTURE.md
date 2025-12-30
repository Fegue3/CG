# Architecture

## Top-level layout

- `src/engine/`: windowing, input, rendering, timing
- `src/game/`: game orchestration + systems + rendering + UI
- `assets/`: models, textures, shaders, fonts, audio
- `include/`: public headers

## Main loop

- `game::Game::update()` lives in `src/game/GameUpdate.cpp`
  - It’s intentionally small and delegates to `src/game/update/*.cpp` helper modules.
- `game::Game::render()` draws world + UI/HUD.

## Systems (gameplay)

Core systems live in `src/game/systems/`:

- `InitSystem`: initializes levels/modes and spawns brick patterns
- `PhysicsSystem`: updates paddle/balls and basic physics
- `CollisionSystem`: resolves collisions and reports collision info
- `PowerUpSystem`: spawns + updates powerups (and buffers events for audio)
- `RogueSystem`: wave progression rules + rogue-specific pacing
- `InputSystem`: gameplay input; menu input is in `InputSystemMenu.cpp`

## Update modules

The large update logic is split into focused modules under `src/game/update/`:

- `GameUpdateMenu.cpp`: menu state + menu SFX/music
- `GameUpdatePlaying.cpp`: core gameplay, waves, spawns, music director
- `GameUpdateTimers.cpp`: timers + VFX timers + debug helpers
- `GameUpdatePausedOverlay.cpp`: pause UI click handling
- `GameUpdateRogueCardsOverlay.cpp`: rogue card pick UI click handling
- `GameUpdateEndOverlay.cpp`: game over / win UI click handling
- `GameUpdateWinFinisher.cpp`: “win finisher” cinematic timing


