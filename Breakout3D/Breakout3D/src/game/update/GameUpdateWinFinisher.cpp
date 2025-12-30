#include "game/Game.hpp"

namespace game {

bool Game::updateWinFinisher(float dt) {
    // Normal mode: when the last brick is destroyed, we play a short "camera break" finisher
    // before switching to the WIN overlay.
    if (!m_state.winFinisherActive) return false;

    // Snappy parry-ish feel: tiny hit-stop, brief slow-mo, then ramp back to 1x.
    m_state.winFinisherRealTimer += dt;
    float tr = m_state.winFinisherRealTimer;

    // Keep it subtle: just enough slow-mo to feel "counter-parry", not enough to drag.
    const float hitStop = 0.045f; // seconds (freeze)
    const float slowDur = 0.200f; // seconds (slow)
    const float rampDur = 0.180f; // seconds (ease back to 1x)
    const float slowScale = 0.25f;

    float timeScale = 1.0f;
    if (tr < hitStop) {
        timeScale = 0.0f;
    } else if (tr < hitStop + slowDur) {
        timeScale = slowScale;
    } else if (tr < hitStop + slowDur + rampDur) {
        float u = (tr - (hitStop + slowDur)) / rampDur; // 0..1
        u = std::max(0.0f, std::min(1.0f, u));
        float ease = u * u * (3.0f - 2.0f * u);
        timeScale = slowScale + (1.0f - slowScale) * ease;
    } else {
        timeScale = 1.0f;
    }

    m_state.winFinisherTimer += dt * timeScale;

    const float dur = 1.05f;
    if (m_state.winFinisherTimer >= dur) {
        m_state.winFinisherActive = false;
        m_state.winFinisherTimer = 0.0f;
        m_state.winFinisherRealTimer = 0.0f;
        m_state.winFinisherHoldBrickValid = false;
        m_state.mode = GameMode::WIN;
        // Immediate results music (the finisher path returns early, so we must trigger here).
        setMusic("music_results_loop", 0.25f);
        m_audio.playStinger("stinger_level_clear", +2.5f);
    }
    return true;
}

} // namespace game


