# Breakout3D - Project Context & Documentation

This document provides a comprehensive overview of the **Breakout3D** project, designed to give an LLM or developer full context on the architecture, features, and implementation details.

## 1. Project Overview
**Breakout3D** is a 3D implementation of the classic Breakout game, developed as part of a Computer Graphics (CG) course. While the visuals are 3D, the gameplay logic is primarily constrained to the **XZ plane**.

- **Type**: 3D Arcade Game
- **Language**: C++17
- **Graphics API**: OpenGL (via GLEW & GLFW)
- **Math Library**: GLM (OpenGL Mathematics)

---

## 2. Architecture

The project follows a **component-based architecture** split into two main layers: the **Engine Layer** (reusable utilities) and the **Game Layer** (logic specific to Breakout3D).

### A. Engine Layer (`src/engine`, `include/engine`)
Handles low-level system interactions and graphics abstractions:
- **`Window`**: Manages the GLFW window creation, framebuffer sizing, and buffer swapping.
- **`Renderer`**: The core rendering engine. Supports:
    - **3D Pass**: Drawing meshes with shaders, camera matrices (VP), and lighting/tinting.
    - **UI/2D Pass**: Orthographic projection for HUD, text, and quads.
    - **Backgrounds**: Dedicated method for full-screen background textures.
- **`Shader`**: Loads, compiles, and manages GLSL shader programs.
- **`Texture`**: Handles texture loading and binding.
- **`Mesh`**: Loads and stores 3D geometry from `.obj` files, including material (`.mtl`) support.
- **`Input`**: Abstracts GLFW input (keyboard and mouse) into a cleaner API.
- **`Time`**: Manages delta time and frame-independent updates.

### B. Game Layer (`src/game`, `include/game`)
Handles the high-level game state and mechanics using a **system-based architecture**:

#### Core Components:
- **`Game`**: The main controller class (~176 lines). Orchestrates systems, manages the game loop, initialization, updates, and rendering calls.
- **`GameState`**: A struct-heavy container for the current state of the world (balls, paddle, bricks, power-ups, scores, lives, active timers, UI state).
- **`GameConfig`**: Static and tunable parameters (speeds, sizes, arena bounds, power-up settings).
- **`GameAssets`**: A centralized loader for all models and textures.

#### Entities (`game/entities/`):
Header-only data structures:
- **`Ball`**: Position, velocity, attachment state
- **`Brick`**: Position, size, health points (HP system)
- **`Paddle`**: Position (size/velocity from GameConfig)
- **`PowerUp`**: Type, position, alive state (includes `PowerUpType` enum)

#### Systems (`game/systems/`):
Modular, stateless systems that process game logic:
- **`InitSystem`**: Game initialization, brick generation, ball/paddle reset
- **`InputSystem`**: Menu input handling, game input (paddle movement, ball launch, pause, camera switching)
- **`PhysicsSystem`**: Ball and paddle movement, physics integration
- **`CollisionSystem`**: All collision detection and resolution (walls, paddle, bricks)
- **`PowerUpSystem`**: Power-up spawning, falling physics, collection, and effect application

**Architecture Benefits:**
- `Game.cpp` reduced from **1085 lines** to **~176 lines** through system delegation
- Each system has a single, clear responsibility
- Systems are stateless (static methods) making them easily testable
- Clear separation of concerns improves maintainability

---

## 3. Gameplay Features & Mechanics

### Core Loop
1.  **Launch**: Ball starts attached to the paddle; spacebar launches it.
2.  **Bounce**: Ball bounces off walls (MinX, MaxX, MinZ) and the paddle.
3.  **Break**: Ball destroys bricks on impact. Bricks have varying HP (1 to 6 in endless mode).
4.  **Lives**: Lose a life if all balls fall past the paddle (MaxZ border).
5.  **Win/Loss**: Clear all bricks to win (normal mode); lose all lives for Game Over.

### Game Modes
- **Normal Mode**: Clear all bricks to win. Traditional Breakout gameplay.
- **Endless Mode**: Infinite waves. Bricks spawn dynamically as you destroy them. No win condition - see how long you can survive.

### Bricks & Health System
Bricks change appearance based on their current health:
- **Green**: 1 HP
- **Yellow**: 2 HP (cracks at 1 HP)
- **Blue**: 3 HP (cracks at 2, 1 HP)
- **Purple**: 4 HP (cracks at 3, 2, 1 HP)
- **Harder bricks**: Up to 6 HP in endless mode (based on wave difficulty)

Cracking effect is achieved by swapping meshes/textures (`brick02_1hit`, `brick03_1hit`, `brick03_2hit`, etc.).

### Power-Ups
When a brick is destroyed, there's a chance (`powerUpChance = 40%`) to spawn a power-up:
- **Extra Life** (15%): Adds a life (Heart icon).
- **Expand** (28%): Grows the paddle width temporarily (`expandScaleFactor = 1.6x`).
- **Slow** (28%): Slows down the paddle movement (`slowSpeedFactor = 0.5x`).
- **Extra Ball** (29%): Spawns 3 new balls from the paddle/current ball position.

Power-ups fall downward and are collected by the paddle. Effects have a duration (`powerUpDuration = 7 seconds`).

### Camera Modes
Toggle with keys 1/2:
- **Mode 1 (Top-Down)**: Standard focused view from above.
- **Mode 2 (Angled)**: A more cinematic, tight-zoomed perspective.

### Visual Customization
- **Background Selector**: A HUD element (top right) allows switching between different space/nebula backgrounds or a "No Background" solid color.

