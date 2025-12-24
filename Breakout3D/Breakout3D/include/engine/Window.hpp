#pragma once
#include <utility>
#include <string>

namespace engine {

class Window {
public:
    Window();
    ~Window();

    bool create(int width, int height, const std::string& title, bool fullscreen = false);
    void pollEvents();
    bool shouldClose() const;
    void swapBuffers();
    void destroy();

    void requestClose();

    std::pair<int,int> getFramebufferSize() const;
    std::pair<int,int> getWindowSize() const;

    // não expõe GLFW types ao game
    void* nativeHandle() const { return m_window; }

private:
    void* m_window = nullptr; // GLFWwindow* escondido
};

} // namespace engine
