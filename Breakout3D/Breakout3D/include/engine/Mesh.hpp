#pragma once
#include <GL/glew.h>

namespace engine {

struct Mesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    int vertexCount = 0;

    void destroy();
    static Mesh createCube();
};

} // namespace engine
