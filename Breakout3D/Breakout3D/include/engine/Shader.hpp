#pragma once

#include <string>
#include <glm/glm.hpp>

namespace engine {

class Shader {
public:
    Shader();
    ~Shader();

    bool load(const std::string& vertPath, const std::string& fragPath);
    void use() const;

    // ✅ para o Renderer
    void destroy();
    unsigned int id() const;

    void setMat4(const std::string& name, const glm::mat4& mat) const;
    void setVec3(const std::string& name, const glm::vec3& v) const;
    void setFloat(const std::string& name, float v) const;

private:
    unsigned int m_id = 0;
};

} // namespace engine
