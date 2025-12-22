#pragma once

// ⚠️ ORDEM CORRETA: GLEW antes de GLFW
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <utility>
#include <string>

namespace engine {

class Window {
public:
    Window();
    ~Window();

    bool create(int width, int height, const std::string& title);
    void pollEvents();
    bool shouldClose() const;
    void swapBuffers();
    void destroy();

    std::pair<int,int> getFramebufferSize() const;

    // ✅ Nome final (Game.cpp usa isto)
    GLFWwindow* getHandle() const { return m_window; }

private:
    GLFWwindow* m_window = nullptr;
};

}
