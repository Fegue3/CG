// OverlayLayout.hpp
#pragma once

namespace engine { class Renderer; }

namespace game::ui {

/**
 * @file OverlayLayout.hpp
 * @brief Helpers de layout (Rect + layouts de overlays/menu/som).
 *
 * Objectivo:
 * - definir rectângulos e layouts em px do framebuffer;
 * - assegurar que render e input usam exactamente a mesma geometria;
 * - manter “números” do UI num único sítio.
 */

/**
 * @brief Rectângulo em pixels do framebuffer (x,y,w,h).
 *
 * @note
 * `contains()` é usado para hit-testing (cliques/hover).
 */
struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    /// @return true se o ponto (px,py) estiver dentro do rectângulo (inclusive).
    bool contains(float px, float py) const {
        return px >= x && px <= (x + w) && py >= y && py <= (y + h);
    }
};

struct OverlayLayout {
    Rect panel;
    Rect leftBtn;
    Rect rightBtn;
};

/**
 * @brief Layout do overlay de pausa (modo normal/endless).
 *
 * @note Tem de bater certo com o que é desenhado em `game/render/UIRender.cpp`.
 */
inline OverlayLayout pauseOverlay(int fbW, int fbH) {
    OverlayLayout L;
    L.panel.w = 650.0f;
    L.panel.h = 320.0f; // mais alto para caber "PAUSED" maior + botões
    L.panel.x = (fbW - L.panel.w) * 0.5f;
    L.panel.y = (fbH - L.panel.h) * 0.5f;

    const float btnW = 140.0f;
    const float btnH = 60.0f;
    const float btnGap = 50.0f;
    const float btnXLeft = L.panel.x + (L.panel.w - 2.0f * btnW - btnGap) * 0.5f;
    const float btnXRight = btnXLeft + btnW + btnGap;
    const float btnY = L.panel.y + 40.0f;

    L.leftBtn = Rect{btnXLeft, btnY, btnW, btnH};
    L.rightBtn = Rect{btnXRight, btnY, btnW, btnH};
    return L;
}

/**
 * @brief Layout maior do overlay de pausa para o modo Rogue (para caber a lista de cartas).
 *
 * @note Tem de bater certo com o que é desenhado em `game/render/UIRender.cpp`.
 */
inline OverlayLayout pauseOverlayRogue(int fbW, int fbH) {
    OverlayLayout L;
    L.panel.w = 920.0f;
    L.panel.h = 700.0f;
    L.panel.x = (fbW - L.panel.w) * 0.5f;
    L.panel.y = (fbH - L.panel.h) * 0.5f;

    const float btnW = 140.0f;
    const float btnH = 60.0f;
    const float btnGap = 50.0f;
    const float btnXLeft = L.panel.x + (L.panel.w - 2.0f * btnW - btnGap) * 0.5f;
    const float btnXRight = btnXLeft + btnW + btnGap;
    const float btnY = L.panel.y + 40.0f;

    L.leftBtn = Rect{btnXLeft, btnY, btnW, btnH};
    L.rightBtn = Rect{btnXRight, btnY, btnW, btnH};
    return L;
}

/**
 * @brief Layout do overlay de fim de jogo (game over / vitória simples).
 *
 * @note Tem de bater certo com o que é desenhado em `game/render/UIRender.cpp`.
 */
inline OverlayLayout endOverlay(int fbW, int fbH) {
    OverlayLayout L;
    L.panel.w = 650.0f;
    L.panel.h = 260.0f;
    L.panel.x = (fbW - L.panel.w) * 0.5f;
    L.panel.y = (fbH - L.panel.h) * 0.5f;

    const float btnW = 140.0f;
    const float btnH = 60.0f;
    const float btnGap = 50.0f;
    const float btnXLeft = L.panel.x + (L.panel.w - 2.0f * btnW - btnGap) * 0.5f;
    const float btnXRight = btnXLeft + btnW + btnGap;
    const float btnY = L.panel.y + 40.0f;

    L.leftBtn = Rect{btnXLeft, btnY, btnW, btnH};
    L.rightBtn = Rect{btnXRight, btnY, btnW, btnH};
    return L;
}

/**
 * @brief Layout base do menu (painel, título, botões e cartões de modos).
 *
 * É calculado a partir do tamanho do framebuffer e da fonte actual (via renderer),
 * para manter consistência entre resoluções e assegurar que input/render coincidem.
 */
struct MenuLayout {
    float uiScale = 1.0f;
    float panelX = 0.0f;
    float panelY = 0.0f;
    float panelW = 0.0f;
    float panelH = 0.0f;

    // Título (posicionamento para render + input bater certo).
    float titleX = 0.0f;
    float titleY = 0.0f;
    float titleW = 0.0f;
    float titleH = 0.0f;
    float titleScale = 1.0f;

    Rect btn1;      // Normal
    Rect btn2;      // Endless
    Rect btn3;      // Instructions
    Rect btn4;      // Exit
    Rect testBadge; // One brick test

    // PLAY_MODES: um cartão por modo, com um botão PLAY dentro de cada cartão.
    struct ModeCard {
        Rect card;
        Rect playBtn;
    };
    ModeCard normal;
    ModeCard endless;
    ModeCard rogue;
    ModeCard levels;

    Rect backBtn;
};

struct SoundSettingsLayout {
    Rect panel;
    Rect backBtn;

    struct Slider {
        Rect track;
        Rect knob;
    };

    Slider master;
    Slider sfx;
    Slider music;
    Slider stinger;
};

/// Layout do ecrã de som (sliders), derivado do MenuLayout já calculado.
SoundSettingsLayout soundSettingsLayout(const MenuLayout& menu, int fbW, int fbH);

/**
 * @brief Calcula o layout do menu para a resolução actual.
 *
 * @note
 * Implementado em `.cpp` para evitar trazer o `Renderer` completo para todos os headers.
 */
MenuLayout calculateMenuLayout(engine::Renderer& renderer, int fbW, int fbH);

} // namespace game::ui
