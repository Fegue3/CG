// Texture.hpp
#pragma once
#include <GL/glew.h>
#include <string>

namespace engine {

/**
 * @file Texture.hpp
 * @brief Textura 2D OpenGL (load do disco e upload directo de RGBA).
 *
 * Notas:
 * - `loadFromFile()` suporta flipY para alinhar UV/origem.
 * - `loadFromRGBA()` é útil para atlas de fonte, frames GIF e texturas geradas.
 * - `destroy()` liberta o id OpenGL (contexto GL activo).
 */
struct Texture2D {
    GLuint id = 0;
    int w = 0, h = 0, channels = 0;

    void destroy();

    static Texture2D loadFromFile(const std::string& path, bool flipY = true);
    static Texture2D loadFromRGBA(const unsigned char* rgba, int w, int h, bool generateMips = false);
};

} // namespace engine
