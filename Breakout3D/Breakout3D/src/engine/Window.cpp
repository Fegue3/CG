#include "engine/Window.hpp"
#include <iostream>

namespace engine {

Window::Window() {}

Window::~Window() {
    destroy();
}

bool Window::create(int width, int height, const std::string& title) {
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);

    // ✅ opcional mas recomendado (vsync)
    glfwSwapInterval(1);

    // 🔥 GLEW INIT (depois do contexto)
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Failed to init GLEW\n";
        return false;
    }

    glEnable(GL_DEPTH_TEST);

    return true;
}

void Window::pollEvents() {
    glfwPollEvents();
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_window);
}

void Window::swapBuffers() {
    glfwSwapBuffers(m_window);
}

void Window::destroy() {
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
        glfwTerminate();
    }
}

std::pair<int,int> Window::getFramebufferSize() const {
    int w = 1, h = 1;
    glfwGetFramebufferSize(m_window, &w, &h);
    return {w, h};
}

}
