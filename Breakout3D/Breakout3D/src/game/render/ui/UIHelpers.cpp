/**
 * @file UIHelpers.cpp
 * @brief Helpers de UI (texto e widgets simples) para render 2D.
 *
 * Contém utilitários para:
 * - Ajustar escala para caber numa largura (fitScaleToWidth)
 * - Quebrar texto em linhas (wrapText) e desenhar texto multi-linha (drawWrappedText)
 * - Converter HSV -> RGB (hsv2rgb) para efeitos neon
 * - Widgets rápidos: badge de tecla (drawKeyBadge) e botão de overlay (drawOverlayButton)
 *
 * Convenções:
 * - O sistema de UI aqui desenha texto com origem no canto inferior-esquerdo (y diminui para "baixo" visual).
 * - drawWrappedText assume yTop como “top” do bloco e vai descendo (y -= lh+gap).
 */
 #include "game/render/ui/UIHelpers.hpp"

 #include "engine/Renderer.hpp"
 
 #include <algorithm>
 #include <cmath>
 
 namespace game::render::ui {
 
 /**
  * @brief Ajusta uma escala desejada para o texto caber numa largura máxima.
  *
  * Uso típico: títulos que podem variar de comprimento (ex: "CHOOSE A CARD").
  * Se o texto já cabe, devolve desiredScale.
  */
 float fitScaleToWidth(engine::Renderer& renderer, const std::string& text, float desiredScale, float maxWidthPx) {
     float s = desiredScale;
     if (maxWidthPx <= 1.0f) return s;
 
     float w = renderer.measureUITextWidth(text, s);
     if (w > maxWidthPx && w > 1.0f) {
         // Regra de 3 direta: nova escala = escalaAtual * (max / atual).
         s *= (maxWidthPx / w);
     }
     return s;
 }
 
 /**
  * @brief Quebra uma string em várias linhas (word-wrapping) para caber em maxWidthPx.
  *
  * Regras:
  * - Quebra por palavras (espaços/tabs).
  * - '\n' força quebra de linha.
  * - Se uma palavra for maior do que maxWidthPx, ela fica sozinha numa linha (não faz hyphenation).
  */
 std::vector<std::string> wrapText(engine::Renderer& renderer, const std::string& text, float scale, float maxWidthPx) {
     std::vector<std::string> lines;
     if (maxWidthPx <= 1.0f || text.empty()) return lines;
 
     std::string cur;
     std::string word;
 
     auto flushWord = [&]() {
         if (word.empty()) return;
 
         if (cur.empty()) {
             cur = word;
         } else {
             std::string trial = cur + " " + word;
             if (renderer.measureUITextWidth(trial, scale) <= maxWidthPx) {
                 cur = trial;
             } else {
                 lines.push_back(cur);
                 cur = word;
             }
         }
         word.clear();
     };
 
     for (char ch : text) {
         if (ch == '\n') {
             flushWord();
             if (!cur.empty()) lines.push_back(cur);
             cur.clear();
             continue;
         }
         if (ch == ' ' || ch == '\t') {
             flushWord();
             continue;
         }
         word.push_back(ch);
     }
 
     flushWord();
     if (!cur.empty()) lines.push_back(cur);
     return lines;
 }
 
 /**
  * @brief Desenha texto multi-linha (wrap automático) num bloco com largura máxima.
  *
  * @param x Posição X do início da linha.
  * @param yTop Y do topo do bloco.
  * @param lineGapPx Espaçamento extra entre linhas (para além do line-height).
  */
 void drawWrappedText(engine::Renderer& renderer,
                      float x, float yTop,
                      float maxWidthPx,
                      const std::string& text,
                      float scale,
                      const glm::vec4& color,
                      float lineGapPx) {
     auto lines = wrapText(renderer, text, scale, maxWidthPx);
     float lh = renderer.getUIFontLineHeight(scale);
 
     float y = yTop;
     for (const auto& ln : lines) {
         renderer.drawUIText(x, y, ln, scale, color);
         y -= (lh + lineGapPx);
     }
 }
 
 /**
  * @brief Conversão HSV->RGB (h, s, v em [0..1]).
  *
  * Usado para:
  * - borders RGB animadas (menu/overlays)
  * - neon accents
  */
 glm::vec3 hsv2rgb(float h, float s, float v) {
     h = std::fmod(h, 1.0f); if (h < 0.0f) h += 1.0f;
 
     float c = v * s;
     float x = c * (1.0f - std::fabs(std::fmod(h * 6.0f, 2.0f) - 1.0f));
     float m = v - c;
 
     glm::vec3 rgb;
     if      (h < 1.0f/6.0f) rgb = {c, x, 0};
     else if (h < 2.0f/6.0f) rgb = {x, c, 0};
     else if (h < 3.0f/6.0f) rgb = {0, c, x};
     else if (h < 4.0f/6.0f) rgb = {0, x, c};
     else if (h < 5.0f/6.0f) rgb = {x, 0, c};
     else                    rgb = {c, 0, x};
 
     return rgb + glm::vec3(m);
 }
 
 /**
  * @brief Desenha um "badge" para teclas (ex: "SPACE", "A / D") com border neon animado.
  *
  * Ideia:
  * - retângulo escuro como fundo
  * - border neon com hue animado
  * - texto centrado
  */
 void drawKeyBadge(engine::Renderer& renderer, float x, float y, const std::string& label, float scale, float time) {
     float textW = renderer.measureUITextWidth(label, scale);
     float textH = renderer.getUIFontLineHeight(scale);
 
     float padX = 12.0f;
     float padY = 8.0f;
     float w = textW + padX * 2.0f;
     float h = textH + padY * 2.0f;
 
     // Fundo
     renderer.drawUIQuad(x, y, w, h, glm::vec4(0.08f, 0.08f, 0.14f, 0.95f));
 
     // Border neon (hue animado)
     float hue = std::fmod(0.56f + 0.08f * std::sin(time * 1.2f), 1.0f);
     glm::vec3 neon = hsv2rgb(hue, 0.85f, 1.0f);
     glm::vec4 border(neon, 1.0f);
     float bt = 2.0f;
 
     renderer.drawUIQuad(x, y, w, bt, border);
     renderer.drawUIQuad(x, y + h - bt, w, bt, border);
     renderer.drawUIQuad(x, y, bt, h, border);
     renderer.drawUIQuad(x + w - bt, y, bt, h, border);
 
     // Texto centrado
     float tx = x + (w - textW) * 0.5f;
     float ty = y + (h - textH) * 0.5f;
     renderer.drawUIText(tx, ty, label, scale, glm::vec3(0.95f, 0.98f, 1.0f));
 }
 
 /**
  * @brief Botão “genérico” de overlay (Pause/GameOver): sombra + corpo + highlight no hover.
  *
  * Este estilo é o “default” dos overlays:
  * - sombra deslocada
  * - corpo com leve gradiente (2 quads)
  * - borda mais clara no hover
  * - texto centrado + shadow
  */
 void drawOverlayButton(engine::Renderer& renderer,
                        float bx, float by, float bw, float bh,
                        const std::string& label, const glm::vec3& baseColor,
                        bool hovered) {
     float hoverBrightness = hovered ? 1.3f : 1.0f;
 
     // Sombra (maior no hover para parecer "levantado")
     float btnShadowOffset = hovered ? 5.0f : 3.0f;
     renderer.drawUIQuad(bx + btnShadowOffset, by - btnShadowOffset,
                         bw, bh, glm::vec4(0.0f, 0.0f, 0.0f, hovered ? 0.6f : 0.5f));
 
     // Corpo (1 quad sólido + 1 quad semi-transparente em baixo = "gradiente fake")
     glm::vec3 color = baseColor * hoverBrightness;
     renderer.drawUIQuad(bx, by, bw, bh, glm::vec4(color.r, color.g, color.b, 1.0f));
     renderer.drawUIQuad(bx, by + bh * 0.5f, bw, bh * 0.5f,
                         glm::vec4(color.r * 0.7f, color.g * 0.7f, color.b * 0.7f, 0.3f));
 
     // Bordas: mais branca no hover, mais discreta caso contrário
     float btnBorderThickness = hovered ? 3.0f : 2.0f;
     glm::vec4 btnBorder = hovered ? glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)
                                   : glm::vec4(0.0f, 0.0f, 0.0f, 0.4f);
     renderer.drawUIQuad(bx, by, bw, btnBorderThickness, btnBorder);
     renderer.drawUIQuad(bx, by + bh - btnBorderThickness, bw, btnBorderThickness, btnBorder);
     renderer.drawUIQuad(bx, by, btnBorderThickness, bh, btnBorder);
     renderer.drawUIQuad(bx + bw - btnBorderThickness, by, btnBorderThickness, bh, btnBorder);
 
     // Texto centrado
     float labelScale = 1.0f;
     float labelW = renderer.measureUITextWidth(label, labelScale);
     float labelH = renderer.getUIFontLineHeight(labelScale);
     float labelX = bx + (bw - labelW) * 0.5f;
     float labelY = by + (bh - labelH) * 0.5f;
 
     // Shadow do texto para legibilidade
     float shadowStrength = hovered ? 0.8f : 0.6f;
     renderer.drawUIText(labelX + 1.0f, labelY - 1.0f, label, labelScale,
                         glm::vec3(0, 0, 0) * shadowStrength);
     renderer.drawUIText(labelX, labelY, label, labelScale, glm::vec3(1, 1, 1));
 }
 
 } // namespace game::render::ui
 