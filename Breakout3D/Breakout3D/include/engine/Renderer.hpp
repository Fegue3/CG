#pragma once
#include <glm/glm.hpp>
#include "engine/Shader.hpp"
#include "engine/Mesh.hpp"

namespace engine {

class Renderer {
public:
    bool init();
    void shutdown();

    void beginFrame(int fbW, int fbH);
    void setCamera(const glm::mat4& V, const glm::mat4& P, const glm::vec3& camPos);

    void drawCube(const Mesh& cube, const glm::vec3& pos, const glm::vec3& size, const glm::vec3& color);

private:
    Shader m_shader;
    Mesh m_cube;

    glm::mat4 m_V{1.0f}, m_P{1.0f};
    glm::vec3 m_camPos{0,0,0};
};

} // namespace engine
