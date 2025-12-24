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
    void drawUIQuad(float x, float y, float w, float h, const glm::vec3& color);
    void drawUIText(float x, float y, const std::string& text, float scale = 1.0f, const glm::vec3& color = glm::vec3(1.0f));
    void endUI();

private:
    Shader m_shader;

    glm::mat4 m_V{1.0f}, m_P{1.0f};
    glm::vec3 m_camPos{0,0,0};

    glm::vec3 m_lightPos{0.0f, 10.0f, 5.0f};
    glm::vec3 m_lightColor{1.0f, 1.0f, 1.0f};
    float m_ambientK = 0.15f;
    float m_diffuseK = 1.00f;
    float m_specK    = 1.00f;
    float m_shininess = 32.0f;
};

} // namespace engine
