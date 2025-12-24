#define STB_EASY_FONT_IMPLEMENTATION

#include "engine/Renderer.hpp"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>

static void setMat4(GLuint p, const char* n, const glm::mat4& m) {
    glUniformMatrix4fv(glGetUniformLocation(p, n), 1, GL_FALSE, &m[0][0]);
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
    if (!m_shader.load("assets/shaders/basic_phong.vert", "assets/shaders/basic_phong.frag"))
        return false;
    return true;
}

void Renderer::shutdown() {
    m_shader.destroy();
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

void Renderer::drawUIQuad(float x, float y, float w, float h, const glm::vec3& color) {
    m_shader.use();
    GLuint p = m_shader.id();

    glm::mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(x + w*0.5f, y + h*0.5f, 0.0f));
    M = glm::scale(M, glm::vec3(w, h, 1.0f));

    setMat4(p, "uV", m_V);
    setMat4(p, "uP", m_P);
    setMat4(p, "uM", M);

    setInt(p, "uUseTex", 0);
    setVec3(p, "uAlbedo", color);

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
        if (c == 'R') {
            // R = left bar + top + right-top + middle + diagonal leg
            drawUIQuad(px, py, blockW, labelH, color);                      // left
            drawUIQuad(px+blockW, py+labelH-blockW, labelW-blockW, blockW, color);  // top
            drawUIQuad(px+labelW-blockW, py+labelH*0.5f, blockW, labelH*0.5f, color); // right-top
            drawUIQuad(px+blockW, py+labelH*0.5f-blockW*0.5f, labelW-blockW, blockW, color); // middle
            drawUIQuad(px+labelW*0.5f, py, blockW*1.5f, labelH*0.5f, color); // diagonal leg
        }
        else if (c == 'E') {
            // E = left + top + middle + bottom
            drawUIQuad(px, py, blockW, labelH, color);                      // left
            drawUIQuad(px+blockW, py+labelH-blockW, labelW-blockW, blockW, color);  // top
            drawUIQuad(px+blockW, py+labelH*0.5f-blockW*0.5f, labelW-blockW, blockW, color); // middle
            drawUIQuad(px+blockW, py, labelW-blockW, blockW, color);        // bottom
        }
        else if (c == 'S') {
            // S = top + left-top + middle + right-bottom + bottom
            drawUIQuad(px+blockW, py+labelH-blockW, labelW-blockW, blockW, color);  // top
            drawUIQuad(px, py+labelH*0.5f, blockW, labelH*0.5f, color);     // left-top
            drawUIQuad(px+blockW, py+labelH*0.5f-blockW*0.5f, labelW-blockW, blockW, color); // middle
            drawUIQuad(px+labelW-blockW, py, blockW, labelH*0.5f, color);   // right-bottom
            drawUIQuad(px+blockW, py, labelW-blockW, blockW, color);        // bottom
        }
        else if (c == 'T') {
            // T = top + stem
            drawUIQuad(px, py+labelH-blockW, labelW, blockW, color);        // top
            drawUIQuad(px+labelW*0.5f-blockW*0.5f, py, blockW, labelH-blockW, color); // stem
        }
        else if (c == 'A') {
            // A = left + right + top + middle
            drawUIQuad(px, py, blockW, labelH, color);                      // left
            drawUIQuad(px+labelW-blockW, py, blockW, labelH, color);        // right
            drawUIQuad(px+blockW, py+labelH-blockW, labelW-blockW*2, blockW, color); // top
            drawUIQuad(px+blockW, py+labelH*0.5f, labelW-blockW*2, blockW, color);   // middle
        }
        else if (c == 'I') {
            // I = vertical stem
            drawUIQuad(px+labelW*0.5f-blockW*0.5f, py, blockW, labelH, color); // stem
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
