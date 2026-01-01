// GameUpdateWinFinisher.cpp
/**
 * @file GameUpdateWinFinisher.cpp
 * @brief “Finisher” de vitória no modo Normal: hit-stop + slow-mo curto antes do overlay WIN.
 *
 * Ideia:
 *  - Quando a última brick morre no Normal, ativa-se winFinisherActive.
 *  - Durante ~1s, aplica-se:
 *      - hit-stop (freeze)
 *      - slow-motion curto
 *      - ramp suave de volta ao 1x
 *  - No fim, desativa finisher e muda para GameMode::WIN,
 *    disparando música e stinger imediatamente (porque este path “return early”).
 */
 #include "game/Game.hpp"

 namespace game {
 
 bool Game::updateWinFinisher(float dt) {
     if (!m_state.winFinisherActive) return false;
 
     m_state.winFinisherRealTimer += dt;
     float tr = m_state.winFinisherRealTimer;
 
     const float hitStop   = 0.045f;
     const float slowDur   = 0.200f;
     const float rampDur   = 0.180f;
     const float slowScale = 0.25f;
 
     float timeScale = 1.0f;
     if (tr < hitStop) {
         timeScale = 0.0f;
     } else if (tr < hitStop + slowDur) {
         timeScale = slowScale;
     } else if (tr < hitStop + slowDur + rampDur) {
         float u = (tr - (hitStop + slowDur)) / rampDur;
         u = std::max(0.0f, std::min(1.0f, u));
         float ease = u * u * (3.0f - 2.0f * u); // smoothstep
         timeScale = slowScale + (1.0f - slowScale) * ease;
     } else {
         timeScale = 1.0f;
     }
 
     // Timer do finisher avança com o timeScale aplicado.
     m_state.winFinisherTimer += dt * timeScale;
 
     const float dur = 1.05f;
     if (m_state.winFinisherTimer >= dur) {
         m_state.winFinisherActive = false;
         m_state.winFinisherTimer = 0.0f;
         m_state.winFinisherRealTimer = 0.0f;
         m_state.winFinisherHoldBrickValid = false;
 
         m_state.mode = GameMode::WIN;
 
         // Música/stinger aqui porque o finisher retorna cedo e pode “saltar” o handler normal.
         setMusic("music_results_loop", 0.25f);
         m_audio.playStinger("stinger_level_clear", +2.5f);
     }
     return true;
 }
 
 } // namespace game
 