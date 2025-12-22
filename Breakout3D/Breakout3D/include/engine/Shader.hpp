#pragma once
#include <GL/glew.h>

namespace engine {

class Shader {
public:
    bool load(const char* vertPath, const char* fragPath);
    void destroy();

    void use() const;
    GLuint id() const { return m_program; }

private:
    GLuint m_program = 0;
};

} // namespace engine
