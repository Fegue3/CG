#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace engine { class Renderer; }

namespace game::render::ui {

// Small UI helpers shared by menu / HUD rendering.
float fitScaleToWidth(engine::Renderer& renderer, const std::string& text, float desiredScale, float maxWidthPx);

std::vector<std::string> wrapText(engine::Renderer& renderer, const std::string& text, float scale, float maxWidthPx);

void drawWrappedText(engine::Renderer& renderer,
                     float x, float yTop,
                     float maxWidthPx,
                     const std::string& text,
                     float scale,
                     const glm::vec4& color,
                     float lineGapPx);

glm::vec3 hsv2rgb(float h, float s, float v);

// Neon-styled key badge
void drawKeyBadge(engine::Renderer& renderer, float x, float y, const std::string& label, float scale, float time);

// Button with hover effects (used in pause/game over overlays)
void drawOverlayButton(engine::Renderer& renderer,
                       float bx, float by, float bw, float bh,
                       const std::string& label, const glm::vec3& baseColor,
                       bool hovered);

} // namespace game::render::ui


