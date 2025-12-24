#include "engine/Window.hpp"
#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace engine {

Window::Window() {}
Window::~Window() { destroy(); }

bool Window::create(int width, int height, const std::string& title, bool fullscreen) {
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    GLFWmonitor* monitor = fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    GLFWwindow* w = glfwCreateWindow(width, height, title.c_str(), monitor, nullptr);
    if (!w) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return false;
    }

    m_window = w;
    glfwMakeContextCurrent(w);
    glfwSwapInterval(1);

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Failed to init GLEW\n";
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    return true;
}

void Window::pollEvents() { glfwPollEvents(); }

bool Window::shouldClose() const {
    GLFWwindow* w = (GLFWwindow*)m_window;
    return glfwWindowShouldClose(w);
}

void Window::swapBuffers() {
    GLFWwindow* w = (GLFWwindow*)m_window;
    glfwSwapBuffers(w);
}

void Window::requestClose() {
    GLFWwindow* w = (GLFWwindow*)m_window;
    glfwSetWindowShouldClose(w, 1);
}

void Window::destroy() {
    if (m_window) {
        GLFWwindow* w = (GLFWwindow*)m_window;
        glfwDestroyWindow(w);
        m_window = nullptr;
        glfwTerminate();
    }
}

std::pair<int,int> Window::getFramebufferSize() const {
    GLFWwindow* w = (GLFWwindow*)m_window;
    int fw = 1, fh = 1;
    glfwGetFramebufferSize(w, &fw, &fh);
    return {fw, fh};
}

std::pair<int,int> Window::getWindowSize() const {
    GLFWwindow* w = (GLFWwindow*)m_window;
    int ww = 1, wh = 1;
    glfwGetWindowSize(w, &ww, &wh);
    return {ww, wh};
}

} // namespace engine
