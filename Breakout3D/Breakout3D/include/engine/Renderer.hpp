// Renderer.hpp
#pragma once
#include <glm/glm.hpp>
#include <string>
#include "engine/Shader.hpp"
#include "engine/Mesh.hpp"

namespace engine {

/**
 * @file Renderer.hpp
 * @brief Renderer OpenGL: pass 3D (mundo) + pass UI (ortho) com shader unificado.
 *
 * Notas:
 * - UI trabalha em pixels do framebuffer (x,y,w,h).
 * - Fonte UI é um atlas baked (stb_truetype) gerido internamente.
 * - Helpers `uiSetDepthTest` e `uiSetScissor` cobrem casos especiais no UI.
 * - Inicialização/destruição requerem contexto OpenGL activo.
 */
class Renderer {
public:
    bool init();
    void shutdown();

    /// Prepara o frame 3D (viewport/clear/estado base).
    void beginFrame(int fbW, int fbH);

    /// Desenha um background com textura (tipicamente fullscreen/quad).
    void drawBackground(unsigned int textureId);

    /// Define as matrizes de câmara e a posição do observador (para lighting/specular).
    void setCamera(const glm::mat4& V, const glm::mat4& P, const glm::vec3& camPos);

    /// Overload simples (pos+escala) para entidades sem rotação.
    void drawMesh(const Mesh& mesh, const glm::vec3& pos, const glm::vec3& size, const glm::vec3& tint = glm::vec3(1.0f));

    /// Overload completo (matriz modelo).
    void drawMesh(const Mesh& mesh, const glm::mat4& M, const glm::vec3& tint = glm::vec3(1.0f));

    // ---------- UI PASS ----------

    /// Inicia pass UI em ortho (coords em px do framebuffer).
    void beginUI(int fbW, int fbH);

    /// Quad UI sólido (com opcional máscara UV no shader).
    void drawUIQuad(float x, float y, float w, float h, const glm::vec4& color, bool useMask = false,
                    glm::vec2 maskMin = glm::vec2(0.0f), glm::vec2 maskMax = glm::vec2(0.0f));

    /// Quad UI com textura (tint/alpha via color).
    void drawUIQuad(float x, float y, float w, float h, const glm::vec4& color, unsigned int textureId);

    /// Texto no HUD via atlas baked.
    void drawUIText(float x, float y, const std::string& text, float scale = 1.0f, const glm::vec4& color = glm::vec4(1.0f));

    void drawUIText(float x, float y, const std::string& text, float scale, const glm::vec3& color) {
        drawUIText(x, y, text, scale, glm::vec4(color, 1.0f));
    }

    /// Mede largura do texto em px (para alinhamento/centragem).
    float measureUITextWidth(const std::string& text, float scale = 1.0f) const;

    /// Altura de linha da fonte em px.
    float getUIFontLineHeight(float scale = 1.0f) const;

    /// Termina pass UI (pode repor estado GL).
    void endUI();

    glm::mat4 getV() const { return m_V; }
    glm::mat4 getP() const { return m_P; }

    /// Triângulo 2D em UI (para setas/ícones simples).
    void drawUITriangle(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, const glm::vec4& color);

    /// Liga/desliga depth test no UI (opcionalmente limpa depth).
    void uiSetDepthTest(bool enabled, bool clearDepth = false);

    /// Liga/desliga scissor (clipping real) no UI.
    void uiSetScissor(bool enabled, float x = 0.0f, float y = 0.0f, float w = 0.0f, float h = 0.0f);

private:
    Shader m_shader;

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

    // Fonte UI (atlas baked)
    GLuint m_uiFontTex = 0;
    int m_uiFontTexW = 0;
    int m_uiFontTexH = 0;
    void* m_uiFontChars = nullptr; // stbtt_bakedchar[96] (opaco no header)

    float m_uiFontPixelHeight = 96.0f;
    float m_uiFontLegacyPixelHeight = 20.0f;
    float m_uiFontAscentPx = 0.0f;
    float m_uiFontDescentPx = 0.0f;
    float m_uiFontLineGapPx = 0.0f;

    int m_uiFbW = 0;
    int m_uiFbH = 0;

    bool loadUIFont(const std::string& ttfPath);
};

} // namespace engine
