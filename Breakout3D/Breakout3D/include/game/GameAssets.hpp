// GameAssets.hpp
#pragma once
#include "engine/Mesh.hpp"
#include "engine/Texture.hpp"
#include "engine/Shader.hpp"
#include "engine/AnimatedTexture.hpp"
#include <array>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

namespace game {

/**
 * @file GameAssets.hpp
 * @brief Assets carregados do jogo (meshes, texturas, shaders e previews animados).
 *
 * Inclui:
 * - meshes de entidades (paddle/ball/bricks/powerups)
 * - backgrounds
 * - shader/mesh do fundo “scrolling”
 * - GIFs opcionais para previews no UI (carregamento lazy + decode em threads)
 *
 * @note
 * A parte dos GIFs está desenhada para não bloquear o arranque nem congelar o menu:
 * - threads descodificam para RGBA
 * - o upload para GPU é feito aos poucos (pump) com um budget por tick
 */
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

    // GIFs opcionais para “vídeo” no UI (lazy-loaded).
    // Ficheiros em `assets/video/*.gif`.
    // Ordem deve bater certo com o inspector (MenuRender):
    // 0 EXPAND, 1 EXTRA_BALL, 2 EXTRA_LIFE, 3 FIREBALL, 4 SLOW, 5 SHIELD, 6 REVERSE, 7 TINY
    std::string powerupVideoPaths[8];
    mutable engine::AnimatedTexture2D powerupVideos[8];
    mutable bool powerupVideoLoaded[8]{};

    // Estado de decode/upload assíncrono (decode em thread; upload é “pumped” no thread do GL).
    struct DecodedGif {
        int w = 0;
        int h = 0;
        std::vector<std::vector<unsigned char>> framesRGBA;
        std::vector<int> delaysMs;
    };

    mutable std::array<std::thread, 8> powerupVideoThreads{};
    mutable std::array<std::atomic<bool>, 8> powerupVideoDecoding{};
    mutable std::array<std::atomic<bool>, 8> powerupVideoDecoded{};
    mutable std::array<std::atomic<bool>, 8> powerupVideoTried{};
    mutable std::array<std::mutex, 8> powerupVideoMutex{};
    mutable std::array<DecodedGif, 8> powerupVideoDecodedData{};
    mutable std::array<int, 8> powerupVideoUploadCursor{};
    mutable std::atomic<bool> powerupVideoPreloadStarted{false};

    /// @return GIF animado do power-up idx (pode estar vazio se ainda não carregou).
    const engine::AnimatedTexture2D& powerupVideo(int idx) const;

    /// Lança threads de decode para todos os GIFs (chamar quando for oportuno).
    void startPowerupVideoPreload() const;

    /// Faz upload de alguns frames por tick (para evitar spikes). `uploadBudgetFrames` controla o “orçamento”.
    void pumpPowerupVideoPreload(int uploadBudgetFrames = 6) const;

    // Fundo “scrolling”
    engine::Shader scrollingBgShader;
    engine::Mesh backgroundMesh;

    /// Carrega todos os assets base (meshes/texturas/shaders).
    bool loadAll();

    /// Liberta recursos (meshes/texturas/shaders) e encerra tarefas pendentes.
    void destroy();
};

} // namespace game
