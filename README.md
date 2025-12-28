# Breakout3D

A 3D Breakout game implemented in C++ with OpenGL.

## Project Documentation

- **[PROJECT_CONTEXT.md](./docs/PROJECT_CONTEXT.md)** - Comprehensive project overview and technical details
- **[Breakout3D_Architecture.md](./docs/Breakout3D_Architecture.md)** - Detailed architecture documentation (in Portuguese)
- **[STRUCTURE_ANALYSIS.md](./STRUCTURE_ANALYSIS.md)** - Structure analysis and refactoring summary
- **[CONTROLS_AND_DEBUG.md](./docs/CONTROLS_AND_DEBUG.md)** - Controls (menu/game), modes, power-ups, and debug-only keybinds

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
