#define STB_EASY_FONT_IMPLEMENTATION

#include "engine/Renderer.hpp"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>

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

bool Renderer::init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Load world shader (Phong lighting for 3D meshes)
    if (!m_worldShader.load("assets/shaders/world_phong.vert", "assets/shaders/world_phong.frag"))
        return false;
    
    // Load UI shader (flat color for UI elements)
    if (!m_uiShader.load("assets/shaders/ui_flat.vert", "assets/shaders/ui_flat.frag"))
        return false;
    
    return true;
}

void Renderer::shutdown() {
    m_worldShader.destroy();
    m_uiShader.destroy();
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
    m_uiShader.use();  // Use UI shader for background
    GLuint p = m_uiShader.id();

    glm::mat4 I(1.0f);
    setMat4(p, "uV", I);
    setMat4(p, "uP", I);
    setMat4(p, "uM", I);

    setInt(p, "uUseTex", 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    setInt(p, "uTex", 0);

    setVec3(p, "uAlbedo", glm::vec3(1.0f));
    setFloat(p, "uAlpha", 1.0f);
    setInt(p, "uUseMask", 0);  // No masking for background

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
    m_worldShader.use();  // Use world shader for 3D meshes
    GLuint p = m_worldShader.id();

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

    glm::vec3 kd(mesh.kd[0], mesh.kd[1], mesh.kd[2]);
    setVec3(p, "uAlbedo", kd * tint);

    const bool useTex = (mesh.textureId != 0);
    setInt(p, "uUseTex", useTex ? 1 : 0);
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
    m_uiShader.use();  // Use UI shader for UI elements
    GLuint p = m_uiShader.id();

    glm::mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(x + w*0.5f, y + h*0.5f, 0.0f));
    M = glm::scale(M, glm::vec3(w, h, 1.0f));

    setMat4(p, "uV", m_V);
    setMat4(p, "uP", m_P);
    setMat4(p, "uM", M);

    setInt(p, "uUseTex", 0);
    setVec3(p, "uAlbedo", glm::vec3(color));
    setFloat(p, "uAlpha", color.a);
    
    setInt(p, "uUseMask", useMask ? 1 : 0);
    if (useMask) {
        setVec2(p, "uMaskMin", maskMin);
        setVec2(p, "uMaskMax", maskMax);
    }

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // pos(3) + normal(3) + uv(2) = 8 floats per vertex
    float vertices[] = {
        -0.5f,-0.5f,0,  0,0,1,  0,0,
         0.5f,-0.5f,0,  0,0,1,  1,0,
         0.5f, 0.5f,0,  0,0,1,  1,1,
        -0.5f, 0.5f,0,  0,0,1,  0,1
    };
    unsigned int indices[] = {0,1,2, 0,2,3};

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // aPos (location 0)
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    // aNormal (location 1) - required but unused by UI shader
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    // aUV (location 2)
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);

    glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);

    glDeleteBuffers(1,&VBO);
    glDeleteBuffers(1,&EBO);
    glDeleteVertexArrays(1,&VAO);
}

