/**
 * @file MenuRenderLevelSelect.cpp
 * @brief Render do ecrã de selecção de níveis (LEVELS): painel grande com grelha 4x5 (20 níveis),
 *        título, botões de nível (locked/unlocked + hover) e botão BACK.
 *
 * @details
 *  Este ecrã não usa o painel “default” do menu (drawMainPanelIfNeeded) — desenha o seu próprio
 *  painel maior, centrado, para caber numa grelha 4 linhas x 5 colunas.
 *
 *  Estrutura visual:
 *   - Painel: sombra + corpo + borda RGB animada (mesmo estilo do menu).
 *   - Título “SELECT LEVEL” com glow ciano (mais “tech/arcade” do que RGB).
 *   - Grelha de 20 botões:
 *      * unlocked: tons ciano/azulado, alpha alto
 *      * locked: cinza escuro, alpha baixo
 *      * hover: sombra maior + cor mais “neon”
 *   - Botão BACK no canto inferior-esquerdo do painel, reusa drawButton().
 *
 *  Dependências do GameState:
 *   - state.levelsBestLevel:
 *        usado para decidir se um nível está desbloqueado.
 *        O código usa: unlocked = (i < levelsBestLevel)
 *        → interpreta-se como “bestLevel” ser o número de níveis desbloqueados (ou o primeiro bloqueado).
 *   - state.hoveredLevelButton:
 *        índice do botão de nível em hover (0..19).
 *
 *  Nota importante:
 *   - Se queres que o nível “actual” (o próximo a jogar) também esteja disponível,
 *     normalmente a condição seria (i <= levelsBestLevel) em vez de (i < levelsBestLevel),
 *     dependendo de como guardas o progresso.
 */

 #include "game/render/menu/MenuRenderParts.hpp"

 #include "game/render/ui/UIHelpers.hpp"
 
 #include <algorithm>
 #include <string>
 
 namespace game::render::menu {
 
 void drawLevelSelectScreen(const MenuCtx& m) {
     // -------------------------------------------------------------------------
     // (1) Painel custom do LEVEL SELECT (maior, para a grelha).
     // -------------------------------------------------------------------------
     float levelPanelW = 800.0f * m.uiS;
     float levelPanelH = 650.0f * m.uiS;  // Aumentado para 4x5
     float levelPanelX = ((float)m.ctx.fbW - levelPanelW) * 0.5f;
 
     // Y: usa ~40% da altura do ecrã como âncora e centra o painel à volta disso.
     float levelPanelY = ((float)m.ctx.fbH * 0.40f) - levelPanelH * 0.5f;
 
     // Sombra do painel (profundidade)
     float shadowOffset = 6.0f * m.uiS;
     m.ctx.renderer.drawUIQuad(levelPanelX + shadowOffset, levelPanelY - shadowOffset,
                              levelPanelW, levelPanelH,
                              glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
 
     // Corpo do painel (azul escuro)
     m.ctx.renderer.drawUIQuad(levelPanelX, levelPanelY,
                              levelPanelW, levelPanelH,
                              glm::vec4(0.08f, 0.08f, 0.14f, 0.98f));
 
     // -------------------------------------------------------------------------
     // (2) Borda RGB animada (mesmo look do resto do menu).
     // -------------------------------------------------------------------------
     float borderThickness = 3.0f * m.uiS;
     float tRgb = m.ctx.time.now();
     float hueRgb = std::fmod(0.56f + 0.08f * std::sin(tRgb * 1.2f), 1.0f);
     glm::vec3 neonRgb = ui::hsv2rgb(hueRgb, 0.85f, 1.0f);
     glm::vec4 borderColor(neonRgb, 1.0f);
 
     m.ctx.renderer.drawUIQuad(levelPanelX - borderThickness, levelPanelY - borderThickness,
                              levelPanelW + 2*borderThickness, borderThickness, borderColor);
     m.ctx.renderer.drawUIQuad(levelPanelX - borderThickness, levelPanelY + levelPanelH,
                              levelPanelW + 2*borderThickness, borderThickness, borderColor);
     m.ctx.renderer.drawUIQuad(levelPanelX - borderThickness, levelPanelY,
                              borderThickness, levelPanelH, borderColor);
     m.ctx.renderer.drawUIQuad(levelPanelX + levelPanelW, levelPanelY,
                              borderThickness, levelPanelH, borderColor);
 
     // -------------------------------------------------------------------------
     // (3) Parâmetros da grelha (20 níveis = 4 linhas x 5 colunas).
     // -------------------------------------------------------------------------
     const int totalLevels = 20;
     const int cols = 5;
     const int rows = 4;
 
     // Dimensões dos botões e gaps.
     float btnSize = 110.0f * m.uiS;
     float gapX = 25.0f * m.uiS;
     float gapY = 25.0f * m.uiS;
 
     // Largura total da grelha e X inicial centrado no painel.
     float gridW = cols * btnSize + (cols - 1) * gapX;
     float startX = levelPanelX + (levelPanelW - gridW) * 0.5f;
 
     // Y de referência da grelha: “80px” acima da base do painel (em UI scale).
     float startY = levelPanelY + 80.0f * m.uiS;
 
     // -------------------------------------------------------------------------
     // (4) Título do painel: “SELECT LEVEL”.
     // -------------------------------------------------------------------------
     {
         std::string title = "SELECT LEVEL";
         float titleScale = 2.0f * m.uiS;
 
         float titleW = m.ctx.renderer.measureUITextWidth(title, titleScale);
         float titleX = levelPanelX + (levelPanelW - titleW) * 0.5f;
         float titleY = levelPanelY + 25.0f * m.uiS;
 
         // Glow simples (4 offsets) + texto em ciano.
         glm::vec3 titleColor(0.20f, 0.75f, 0.85f);
 
         // “Glow” preto translúcido por trás (faz outline/shadow).
         m.ctx.renderer.drawUIText(titleX - 2.0f * m.uiS, titleY, title, titleScale, glm::vec4(0,0,0,0.6f));
         m.ctx.renderer.drawUIText(titleX + 2.0f * m.uiS, titleY, title, titleScale, glm::vec4(0,0,0,0.6f));
         m.ctx.renderer.drawUIText(titleX, titleY - 2.0f * m.uiS, title, titleScale, glm::vec4(0,0,0,0.6f));
         m.ctx.renderer.drawUIText(titleX, titleY + 2.0f * m.uiS, title, titleScale, glm::vec4(0,0,0,0.6f));
 
         // Texto principal.
         m.ctx.renderer.drawUIText(titleX, titleY, title, titleScale, glm::vec4(titleColor, 1.0f));
     }
 
     // -------------------------------------------------------------------------
     // (5) Botões de nível (20) em grelha:
     // - row/col calculados por i
     // - posicionamento: y inverte para a primeira linha ficar em cima
     // - visual: sombra + corpo + borda + número
     // -------------------------------------------------------------------------
     for (int i = 0; i < totalLevels; i++) {
         int row = i / cols;
         int col = i % cols;
 
         float x = startX + col * (btnSize + gapX);
 
         // (rows - 1 - row): inverte o eixo para row=0 estar no topo.
         float y = startY + (rows - 1 - row) * (btnSize + gapY);
 
         // Progressão/hover vindos do state.
         bool unlocked = (i < m.state.levelsBestLevel);
         bool hovered  = (m.state.hoveredLevelButton == i);
 
         // ---------------------------------------------------------------------
         // Sombra do botão (maior em hover).
         // ---------------------------------------------------------------------
         float shadow = (hovered ? 5.0f : 3.0f) * m.uiS;
         m.ctx.renderer.drawUIQuad(x + shadow, y - shadow,
                                  btnSize, btnSize,
                                  glm::vec4(0,0,0, hovered ? 0.6f : 0.5f));
 
         // ---------------------------------------------------------------------
         // Corpo do botão:
         //  - unlocked: azul/ciano (mais vivo em hover)
         //  - locked: cinza escuro
         // ---------------------------------------------------------------------
         glm::vec3 bgColor = unlocked
             ? (hovered ? glm::vec3(0.25f, 0.90f, 1.0f)   // hover unlocked
                        : glm::vec3(0.15f, 0.50f, 0.65f)) // unlocked normal
             : glm::vec3(0.12f, 0.12f, 0.15f);            // locked
 
         float alpha = unlocked ? 0.95f : 0.50f;
 
         m.ctx.renderer.drawUIQuad(x, y, btnSize, btnSize, glm::vec4(bgColor, alpha));
 
         // ---------------------------------------------------------------------
         // Borda do botão:
         //  - mais grossa em hover
         //  - unlocked usa ciano, locked usa cinza
         // ---------------------------------------------------------------------
         float borderThick = (hovered ? 3.5f : 2.5f) * m.uiS;
 
         glm::vec3 borderCol = unlocked
             ? glm::vec3(0.20f, 0.75f, 0.85f)
             : glm::vec3(0.25f, 0.25f, 0.30f);
 
         m.ctx.renderer.drawUIQuad(x - borderThick, y - borderThick, btnSize + 2*borderThick, borderThick, glm::vec4(borderCol, 1));
         m.ctx.renderer.drawUIQuad(x - borderThick, y + btnSize,     btnSize + 2*borderThick, borderThick, glm::vec4(borderCol, 1));
         m.ctx.renderer.drawUIQuad(x - borderThick, y,               borderThick, btnSize,     glm::vec4(borderCol, 1));
         m.ctx.renderer.drawUIQuad(x + btnSize,     y,               borderThick, btnSize,     glm::vec4(borderCol, 1));
 
         // ---------------------------------------------------------------------
         // Número do nível no centro.
         //  - unlocked: branco
         //  - locked: cinza claro com alpha
         // ---------------------------------------------------------------------
         std::string numStr = std::to_string(i + 1);
         float numScale = 2.5f * m.uiS;
 
         float numW = m.ctx.renderer.measureUITextWidth(numStr, numScale);
         float numH = m.ctx.renderer.getUIFontLineHeight(numScale);
 
         float numX = x + (btnSize - numW) * 0.5f;
         float numY = y + (btnSize - numH) * 0.5f;
 
         glm::vec4 numColor = unlocked
             ? glm::vec4(1,1,1,1)
             : glm::vec4(0.4f, 0.4f, 0.45f, 0.8f);
 
         m.ctx.renderer.drawUIText(numX, numY, numStr, numScale, numColor);
     }
 
     // -------------------------------------------------------------------------
     // (6) Botão BACK (canto inferior-esquerdo do painel).
     // Reusa o drawButton() do menu para manter consistência de estilo.
     // -------------------------------------------------------------------------
     float backW = 120.0f * m.uiS;
     float backH = 50.0f * m.uiS;
     float backX = levelPanelX + 20.0f * m.uiS;
     float backY = levelPanelY + 15.0f * m.uiS;
 
     // btnIndex=10: convém garantir que o input/hover do Level Select usa este índice.
     drawButton(m, 10, backX, backY, backW, backH, "< BACK", glm::vec3(0.5f, 0.5f, 0.5f), "");
 }
 
 } // namespace game::render::menu
 