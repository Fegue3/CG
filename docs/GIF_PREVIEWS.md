# Powerups GIF Previews (Instructions → Powerups)

This project shows “video previews” for powerups in the **Instructions → Powerups** screen by playing **animated GIFs** (not MP4) inside the UI.

## Why GIF (and not MP4)?

- The codebase already uses `stb_image` for images.
- `stb_image` **supports GIF decoding**, but it **does not decode MP4**.
- Adding a full MP4 stack (FFmpeg / libavformat+libavcodec) would introduce new native dependencies and build complexity.

## Where the GIFs live

Put the GIF files here:

- `Breakout3D/Breakout3D/assets/video/*.gif`

Expected filenames:

- `Expand_powerup.gif`
- `Extra-Ball_powerup.gif`
- `Extra-life_powerup.gif`
- `Fireball_powerup.gif`
- `Slow_powerup.gif`
- `Shield_powerup.gif`
- `Reserve_powerup.gif` (this corresponds to **REVERSE**)
- `Tiny_powerup.gif`

## How loading works (no menu lag)

Goal: **no freezing** when opening the Powerups inspector.

What we do instead:

### Threading model (what runs where)

#### 1) Decode on worker threads (CPU)

- **Where**: `Breakout3D/Breakout3D/src/game/GameAssets.cpp`
- **How**:
  - `GameAssets::startPowerupVideoPreload()` starts up to 8 `std::thread`s (one per GIF) in `powerupVideoThreads[]`.
  - Each thread reads the `.gif` file, then decodes it with:
    - `stbi_load_gif_from_memory(...)`
  - The decoded output is stored as raw RGBA bytes per frame (CPU memory) in:
    - `GameAssets::powerupVideoDecodedData[idx]`
- **Synchronization**:
  - The thread marks progress using atomics:
    - `powerupVideoDecoding[idx]`, `powerupVideoDecoded[idx]`, `powerupVideoTried[idx]`
  - Access to the decoded frame buffers is protected with a per-video mutex:
    - `powerupVideoMutex[idx]`

Why threads here? GIF decode is CPU-heavy and can stall the UI if done synchronously.

#### 2) Upload on the main/render thread (GPU)

- **Where**: `GameAssets::pumpPowerupVideoPreload(...)` (called from `Game::update()`)
- **How**:
  - Each tick, we upload a small number of frames (budgeted) using:
    - `engine::Texture2D::loadFromRGBA(...)`
  - Upload happens round-robin across the 8 videos so no single GIF dominates a frame.

Why main thread here? OpenGL texture creation (`glTexImage2D`, etc.) must happen on the thread that owns the GL context (the game’s render thread).

#### 3) Preload is automatic while running

- `Game::update()` calls every frame:
  - `m_assets.startPowerupVideoPreload();`  (kicks off decode threads once)
  - `m_assets.pumpPowerupVideoPreload(6);` (uploads a few frames per frame)

Result:

- The game starts quickly.
- GIFs become available progressively in the background.
- By the time you open **Instructions → Powerups**, previews are usually already ready (and if not, the UI shows “loading…” briefly).

## Performance knobs (current defaults)

To keep memory and load time reasonable, decoding keeps:

- **Max frames**: 120
- **Frame step**: 2 (keeps every other frame)
- **Upload budget per tick**: 6 frames (across all GIFs, round-robin)

You can tune these in:

- `Breakout3D/Breakout3D/src/game/GameAssets.cpp` (decode cap/step + upload budget behavior)
- `Breakout3D/Breakout3D/src/game/Game.cpp` (the `pumpPowerupVideoPreload(6)` call)

## Render path

The Powerups inspector draws the current GIF frame as a textured UI quad in:

- `Breakout3D/Breakout3D/src/game/render/MenuRender.cpp`

## Notes / caveats

- GIFs are uploaded as **one GL texture per frame**. Very long/high-res GIFs can consume a lot of GPU memory.
- If a GIF is missing, the UI will show a fallback message and keep working.


