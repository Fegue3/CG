#pragma once
#include <GL/glew.h>
#include <string>

namespace engine {

struct Mesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    int indexCount = 0;

    // Material básico vindo do MTL
    float kd[3] = {1.0f, 1.0f, 1.0f};   // diffuse color
    GLuint textureId = 0;              // 0 = sem textura

    void destroy();

    // baseDirPath: onde tu guardas os .obj/.mtl/.png (o user muda aqui)
    static void setBaseDirPath(const std::string& baseDirPath);

    // objRelativeOrFullPath: ex "Paddle.obj" se baseDirPath estiver set
    // ou "assets/models/Paddle.obj" se preferires passar full
    static Mesh loadOBJ(const std::string& objRelativeOrFullPath);
};

} // namespace engine
