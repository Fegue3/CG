#include "game/render/ui/HudParts.hpp"

#include "game/GameAssets.hpp"
#include "game/GameState.hpp"
#include "game/rogue/RogueCards.hpp"
#include <algorithm>
#include <string>

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

    // Rogue deck icons (stacked cards) under the hearts.
    if (state.gameType == GameType::ROGUE && !state.rogueChosen.empty()) {
        const int cols = 6;
        const float sz = 30.0f;
        const float g = 8.0f;
        // Place below the heart row.
        float yTopHearts = (float)ctx.fbH - padTop;
        float yBottomHearts = yTopHearts - hs;
        float baseY = yBottomHearts - 16.0f - sz;
        float baseX = padX;

        for (size_t i = 0; i < state.rogueChosen.size(); ++i) {
            int col = (int)(i % cols);
            int row = (int)(i / cols);
            float x = baseX + col * (sz + g);
            float y = baseY - row * (sz + g);

            glm::vec3 acc = game::rogue::cardAccent(state.rogueChosen[i]);
            // shadow
            ctx.renderer.drawUIQuad(x + 2.5f, y - 2.5f, sz, sz, glm::vec4(0, 0, 0, 0.45f));
            // body
            ctx.renderer.drawUIQuad(x, y, sz, sz, glm::vec4(0.08f, 0.08f, 0.14f, 0.98f));
            // border
            float bt = 2.0f;
            ctx.renderer.drawUIQuad(x - bt, y - bt, sz + 2*bt, bt, glm::vec4(acc, 1.0f));
            ctx.renderer.drawUIQuad(x - bt, y + sz, sz + 2*bt, bt, glm::vec4(acc, 1.0f));
            ctx.renderer.drawUIQuad(x - bt, y, bt, sz, glm::vec4(acc, 1.0f));
            ctx.renderer.drawUIQuad(x + sz, y, bt, sz, glm::vec4(acc, 1.0f));

            // label
            std::string lbl = game::rogue::cardAbbrev(state.rogueChosen[i]);
            float s = 0.62f;
            float tw = ctx.renderer.measureUITextWidth(lbl, s);
            float th = ctx.renderer.getUIFontLineHeight(s);
            float tx = x + (sz - tw) * 0.5f;
            float ty = y + (sz - th) * 0.5f;
            ctx.renderer.drawUIText(tx + 1.0f, ty - 1.0f, lbl, s, glm::vec4(0, 0, 0, 0.55f));
            ctx.renderer.drawUIText(tx, ty, lbl, s, glm::vec4(1, 1, 1, 0.92f));
        }
    }
}

} // namespace game::render::hud