void Renderer::drawUIText(float x, float y, const std::string& text, float scale, const glm::vec3& color) {
    // Simple block text rendering
    // Character dimensions
    float labelW = 14.0f * scale;
    float labelH = 20.0f * scale;
    float labelSpacing = 4.0f * scale;
    float blockW = 2.0f * scale;
    
    float px = x;
    float py = y;
    
    for (char c : text) {
        if (c == 'P') {
            drawUIQuad(px, py, blockW, labelH, glm::vec4(color, 1.0f)); // left
            drawUIQuad(px+blockW, py+labelH-blockW, labelW-blockW, blockW, glm::vec4(color, 1.0f)); // top
            drawUIQuad(px+labelW-blockW, py+labelH*0.5f, blockW, labelH*0.5f, glm::vec4(color, 1.0f)); // right-top
            drawUIQuad(px+blockW, py+labelH*0.5f-blockW*0.5f, labelW-blockW, blockW, glm::vec4(color, 1.0f)); // middle
        }
        else if (c == 'A') {
            drawUIQuad(px, py, blockW, labelH, glm::vec4(color, 1.0f)); // left
            drawUIQuad(px+labelW-blockW, py, blockW, labelH, glm::vec4(color, 1.0f)); // right
            drawUIQuad(px+blockW, py+labelH-blockW, labelW-blockW*2, blockW, glm::vec4(color, 1.0f)); // top
            drawUIQuad(px+blockW, py+labelH*0.5f-blockW*0.5f, labelW-blockW*2, blockW, glm::vec4(color, 1.0f)); // middle
        }
        else if (c == 'U') {
            drawUIQuad(px, py, blockW, labelH, glm::vec4(color, 1.0f)); // left
            drawUIQuad(px+labelW-blockW, py, blockW, labelH, glm::vec4(color, 1.0f)); // right
            drawUIQuad(px+blockW, py, labelW-blockW*2, blockW, glm::vec4(color, 1.0f)); // bottom
        }
        else if (c == 'S') {
            drawUIQuad(px+blockW, py+labelH-blockW, labelW-blockW, blockW, glm::vec4(color, 1.0f)); // top
            drawUIQuad(px, py+labelH*0.5f, blockW, labelH*0.5f, glm::vec4(color, 1.0f)); // left-top
            drawUIQuad(px+blockW, py+labelH*0.5f-blockW*0.5f, labelW-blockW*2, blockW, glm::vec4(color, 1.0f)); // middle
            drawUIQuad(px+labelW-blockW, py, blockW, labelH*0.5f, glm::vec4(color, 1.0f)); // right-bottom
            drawUIQuad(px, py, labelW-blockW, blockW, glm::vec4(color, 1.0f)); // bottom
        }
        else if (c == 'E') {
            drawUIQuad(px, py, blockW, labelH, glm::vec4(color, 1.0f)); // left
            drawUIQuad(px+blockW, py+labelH-blockW, labelW-blockW, blockW, glm::vec4(color, 1.0f)); // top
            drawUIQuad(px+blockW, py+labelH*0.5f-blockW*0.5f, labelW-blockW, blockW, glm::vec4(color, 1.0f)); // middle
            drawUIQuad(px+blockW, py, labelW-blockW, blockW, glm::vec4(color, 1.0f)); // bottom
        }
        else if (c == 'D') {
            drawUIQuad(px, py, blockW, labelH, glm::vec4(color, 1.0f)); // left
            drawUIQuad(px+blockW, py+labelH-blockW, labelW-blockW*2, blockW, glm::vec4(color, 1.0f)); // top
            drawUIQuad(px+blockW, py, labelW-blockW*2, blockW, glm::vec4(color, 1.0f)); // bottom
            drawUIQuad(px+labelW-blockW, py+blockW, blockW, labelH-blockW*2, glm::vec4(color, 1.0f)); // right
        }
        else if (c == 'G') {
            drawUIQuad(px, py, blockW, labelH, glm::vec4(color, 1.0f)); // left
            drawUIQuad(px+blockW, py+labelH-blockW, labelW-blockW, blockW, glm::vec4(color, 1.0f)); // top
            drawUIQuad(px+blockW, py, labelW-blockW, blockW, glm::vec4(color, 1.0f)); // bottom
            drawUIQuad(px+labelW-blockW, py, blockW, labelH*0.5f, glm::vec4(color, 1.0f)); // right-bottom
            drawUIQuad(px+labelW*0.5f, py+labelH*0.5f-blockW*0.5f, labelW*0.5f, blockW, glm::vec4(color, 1.0f)); // short middle
        }
        else if (c == 'M') {
            drawUIQuad(px, py, blockW, labelH, glm::vec4(color, 1.0f)); // left
            drawUIQuad(px+labelW-blockW, py, blockW, labelH, glm::vec4(color, 1.0f)); // right
            drawUIQuad(px+blockW, py+labelH-blockW, labelW-blockW*2, blockW, glm::vec4(color, 1.0f)); // top
            drawUIQuad(px+labelW*0.5f-blockW*0.5f, py+labelH*0.5f, blockW, labelH*0.5f, glm::vec4(color, 1.0f)); // middle stem
        }
        else if (c == 'O') {
            drawUIQuad(px, py, blockW, labelH, glm::vec4(color, 1.0f)); // left
            drawUIQuad(px+labelW-blockW, py, blockW, labelH, glm::vec4(color, 1.0f)); // right
            drawUIQuad(px+blockW, py+labelH-blockW, labelW-blockW*2, blockW, glm::vec4(color, 1.0f)); // top
            drawUIQuad(px+blockW, py, labelW-blockW*2, blockW, glm::vec4(color, 1.0f)); // bottom
        }
        else if (c == 'V') {
            drawUIQuad(px, py+blockW*2.0f, blockW, labelH-blockW*2.0f, glm::vec4(color, 1.0f)); // left
            drawUIQuad(px+labelW-blockW, py+blockW*2.0f, blockW, labelH-blockW*2.0f, glm::vec4(color, 1.0f)); // right
            drawUIQuad(px+blockW, py, labelW-blockW*2, blockW*2.0f, glm::vec4(color, 1.0f)); // bottom
        }
        else if (c == 'W') {
            drawUIQuad(px, py, blockW, labelH, glm::vec4(color, 1.0f)); // left
            drawUIQuad(px+labelW-blockW, py, blockW, labelH, glm::vec4(color, 1.0f)); // right
            drawUIQuad(px+blockW, py, labelW-blockW*2, blockW, glm::vec4(color, 1.0f)); // bottom
            drawUIQuad(px+labelW*0.5f-blockW*0.5f, py, blockW, labelH*0.5f, glm::vec4(color, 1.0f)); // middle stem
        }
        else if (c == 'N') {
            drawUIQuad(px, py, blockW, labelH, glm::vec4(color, 1.0f)); // left
            drawUIQuad(px+labelW-blockW, py, blockW, labelH, glm::vec4(color, 1.0f)); // right
            drawUIQuad(px+blockW, py+labelH-blockW, labelW-blockW*2, blockW, glm::vec4(color, 1.0f)); // top-leftish join
        }
        else if (c == 'R') {
            drawUIQuad(px, py, blockW, labelH, glm::vec4(color, 1.0f)); // left
            drawUIQuad(px+blockW, py+labelH-blockW, labelW-blockW, blockW, glm::vec4(color, 1.0f)); // top
            drawUIQuad(px+labelW-blockW, py+labelH*0.5f, blockW, labelH*0.5f, glm::vec4(color, 1.0f)); // right-top
            drawUIQuad(px+blockW, py+labelH*0.5f-blockW*0.5f, labelW-blockW, blockW, glm::vec4(color, 1.0f)); // middle
            drawUIQuad(px+labelW-blockW*2.0f, py, blockW*2.0f, labelH*0.5f, glm::vec4(color, 1.0f)); // stronger leg
        }
        else if (c == 'T') {
            drawUIQuad(px, py+labelH-blockW, labelW, blockW, glm::vec4(color, 1.0f)); // top
            drawUIQuad(px+labelW*0.5f-blockW*0.5f, py, blockW, labelH-blockW, glm::vec4(color, 1.0f)); // stem
        }
        else if (c == 'I') {
            drawUIQuad(px+labelW*0.5f-blockW*0.5f, py, blockW, labelH, glm::vec4(color, 1.0f)); // stem
        }
        else if (c == '!') {
            drawUIQuad(px+labelW*0.5f-blockW*0.5f, py+blockW*2.0f, blockW, labelH-blockW*2.0f, glm::vec4(color, 1.0f)); // stem
            drawUIQuad(px+labelW*0.5f-blockW*0.5f, py, blockW, blockW, glm::vec4(color, 1.0f)); // dot
        }
        else if (c == '/') {
            // Diagonal line from bottom-left to top-right
            // We can approximate it with several small quads or one rotated (but drawUIQuad is axis-aligned)
            // Let's use a simple stepped approach or a single box if Renderer supported rotation.
            // Actually, I'll just draw a few blocks to make it look like a slash.
            drawUIQuad(px, py, blockW, blockW, glm::vec4(color, 1.0f));
            drawUIQuad(px+labelW*0.25f, py+labelH*0.25f, blockW, blockW, glm::vec4(color, 1.0f));
            drawUIQuad(px+labelW*0.5f, py+labelH*0.5f, blockW, blockW, glm::vec4(color, 1.0f));
            drawUIQuad(px+labelW*0.75f, py+labelH*0.75f, blockW, blockW, glm::vec4(color, 1.0f));
            drawUIQuad(px+labelW-blockW, py+labelH-blockW, blockW, blockW, glm::vec4(color, 1.0f));
        }
        else if (c == ' ') {
            // space - no drawing
        }
        
        px += labelW + labelSpacing;
    }
}

void Renderer::endUI() {
    glEnable(GL_DEPTH_TEST);
}

} // namespace engine