---

## 4. Technical Implementation Details

### Coordinate System
- **Plane**: Gameplay occurs on the **XZ plane** (`Y=0`).
- **X-axis**: Lateral movement (Left/Right). Bounds: `arenaMinX` to `arenaMaxX`.
- **Z-axis**: Depth movement (Forward/Back). `arenaMinZ` is the top (bricks), `arenaMaxZ` is the bottom (paddle/fail zone).
- **Y-axis**: Used for visual height (UI hearts, power-up bobbing/spinning, camera position).

### Asset Pipeline
- **Models**: Loaded from `assets/models/` as `.obj` files.
- **Materials**: `.mtl` files define textures via `map_Kd`.
- **Shaders**: Located in `assets/shaders/`. Standard implementation uses basic lighting and texture mapping.
- **Textures**: Loaded via `stb_image.h`, bound to OpenGL texture units.

### Collision Detection
- Uses **Sphere-AABB collision** in the XZ plane (`sphereAabbXZ()`).
- Custom logic in `CollisionSystem` handles bounce angles:
  - **Walls**: Simple reflection (flip velocity component)
  - **Paddle**: Angle based on normalized distance from center (up to 60 degrees)
  - **Bricks**: Determines collision axis (X or Z) and reflects accordingly

### System Architecture Details

**Update Loop** (in `Game::update()`):
1. Handle menu input if in MENU mode
2. Handle game input (movement, pause, camera)
3. Update timers (power-ups, cooldowns)
4. Check win/loss conditions
5. Update physics (balls move)
6. Process collisions (walls, paddle, bricks)
7. Spawn new bricks if needed (endless mode)
8. Update power-ups (falling, collection)
9. Handle lives loss

**Rendering** (in `Game::render()`):
1. Menu rendering (if in MENU mode)
2. Set camera matrices (based on camera mode)
3. 3D Pass: Draw walls, bricks, paddle, balls, power-ups
4. UI Pass: Draw HUD (lives, score, wave), overlays (pause, game over, win)

---

## 5. Directory Structure

```text
Breakout3D/Breakout3D/
├── assets/             # Raw resources
│   ├── models/         # .obj and .mtl files
│   ├── shaders/        # .glsl/.vert/.frag files
│   └── textures/       # Image assets (PNG, etc.)
│
├── external/           # Third-party headers
│   └── stb_image.h     # Image loading
│
├── include/
│   ├── engine/         # Engine layer (7 files)
│   │   ├── Window.hpp
│   │   ├── Input.hpp
│   │   ├── Time.hpp
│   │   ├── Shader.hpp
│   │   ├── Texture.hpp
│   │   ├── Mesh.hpp
│   │   └── Renderer.hpp
│   │
│   └── game/           # Game layer
│       ├── Game.hpp
│       ├── GameState.hpp
│       ├── GameConfig.hpp
│       ├── GameAssets.hpp
│       ├── entities/   # Data structures (4 files)
│       │   ├── Ball.hpp
│       │   ├── Brick.hpp
│       │   ├── Paddle.hpp
│       │   └── PowerUp.hpp
│       └── systems/    # Logic systems (5 files)
│           ├── InitSystem.hpp
│           ├── InputSystem.hpp
│           ├── PhysicsSystem.hpp
│           ├── CollisionSystem.hpp
│           └── PowerUpSystem.hpp
│
├── src/
│   ├── main.cpp        # Entry point
│   │
│   ├── engine/         # Engine implementation (7 files)
│   │   └── [.cpp files matching include/engine/]
│   │
│   └── game/           # Game implementation
│       ├── Game.cpp        (~176 lines - orchestrator)
│       ├── GameState.cpp
│       ├── GameConfig.cpp
│       ├── GameAssets.cpp
│       └── systems/        # System implementation (5 files)
│           └── [.cpp files matching include/game/systems/]
│
└── Makefile           # Build configuration
```

### File Size Breakdown

- **Game.cpp**: ~176 lines (was 1085 lines before refactoring)
- **System files**: ~50-200 lines each (focused, single responsibility)
- **Entity files**: Header-only, ~10-20 lines each
- **Engine files**: ~50-300 lines each

---

## 6. Build & Run

```bash
cd Breakout3D/Breakout3D
make          # Build
make run      # Build and run
make clean    # Clean build artifacts
```

**Dependencies**:
- C++17 compatible compiler (g++/clang++)
- OpenGL development libraries
- GLFW3
- GLEW
- GLM (header-only)

---

## 7. Key Design Decisions

1. **System-Based Architecture**: Separated game logic into independent systems for better maintainability and testability.
2. **Stateless Systems**: All system methods are static, taking state as parameters. This makes testing easier and dependencies explicit.
3. **Entity/Component Split**: Entities are pure data structures; all logic is in systems.
4. **Engine Independence**: Engine layer has zero knowledge of game logic, making it reusable.
5. **No ResourceManager**: Assets are loaded directly in `GameAssets` class. Could be extracted if needed.
6. **Direct Input Handling**: Input mapping is done directly in `InputSystem` rather than a separate mapping layer.

---

## 8. Future Enhancements

Potential improvements:
- **LevelSystem**: Dedicated system for level/wave progression logic
- **GameStateSystem**: System for global state rules (lives management, win/loss conditions)
- **InputMapping Layer**: Abstract input to game actions for easier control remapping
- **ResourceManager**: Centralized asset management with caching
- **Camera Class**: Separate camera management from rendering
- **Save System**: Persistent high scores, settings

---

This document reflects the **current implementation** as of the refactoring completion.
