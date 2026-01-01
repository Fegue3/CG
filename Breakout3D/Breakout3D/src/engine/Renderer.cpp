// Renderer.cpp
#include "engine/Renderer.hpp"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <vector>
#include <stdexcept>

#define STB_TRUETYPE_IMPLEMENTATION
#include "external/stb_truetype.h"

// Vertex simples usado para desenhar UI (quads/triângulos/texto) com um VBO dinâmico.
struct UiVertex {
    float x, y, z;
    float u, v;
};

// Helpers de uniforms (mantém o código de render mais limpo).
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

// Modo de textura no shader (ex.: 0 = normal, 1 = alpha-mask / font atlas, etc.).
static void setTexMode(GLuint p, int mode) {
    glUniform1i(glGetUniformLocation(p, "uTexMode"), mode);
}

bool Renderer::loadUIFont(const std::string& ttfPath) {
    // Lê o TTF inteiro para memória (stb_truetype usa ponteiros para o buffer).
    std::ifstream f(ttfPath, std::ios::binary);
    if (!f) return false;
    f.seekg(0, std::ios::end);
    std::streamoff len = f.tellg();
    f.seekg(0, std::ios::beg);
    if (len <= 0) return false;

    std::vector<unsigned char> ttf((size_t)len);
    f.read(reinterpret_cast<char*>(ttf.data()), len);
    if (!f) return false;

    // Inicializa para métricas (ascent/descent/lineGap), usado para alinhamento e alturas.
    stbtt_fontinfo info{};
    if (!stbtt_InitFont(&info, ttf.data(), stbtt_GetFontOffsetForIndex(ttf.data(), 0))) {
        return false;
    }

    float scale = stbtt_ScaleForPixelHeight(&info, m_uiFontPixelHeight);
    int ascent = 0, descent = 0, lineGap = 0;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
    m_uiFontAscentPx  = ascent * scale;
    m_uiFontDescentPx = descent * scale; // normalmente negativo
    m_uiFontLineGapPx = lineGap * scale;

    // Atlas baked para ASCII 32..127 (96 caracteres).
    if (!m_uiFontChars) {
        m_uiFontChars = (void*)new stbtt_bakedchar[96];
    }
    stbtt_bakedchar* chars = (stbtt_bakedchar*)m_uiFontChars;

    // Tenta tamanhos progressivos até caber tudo no atlas (evita texturas minúsculas).
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

    // Cria/actualiza a textura do atlas (canal único: GL_RED).
    if (m_uiFontTex) {
        glDeleteTextures(1, &m_uiFontTex);
        m_uiFontTex = 0;
    }

    glGenTextures(1, &m_uiFontTex);
    glBindTexture(GL_TEXTURE_2D, m_uiFontTex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_uiFontTexW, m_uiFontTexH, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap.data());

    // GL_LINEAR sem mipmaps: para texto/atlas 2D é o mais estável.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

bool Renderer::init() {
    // Estado base do renderer.
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (!m_shader.load("assets/shaders/basic_phong.vert", "assets/shaders/basic_phong.frag"))
        return false;

    // Fonte default para UI (tenta vários ficheiros por ordem, com fallback).
    if (!loadUIFont("assets/fonts/Orbitron-Bold.ttf")) {
        if (!loadUIFont("assets/fonts/Orbitron-VariableFont_wght.ttf")) {
            if (!loadUIFont("assets/fonts/Orbitron-Regular.ttf")) {
                if (!loadUIFont("assets/fonts/Orbitron-Medium.ttf")) {
                    // Sem fonte: o jogo ainda corre, mas o texto não aparece.
                }
            }
        }
    }

    // Buffers de UI: um VAO/VBO dinâmico reutilizado para quads/triângulos/texto.
    if (!m_uiVao) {
        glGenVertexArrays(1, &m_uiVao);
        glGenBuffers(1, &m_uiVbo);

        glBindVertexArray(m_uiVao);
        glBindBuffer(GL_ARRAY_BUFFER, m_uiVbo);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

        // aPos: location 0
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(UiVertex), (void*)offsetof(UiVertex, x));

        // aNormal: location 1 — UI usa uma normal constante (shader continua a funcionar).
        glDisableVertexAttribArray(1);
        glVertexAttrib3f(1, 0.0f, 0.0f, 1.0f);

        // aUV: location 2
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(UiVertex), (void*)offsetof(UiVertex, u));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    return true;
}

