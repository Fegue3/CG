# Shaders

This project currently uses a single “do-most-things” shader pair:

- `assets/shaders/basic_phong.vert`
- `assets/shaders/basic_phong.frag`

The renderer loads them in `engine::Renderer::init()`:

- `src/engine/Renderer.cpp` loads:
  - `assets/shaders/basic_phong.vert`
  - `assets/shaders/basic_phong.frag`

## Vertex shader (`basic_phong.vert`)

Inputs:

- `aPos` (location 0): position
- `aNormal` (location 1): normal
- `aUV` (location 2): UV

Uniforms:

- `uM`, `uV`, `uP`: model/view/projection matrices

Outputs to fragment:

- `vWorldPos`, `vNormal`, `vUV`

## Fragment shader (`basic_phong.frag`)

Lighting uniforms:

- `uViewPos`
- `uLightPos`
- `uLightColor`
- `uAmbientK`, `uDiffuseK`, `uSpecK`, `uShininess`

Material/texture uniforms:

- `uAlbedo`
- `uUseTex` (0/1)
- `uTex` (sampler2D)
- `uTexMode`
  - `0`: modulate `uAlbedo` with `texture(uTex).rgb`
  - `1`: treat `texture(uTex).r` as an **alpha mask** (font atlas style)

Alpha/masking uniforms:

- `uAlpha` (explicit; CPU should always set it)
- `uUseMask` (0/1)
- `uMaskMin`, `uMaskMax`
  - If `uUseMask == 1` and `gl_FragCoord` is inside `[uMaskMin, uMaskMax]`, alpha is forced to 0.
  - This is used as a cheap UI clip/mask.

## UI vs 3D usage

The same shader is reused for UI:

- UI typically sets `uDiffuseK = 0` and `uSpecK = 0` and relies on `uAmbientK` + `uAlpha`.
- Text uses `uTexMode = 1` (R-channel alpha mask).


