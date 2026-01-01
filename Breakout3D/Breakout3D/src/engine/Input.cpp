// Input.cpp
#include "engine/Input.hpp"
#include "engine/Window.hpp"

#include <GLFW/glfw3.h>
#include <algorithm>

namespace engine {

/**
 * @file Input.cpp
 * @brief Adaptação de input (teclado/rato) para uma enum própria do jogo.
 *
 * O Input faz “polling” do estado no fim/início de cada frame:
 * - guarda estado actual (down) e o anterior (prev) para detectar edges (pressed)
 * - converte coordenadas do cursor para pixels do framebuffer (para UI / hit-testing)
 * - lê o scroll acumulado do Window (o callback mete lá os deltas)
 */

// Converte a enum Key do jogo para o código GLFW correspondente.
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
        case Key::R:      return GLFW_KEY_R;
    }
    return GLFW_KEY_UNKNOWN;
}

// Converte a enum MouseButton do jogo para o código GLFW correspondente.
static int mouseToGlfw(MouseButton b) {
    switch (b) {
        case MouseButton::Left: return GLFW_MOUSE_BUTTON_LEFT;
    }
    // Fallback defensivo.
    return GLFW_MOUSE_BUTTON_LEFT;
}

void Input::update(Window& window) {
    // Guarda o estado anterior para poderes fazer "pressed" (edge).
    std::copy(std::begin(m_keyDown),   std::end(m_keyDown),   std::begin(m_keyPrev));
    std::copy(std::begin(m_mouseDown), std::end(m_mouseDown), std::begin(m_mousePrev));

    // O scroll é acumulado no Window (via callbacks); aqui consumimos e fazemos reset.
    m_scrollY = window.consumeScrollY();

    GLFWwindow* w = (GLFWwindow*)window.nativeHandle();

    // Teclas: itera pela enum (assumindo que Key tem valores contíguos 0..KEY_COUNT-1).
    for (int i = 0; i < Input::KEY_COUNT; ++i) {
        m_keyDown[i] = (glfwGetKey(w, keyToGlfw((Key)i)) == GLFW_PRESS);
    }

    // Botões do rato (actualmente só 1).
    for (int i = 0; i < 1; ++i) {
        m_mouseDown[i] = (glfwGetMouseButton(w, mouseToGlfw((MouseButton)i)) == GLFW_PRESS);
    }

    // Posição do cursor em coordenadas de janela (pixels “window space”).
    double mx, my;
    glfwGetCursorPos(w, &mx, &my);

    // Converte para pixels de framebuffer (importante em HiDPI / scaling).
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
