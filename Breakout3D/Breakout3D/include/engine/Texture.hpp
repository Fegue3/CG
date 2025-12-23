#pragma once
#include <GL/glew.h>
#include <string>

namespace engine {

struct Texture2D {
    GLuint id = 0;
    int w = 0, h = 0, channels = 0;

    void destroy();
    static Texture2D loadFromFile(const std::string& path, bool flipY = true);
};

} // namespace engine
