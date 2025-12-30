# Breakout3D — Architecture (as implemented)

Goal: keep the codebase **scalable**, **performant**, and **easy to maintain**, with a clear separation between:

- **Engine / OpenGL / infrastructure**
- **Game rules (Breakout3D)**

> Source code lives under `Breakout3D/Breakout3D/`.

---

## Directory layout (current)

```text
Breakout3D/Breakout3D/
  Makefile

  assets/
    shaders/
    models/
    textures/
    fonts/

  external/
    stb_image.h
    stb_truetype.h

  include/
    engine/
      Window.hpp
      Input.hpp
      Time.hpp
      Shader.hpp
      Texture.hpp
      Mesh.hpp
      Renderer.hpp

    game/
      Game.hpp
      GameState.hpp
      GameConfig.hpp
      GameAssets.hpp

      entities/
        Ball.hpp
        Brick.hpp
        PowerUp.hpp

      systems/
        InitSystem.hpp
        InputSystem.hpp
        PhysicsSystem.hpp
        CollisionSystem.hpp
        PowerUpSystem.hpp

  src/
    main.cpp

    engine/
      Window.cpp
      Input.cpp
      Time.cpp
      Shader.cpp
      Texture.cpp
      Mesh.cpp
      Renderer.cpp

    game/
      Game.cpp (ctor/init)
      GameUpdate.cpp (update loop)
      GameRender.cpp (render loop)
      GameUpdate.cpp
      GameRender.cpp
      GameState.cpp
      GameConfig.cpp
      GameAssets.cpp

      systems/
        InitSystem.cpp
        InputSystem.cpp
        PhysicsSystem.cpp
        CollisionSystem.cpp
        PowerUpSystem.cpp

      render/menu/
        MenuRenderBase.cpp
        MenuRenderScreens.cpp
        MenuRenderPlayModes.cpp
        MenuRenderLevelSelect.cpp
        MenuRenderInstructionsOverlay.cpp
```

---

## Dependency rules

- The **engine** layer does **not** depend on the game layer.
- The **game** layer can use engine types (renderer/window/input/time).
- `main.cpp` wires everything together.

Simplified dependency diagram:

```text
external libs (GLFW/GLEW/GLM/stb)
        ↓
      engine/
        ↓
       game/
        ↓
     src/main.cpp
```

---

## Engine layer (infrastructure)

### Window (`engine::Window`)

Responsible for:

- Creating/destroying the GLFW window + OpenGL context
- Framebuffer sizing
- Event polling + buffer swap
- Requesting close

### Input (`engine::Input`)

Responsible for:

- Raw key/mouse state (including edge detection via `keyPressed`)
- Mouse position in framebuffer pixels

Game “actions” (move paddle, click menu buttons, pause, etc.) are implemented in `game::InputSystem`.

### Renderer (`engine::Renderer`)

Provides two rendering paths:

- **3D pass**: lit textured meshes (walls, bricks, paddle, balls, powerups)
- **UI pass**: ortho quads/triangles + text

UI text uses **`stb_truetype`** with a baked atlas loaded from `assets/fonts/*.ttf`.

---

## Game layer (Breakout3D rules)

### GameConfig (`game::GameConfig`)

Holds gameplay parameters (arena bounds, speeds, sizes, power-up durations, etc.). See:

- `Breakout3D/Breakout3D/include/game/GameConfig.hpp`

### GameState (`game::GameState`)

Owns all mutable state for a run:

- Mode: `MENU`, `PLAYING`, `PAUSED`, `GAME_OVER`, `WIN`
- Type: `NORMAL` or `ENDLESS`
- Lists: bricks, balls, powerups
- Timers: powerups, hit cooldowns, Endless mode timers

See:

- `Breakout3D/Breakout3D/include/game/GameState.hpp`

### Systems

Each system is stateless (static functions) and operates on `GameState`:

- `InitSystem`: initialization + brick field generation + incremental row insertions (Endless)
- `InputSystem`: menu clicks + in-game controls (pause, camera, background selector, ball launch)
- `PhysicsSystem`: paddle/ball integration, clamps, ball removal
- `CollisionSystem`: wall/paddle/brick collisions + scoring hooks
- `PowerUpSystem`: power-up spawning, falling, pickup, effects

---

## Entry point (`src/main.cpp`)

`Breakout3D/Breakout3D/src/main.cpp`:

- Creates the window (currently default `1280x900`)
- Creates renderer + assets + game
- Runs the update/render loop until close

---

## Notes / known design points

- “Wave” (`GameState::wave`) exists for Endless but is **not currently incremented**, so any wave-based bonus is constant within a run.
- The best-score persistence path exists in two places (Init/Game). This is fine but could be deduplicated if desired.

---

## Related engineering docs

- `docs/RENDERING_OPENGL.md` (rendering pipeline and OpenGL constraints)
- `docs/THREADING.md` (async GIF preview decoding/upload)
- `docs/ROGUE_MODE.md` and `docs/ROGUE_CARDS.md` (Rogue systems and cards)

