# Breakout3D

A 3D Breakout game implemented in C++ with OpenGL.

## Project Documentation

- **[docs/README.md](./docs/README.md)** - Documentation index (start here)
- **[docs/PROJECT_CONTEXT.md](./docs/PROJECT_CONTEXT.md)** - Current project overview (code-accurate)
- **[docs/CONTROLS_AND_DEBUG.md](./docs/CONTROLS_AND_DEBUG.md)** - Controls, modes, power-ups, and debug-only keybinds
- **[docs/Breakout3D_Architecture.md](./docs/Breakout3D_Architecture.md)** - Architecture (engine/game split and systems)
- **[docs/RENDERING_OPENGL.md](./docs/RENDERING_OPENGL.md)** - Rendering/OpenGL notes
- **[docs/THREADING.md](./docs/THREADING.md)** - Thread usage (GIF preview decoding + main-thread GL upload)
- **[docs/ENDLESS_MODE.md](./docs/ENDLESS_MODE.md)** - Endless mode deep dive
- **[docs/ROGUE_MODE.md](./docs/ROGUE_MODE.md)** - Rogue mode deep dive
- **[docs/ROGUE_CARDS.md](./docs/ROGUE_CARDS.md)** - Rogue cards list and effects

## Quick Start

1. Go to `Breakout3D/Breakout3D`
2. Run `make`
3. Run `./breakout3d` (or `make run`)

## Project Structure

The project follows a **component-based architecture** with clear separation:

### Engine Layer (`engine/`)
Reusable graphics and system utilities:
- Window management (GLFW)
- Rendering (OpenGL abstractions)
- Input handling
- Time management
- Asset loading (shaders, textures, meshes)

### Game Layer (`game/`)
Breakout-specific logic organized into:
- **Entities**: Data structures (Ball, Brick, Paddle, PowerUp)
- **Systems**: Modular logic (Init, Input, Physics, Collision, PowerUp)
- **Core**: Game orchestrator, state, config, assets

### Architecture Benefits
- ✅ **Maintainable**: Single-responsibility systems
- ✅ **Testable**: Stateless, modular design
- ✅ **Scalable**: Easy to add features
- ✅ **Clean**: `Game.cpp` reduced from 1085 to ~176 lines

For detailed architecture information, see [Breakout3D_Architecture.md](./docs/Breakout3D_Architecture.md).
