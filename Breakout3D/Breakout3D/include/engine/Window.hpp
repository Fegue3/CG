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

    // Mouse wheel scroll (Y axis). Returns delta accumulated since last call and resets it to 0.
    float consumeScrollY();
    void addScrollY(float dy);

    // não expõe GLFW types ao game
    void* nativeHandle() const { return m_window; }

private:
    void* m_window = nullptr; // GLFWwindow* escondido
    float m_scrollY = 0.0f;
};

} // namespace engine
