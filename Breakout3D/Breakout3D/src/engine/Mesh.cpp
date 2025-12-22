#include "engine/Mesh.hpp"

namespace engine {

void Mesh::destroy() {
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
    vbo = vao = 0;
    vertexCount = 0;
}

Mesh Mesh::createCube() {
    static const float v[] = {
        // +X
        0.5f,-0.5f,-0.5f,  1,0,0,  0.5f, 0.5f,-0.5f,  1,0,0,  0.5f, 0.5f, 0.5f,  1,0,0,
        0.5f,-0.5f,-0.5f,  1,0,0,  0.5f, 0.5f, 0.5f,  1,0,0,  0.5f,-0.5f, 0.5f,  1,0,0,
        // -X
       -0.5f,-0.5f, 0.5f, -1,0,0, -0.5f, 0.5f, 0.5f, -1,0,0, -0.5f, 0.5f,-0.5f, -1,0,0,
       -0.5f,-0.5f, 0.5f, -1,0,0, -0.5f, 0.5f,-0.5f, -1,0,0, -0.5f,-0.5f,-0.5f, -1,0,0,
        // +Y
       -0.5f, 0.5f,-0.5f,  0,1,0, -0.5f, 0.5f, 0.5f,  0,1,0,  0.5f, 0.5f, 0.5f,  0,1,0,
       -0.5f, 0.5f,-0.5f,  0,1,0,  0.5f, 0.5f, 0.5f,  0,1,0,  0.5f, 0.5f,-0.5f,  0,1,0,
        // -Y
       -0.5f,-0.5f, 0.5f,  0,-1,0, -0.5f,-0.5f,-0.5f,  0,-1,0,  0.5f,-0.5f,-0.5f,  0,-1,0,
       -0.5f,-0.5f, 0.5f,  0,-1,0,  0.5f,-0.5f,-0.5f,  0,-1,0,  0.5f,-0.5f, 0.5f,  0,-1,0,
        // +Z
       -0.5f,-0.5f, 0.5f,  0,0,1,  0.5f,-0.5f, 0.5f,  0,0,1,  0.5f, 0.5f, 0.5f,  0,0,1,
       -0.5f,-0.5f, 0.5f,  0,0,1,  0.5f, 0.5f, 0.5f,  0,0,1, -0.5f, 0.5f, 0.5f,  0,0,1,
        // -Z
        0.5f,-0.5f,-0.5f,  0,0,-1, -0.5f,-0.5f,-0.5f, 0,0,-1, -0.5f, 0.5f,-0.5f, 0,0,-1,
        0.5f,-0.5f,-0.5f,  0,0,-1, -0.5f, 0.5f,-0.5f, 0,0,-1,  0.5f, 0.5f,-0.5f, 0,0,-1,
    };

    Mesh m;
    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);

    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);

    m.vertexCount = 36;
    return m;
}

} // namespace engine
