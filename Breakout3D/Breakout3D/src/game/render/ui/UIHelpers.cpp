#include "game/render/ui/UIHelpers.hpp"

#include "engine/Renderer.hpp"

#include <algorithm>
#include <cmath>

namespace game::render::ui {

float fitScaleToWidth(engine::Renderer& renderer, const std::string& text, float desiredScale, float maxWidthPx) {
    float s = desiredScale;
    if (maxWidthPx <= 1.0f) return s;
    float w = renderer.measureUITextWidth(text, s);
    if (w > maxWidthPx && w > 1.0f) {
        s *= (maxWidthPx / w);
    }
    return s;
}

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

void drawKeyBadge(engine::Renderer& renderer, float x, float y, const std::string& label, float scale, float time) {
    float textW = renderer.measureUITextWidth(label, scale);
    float textH = renderer.getUIFontLineHeight(scale);
    float padX = 12.0f;
    float padY = 8.0f;
    float w = textW + padX * 2.0f;
    float h = textH + padY * 2.0f;

    renderer.drawUIQuad(x, y, w, h, glm::vec4(0.08f, 0.08f, 0.14f, 0.95f));

    float hue = std::fmod(0.56f + 0.08f * std::sin(time * 1.2f), 1.0f);
    glm::vec3 neon = hsv2rgb(hue, 0.85f, 1.0f);
    glm::vec4 border(neon, 1.0f);
    float bt = 2.0f;
    renderer.drawUIQuad(x, y, w, bt, border);
    renderer.drawUIQuad(x, y + h - bt, w, bt, border);
    renderer.drawUIQuad(x, y, bt, h, border);
    renderer.drawUIQuad(x + w - bt, y, bt, h, border);

    float tx = x + (w - textW) * 0.5f;
    float ty = y + (h - textH) * 0.5f;
    renderer.drawUIText(tx, ty, label, scale, glm::vec3(0.95f, 0.98f, 1.0f));
}

void drawOverlayButton(engine::Renderer& renderer,
                       float bx, float by, float bw, float bh,
                       const std::string& label, const glm::vec3& baseColor,
                       bool hovered) {
    float hoverBrightness = hovered ? 1.3f : 1.0f;

    // Button shadow (larger on hover)
    float btnShadowOffset = hovered ? 5.0f : 3.0f;
    renderer.drawUIQuad(bx + btnShadowOffset, by - btnShadowOffset,
                        bw, bh, glm::vec4(0.0f, 0.0f, 0.0f, hovered ? 0.6f : 0.5f));

    // Button background (gradient effect with two quads)
    glm::vec3 color = baseColor * hoverBrightness;
    renderer.drawUIQuad(bx, by, bw, bh,
                        glm::vec4(color.r, color.g, color.b, 1.0f));
    renderer.drawUIQuad(bx, by + bh * 0.5f, bw, bh * 0.5f,
                        glm::vec4(color.r * 0.7f, color.g * 0.7f, color.b * 0.7f, 0.3f));

    // Button border (brighter and thicker on hover)
    float btnBorderThickness = hovered ? 3.0f : 2.0f;
    glm::vec4 btnBorder = hovered ? glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) : glm::vec4(0.0f, 0.0f, 0.0f, 0.4f);
    renderer.drawUIQuad(bx, by, bw, btnBorderThickness, btnBorder);
    renderer.drawUIQuad(bx, by + bh - btnBorderThickness, bw, btnBorderThickness, btnBorder);
    renderer.drawUIQuad(bx, by, btnBorderThickness, bh, btnBorder);
    renderer.drawUIQuad(bx + bw - btnBorderThickness, by, btnBorderThickness, bh, btnBorder);

    // Button label
    float labelScale = 1.0f;
    float labelW = renderer.measureUITextWidth(label, labelScale);
    float labelH = renderer.getUIFontLineHeight(labelScale);
    float labelX = bx + (bw - labelW) * 0.5f;
    float labelY = by + (bh - labelH) * 0.5f;

    // Label shadow (stronger on hover)
    float shadowStrength = hovered ? 0.8f : 0.6f;
    renderer.drawUIText(labelX + 1.0f, labelY - 1.0f, label, labelScale, glm::vec3(0, 0, 0) * shadowStrength);
    renderer.drawUIText(labelX, labelY, label, labelScale, glm::vec3(1, 1, 1));
}

} // namespace game::render::ui


