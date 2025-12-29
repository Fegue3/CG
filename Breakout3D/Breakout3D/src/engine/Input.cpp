#include "engine/Input.hpp"
#include "engine/Window.hpp"

#include <GLFW/glfw3.h>
#include <algorithm>

namespace engine {

static int keyToGlfw(Key k) {
    switch (k) {
        case Key::Escape: return GLFW_KEY_ESCAPE;
        case Key::Left:   return GLFW_KEY_LEFT;
        case Key::Right:  return GLFW_KEY_RIGHT;
        case Key::A:      return GLFW_KEY_A;
        case Key::D:      return GLFW_KEY_D;
        case Key::Space:  return GLFW_KEY_SPACE;
        case Key::K1:     return GLFW_KEY_1;
        case Key::K2:     return GLFW_KEY_2;
        case Key::K3:     return GLFW_KEY_3;
        case Key::K4:     return GLFW_KEY_4;
        case Key::K5:     return GLFW_KEY_5;
        case Key::K6:     return GLFW_KEY_6;
        case Key::K7:     return GLFW_KEY_7;
        case Key::K8:     return GLFW_KEY_8;
        case Key::K9:     return GLFW_KEY_9;
        case Key::K0:     return GLFW_KEY_0;
        case Key::Minus:  return GLFW_KEY_MINUS;
        case Key::L:      return GLFW_KEY_L;
    }
    return GLFW_KEY_UNKNOWN;
}

static int mouseToGlfw(MouseButton b) {
    switch (b) {
        case MouseButton::Left: return GLFW_MOUSE_BUTTON_LEFT;
    }
    return GLFW_MOUSE_BUTTON_LEFT;
}

void Input::update(Window& window) {
    std::copy(std::begin(m_keyDown), std::end(m_keyDown), std::begin(m_keyPrev));
    std::copy(std::begin(m_mouseDown), std::end(m_mouseDown), std::begin(m_mousePrev));
    m_scrollY = window.consumeScrollY();

    GLFWwindow* w = (GLFWwindow*)window.nativeHandle();

    for (int i = 0; i < Input::KEY_COUNT; ++i) {
        m_keyDown[i] = (glfwGetKey(w, keyToGlfw((Key)i)) == GLFW_PRESS);
    }

    for (int i = 0; i < 1; ++i) {
        m_mouseDown[i] = (glfwGetMouseButton(w, mouseToGlfw((MouseButton)i)) == GLFW_PRESS);
    }

    double mx, my;
    glfwGetCursorPos(w, &mx, &my);

    auto [fbW, fbH] = window.getFramebufferSize();
    auto [ww, wh]   = window.getWindowSize();
    ww = std::max(1, ww);
    wh = std::max(1, wh);

    float sx = (float)fbW / (float)ww;
    float sy = (float)fbH / (float)wh;

    m_mouseX = (float)mx * sx;
    m_mouseY = (float)my * sy;
}

bool Input::keyDown(Key k) const { return m_keyDown[(int)k]; }
bool Input::keyPressed(Key k) const { return m_keyDown[(int)k] && !m_keyPrev[(int)k]; }

bool Input::mouseDown(MouseButton b) const { return m_mouseDown[(int)b]; }
bool Input::mousePressed(MouseButton b) const { return m_mouseDown[(int)b] && !m_mousePrev[(int)b]; }

std::pair<float,float> Input::mousePosFbPx() const { return {m_mouseX, m_mouseY}; }

} // namespace engine
