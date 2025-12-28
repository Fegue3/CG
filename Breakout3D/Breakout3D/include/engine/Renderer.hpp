#pragma once
#include <glm/glm.hpp>
#include <string>
#include "engine/Shader.hpp"
#include "engine/Mesh.hpp"

namespace engine {

class Renderer {
public:
    bool init();
    void shutdown();

    void beginFrame(int fbW, int fbH);
    void drawBackground(unsigned int textureId);
    void setCamera(const glm::mat4& V, const glm::mat4& P, const glm::vec3& camPos);

    void drawMesh(const Mesh& mesh, const glm::vec3& pos, const glm::vec3& size, const glm::vec3& tint = glm::vec3(1.0f));
    void drawMesh(const Mesh& mesh, const glm::mat4& M, const glm::vec3& tint = glm::vec3(1.0f));

    // UI pass (HUD 3D em ortho)
    void beginUI(int fbW, int fbH);
    void drawUIQuad(float x, float y, float w, float h, const glm::vec4& color, bool useMask = false, glm::vec2 maskMin = glm::vec2(0.0f), glm::vec2 maskMax = glm::vec2(0.0f));
    void drawUIText(float x, float y, const std::string& text, float scale = 1.0f, const glm::vec4& color = glm::vec4(1.0f));
    void drawUIText(float x, float y, const std::string& text, float scale, const glm::vec3& color) {
        drawUIText(x, y, text, scale, glm::vec4(color, 1.0f));
    }
    float measureUITextWidth(const std::string& text, float scale = 1.0f) const;
    float getUIFontLineHeight(float scale = 1.0f) const;
    void endUI();
    
    glm::mat4 getV() const { return m_V; }
    glm::mat4 getP() const { return m_P; }
    
    void drawUITriangle(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, const glm::vec4& color);

private:
    Shader m_shader;

    // Reusable UI geometry buffers (avoid per-draw VAO/VBO churn).
    GLuint m_uiVao = 0;
    GLuint m_uiVbo = 0;

    glm::mat4 m_V{1.0f}, m_P{1.0f};
    glm::vec3 m_camPos{0,0,0};

    glm::vec3 m_lightPos{0.0f, 10.0f, 5.0f};
    glm::vec3 m_lightColor{1.0f, 1.0f, 1.0f};
    float m_ambientK = 0.15f;
    float m_diffuseK = 1.00f;
    float m_specK    = 1.00f;
    float m_shininess = 32.0f;

    // UI font (stb_truetype baked atlas)
    GLuint m_uiFontTex = 0;
    int m_uiFontTexW = 0;
    int m_uiFontTexH = 0;
    void* m_uiFontChars = nullptr; // stbtt_bakedchar[96] (kept opaque to avoid stb type leakage in headers)
    // Bake at higher resolution for large titles; map old "scale" values via legacy height.
    float m_uiFontPixelHeight = 96.0f;        // baked atlas pixel height (quality)
    float m_uiFontLegacyPixelHeight = 20.0f;  // old baseline used by game/UI code (sizing)
    float m_uiFontAscentPx = 0.0f;
    float m_uiFontDescentPx = 0.0f;      // typically negative
    float m_uiFontLineGapPx = 0.0f;

    int m_uiFbW = 0;
    int m_uiFbH = 0;

    bool loadUIFont(const std::string& ttfPath);
};

} // namespace engine
