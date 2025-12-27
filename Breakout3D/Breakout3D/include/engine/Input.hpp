#pragma once
#include <utility>

namespace engine {

class Window;

enum class Key { Escape, Left, Right, A, D, Space, K1, K2, K3, K4, K5, K6, K7 };
enum class MouseButton { Left };

class Input {
public:
    void update(const Window& window);

    bool keyDown(Key k) const;
    bool keyPressed(Key k) const;

    bool mouseDown(MouseButton b) const;
    bool mousePressed(MouseButton b) const;

    // mouse em pixels do framebuffer (já convertido window->fb)
    std::pair<float,float> mousePosFbPx() const;

private:
    bool m_keyDown[13]{};
    bool m_keyPrev[13]{};

    bool m_mouseDown[1]{};
    bool m_mousePrev[1]{};

    float m_mouseX = 0.0f;
    float m_mouseY = 0.0f;
};

} // namespace engine
