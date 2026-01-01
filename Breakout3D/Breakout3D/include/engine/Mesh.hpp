// Mesh.hpp
#pragma once
#include <GL/glew.h>
#include <string>

namespace engine {

/**
 * @file Mesh.hpp
 * @brief Mesh estática (OBJ/MTL) com buffers OpenGL (VAO/VBO/EBO) + material básico.
 *
 * Notas:
 * - `kd` vem do MTL (diffuse).
 * - `textureId` = 0 significa “sem textura”.
 * - `destroy()` liberta VAO/VBO/EBO (contexto GL activo).
 * - `setBaseDirPath()` facilita caminhos relativos para assets.
 */
struct Mesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    int indexCount = 0;

    float kd[3] = {1.0f, 1.0f, 1.0f};
    GLuint textureId = 0;

    /// Liberta buffers OpenGL e reseta IDs.
    void destroy();

    /// Define a pasta base usada para resolver caminhos relativos (OBJ/MTL/texturas).
    static void setBaseDirPath(const std::string& baseDirPath);

    /// Carrega um OBJ (relativo à baseDirPath ou full path) e cria buffers para render.
    static Mesh loadOBJ(const std::string& objRelativeOrFullPath);
};

} // namespace engine
