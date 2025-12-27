#pragma once
#include <glm/glm.hpp>

namespace game::effects {

struct WinFinisherCamera {
    glm::vec3 camPos{0.0f};
    glm::vec3 camTarget{0.0f};
    float fovDeg = 45.0f;
    float rollRad = 0.0f;
};

struct WinFinisherOverlay {
    glm::vec2 centerPx{0.0f};
    float flashAlpha = 0.0f;
    float ringAlpha = 0.0f;
    float ringRadiusPx = 0.0f;
    float ringThicknessPx = 0.0f;
};

// Computes camera shake/roll/fov punch for the finisher (pure math, no rendering).
WinFinisherCamera computeWinFinisherCamera(
    const glm::vec3& baseCamPos,
    const glm::vec3& baseCamTarget,
    float baseFovDeg,
    float arenaBaseScale,
    float finisherCineTimeSec,
    float finisherRealTimeSec
);

// Computes overlay values and projects the anchor world position to screen space.
// `V` and `P` should be the final camera matrices used for rendering the scene.
WinFinisherOverlay computeWinFinisherOverlay(
    int fbW, int fbH,
    const glm::mat4& V,
    const glm::mat4& P,
    const glm::vec3& anchorWorldPos,
    float finisherCineTimeSec,
    bool anchorValid
);

} // namespace game::effects


