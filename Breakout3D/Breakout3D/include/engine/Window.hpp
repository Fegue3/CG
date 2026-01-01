// Window.hpp
#pragma once
#include <utility>
#include <string>

namespace engine {

/**
 * @file Window.hpp
 * @brief Janela + contexto OpenGL + eventos, escondendo tipos GLFW do resto do projeto.
 *
 * Notas:
 * - `getFramebufferSize()` vs `getWindowSize()` (DPI scaling).
 * - Scroll é acumulado (callback) e consumido por frame via `consumeScrollY()`.
 * - `nativeHandle()` devolve um ponteiro opaco (void*).
 */
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

    float consumeScrollY();
    void addScrollY(float dy);

    void* nativeHandle() const { return m_window; }

private:
    void* m_window = nullptr; // GLFWwindow* escondido
    float m_scrollY = 0.0f;
};

} // namespace engine
