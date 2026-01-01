// Input.hpp
#pragma once
#include <utility>

namespace engine {

class Window;

/**
 * @file Input.hpp
 * @brief Estado de input por frame (teclas/rato/scroll) sem expor GLFW ao jogo.
 *
 * Notas:
 * - `keyDown` = mantida; `keyPressed` = transição (solta->carregada) no frame.
 * - Posição do rato em pixels do framebuffer (útil para UI com DPI scaling).
 * - `update(Window&)` deve ser chamado 1x por frame (após pollEvents).
 */
enum class Key { Escape, Left, Right, A, D, Space, K1, K2, K3, K4, K5, K6, K7, K8, K9, K0, Minus, L, R };
enum class MouseButton { Left };

class Input {
public:
    static constexpr int KEY_COUNT = 19;

    /// Actualiza estados (down/pressed), posição do rato e delta de scroll do frame.
    void update(Window& window);

    bool keyDown(Key k) const;
    bool keyPressed(Key k) const;

    bool mouseDown(MouseButton b) const;
    bool mousePressed(MouseButton b) const;

    /// @return Posição do rato em px do framebuffer (x,y).
    std::pair<float,float> mousePosFbPx() const;

    /// @return Delta de scroll Y desde o último `update()` (positivo = scroll up no GLFW).
    float mouseScrollY() const { return m_scrollY; }

private:
    bool m_keyDown[KEY_COUNT]{};
    bool m_keyPrev[KEY_COUNT]{};

    bool m_mouseDown[1]{};
    bool m_mousePrev[1]{};

    float m_mouseX = 0.0f;
    float m_mouseY = 0.0f;
    float m_scrollY = 0.0f;
};

} // namespace engine
