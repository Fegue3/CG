#include "game/render/ui/HudParts.hpp"

#include "game/GameAssets.hpp"
#include "game/GameState.hpp"

#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace game::render::hud {

void drawLivesHud(const RenderContext& ctx, const GameState& state, const GameAssets& assets) {
    const float padX = 22.0f;
    const float padTop = 18.0f;

    const float hs = 56.0f;
    const float gap = 12.0f;

    const float rx = glm::radians(18.0f);
    const float ry = glm::radians(-12.0f);

    int heartCount = std::max(3, state.lives);
    for (int i = 0; i < heartCount; ++i) {
        float xCenter = padX + hs * 0.5f + i * (hs + gap);
        float yCenter = (float)ctx.fbH - (padTop + hs * 0.5f);

        glm::mat4 M(1.0f);
        M = glm::translate(M, glm::vec3(xCenter, yCenter, 0.0f));
        M = glm::rotate(M, ry, glm::vec3(0,1,0));
        M = glm::rotate(M, rx, glm::vec3(1,0,0));
        M = glm::scale(M, glm::vec3(hs, hs, hs * 0.55f));

        glm::vec3 col = (i < state.lives)
            ? glm::vec3(1.0f, 0.20f, 0.25f)
            : glm::vec3(0.20f, 0.20f, 0.22f);

        ctx.renderer.drawMesh(assets.heart, M, col);
    }
}

} // namespace game::render::hud


