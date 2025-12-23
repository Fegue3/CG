#pragma once
#include <chrono>

namespace engine {

class Time {
public:
    void tick();
    float delta() const { return m_dt; }
    float now()   const { return m_now; }

private:
    using clock = std::chrono::steady_clock;
    clock::time_point m_lastTp{};
    bool m_hasLast = false;

    float m_dt  = 0.0f;
    float m_now = 0.0f;
};

} // namespace engine
