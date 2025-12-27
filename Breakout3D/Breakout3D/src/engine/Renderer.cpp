#include "engine/Renderer.hpp"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <vector>
#include <stdexcept>

#define STB_TRUETYPE_IMPLEMENTATION
#include "external/stb_truetype.h"

static void setMat4(GLuint p, const char* n, const glm::mat4& m) {
    glUniformMatrix4fv(glGetUniformLocation(p, n), 1, GL_FALSE, &m[0][0]);
}
static void setVec2(GLuint p, const char* n, const glm::vec2& v) {
    glUniform2f(glGetUniformLocation(p, n), v.x, v.y);
}
static void setVec3(GLuint p, const char* n, const glm::vec3& v) {
    glUniform3f(glGetUniformLocation(p, n), v.x, v.y, v.z);
}
static void setInt(GLuint p, const char* n, int v) {
    glUniform1i(glGetUniformLocation(p, n), v);
}
static void setFloat(GLuint p, const char* n, float v) {
    glUniform1f(glGetUniformLocation(p, n), v);
}

namespace engine {

static void setTexMode(GLuint p, int mode) {
    glUniform1i(glGetUniformLocation(p, "uTexMode"), mode);
}

bool Renderer::loadUIFont(const std::string& ttfPath) {
    // Read TTF into memory
    std::ifstream f(ttfPath, std::ios::binary);
    if (!f) return false;
    f.seekg(0, std::ios::end);
    std::streamoff len = f.tellg();
    f.seekg(0, std::ios::beg);
    if (len <= 0) return false;

    std::vector<unsigned char> ttf((size_t)len);
    f.read(reinterpret_cast<char*>(ttf.data()), len);
    if (!f) return false;

    // Init font for metrics
    stbtt_fontinfo info{};
    if (!stbtt_InitFont(&info, ttf.data(), stbtt_GetFontOffsetForIndex(ttf.data(), 0))) {
        return false;
    }

    float scale = stbtt_ScaleForPixelHeight(&info, m_uiFontPixelHeight);
    int ascent = 0, descent = 0, lineGap = 0;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
    m_uiFontAscentPx = ascent * scale;
    m_uiFontDescentPx = descent * scale; // negative
    m_uiFontLineGapPx = lineGap * scale;

    // Bake ASCII 32..127 into atlas (auto-grow if needed)
    if (!m_uiFontChars) {
        m_uiFontChars = (void*)new stbtt_bakedchar[96];
    }
    stbtt_bakedchar* chars = (stbtt_bakedchar*)m_uiFontChars;

    int ok = 0;
    int sizes[] = {512, 1024, 2048, 4096};
    std::vector<unsigned char> bitmap;
    for (int s : sizes) {
        m_uiFontTexW = s;
        m_uiFontTexH = s;
        bitmap.assign((size_t)m_uiFontTexW * (size_t)m_uiFontTexH, 0);
        ok = stbtt_BakeFontBitmap(
            ttf.data(), 0,
            m_uiFontPixelHeight,
            bitmap.data(), m_uiFontTexW, m_uiFontTexH,
            32, 96,
            chars
        );
        if (ok > 0) break;
    }
    if (ok <= 0) return false;

    if (m_uiFontTex) {
        glDeleteTextures(1, &m_uiFontTex);
        m_uiFontTex = 0;
    }

    glGenTextures(1, &m_uiFontTex);
    glBindTexture(GL_TEXTURE_2D, m_uiFontTex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_uiFontTexW, m_uiFontTexH, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap.data());

    // Mipmaps help keep small UI text crisp/stable while still looking good when scaled up.
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

bool Renderer::init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (!m_shader.load("assets/shaders/basic_phong.vert", "assets/shaders/basic_phong.frag"))
        return false;
    // Default UI font: prefer bold for stronger HUD readability.
    if (!loadUIFont("assets/fonts/Orbitron-Bold.ttf")) {
        // Fall back to variable, then regular weights.
        if (!loadUIFont("assets/fonts/Orbitron-VariableFont_wght.ttf")) {
            if (!loadUIFont("assets/fonts/Orbitron-Regular.ttf")) {
                if (!loadUIFont("assets/fonts/Orbitron-Medium.ttf")) {
                    // Keep running, but text will effectively be invisible without a font atlas.
                }
            }
        }
    }
    return true;
}

void Renderer::shutdown() {
    m_shader.destroy();
    if (m_uiFontTex) {
        glDeleteTextures(1, &m_uiFontTex);
        m_uiFontTex = 0;
    }
    delete[] (stbtt_bakedchar*)m_uiFontChars;
    m_uiFontChars = nullptr;
}

void Renderer::beginFrame(int fbW, int fbH) {
    glViewport(0, 0, fbW, fbH);
    glClearColor(0.05f, 0.06f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_lightPos   = glm::vec3(0.0f, 10.0f, 5.0f);
    m_lightColor = glm::vec3(1.0f);
    m_ambientK   = 0.15f;
    m_diffuseK   = 1.00f;
    m_specK      = 1.00f;
    m_shininess  = 32.0f;
}

void Renderer::drawBackground(unsigned int textureId) {
    glDisable(GL_DEPTH_TEST);
    m_shader.use();
    GLuint p = m_shader.id();

    glm::mat4 I(1.0f);
    setMat4(p, "uV", I);
    setMat4(p, "uP", I);
    setMat4(p, "uM", I);

    setInt(p, "uUseTex", 1);
    setTexMode(p, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    setInt(p, "uTex", 0);

    setVec3(p, "uAlbedo", glm::vec3(1.0f));
    setFloat(p, "uAmbientK", 1.0f);
    setFloat(p, "uDiffuseK", 0.0f);
    setFloat(p, "uSpecK", 0.0f);

    // ✅ FIX: evitar leak do UI (alpha/mask) a apagar o background
    setFloat(p, "uAlpha", 1.0f);
    setInt(p, "uUseMask", 0);
    setVec2(p, "uMaskMin", glm::vec2(0.0f));
    setVec2(p, "uMaskMax", glm::vec2(0.0f));

    static GLuint VAO = 0, VBO, EBO;
    if (VAO == 0) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        static float vertices[] = {
            -1,-1,0,  0,0,1,  0,0,
             1,-1,0,  0,0,1,  1,0,
             1, 1,0,  0,0,1,  1,1,
            -1, 1,0,  0,0,1,  0,1
        };
        unsigned int indices[] = {0,1,2, 0,2,3};
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // aPos: location 0
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // aNormal: location 1
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);
        // aUV: location 2
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(6*sizeof(float)));
        glEnableVertexAttribArray(2);
    }

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

void Renderer::setCamera(const glm::mat4& V, const glm::mat4& P, const glm::vec3& camPos) {
    m_V = V; m_P = P; m_camPos = camPos;
}

void Renderer::drawMesh(const Mesh& mesh, const glm::mat4& M, const glm::vec3& tint) {
    m_shader.use();
    GLuint p = m_shader.id();

    setMat4(p, "uV", m_V);
    setMat4(p, "uP", m_P);
    setMat4(p, "uM", M);

    setVec3(p, "uViewPos", m_camPos);
    setVec3(p, "uLightPos", m_lightPos);
    setVec3(p, "uLightColor", m_lightColor);

    setFloat(p, "uAmbientK",  m_ambientK);
    setFloat(p, "uDiffuseK",  m_diffuseK);
    setFloat(p, "uSpecK",     m_specK);
    setFloat(p, "uShininess", m_shininess);

    // ✅ FIX: evitar leak do UI (alpha/mask) a apagar meshes
    setFloat(p, "uAlpha", 1.0f);
    setInt(p, "uUseMask", 0);
    setVec2(p, "uMaskMin", glm::vec2(0.0f));
    setVec2(p, "uMaskMax", glm::vec2(0.0f));

    glm::vec3 kd(mesh.kd[0], mesh.kd[1], mesh.kd[2]);
    setVec3(p, "uAlbedo", kd * tint);

    const bool useTex = (mesh.textureId != 0);
    setInt(p, "uUseTex", useTex ? 1 : 0);
    setTexMode(p, 0);
    if (useTex) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mesh.textureId);
        setInt(p, "uTex", 0);
    }

    glBindVertexArray(mesh.vao);
    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, (void*)0);
    glBindVertexArray(0);

    if (useTex) glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::drawMesh(const Mesh& mesh, const glm::vec3& pos, const glm::vec3& size, const glm::vec3& tint) {
    glm::mat4 M(1.0f);
    M = glm::translate(M, pos);
    M = glm::scale(M, size);
    drawMesh(mesh, M, tint);
}

