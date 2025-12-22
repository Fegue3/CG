#include "engine/Time.hpp"
#include <GLFW/glfw3.h>

namespace engine {

void Time::tick() {
    float t = (float)glfwGetTime();
    m_dt = t - m_last;
    m_last = t;
    if (m_dt < 0.0f) m_dt = 0.0f;
    if (m_dt > 0.05f) m_dt = 0.05f;
}

float Time::delta() const { return m_dt; }
float Time::now() const { return (float)glfwGetTime(); }

} // namespace engine
