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

} // namespace engine
