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

        expand        = engine::Mesh::loadOBJ("Expand.obj");
        extraBall     = engine::Mesh::loadOBJ("Extra_Ball.obj");
        slow          = engine::Mesh::loadOBJ("Slow.obj");
        extraLife     = engine::Mesh::loadOBJ("extralife.obj");

        // Se quiseres walls com o mesmo mesh do brick:
        wall = brick01;

        backgroundTexs[0] = engine::Texture2D::loadFromFile("assets/textures/Background.png", true);
        backgroundTexs[1] = engine::Texture2D::loadFromFile("assets/textures/Background2.png", true);
        backgroundTexs[2] = engine::Texture2D::loadFromFile("assets/textures/Background3.png", true);
        backgroundTexs[3] = engine::Texture2D::loadFromFile("assets/textures/Background4.png", true);

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

    expand.destroy();
    extraBall.destroy();
    slow.destroy();
    extraLife.destroy();
    for (int i = 0; i < 4; i++) backgroundTexs[i].destroy();
}

} // namespace game
