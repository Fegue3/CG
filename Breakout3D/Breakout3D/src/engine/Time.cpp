#include "engine/Time.hpp"
#include <algorithm>

namespace engine {

void Time::tick() {
    auto tp = clock::now();

    if (!m_hasLast) {
        m_lastTp = tp;
        m_hasLast = true;
        m_dt = 0.0f;
        return;
    }

    std::chrono::duration<float> d = tp - m_lastTp;
    m_lastTp = tp;

    m_dt = d.count();
    if (m_dt < 0.0f) m_dt = 0.0f;
    if (m_dt > 0.05f) m_dt = 0.05f;

    m_now += m_dt;
}

} // namespace engine
