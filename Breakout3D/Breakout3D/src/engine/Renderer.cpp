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

    // ✅ parâmetros “mundo”
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
    // ✅ HUD: não tocar no color buffer => “transparente” por cima do jogo
    glDisable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    // ✅ UI em ortho com Y para CIMA (evita scale negativo e normals maradas)
    glm::mat4 Vui(1.0f);
    glm::mat4 Pui = glm::ortho(0.0f, (float)fbW, 0.0f, (float)fbH, -1000.0f, 1000.0f);

    // câmara “a olhar” para z=0
    glm::vec3 cam = glm::vec3((float)fbW * 0.5f, (float)fbH * 0.5f, 700.0f);
    setCamera(Vui, Pui, cam);

    // ✅ luz própria do HUD (mais fraca)
    // luz acima e à esquerda dos corações, a apontar para o plano z=0
    m_lightPos   = glm::vec3(120.0f, (float)fbH - 80.0f, 260.0f);
    m_lightColor = glm::vec3(1.0f);

    // menos luz/spec, mas ainda com “shape”
    m_ambientK   = 0.28f;
    m_diffuseK   = 0.35f;
    m_specK      = 0.28f;
    m_shininess  = 64.0f;
}

void Renderer::endUI() {
    glEnable(GL_DEPTH_TEST);
}

} // namespace engine
