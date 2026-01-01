// InstructionsOverlayLayout.hpp
#pragma once

#include "game/ui/OverlayLayout.hpp"

namespace game::ui {

/**
 * @file InstructionsOverlayLayout.hpp
 * @brief Layout do overlay de Instruções (por cima do menu).
 *
 * Este layout existe para garantir que:
 * - o que é desenhado (rectângulos no ecrã) bate certo com as hitboxes do input;
 * - a geometria fica centralizada num sítio só (evita “magic numbers” espalhados).
 *
 * O overlay está organizado em:
 * - painel principal + botão de voltar
 * - área esquerda para preview/modelo 3D
 * - área direita para texto/info + zona de “vídeo” (placeholder)
 * - setas de navegação (ex.: power-ups anterior/seguinte)
 */
struct InstructionsOverlayLayout {
    Rect panel;
    Rect backBtn;
    Rect modelRect;   ///< Metade esquerda (preview 3D)
    Rect infoRect;    ///< Metade direita (texto + zona de vídeo)
    Rect navLeft;     ///< Navegar para trás (ex.: power-up anterior)
    Rect navRight;    ///< Navegar para a frente (ex.: power-up seguinte)
    Rect videoRect;   ///< Placeholder de vídeo dentro de infoRect
};

/**
 * @brief Calcula o layout do overlay de Instruções para a resolução actual.
 *
 * @param menu Layout base do menu (já calculado para o fbW/fbH).
 * @param fbW Largura do framebuffer (px).
 * @param fbH Altura do framebuffer (px).
 * @param instructionsTab Tab actual (ex.: controlos / power-ups / cartas rogue).
 */
InstructionsOverlayLayout instructionsOverlayLayout(const MenuLayout& menu, int fbW, int fbH, int instructionsTab);

} // namespace game::ui
