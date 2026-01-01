/**
 * @file InstructionsOverlayLayout.cpp
 * @brief Calcula o layout (rects) do overlay de "Instructions" (tabs: Controls / Powerups / Rogue Cards).
 *
 * Objetivo:
 *  - Gerar um conjunto de Rects com dimensões/posições consistentes para render + input.
 *  - Garantir que:
 *      - O painel não invade o título principal ("BREAKOUT 3D").
 *      - O botão BACK fica no MESMO sítio do BACK da página PLAY MODES (consistência UX).
 *      - O conteúdo é dividido em:
 *          - zona esquerda (modelRect)  -> viewport / lista (dependendo do tab)
 *          - zona direita (infoRect)    -> texto / cards / widgets
 *      - Para tab Powerups e RogueCards:
 *          - O painel cresce mais em fullscreen (mais “respiro” e leitura).
 */
 #include "game/ui/InstructionsOverlayLayout.hpp"

 #include <algorithm>
 
 namespace game::ui {
 
 InstructionsOverlayLayout instructionsOverlayLayout(const MenuLayout& menu, int fbW, int fbH, int instructionsTab) {
     InstructionsOverlayLayout L{};
 
     // Tabs:
     // 0 = Controls
     // 1 = Powerups
     // 2 = Rogue Cards
     const bool isPowerups   = (instructionsTab == 1);
     const bool isRogueCards = (instructionsTab == 2);
 
     const float uiS = menu.uiScale;
 
     // ------------------------------------------------------------------
     // 1) Painel principal: tamanho e posição
     // ------------------------------------------------------------------
     // Regras:
     //  - Powerups/RogueCards -> painel maior (aproveita fullscreen melhor)
     //  - Controls -> painel mais compacto
     float instrW = (isPowerups || isRogueCards)
         ? std::min(1400.0f, (float)fbW * 0.92f)
         : std::min(980.0f,  (float)fbW * 0.75f);
 
     float instrH = (isPowerups || isRogueCards)
         ? std::min(860.0f,  (float)fbH * 0.78f)
         : std::min(690.0f,  (float)fbH * 0.72f);
 
     float instrX = ((float)fbW - instrW) * 0.5f;
 
     // Nota: coordenadas y-up (como o resto do UI no projeto)
     // Posição vertical com “lift” para dar espaço ao BACK e composição com o título.
     float instrY = std::max(
         40.0f,
         ((float)fbH - instrH) * 0.5f - ((isPowerups || isRogueCards) ? 80.0f : 60.0f)
     );
 
     // Mantém o painel abaixo do título grande (mesma regra de OverlayLayout).
     const float titlePanelGap = 26.0f;
     float maxTopY = menu.titleY - titlePanelGap; // y-up
     if (instrY + instrH > maxTopY) {
         instrY = maxTopY - instrH;
         instrY = std::max(40.0f, instrY);
     }
 
     L.panel = Rect{instrX, instrY, instrW, instrH};
 
     // ------------------------------------------------------------------
     // 2) Botão BACK
     // ------------------------------------------------------------------
     // Requisito: "Back button should match PLAY screen position exactly."
     // Ou seja, reutiliza o backBtn do layout do menu (PLAY MODES).
     L.backBtn = menu.backBtn;
 
     // ------------------------------------------------------------------
     // 3) Região de conteúdo interna (margens + reservas)
     // ------------------------------------------------------------------
     // pad: margem lateral/baixo
     // topReserve: espaço no topo para título do overlay + underline + “breathing room”
     const float pad = ((isPowerups || isRogueCards) ? 34.0f : 44.0f) * uiS;
     const float topReserve = ((isPowerups || isRogueCards) ? 132.0f : 110.0f) * uiS;
 
     float contentTop = instrY + instrH - topReserve;
 
     // contentBottom também tem de ficar acima do BACK para não haver overlap visual/click.
     float contentBottom = std::max(
         instrY + pad,
         L.backBtn.y + L.backBtn.h + 18.0f * uiS
     );
 
     float contentH = std::max(1.0f, contentTop - contentBottom);
     float contentX = instrX + pad;
     float contentW = std::max(1.0f, instrW - pad * 2.0f);
 
     // ------------------------------------------------------------------
     // 4) Split: esquerda (preview/lista) vs direita (info)
     // ------------------------------------------------------------------
     // gap maior para Powerups/RogueCards (UI “mais premium” em fullscreen).
     const float gap = ((isPowerups || isRogueCards) ? 34.0f : 26.0f) * uiS;
 
     // split: percentagem para esquerda
     // Powerups/RogueCards -> mais espaço para preview/lista (0.58)
     float split = (isPowerups || isRogueCards) ? 0.58f : 0.50f;
 
     float leftW  = (contentW - gap) * split;
     float rightW = contentW - gap - leftW;
 
     L.modelRect = Rect{contentX, contentBottom, leftW,  contentH};
     L.infoRect  = Rect{contentX + leftW + gap, contentBottom, rightW, contentH};
 
     // ------------------------------------------------------------------
     // 5) Navegação (setas) no fundo do infoRect
     // ------------------------------------------------------------------
     float navW = 64.0f * uiS;
     float navH = 56.0f * uiS;
     float navY = L.infoRect.y + 10.0f * uiS;
 
     L.navLeft  = Rect{L.infoRect.x + 12.0f * uiS, navY, navW, navH};
     L.navRight = Rect{L.infoRect.x + L.infoRect.w - navW - 12.0f * uiS, navY, navW, navH};
 
     // ------------------------------------------------------------------
     // 6) "Video placeholder": card 16:9 acima das setas
     // ------------------------------------------------------------------
     float vPadX = 12.0f * uiS;
     float vW = std::max(1.0f, L.infoRect.w - vPadX * 2.0f);
 
     // Aspect ratio 16:9
     float vH = vW * (9.0f / 16.0f);
 
     // Limite: não ocupar demasiado do painel direito
     float maxVH = std::max(1.0f, L.infoRect.h * 0.46f);
     if (vH > maxVH) vH = maxVH;
 
     float vX = L.infoRect.x + vPadX;
     float vY = L.navLeft.y + L.navLeft.h + 18.0f * uiS;
 
     L.videoRect = Rect{vX, vY, vW, vH};
 
     return L;
 }
 
 } // namespace game::ui
 