void Renderer::beginUI(int fbW, int fbH) {
    glDisable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    m_uiFbW = fbW;
    m_uiFbH = fbH;

    glm::mat4 Vui(1.0f);
    glm::mat4 Pui = glm::ortho(0.0f, (float)fbW, 0.0f, (float)fbH, -1000.0f, 1000.0f);
    glm::vec3 cam = glm::vec3(fbW * 0.5f, fbH * 0.5f, 700.0f);
    setCamera(Vui, Pui, cam);

    m_lightPos   = glm::vec3(120.0f, fbH - 80.0f, 260.0f);
    m_ambientK   = 0.28f;
    m_diffuseK   = 0.35f;
    m_specK      = 0.28f;
    m_shininess  = 64.0f;
}

void Renderer::drawUIQuad(float x, float y, float w, float h, const glm::vec4& color, bool useMask, glm::vec2 maskMin, glm::vec2 maskMax) {
    m_shader.use();
    GLuint p = m_shader.id();

    glm::mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(x + w*0.5f, y + h*0.5f, 0.0f));
    M = glm::scale(M, glm::vec3(w, h, 1.0f));

    setMat4(p, "uV", m_V);
    setMat4(p, "uP", m_P);
    setMat4(p, "uM", M);

    setInt(p, "uUseTex", 0);
    setTexMode(p, 0);
    setVec3(p, "uAlbedo", glm::vec3(color));
    setFloat(p, "uAlpha", color.a);

    // UI should be flat and crisp
    setFloat(p, "uAmbientK", 1.0f);
    setFloat(p, "uDiffuseK", 0.0f);
    setFloat(p, "uSpecK",    0.0f);

    setInt(p, "uUseMask", useMask ? 1 : 0);
    if (useMask) {
        setVec2(p, "uMaskMin", maskMin);
        setVec2(p, "uMaskMax", maskMax);
    } else {
        // (opcional) limpar bounds por segurança
        setVec2(p, "uMaskMin", glm::vec2(0.0f));
        setVec2(p, "uMaskMax", glm::vec2(0.0f));
    }

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    float vertices[] = {
        -0.5f,-0.5f,0,  0.5f,-0.5f,0,
         0.5f, 0.5f,0, -0.5f, 0.5f,0
    };
    unsigned int indices[] = {0,1,2, 0,2,3};

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);

    glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);

    glDeleteBuffers(1,&VBO);
    glDeleteBuffers(1,&EBO);
    glDeleteVertexArrays(1,&VAO);
}

