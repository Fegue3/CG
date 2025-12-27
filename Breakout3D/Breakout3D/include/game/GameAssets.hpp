#pragma once
#include "engine/Mesh.hpp"
#include "engine/Texture.hpp"

namespace game {

struct GameAssets {
    engine::Mesh paddle;
    engine::Mesh ball;
    engine::Mesh heart;
    engine::Mesh wall;

    engine::Mesh brick01;

    engine::Mesh brick02;
    engine::Mesh brick02_1hit;

    engine::Mesh brick03;
    engine::Mesh brick03_1hit;
    engine::Mesh brick03_2hit;

    engine::Mesh brick04;
    engine::Mesh brick04_1hit;
    engine::Mesh brick04_2hit;
    engine::Mesh brick04_3hit;

    engine::Mesh expand;
    engine::Mesh extraBall;
    engine::Mesh slow;
    engine::Mesh extraLife;

    // New powerups
    engine::Mesh fireball;
    engine::Mesh shield;
    engine::Mesh skull; // reverse controls curse
    engine::Mesh minus; // tiny paddle curse
    engine::Texture2D backgroundTexs[4];

    bool loadAll();
    void destroy();
};

} // namespace game
