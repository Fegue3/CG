#include "engine/Window.hpp"
#include <iostream>

namespace engine {

bool Window::create(int width, int height, const char* title) {
    if (!glfwInit()) return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);

    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW init failed\n";
        return false;
    }

    getFramebufferSize(m_fbW, m_fbH);
    glViewport(0, 0, m_fbW, m_fbH);

    return true;
}

void Window::destroy() {
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}

bool Window::shouldClose() const { return glfwWindowShouldClose(m_window); }
void Window::pollEvents() { glfwPollEvents(); }
void Window::swapBuffers() { glfwSwapBuffers(m_window); }

void Window::getFramebufferSize(int& w, int& h) const {
    glfwGetFramebufferSize(m_window, &w, &h);
    if (w < 1) w = 1;
    if (h < 1) h = 1;
}

void Window::framebuffer_size_callback(GLFWwindow* win, int w, int h) {
    auto* self = (Window*)glfwGetWindowUserPointer(win);
    if (!self) return;
    self->m_fbW = (w > 1) ? w : 1;
    self->m_fbH = (h > 1) ? h : 1;
    glViewport(0, 0, self->m_fbW, self->m_fbH);
}

void Window::toggleFullscreen() {
    m_fullscreen = !m_fullscreen;

    if (m_fullscreen) {
        glfwGetWindowPos(m_window, &m_winX, &m_winY);
        glfwGetWindowSize(m_window, &m_winW, &m_winH);

        GLFWmonitor* m = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(m);
        glfwSetWindowMonitor(m_window, m, 0, 0, mode->width, mode->height, mode->refreshRate);
        glfwSetWindowAttrib(m_window, GLFW_DECORATED, GLFW_FALSE);
    } else {
        glfwSetWindowMonitor(m_window, nullptr, m_winX, m_winY, m_winW, m_winH, 0);
        glfwSetWindowAttrib(m_window, GLFW_DECORATED, GLFW_TRUE);
    }

    getFramebufferSize(m_fbW, m_fbH);
    glViewport(0, 0, m_fbW, m_fbH);
}

} // namespace engine
