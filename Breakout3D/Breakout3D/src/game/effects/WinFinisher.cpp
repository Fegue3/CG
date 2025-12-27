#include "game/effects/WinFinisher.hpp"

#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>

namespace game::effects {

WinFinisherCamera computeWinFinisherCamera(
    const glm::vec3& baseCamPos,
    const glm::vec3& baseCamTarget,
    float baseFovDeg,
    float arenaBaseScale,
    float finisherCineTimeSec,
    float finisherRealTimeSec
) {
    (void)finisherRealTimeSec; // kept for API stability; finisher uses cinematic time for sync
    WinFinisherCamera out;
    out.camPos = baseCamPos;
    out.camTarget = baseCamTarget;
    out.fovDeg = baseFovDeg;
    out.rollRad = 0.0f;

    // Keep camera break and shockwave perfectly in sync:
    // use the SAME "cinematic" time for the envelope as the overlay uses.
    float t = finisherCineTimeSec;
    const float dur = 1.05f;
    float s = std::max(0.0f, std::min(1.0f, t / dur));
    float kick = std::exp(-8.0f * s);

    // deterministic multi-sine "noise"
    float n1 = std::sin(t * 72.0f);
    float n2 = std::sin(t * 141.0f);
    float n3 = std::sin(t * 203.0f);
    float m1 = std::sin(t * 95.0f);
    float m2 = std::sin(t * 173.0f);
    float m3 = std::sin(t * 221.0f);

    glm::vec3 shake(
        0.55f * n1 + 0.25f * n2 + 0.20f * n3,
        0.55f * m1 + 0.25f * m2 + 0.20f * m3,
        0.55f * n2 + 0.25f * m2 + 0.20f * n1
    );

    float amp = arenaBaseScale * 0.0100f * kick;
    out.camPos += glm::vec3(shake.x * 1.0f, shake.y * 0.30f, shake.z * 0.90f) * amp;
    out.camTarget += glm::vec3(shake.x * 0.55f, shake.y * 0.18f, shake.z * 0.45f) * (amp * 0.6f);

    // FOV punch: widen quickly, then overshoot back (same timeline as ring).
    out.fovDeg += (15.0f * kick) * std::sin(s * glm::pi<float>() * 2.75f);

    // camera roll
    out.rollRad = glm::radians(7.0f) * kick * std::sin(t * 48.0f);

    return out;
}

static float smooth01(float x) {
    x = std::max(0.0f, std::min(1.0f, x));
    return x * x * (3.0f - 2.0f * x);
}

WinFinisherOverlay computeWinFinisherOverlay(
    int fbW, int fbH,
    const glm::mat4& V,
    const glm::mat4& P,
    const glm::vec3& anchorWorldPos,
    float finisherCineTimeSec,
    bool anchorValid
) {
    WinFinisherOverlay o;

    float t = finisherCineTimeSec;
    const float dur = 1.05f;
    float s = std::max(0.0f, std::min(1.0f, t / dur));

    // project anchor point to screen (UI coords are bottom-up)
    if (anchorValid) {
        glm::vec4 clip = P * V * glm::vec4(anchorWorldPos, 1.0f);
        if (std::abs(clip.w) > 1e-6f) {
            float ndcX = clip.x / clip.w;
            float ndcY = clip.y / clip.w;
            o.centerPx.x = (ndcX * 0.5f + 0.5f) * (float)fbW;
            o.centerPx.y = (ndcY * 0.5f + 0.5f) * (float)fbH;
        } else {
            o.centerPx = glm::vec2((float)fbW * 0.5f, (float)fbH * 0.52f);
        }
    } else {
        o.centerPx = glm::vec2((float)fbW * 0.5f, (float)fbH * 0.52f);
    }

    // flash + ring
    o.flashAlpha = 0.80f * std::exp(-10.0f * s);

    float maxR = 0.82f * (float)std::max(fbW, fbH);
    o.ringRadiusPx = 30.0f + maxR * smooth01(s);
    o.ringThicknessPx = 10.0f + 22.0f * (1.0f - smooth01(s));
    o.ringAlpha = 0.70f * std::exp(-4.0f * s);

    return o;
}

} // namespace game::effects


