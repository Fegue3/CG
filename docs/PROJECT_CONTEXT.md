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
The project is split into two main layers: the **Engine Layer** (reusable utilities) and the **Game Layer** (logic specific to Breakout3D).

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
Handles the high-level game state and mechanics:
- **`Game`**: The main controller class. Manages the game loop, initialization, updates, and rendering calls.
- **`GameState`**: A struct-heavy container for the current state of the world (balls, paddle, bricks, power-ups, scores, lives, active timers).
- **`GameConfig`**: Static and tunable parameters (speeds, sizes, arena bounds).
- **`GameAssets`**: A centralized loader for all models and textures.
- **`Entities`**: Logic for specific game objects (e.g., `Brick` health and persistence).

---

## 3. Gameplay Features & Mechanics

### Core Loop
1.  **Launch**: Ball starts attached to the paddle; spacebar launches it.
2.  **Bounce**: Ball bounces off walls (MinX, MaxX, MinZ) and the paddle.
3.  **Break**: Ball destroys bricks on impact. Bricks have varying HP (1 to 4).
4.  **Lives**: Lose a life if all balls fall past the paddle (MaxZ border).
5.  **Win/Loss**: Clear all bricks to win; lose all lives for Game Over.

### Bricks & Health System
Bricks change appearance based on their current health:
- **Purple**: 4 HP (Cracks at 3, 2, 1).
- **Blue**: 3 HP (Cracks at 2, 1).
- **Yellow**: 2 HP (Cracks at 1).
- **Green**: 1 HP.
- Cracking effect is achieved by swapping meshes/textures (`brick04_1hit`, `brick04_2hit`, etc.).

### Power-Ups
When a brick is destroyed, there's a chance (`powerUpChance`) to spawn a power-up:
- **Extra Life**: Adds a life (Heart).
- **Expand**: Grows the paddle width temporarily.
- **Slow**: Slows down the paddle movement (negative debuff or challenge).
- **Extra Ball**: Spawns 3 new balls from the paddle.

### Camera Modes
- **Mode 1 (Top-Down)**: Standard focused view from above.
- **Mode 2 (Angled)**: A more cinematic, tight-zoomed perspective.

### Visual Customization
- **Background Selector**: A HUD element (top right) allows switching between different space/nebula backgrounds or a "No Background" solid color.

---

## 4. Technical Implementation Details

### Coordinate System
- **Plane**: Gameplay occurs on the **XZ plane** (`Y=0`).
- **X-axis**: Lateral movement (Left/Right).
- **Z-axis**: Depth movement (Forward/Back). `MinZ` is the top (bricks), `MaxZ` is the bottom (paddle/fail zone).
- **Y-axis**: Used for visual height (UI hearts, power-up bobbing/spinning).

### Asset Pipeline
- **Models**: Loaded from `assets/models/` as `.obj` files.
- **Materials**: `.mtl` files define textures via `map_Kd`.
- **Shaders**: Located in `assets/shaders/`. Standard implementation uses basic lighting and texture mapping.

### Collision Detection
- Uses **Sphere-AABB collision** in the XZ plane.
- Custom logic in `Game::update` handles bounce angles based on where the ball hits the paddle (normalized distance from center).

---

## 5. Directory Structure
```text
.
├── Breakout3D/
│   ├── assets/             # Raw resources
│   │   ├── models/         # .obj and .mtl files
│   │   ├── shaders/        # .glsl files
│   │   └── textures/       # Image assets
│   ├── include/            # Header files (.hpp)
│   │   ├── engine/         # Engine subsystem headers
│   │   └── game/           # Game logic headers
│   ├── src/                # Implementation files (.cpp)
│   │   ├── engine/         # Engine subsystem logic
│   │   ├── game/           # Game logic and entities
│   │   └── main.cpp        # Entry point
│   ├── Makefile            # Build configuration
│   └── breakout3d          # Compiled binary (after make)
└── PROJECT_CONTEXT.md      # This file
```

---

## 6. Build and Run
### Requirements
- Linux or macOS
- GCC (supporting C++17)
- Libraries: `GLEW`, `GLFW`, `OpenGL`

### Commands
```bash
cd Breakout3D/Breakout3D
make        # Compile the project
make run    # Execute the game
make clean  # Remove build artifacts
```

---

## 7. Context for LLMs
When assisting with this codebase, keep in mind:
- **Coordinate System**: Always think in XZ for gameplay logic.
- **Renderer Hierarchy**: Don't call OpenGL functions directly in the game logic; use the `Renderer` abstraction.
- **Asset Loading**: New models must be added to `GameAssets.cpp` / `GameAssets.hpp` and loaded in `loadAll()`.
- **Entity State**: `GameState` is the source of truth for all dynamic objects. Ensure updates are frame-rate independent using `m_time.delta()`.