void Renderer::shutdown() {
    // Liberta shader e recursos de UI/fonte.
    m_shader.destroy();

    if (m_uiFontTex) {
        glDeleteTextures(1, &m_uiFontTex);
        m_uiFontTex = 0;
    }

    delete[] (stbtt_bakedchar*)m_uiFontChars;
    m_uiFontChars = nullptr;

    if (m_uiVbo) {
        glDeleteBuffers(1, &m_uiVbo);
        m_uiVbo = 0;
    }
    if (m_uiVao) {
        glDeleteVertexArrays(1, &m_uiVao);
        m_uiVao = 0;
    }
}

void Renderer::beginFrame(int fbW, int fbH) {
    // Limpa frame e define defaults de iluminação “mundo 3D”.
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
    // Background é um quad em NDC (-1..1). Desliga depth para não interferir.
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

    // Background “flat”: só albedo/ambient.
    setVec3(p, "uAlbedo", glm::vec3(1.0f));
    setFloat(p, "uAmbientK", 1.0f);
    setFloat(p, "uDiffuseK", 0.0f);
    setFloat(p, "uSpecK", 0.0f);

    // Importante: limpar parâmetros de UI (alpha/mask) para não “vazar” estado.
    setFloat(p, "uAlpha", 1.0f);
    setInt(p, "uUseMask", 0);
    setVec2(p, "uMaskMin", glm::vec2(0.0f));
    setVec2(p, "uMaskMax", glm::vec2(0.0f));

    // VAO estático (criado uma vez) para o quad de fundo.
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

    // Limpa estado típico do UI.
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
    // UI desenha por cima do mundo: depth off e P ortográfica em pixels.
    glDisable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    m_uiFbW = fbW;
    m_uiFbH = fbH;

    glm::mat4 Vui(1.0f);
    glm::mat4 Pui = glm::ortho(0.0f, (float)fbW, 0.0f, (float)fbH, -1000.0f, 1000.0f);
    glm::vec3 cam = glm::vec3(fbW * 0.5f, fbH * 0.5f, 700.0f);
    setCamera(Vui, Pui, cam);

    // Iluminação “UI”: quase flat mas com um bocadinho de spec/shape.
    m_lightPos   = glm::vec3(120.0f, fbH - 80.0f, 260.0f);
    m_ambientK   = 0.28f;
    m_diffuseK   = 0.35f;
    m_specK      = 0.28f;
    m_shininess  = 64.0f;
}

