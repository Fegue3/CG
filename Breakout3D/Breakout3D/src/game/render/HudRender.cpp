#include "game/render/UIRender.hpp"

#include "game/GameAssets.hpp"
#include "game/GameConfig.hpp"
#include "game/GameState.hpp"
#include "game/render/ui/HudParts.hpp"

namespace game::render {

void renderUI(const RenderContext& ctx, const GameState& state, const GameConfig& cfg, const GameAssets& assets) {
    ctx.renderer.beginUI(ctx.fbW, ctx.fbH);

    hud::drawWinFinisherOverlay(ctx, state);
    hud::drawFireballExplosionFx(ctx, state, cfg);
    hud::drawLivesHud(ctx, state, assets);
    hud::drawDangerZoneHud(ctx, state);
    hud::drawScoreAndWaveHud(ctx, state);
    hud::drawBgSelectorHud(ctx, state);
    hud::drawPauseOrEndOverlay(ctx, state);

    ctx.renderer.endUI();
}

} // namespace game::render


