// Time.cpp
// -----------------------------------------------------------------------------
// Time.cpp
//
// Responsabilidade:
//  - Medir delta time (dt) entre frames usando um clock monotónico.
//  - Acumular um "agora" (m_now) em segundos para timing geral do motor.
//
// Notas:
//  - No primeiro tick() inicializa estado e devolve dt=0.
//  - Faz clamp de dt para [0.0, 0.05] para evitar saltos enormes (ex.: breakpoint,
//    alt-tab, stutter grande) que podiam rebentar física/anim/logic.
// -----------------------------------------------------------------------------

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
