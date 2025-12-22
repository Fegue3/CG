#pragma once

namespace engine {

class Time {
public:
    void tick();
    float delta() const;
    float now() const;

private:
    float m_last = 0.0f;
    float m_dt = 0.0f;
};

} // namespace engine
