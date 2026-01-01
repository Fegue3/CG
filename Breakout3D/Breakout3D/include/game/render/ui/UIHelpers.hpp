// UIHelpers.hpp
#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace engine { class Renderer; }

namespace game::render::ui {

/**
 * @file UIHelpers.hpp
 * @brief Helpers pequenos partilhados por menu/HUD (texto, botões e utilitários visuais).
 */

// Calcula uma escala <= desiredScale para caber numa largura máxima (em px).
float fitScaleToWidth(engine::Renderer& renderer, const std::string& text, float desiredScale, float maxWidthPx);

// Faz wrap do texto em linhas, respeitando maxWidthPx para a escala dada.
std::vector<std::string> wrapText(engine::Renderer& renderer, const std::string& text, float scale, float maxWidthPx);

// Desenha texto com wrap automático (yTop = topo do bloco; lineGapPx adiciona espaçamento entre linhas).
void drawWrappedText(engine::Renderer& renderer,
                     float x, float yTop,
                     float maxWidthPx,
                     const std::string& text,
                     float scale,
                     const glm::vec4& color,
                     float lineGapPx);

// Conversão HSV->RGB (valores tipicamente em [0..1] ou conforme implementado no .cpp).
glm::vec3 hsv2rgb(float h, float s, float v);

// “Key badge” estilo neon (ex.: para instruções/controles).
void drawKeyBadge(engine::Renderer& renderer, float x, float y, const std::string& label, float scale, float time);

// Botão para overlays (pausa/game over), com efeito de hover.
void drawOverlayButton(engine::Renderer& renderer,
                       float bx, float by, float bw, float bh,
                       const std::string& label, const glm::vec3& baseColor,
                       bool hovered);

} // namespace game::render::ui
