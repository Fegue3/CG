# Breakout3D — Rendering / OpenGL Notes (Current)

Source code lives under `Breakout3D/Breakout3D/`.

This document focuses on **how rendering is structured** and which OpenGL constraints shape the code.

---

## Coordinate system and gameplay plane

- Gameplay is mostly on the **XZ plane**:
  - **X**: left/right
  - **Z**: “upfield/downfield” (bricks are toward `arenaMinZ`, losing the ball is toward `arenaMaxZ`)
  - **Y**: used for visuals (camera height, mesh height, UI placement)

Arena bounds are in `include/game/GameConfig.hpp`:

- `arenaMinX/arenaMaxX`
- `arenaMinZ/arenaMaxZ`

---

## Render pipeline (high level)

Rendering is effectively a two-pass model:

### 1) World / 3D pass

Draws world meshes:

- walls
- bricks (visual varies with HP)
- paddle
- balls
- power-ups

Typical setup:

- perspective projection
- view matrix depends on camera mode (see `cameraMode` in `GameState`)
- basic lighting shader (mesh normals + textures)

### 2) UI / 2D pass (orthographic)

Draws:

- menu screens + instructions overlay (including Powerups + Rogue Cards browsers)
- HUD (lives/hearts, background selector)
- overlays (pause/game over/win)
- danger band (“DANGER!”) for Endless/Rogue

Text rendering uses `stb_truetype` baked atlas via `engine::Renderer::drawUIText(...)`.

---

## UI and menu rendering layout

The menu is drawn using real font metrics and cached layout rectangles in `GameState`:

- `GameState::menuLayout` is recomputed in MENU mode every frame (to stay aligned with framebuffer size)
- input hit-testing uses the same computed rectangles (to prevent “click mismatch” bugs)

See:

- `src/game/render/MenuRender.cpp` (visuals)
- `src/game/systems/InputSystem.cpp` (hit-testing + state changes)

---

## Animated power-up previews (OpenGL constraints)

Powerup previews are animated GIFs.

Important constraint:

- **OpenGL resources must be created on the thread that owns the context**.

So the implementation decodes GIFs off-thread, but uploads frames as GL textures on the main thread.

See:

- `docs/THREADING.md`
- `docs/GIF_PREVIEWS.md`

---

## Common OpenGL “gotchas” handled by design

- **Framebuffer size vs window size**: the project uses framebuffer pixel coordinates for mouse hit-testing and UI layout (important on HiDPI displays).
- **Texture upload must be main-thread**: enforced by design for GIF previews.
- **Text rendering cost**: UI text can be expensive if drawn per-glyph with per-frame buffer creation; the current approach is fine for the project scope but is a known optimization area.

---

## OpenGL context + global render state

Context creation happens in `engine::Window::create(...)`:

- OpenGL **3.3**
- profile: `GLFW_OPENGL_COMPAT_PROFILE`
- vsync: `glfwSwapInterval(1)`

Renderer init (`engine::Renderer::init()`):

- `glEnable(GL_DEPTH_TEST)`
- `glEnable(GL_BLEND)`
- `glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)`

Frame start (`Renderer::beginFrame(fbW, fbH)`):

- `glViewport(0, 0, fbW, fbH)`
- clear color + `glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)`

---

## Shaders and draw modes

The engine uses one main shader program:

- `assets/shaders/basic_phong.vert`
- `assets/shaders/basic_phong.frag`

It is used for:

- **3D meshes** (Phong-ish lighting)
- **UI quads/triangles/text** (special “texture modes” controlled by uniforms)

The UI pass reuses a single VAO/VBO (`Renderer` keeps `m_uiVao/m_uiVbo`) for quads, triangles, and glyphs to reduce VAO/VBO churn.

---

## Background rendering

`Renderer::drawBackground(textureId)` draws a fullscreen quad:

- temporarily disables depth test
- binds the texture and draws a static screen-aligned mesh

This is also where the code explicitly resets any UI-related alpha/mask uniforms, so UI state does not “leak” into the background.

---

## UI text rendering (TTF via stb_truetype)

`Renderer` bakes an ASCII atlas (32..127) at startup:

- TTF is loaded from `assets/fonts/` (default: Orbitron Bold)
- atlas is uploaded as a `GL_RED` texture
- glyph metrics come from `stbtt_bakedchar[96]`

The game/UI code uses a legacy “scale” concept; the renderer maps that onto a higher-resolution baked atlas for cleaner large titles.

---

## Where to look in code

- **Engine rendering abstractions**:
  - `include/engine/Renderer.hpp`, `src/engine/Renderer.cpp`
  - `include/engine/Shader.hpp`, `src/engine/Shader.cpp`
  - `include/engine/Texture.hpp`, `src/engine/Texture.cpp`
  - `include/engine/Mesh.hpp`, `src/engine/Mesh.cpp`

- **Game rendering orchestration**:
  - `src/game/Game.cpp` (calls into world + UI render paths)
  - `src/game/render/WorldRender.cpp`
  - `src/game/render/UIRender.cpp`
  - `src/game/render/MenuRender.cpp`


