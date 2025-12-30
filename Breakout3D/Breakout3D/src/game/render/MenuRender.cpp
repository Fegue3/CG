#include "game/render/UIRender.hpp"

#include "game/GameAssets.hpp"
#include "game/GameState.hpp"
#include "game/render/menu/MenuRenderParts.hpp"

namespace game::render {

void renderMenu(const RenderContext& ctx, const GameState& state, const GameAssets& assets) {
    ctx.renderer.beginUI(ctx.fbW, ctx.fbH);

    const auto m = game::render::menu::makeMenuCtx(ctx, state, assets);

    // Base layers (always drawn)
    game::render::menu::drawRetroBackground(m);
    game::render::menu::drawTitle(m);
    game::render::menu::drawMainPanelIfNeeded(m);

    // Underlying menu screens are hidden while the Instructions overlay is open,
    // because some overlay tabs intentionally have no opaque background.
    if (!state.showInstructions) {
        switch (state.currentMenuScreen) {
            case MenuScreen::MAIN:
                game::render::menu::drawMainScreen(m);
                break;
            case MenuScreen::PLAY_MODES:
                game::render::menu::drawPlayModesScreen(m);
                break;
            case MenuScreen::OPTIONS:
                game::render::menu::drawOptionsScreen(m);
                break;
            case MenuScreen::INSTRUCTIONS:
                game::render::menu::drawInstructionsScreen(m);
                break;
            case MenuScreen::LEVEL_SELECT:
                game::render::menu::drawLevelSelectScreen(m);
                break;
        }
    }

    // Small clickable "4" badge for one-brick test mode (MAIN screen only).
    game::render::menu::drawOneBrickTestBadge(m);

    // Instructions overlay (modal-ish).
    if (state.showInstructions) {
        game::render::menu::drawInstructionsOverlay(m);
    }

    ctx.renderer.endUI();
}

} // namespace game::render


