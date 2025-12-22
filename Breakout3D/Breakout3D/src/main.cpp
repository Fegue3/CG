#include "engine/Window.hpp"
#include "engine/Time.hpp"
#include "engine/Renderer.hpp"
#include "game/Game.hpp"

int main() {
    engine::Window window;
    if (!window.create(1280, 720, "Breakout3D")) return -1;

    engine::Time time;
    time.tick();

    engine::Renderer renderer;
    if (!renderer.init()) return -1;

    game::Game game(window, time, renderer);
    game.init();

    while (!window.shouldClose()) {
        time.tick();
        window.pollEvents();

        game.update();
        game.render();
    }

    renderer.shutdown();
    window.destroy();
    return 0;
}
