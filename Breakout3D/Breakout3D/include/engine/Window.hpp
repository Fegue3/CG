#pragma once

// GLEW tem de vir ANTES do GLFW, senão o GLFW puxa gl.h primeiro.
#include <GL/glew.h>

#include <GLFW/glfw3.h>

namespace engine {

class Window {
public:
    bool create(int width, int height, const char* title);
    void destroy();

    bool shouldClose() const;
    void pollEvents();
    void swapBuffers();

    GLFWwindow* handle() { return m_window; }

    void getFramebufferSize(int& w, int& h) const;

    void toggleFullscreen();

private:
    static void framebuffer_size_callback(GLFWwindow*, int w, int h);

private:
    GLFWwindow* m_window = nullptr;

    int m_fbW = 1280, m_fbH = 720;

    bool m_fullscreen = false;
    int m_winX = 100, m_winY = 100, m_winW = 1280, m_winH = 720;
};

} // namespace engine
