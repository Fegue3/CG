# Breakout3D — Threading / Async Work (Current)

This project is **mostly single-threaded** (game loop + OpenGL rendering on one thread).

The main exception is **power-up GIF preview decoding**, which uses worker threads to avoid UI stalls.

Source code lives under `Breakout3D/Breakout3D/`.

---

## Why we need threads at all

Opening **Instructions → Powerups** can display animated previews. GIF decoding is CPU-heavy and can freeze the UI if done synchronously.

So we split the work:

- **Worker threads (CPU)**: decode `.gif` bytes into RGBA frames in CPU memory.
- **Main thread (GPU / OpenGL context owner)**: create OpenGL textures from those frames.

OpenGL texture creation (`glTexImage2D`, etc.) must happen on the thread that owns the context (the game/render thread), so uploads are explicitly kept on the main thread.

---

## Where threads are created

- Decode threads are managed in `src/game/GameAssets.cpp`:
  - `GameAssets::startPowerupVideoPreload()`
  - `GameAssets::powerupVideo(int idx)` (lazy-start for a single preview)

Thread storage and synchronization primitives live in `include/game/GameAssets.hpp`:

- `powerupVideoThreads[8]`
- `powerupVideoMutex[8]`
- `powerupVideoDecodedData[8]`
- atomics: `powerupVideoDecoding/Decoded/Tried`, plus `powerupVideoPreloadStarted`

---

## Threading model (what runs where)

### 1) Decode on worker threads (CPU)

Each preview has an optional decode thread:

- File read (binary) → decode with `stbi_load_gif_from_memory(...)`
- Output: a vector of **RGBA frames** + **frame delays**
- Performance caps:
  - frame step: every 2nd frame
  - max frames kept: 120

Decoded output is stored under a per-video mutex in:

- `GameAssets::powerupVideoDecodedData[idx]`

### 2) Upload on the main/render thread (GPU)

Uploads are performed incrementally to avoid hitches:

- `GameAssets::pumpPowerupVideoPreload(6)` uploads a small number of frames per tick (round-robin across previews).
- `GameAssets::powerupVideo(idx)` can also upload a few frames per call if that preview is requested before the preload completes.

Each uploaded frame becomes a `engine::Texture2D` (one GL texture per frame), stored in:

- `GameAssets::powerupVideos[idx]` (an `engine::AnimatedTexture2D`)

---

## Lifetime & shutdown rules

Threads are joined safely in `GameAssets::destroy()`:

- `joinIfRunning(thread)` is called for each preview thread if joinable.
- CPU-side decoded buffers are cleared.
- GPU-side `AnimatedTexture2D` frames are destroyed.

This prevents:

- leaving joinable threads alive at program shutdown
- use-after-free of `GameAssets` captured by the decode lambdas

---

## Pitfalls / constraints (important)

- **No OpenGL calls off-thread**: worker threads only decode bytes; they must never create GL resources.
- **Memory pressure**: GIFs are uploaded as **one texture per frame**. Large/high-frame-count GIFs can consume a lot of GPU memory.
- **Synchronization**: decoded frame buffers must only be read/written under `powerupVideoMutex[idx]`.

---

## Related docs

- `docs/GIF_PREVIEWS.md` (filenames + preview UI behavior)
- `docs/RENDERING_OPENGL.md` (render passes and why GL work is main-thread-only)