void Renderer::drawUITriangle(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, const glm::vec4& color) {
    m_shader.use();
    GLuint p = m_shader.id();

    setMat4(p, "uV", m_V);
    setMat4(p, "uP", m_P);
    setMat4(p, "uM", glm::mat4(1.0f));

    setInt(p, "uUseTex", 0);
    setTexMode(p, 0);
    setVec3(p, "uAlbedo", glm::vec3(color));
    setFloat(p, "uAlpha", color.a);
    setFloat(p, "uAmbientK", 1.0f);
    setFloat(p, "uDiffuseK", 0.0f);
    setFloat(p, "uSpecK",    0.0f);

    setInt(p, "uUseMask", 0);

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    float vertices[] = {
        p0.x, p0.y, 0.0f,
        p1.x, p1.y, 0.0f,
        p2.x, p2.y, 0.0f
    };

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
}

void Renderer::drawUIText(float x, float y, const std::string& text, float scale, const glm::vec4& color) {
    if (!m_uiFontTex || !m_uiFontChars || m_uiFbH <= 0) return;
    stbtt_bakedchar* chars = (stbtt_bakedchar*)m_uiFontChars;

    // Keep old UI sizing stable when we bake the atlas at higher resolution.
    // All public scales are expressed in terms of the legacy baked height (20px).
    const float effectiveScale = scale * (m_uiFontLegacyPixelHeight / m_uiFontPixelHeight);

    m_shader.use();
    GLuint p = m_shader.id();

    setMat4(p, "uV", m_V);
    setMat4(p, "uP", m_P);

    // Font uses alpha-mask mode
    setInt(p, "uUseTex", 1);
    setTexMode(p, 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_uiFontTex);
    setInt(p, "uTex", 0);

    setVec3(p, "uAlbedo", glm::vec3(color));
    setFloat(p, "uAlpha", color.a);

    // UI should be flat and crisp
    setFloat(p, "uAmbientK", 1.0f);
    setFloat(p, "uDiffuseK", 0.0f);
    setFloat(p, "uSpecK",    0.0f);

    setInt(p, "uUseMask", 0);
    setVec2(p, "uMaskMin", glm::vec2(0.0f));
    setVec2(p, "uMaskMax", glm::vec2(0.0f));

    // Convert from "bottom-left y-up" API (old code) to stb's "y-down baseline".
    // Old UI code used y as the bottom of the text box; baseline = y - descent.
    // IMPORTANT: stbtt_GetBakedQuad outputs in baked-font pixel units, so we must
    // apply `scale` ourselves when positioning/sizing quads.
    const float originXDown = x;
    float baselineYUp = y - (m_uiFontDescentPx * effectiveScale);
    const float baselineYDown = (float)m_uiFbH - baselineYUp;
    const float originYDown = baselineYDown;

    float penXDown = originXDown; // in baked-font pixels
    float penYDown = baselineYDown;

    for (unsigned char uc : text) {
        if (uc < 32 || uc >= 128) {
            penXDown += (m_uiFontPixelHeight * 0.4f) * effectiveScale;
            continue;
        }

        stbtt_aligned_quad q;
        // stb expects positions in pixels; y grows downward
        float x0 = penXDown;
        float y0 = penYDown;
        // opengl_fillrule=1: OpenGL/D3D10+ style (no half-texel D3D9 bias)
        stbtt_GetBakedQuad(chars, m_uiFontTexW, m_uiFontTexH, (int)uc - 32, &x0, &y0, &q, 1);
        penXDown = x0;
        penYDown = y0;

        // Apply user scale around the text origin/baseline.
        const float sx0 = originXDown + (q.x0 - originXDown) * effectiveScale;
        const float sx1 = originXDown + (q.x1 - originXDown) * effectiveScale;
        const float sy0 = originYDown + (q.y0 - originYDown) * effectiveScale;
        const float sy1 = originYDown + (q.y1 - originYDown) * effectiveScale;

        float quadW = sx1 - sx0;
        float quadH = sy1 - sy0;
        if (quadW <= 0.0f || quadH <= 0.0f) continue;

        // Convert coords to y-up (origin bottom-left)
        float quadX = sx0;
        float quadY = (float)m_uiFbH - sy1; // bottom

        // stbtt_BakeFontBitmap writes rows starting at y=0 (first rows in memory).
        // OpenGL interprets the first row of pixel data as the bottom of the texture,
        // so q.t0/q.t1 map correctly without flipping here.
        float s0 = q.s0, s1 = q.s1;
        float v0 = q.t0;
        float v1 = q.t1;

        GLuint VAO, VBO, EBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        float verts[] = {
            // pos                // uv
            -0.5f, -0.5f, 0.0f,   s0, v1, // bottom-left
             0.5f, -0.5f, 0.0f,   s1, v1, // bottom-right
             0.5f,  0.5f, 0.0f,   s1, v0, // top-right
            -0.5f,  0.5f, 0.0f,   s0, v0  // top-left
        };
        unsigned int inds[] = {0,1,2, 0,2,3};

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(inds), inds, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(2);

        glm::mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(quadX + quadW*0.5f, quadY + quadH*0.5f, 0.0f));
        M = glm::scale(M, glm::vec3(quadW, quadH, 1.0f));
        setMat4(p, "uM", M);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteVertexArrays(1, &VAO);
    }
}

float Renderer::measureUITextWidth(const std::string& text, float scale) const {
    if (!m_uiFontChars) return 0.0f;
    stbtt_bakedchar* chars = (stbtt_bakedchar*)m_uiFontChars;
    const float effectiveScale = scale * (m_uiFontLegacyPixelHeight / m_uiFontPixelHeight);
    float w = 0.0f;
    for (unsigned char uc : text) {
        if (uc < 32 || uc >= 128) {
            w += (m_uiFontPixelHeight * 0.4f) * effectiveScale;
            continue;
        }
        const stbtt_bakedchar& b = chars[(int)uc - 32];
        w += b.xadvance * effectiveScale;
    }
    return w;
}

float Renderer::getUIFontLineHeight(float scale) const {
    const float effectiveScale = scale * (m_uiFontLegacyPixelHeight / m_uiFontPixelHeight);
    if (!m_uiFontChars) return m_uiFontPixelHeight * effectiveScale;
    // ascent - descent + lineGap (descent is negative)
    return (m_uiFontAscentPx - m_uiFontDescentPx + m_uiFontLineGapPx) * effectiveScale;
}

void Renderer::endUI() {
    glEnable(GL_DEPTH_TEST);
}

} // namespace engine
