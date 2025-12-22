#include "engine/Renderer.hpp"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>

static void setMat4(GLuint p, const char* n, const glm::mat4& m) {
    glUniformMatrix4fv(glGetUniformLocation(p, n), 1, GL_FALSE, &m[0][0]);
}
static void setVec3(GLuint p, const char* n, const glm::vec3& v) {
    glUniform3f(glGetUniformLocation(p, n), v.x, v.y, v.z);
}

namespace engine {

bool Renderer::init() {
    glEnable(GL_DEPTH_TEST);

    if (!m_shader.load("assets/shaders/basic_phong.vert", "assets/shaders/basic_phong.frag"))
        return false;

    m_cube = Mesh::createCube();
    return true;
}

void Renderer::shutdown() {
    m_cube.destroy();
    m_shader.destroy();
}

void Renderer::beginFrame(int fbW, int fbH) {
    glViewport(0, 0, fbW, fbH);
    glClearColor(0.05f, 0.06f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::setCamera(const glm::mat4& V, const glm::mat4& P, const glm::vec3& camPos) {
    m_V = V; m_P = P; m_camPos = camPos;
}

void Renderer::drawCube(const Mesh&, const glm::vec3& pos, const glm::vec3& size, const glm::vec3& color) {
    m_shader.use();
    GLuint p = m_shader.id();

    setMat4(p, "uV", m_V);
    setMat4(p, "uP", m_P);
    setVec3(p, "uViewPos", m_camPos);
    setVec3(p, "uLightPos", glm::vec3(0.0f, 10.0f, 5.0f));
    setVec3(p, "uLightColor", glm::vec3(1.0f));
    setVec3(p, "uAlbedo", color);

    glm::mat4 M(1.0f);
    M = glm::translate(M, pos);
    M = glm::scale(M, size);
    setMat4(p, "uM", M);

    glBindVertexArray(m_cube.vao);
    glDrawArrays(GL_TRIANGLES, 0, m_cube.vertexCount);
    glBindVertexArray(0);
}

} // namespace engine
