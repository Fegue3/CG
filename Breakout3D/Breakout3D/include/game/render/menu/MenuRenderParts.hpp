// MenuRenderParts.hpp
#pragma once

#include "game/render/RenderContext.hpp"
#include "game/GameState.hpp"

#include <string>
#include <glm/glm.hpp>

namespace game {
struct GameAssets;
} // namespace game

namespace game::render::menu {

/**
 * @file MenuRenderParts.hpp
 * @brief Funções e helpers para renderização do menu (ecrãs principais + overlays).
 *
 * @details
 * Este header agrupa as “peças” de render do menu para permitir dividir a implementação
 * por vários `.cpp` (ex.: main screen, options, sound, level select) sem passar uma lista
 * enorme de parâmetros entre funções.
 *
 * A ideia é:
 * - Construir um `MenuCtx` uma vez por frame (com base no `RenderContext`, `GameState`, assets e layout).
 * - Usar o `MenuCtx` para desenhar camadas base (background/título/painel).
 * - Desenhar ecrãs específicos (main/options/sound/...) reutilizando helpers comuns (botões, badges, overlay).
 *
 * Convenções:
 * - Coordenadas são em pixels de framebuffer (UI pass).
 * - `L` (MenuLayout) contém métricas e posicionamento “canónico”; `MenuCtx` guarda valores já calculados/cached.
 */

/**
 * @brief Contexto interno para render do menu (bundle pequeno de referências + geometria cache).
 *
 * @details
 * Mantém:
 * - Referências para contexto de render (renderer, fbW/fbH, etc.), estado do jogo e assets.
 * - Referência ao layout `L` (métricas do menu/UI).
 * - Valores derivados do layout (tamanho/posição de painéis e botões) já calculados.
 *
 * Isto evita “threading” de 10+ parâmetros e reduz recomputação de geometria em cada função.
 */
struct MenuCtx {
    /// Contexto de render do jogo (usado para desenhar UI, ler tamanhos, etc.).
    const RenderContext& ctx;

    /// Estado global do jogo (para saber que ecrã mostrar, highlights, toggles, etc.).
    const GameState& state;

    /// Assets do jogo (texturas, fontes, etc.) usados no menu.
    const GameAssets& assets;

    /// Layout de menu/UI (medidas base, offsets, spacing).
    const game::ui::MenuLayout& L;

    /// Escala global do UI (ex.: para DPI scaling ou “UI scale” nas opções).
    float uiS = 1.0f;

    // ---------- Geometria do painel principal (cache) ----------
    /// Largura do painel do menu principal.
    float panelW = 0.0f;
    /// Altura do painel do menu principal.
    float panelH = 0.0f;
    /// X (top-left ou origem do teu sistema) do painel.
    float panelX = 0.0f;
    /// Y do painel.
    float panelY = 0.0f;

    // ---------- Geometria base dos botões (cache) ----------
    /// Largura padrão dos botões.
    float btnW = 0.0f;
    /// Altura padrão dos botões.
    float btnH = 0.0f;
    /// X base dos botões (coluna principal).
    float btnX = 0.0f;

    /// Y do botão 1 (primeiro botão do menu principal).
    float btn1Y = 0.0f;
    /// Y do botão 2.
    float btn2Y = 0.0f;
    /// Y do botão 3.
    float btn3Y = 0.0f;
    /// Y do botão 4.
    float btn4Y = 0.0f;
};

/**
 * @brief Constrói o `MenuCtx` a partir do estado actual do jogo e do render context.
 *
 * @details
 * Normalmente:
 * - Calcula `uiS` (escala) a partir de opções/estado.
 * - Lê o `MenuLayout` e preenche `panel*` e `btn*` (valores já escalados e posicionados).
 *
 * @return MenuCtx pronto a usar nas funções `draw*`.
 */
MenuCtx makeMenuCtx(const RenderContext& ctx, const GameState& state, const GameAssets& assets);

// ---------------- Camadas base (comuns a vários ecrãs) ----------------

/// Desenha o background “retro” (gradiente, scanlines, noise, etc. dependendo da implementação).
void drawRetroBackground(const MenuCtx& m);

/// Desenha o título do jogo (ou do ecrã actual) no topo.
void drawTitle(const MenuCtx& m);

/// Desenha o painel principal do menu se o ecrã actual o requer (alguns ecrãs podem ser full-screen).
void drawMainPanelIfNeeded(const MenuCtx& m);

// ---------------- Helper comum de botões ----------------

/**
 * @brief Helper partilhado para desenhar um botão do menu com label e (opcional) subtítulo.
 *
 * @param m MenuCtx com layout e refs.
 * @param btnIndex Índice lógico do botão (útil para hover/seleção via estado).
 * @param bx,by,bw,bh Rectângulo do botão em px.
 * @param label Texto principal do botão.
 * @param baseColor Cor base do botão (o .cpp pode ajustar para hover/pressed/disabled).
 * @param subtitle Texto secundário (opcional) por baixo/ao lado do label.
 *
 * @note
 * O estado (hover/selecionado/disabled) é tipicamente inferido através de `m.state`.
 */
void drawButton(
    const MenuCtx& m,
    int btnIndex,
    float bx, float by, float bw, float bh,
    const std::string& label,
    const glm::vec3& baseColor,
    const std::string& subtitle = ""
);

// ---------------- Ecrãs principais (não são overlays) ----------------

/// Ecrã principal (Play/Options/Sair, etc.).
void drawMainScreen(const MenuCtx& m);

/// Ecrã de opções (UI scale, gráficos simples, toggles, etc.).
void drawOptionsScreen(const MenuCtx& m);

/// Ecrã de som (volumes, toggles música/sfx, etc.).
void drawSoundScreen(const MenuCtx& m);

/// Ecrã de instruções (pode mostrar tabs ou encaminhar para overlay).
void drawInstructionsScreen(const MenuCtx& m);

/// Ecrã de escolha de modos (Normal/Endless/Rogue, etc.).
void drawPlayModesScreen(const MenuCtx& m);

/// Ecrã de seleção de nível (Levels mode).
void drawLevelSelectScreen(const MenuCtx& m);

// ---------------- Extra UI pequeno ----------------

/// Badge/etiqueta para debug/feature flag (ex.: “One Brick Test” ligado).
void drawOneBrickTestBadge(const MenuCtx& m);

// ---------------- Overlay de instruções ----------------

/**
 * @brief Overlay de instruções com separadores (ex.: controlos / powerups / rogue cards).
 *
 * @details
 * Desenhado por cima do menu actual (overlay), tipicamente com fundo semi-transparente
 * e navegação por tabs.
 */
void drawInstructionsOverlay(const MenuCtx& m);

} // namespace game::render::menu
