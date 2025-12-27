#include "engine/Window.hpp"
#include "engine/Time.hpp"
#include "engine/Renderer.hpp"
#include "engine/Input.hpp"

#include "game/Game.hpp"
#include "game/GameAssets.hpp"

int main() {
    engine::Window window;
    // Slightly taller default window so the big menu title fits above the options cleanly.
    if (!window.create(1280, 900, "Breakout3D")) return -1;

    engine::Time time;
    time.tick();

    engine::Renderer renderer;
    if (!renderer.init()) return -1;

    // ✅ assets: carrega .obj/.mtl (e texturas via map_Kd)
    game::GameAssets assets;
    if (!assets.loadAll()) return -1;

    // ✅ input: GLFW fica escondido no engine
    engine::Input input;

    // ✅ game recebe assets
    game::Game game(window, time, renderer, assets);
    // Don't call init() here - let the game start at MENU state

    while (!window.shouldClose()) {
        time.tick();
        window.pollEvents();

        input.update(window);
        game.update(input);
        game.render();
    }

    assets.destroy();
    renderer.shutdown();
    window.destroy();
    return 0;
}
