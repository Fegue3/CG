#include "game/GameAssets.hpp"
#include <iostream>

namespace game {

bool GameAssets::loadAll() {
    try {
        // baseDirPath: aqui é o único sítio onde defines isto
        engine::Mesh::setBaseDirPath("assets/models");

        ball   = engine::Mesh::loadOBJ("Ball.obj");
        paddle = engine::Mesh::loadOBJ("Paddle.obj");
        heart  = engine::Mesh::loadOBJ("heart.obj");

        brick01 = engine::Mesh::loadOBJ("Brick_01.obj");

        brick02       = engine::Mesh::loadOBJ("Brick_02.obj");
        brick02_1hit  = engine::Mesh::loadOBJ("Brick_02_1hit.obj");

        brick03       = engine::Mesh::loadOBJ("Brick_03.obj");
        brick03_1hit  = engine::Mesh::loadOBJ("Brick_03_1hit.obj");
        brick03_2hit  = engine::Mesh::loadOBJ("Brick_03_2hit.obj");

        brick04       = engine::Mesh::loadOBJ("Brick_04.obj");
        brick04_1hit  = engine::Mesh::loadOBJ("Brick_04_1hit.obj");
        brick04_2hit  = engine::Mesh::loadOBJ("Brick_04_2hit.obj");
        brick04_3hit  = engine::Mesh::loadOBJ("Brick_04_3hit.obj");

        // Se quiseres walls com o mesmo mesh do brick:
        wall = brick01;



        return true;
    } catch (const std::exception& e) {
        std::cerr << "[GameAssets] loadAll failed: " << e.what() << "\n";
        return false;
    }
}

void GameAssets::destroy() {
    // cuidado: wall = brick01 é cópia de handles OpenGL.
    // Se fizeres isso, NÃO destruas wall separado. Melhor: remove wall e usa brick01.
    // Vou destruir só os "donos reais":
    ball.destroy();
    paddle.destroy();
    heart.destroy();

    brick01.destroy();

    brick02.destroy();
    brick02_1hit.destroy();

    brick03.destroy();
    brick03_1hit.destroy();
    brick03_2hit.destroy();

    brick04.destroy();
    brick04_1hit.destroy();
    brick04_2hit.destroy();
    brick04_3hit.destroy();
}

} // namespace game
