// Texture.cpp
// -----------------------------------------------------------------------------
// Texture.cpp
//
// Responsabilidade:
//  - Carregar imagens do disco via stb_image e criar Textures 2D em OpenGL.
//  - Suportar flip vertical no load (útil para alinhar UVs com a convenção do motor).
//  - Criar texturas a partir de buffers RGBA já existentes (ex.: GIF frames, UI).
//
// Notas:
//  - loadFromFile() escolhe o formato (RED/RGB/RGBA) com base nos channels.
//  - Por default cria mipmaps e usa GL_LINEAR_MIPMAP_LINEAR para minificação.
//  - destroy() liberta a textura OpenGL para evitar leaks.
// -----------------------------------------------------------------------------

#include "engine/Texture.hpp"
#include <stdexcept>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"

namespace engine {

void Texture2D::destroy() {
    if (id) glDeleteTextures(1, &id);
    id = 0;
    w = h = channels = 0;
}

Texture2D Texture2D::loadFromFile(const std::string& path, bool flipY) {
    Texture2D t;

    stbi_set_flip_vertically_on_load(flipY ? 1 : 0);

    unsigned char* data = stbi_load(path.c_str(), &t.w, &t.h, &t.channels, 0);
    if (!data) {
        throw std::runtime_error("stbi_load failed: " + path);
    }

    GLenum format = GL_RGB;
    if (t.channels == 1) format = GL_RED;
    else if (t.channels == 3) format = GL_RGB;
    else if (t.channels == 4) format = GL_RGBA;

    glGenTextures(1, &t.id);
    glBindTexture(GL_TEXTURE_2D, t.id);

    glTexImage2D(GL_TEXTURE_2D, 0, format, t.w, t.h, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    return t;
}

Texture2D Texture2D::loadFromRGBA(const unsigned char* rgba, int w, int h, bool generateMips) {
    Texture2D t;
    if (!rgba || w <= 0 || h <= 0) return t;
    t.w = w;
    t.h = h;
    t.channels = 4;

    glGenTextures(1, &t.id);
    glBindTexture(GL_TEXTURE_2D, t.id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    if (generateMips) glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, generateMips ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
    return t;
}

} // namespace engine