void Renderer::drawUIQuad(float x, float y, float w, float h, const glm::vec4& color,
                          bool useMask, glm::vec2 maskMin, glm::vec2 maskMax) {
    m_shader.use();
    GLuint p = m_shader.id();

    // Model matrix: quad unitário centrado em (0,0), escalado para (w,h) e transladado.
    glm::mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(x + w*0.5f, y + h*0.5f, 0.0f));
    M = glm::scale(M, glm::vec3(w, h, 1.0f));

    setMat4(p, "uV", m_V);
    setMat4(p, "uP", m_P);
    setMat4(p, "uM", M);

    setInt(p, "uUseTex", 0);
    setTexMode(p, 0);
    setVec3(p, "uAlbedo", glm::vec3(color));
    setFloat(p, "uAlpha", color.a);

    // UI sólida: flat (sem diffuse/spec) para ficar “crisp”.
    setFloat(p, "uAmbientK", 1.0f);
    setFloat(p, "uDiffuseK", 0.0f);
    setFloat(p, "uSpecK",    0.0f);

    // Mask opcional (ex.: barras, sliders, recortes UI).
    setInt(p, "uUseMask", useMask ? 1 : 0);
    if (useMask) {
        setVec2(p, "uMaskMin", maskMin);
        setVec2(p, "uMaskMax", maskMax);
    } else {
        setVec2(p, "uMaskMin", glm::vec2(0.0f));
        setVec2(p, "uMaskMax", glm::vec2(0.0f));
    }

    UiVertex verts[6] = {
        {-0.5f, -0.5f, 0.0f, 0.0f, 0.0f},
        { 0.5f, -0.5f, 0.0f, 0.0f, 0.0f},
        { 0.5f,  0.5f, 0.0f, 0.0f, 0.0f},
        {-0.5f, -0.5f, 0.0f, 0.0f, 0.0f},
        { 0.5f,  0.5f, 0.0f, 0.0f, 0.0f},
        {-0.5f,  0.5f, 0.0f, 0.0f, 0.0f},
    };

    glBindVertexArray(m_uiVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_uiVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// Overload: quad com textura (usa UVs 0..1).
void Renderer::drawUIQuad(float x, float y, float w, float h, const glm::vec4& color, unsigned int textureId) {
    m_shader.use();
    GLuint p = m_shader.id();

    glm::mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(x + w*0.5f, y + h*0.5f, 0.0f));
    M = glm::scale(M, glm::vec3(w, h, 1.0f));

    setMat4(p, "uV", m_V);
    setMat4(p, "uP", m_P);
    setMat4(p, "uM", M);

    setInt(p, "uUseTex", 1);
    setTexMode(p, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    setInt(p, "uTex", 0);

    setVec3(p, "uAlbedo", glm::vec3(color));
    setFloat(p, "uAlpha", color.a);

    setFloat(p, "uAmbientK", 1.0f);
    setFloat(p, "uDiffuseK", 0.0f);
    setFloat(p, "uSpecK", 0.0f);

    // Aqui não usamos mask; limpa por segurança.
    setInt(p, "uUseMask", 0);
    setVec2(p, "uMaskMin", glm::vec2(0.0f));
    setVec2(p, "uMaskMax", glm::vec2(0.0f));

    UiVertex verts[6] = {
        {-0.5f, -0.5f, 0.0f, 0.0f, 0.0f},
        { 0.5f, -0.5f, 0.0f, 1.0f, 0.0f},
        { 0.5f,  0.5f, 0.0f, 1.0f, 1.0f},
        {-0.5f, -0.5f, 0.0f, 0.0f, 0.0f},
        { 0.5f,  0.5f, 0.0f, 1.0f, 1.0f},
        {-0.5f,  0.5f, 0.0f, 0.0f, 1.0f},
    };

    glBindVertexArray(m_uiVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_uiVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
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

    UiVertex verts[3] = {
        {p0.x, p0.y, 0.0f, 0.0f, 0.0f},
        {p1.x, p1.y, 0.0f, 0.0f, 0.0f},
        {p2.x, p2.y, 0.0f, 0.0f, 0.0f},
    };

    glBindVertexArray(m_uiVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_uiVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Renderer::drawUIText(float x, float y, const std::string& text, float scale, const glm::vec4& color) {
    if (!m_uiFontTex || !m_uiFontChars || m_uiFbH <= 0) return;
    stbtt_bakedchar* chars = (stbtt_bakedchar*)m_uiFontChars;

    // Mantém “escala antiga” estável caso o atlas tenha sido baked com outro pixel height.
    const float effectiveScale = scale * (m_uiFontLegacyPixelHeight / m_uiFontPixelHeight);

    m_shader.use();
    GLuint p = m_shader.id();

    setMat4(p, "uV", m_V);
    setMat4(p, "uP", m_P);

    // Font: textura usada como máscara (alpha a partir do canal).
    setInt(p, "uUseTex", 1);
    setTexMode(p, 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_uiFontTex);
    setInt(p, "uTex", 0);

    setVec3(p, "uAlbedo", glm::vec3(color));
    setFloat(p, "uAlpha", color.a);

    setFloat(p, "uAmbientK", 1.0f);
    setFloat(p, "uDiffuseK", 0.0f);
    setFloat(p, "uSpecK",    0.0f);

    setInt(p, "uUseMask", 0);
    setVec2(p, "uMaskMin", glm::vec2(0.0f));
    setVec2(p, "uMaskMax", glm::vec2(0.0f));

    // API antiga da UI usa coordenadas com origem em baixo (y-up).
    // stb trabalha em y-down e com baseline; aqui fazemos a conversão.
    const float originXDown = x;
    float baselineYUp = y - (m_uiFontDescentPx * effectiveScale);
    const float baselineYDown = (float)m_uiFbH - baselineYUp;
    const float originYDown = baselineYDown;

    float penXDown = originXDown;
    float penYDown = baselineYDown;

    // Faz batch de todos os glyphs num só update do VBO + um draw call.
    std::vector<UiVertex> verts;
    verts.reserve(text.size() * 6);

    for (unsigned char uc : text) {
        if (uc < 32 || uc >= 128) {
            // Para chars fora do atlas, avança um pouco para não colar tudo.
            penXDown += (m_uiFontPixelHeight * 0.4f) * effectiveScale;
            continue;
        }

        stbtt_aligned_quad q;
        float x0 = penXDown;
        float y0 = penYDown;

        stbtt_GetBakedQuad(chars, m_uiFontTexW, m_uiFontTexH, (int)uc - 32, &x0, &y0, &q, 1);

        penXDown = x0;
        penYDown = y0;

        // Aplica a escala (em torno da origem/baseline).
        const float sx0 = originXDown + (q.x0 - originXDown) * effectiveScale;
        const float sx1 = originXDown + (q.x1 - originXDown) * effectiveScale;
        const float sy0 = originYDown + (q.y0 - originYDown) * effectiveScale;
        const float sy1 = originYDown + (q.y1 - originYDown) * effectiveScale;

        float quadW = sx1 - sx0;
        float quadH = sy1 - sy0;
        if (quadW <= 0.0f || quadH <= 0.0f) continue;

        // Volta para o espaço y-up (origem em baixo-esquerda).
        float quadX = sx0;
        float quadY = (float)m_uiFbH - sy1;

        float s0 = q.s0, s1 = q.s1;
        float v0 = q.t0, v1 = q.t1;

        verts.push_back(UiVertex{quadX,          quadY,          0.0f, s0, v1});
        verts.push_back(UiVertex{quadX + quadW,  quadY,          0.0f, s1, v1});
        verts.push_back(UiVertex{quadX + quadW,  quadY + quadH,  0.0f, s1, v0});
        verts.push_back(UiVertex{quadX,          quadY,          0.0f, s0, v1});
        verts.push_back(UiVertex{quadX + quadW,  quadY + quadH,  0.0f, s1, v0});
        verts.push_back(UiVertex{quadX,          quadY + quadH,  0.0f, s0, v0});
    }

    if (!verts.empty()) {
        setMat4(p, "uM", glm::mat4(1.0f));
        glBindVertexArray(m_uiVao);
        glBindBuffer(GL_ARRAY_BUFFER, m_uiVbo);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(verts.size() * sizeof(UiVertex)), verts.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)verts.size());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
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

    // ascent - descent + lineGap (descent é negativo, por isso soma na prática).
    return (m_uiFontAscentPx - m_uiFontDescentPx + m_uiFontLineGapPx) * effectiveScale;
}

void Renderer::endUI() {
    glEnable(GL_DEPTH_TEST);
}

void Renderer::uiSetDepthTest(bool enabled, bool clearDepth) {
    if (enabled) {
        glEnable(GL_DEPTH_TEST);
        if (clearDepth) glClear(GL_DEPTH_BUFFER_BIT);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

void Renderer::uiSetScissor(bool enabled, float x, float y, float w, float h) {
    if (!enabled) {
        glDisable(GL_SCISSOR_TEST);
        return;
    }

    // Coordenadas de UI são bottom-left (igual ao glScissor).
    int ix = (int)std::max(0.0f, std::floor(x));
    int iy = (int)std::max(0.0f, std::floor(y));
    int iw = (int)std::max(0.0f, std::ceil(w));
    int ih = (int)std::max(0.0f, std::ceil(h));

    glEnable(GL_SCISSOR_TEST);
    glScissor(ix, iy, iw, ih);
}

} // namespace engine
