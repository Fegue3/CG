#include "engine/Window.hpp"
#include "engine/Time.hpp"
#include "engine/Renderer.hpp"
#include "engine/Input.hpp"

#include "game/Game.hpp"
#include "game/GameAssets.hpp"

/*
    Entry point:
    - Cria janela, renderer e carrega assets.
    - Corre loop principal:
        tick time -> poll events -> update input -> update game -> render game
*/
int main() {
    engine::Window window;
    // Slightly taller default window so the big menu title fits above the options cleanly.
    if (!window.create(1280, 900, "Breakout3D")) return -1;

    engine::Time time;
    time.tick();

    engine::Renderer renderer;
    if (!renderer.init()) return -1;

    // Load assets (.obj/.mtl + textures via map_Kd)
    game::GameAssets assets;
    if (!assets.loadAll()) return -1;

    // Input (GLFW fica encapsulado na camada engine)
    engine::Input input;

    // Game recebe window/time/renderer/assets
    game::Game game(window, time, renderer, assets);
    // Não chamar init() aqui - o jogo começa no estado MENU.

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
