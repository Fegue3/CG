#pragma once
#include <utility>

namespace engine {

class Window;

// Keys used by the game.
// Note: number keys map to the TOP ROW digits (GLFW_KEY_0..9), not numpad.
enum class Key { Escape, Left, Right, A, D, Space, K1, K2, K3, K4, K5, K6, K7, K8, K9, K0, Minus, L, R };
enum class MouseButton { Left };

class Input {
public:
    static constexpr int KEY_COUNT = 19;
    void update(Window& window);

    bool keyDown(Key k) const;
    bool keyPressed(Key k) const;

    bool mouseDown(MouseButton b) const;
    bool mousePressed(MouseButton b) const;

    // mouse em pixels do framebuffer (já convertido window->fb)
    std::pair<float,float> mousePosFbPx() const;

    // Mouse wheel scroll delta (Y) since last update() call (positive = scroll up in GLFW).
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
