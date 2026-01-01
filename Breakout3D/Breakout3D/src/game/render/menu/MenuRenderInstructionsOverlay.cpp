/**
 * @file MenuRenderInstructionsOverlay.cpp
 * @brief Render do overlay de instruções no menu: tabs (Controls/Powerups/Rogue Cards),
 *        inspector de powerups (modelo + info + “video”) e browser de cartas Rogue (3 colunas + inspector).
 *
 * @details
 *  Este overlay é desenhado por cima do menu e usa um layout dedicado (instructionsOverlayLayout).
 *  Existem 3 tabs:
 *   0) CONTROLS:
 *      - Painel sólido grande para máxima legibilidade.
 *      - Lista de controlos com “key badges” alinhados.
 *   1) POWERUPS:
 *      - Sem painel grande (conteúdo “flutua” no background) para vibe mais “showcase”.
 *      - Metade esquerda: preview 3D do powerup (scissor + depth test temporário).
 *      - Metade direita: widget com nome, descrição e “video” (gif frames).
 *   2) ROGUE CARDS:
 *      - Sem painel grande.
 *      - 3 colunas scrolláveis: POWERUPS / MODIFIERS / OP.
 *      - Clique abre inspector grande centrado com descrição.
 *
 *  Notas técnicas importantes:
 *   - A UI pass normalmente não usa depth test. Para desenhar um mesh 3D legível no overlay,
 *     activamos depth test só durante o draw e usamos scissor para cortar ao rect do preview.
 *   - O scroll nas listas é em pixels (scrollPx) e é clampado para não sair fora do conteúdo.
 *   - “Hovered” e “Selected” vêm do GameState (hoveredRogueCardsItem, rogueCardsSelected, etc.).
 */

 #include "game/render/menu/MenuRenderParts.hpp"

 #include "game/GameAssets.hpp"
 #include "game/rogue/RogueCards.hpp"
 #include "game/render/ui/UIHelpers.hpp"
 #include "game/ui/InstructionsOverlayLayout.hpp"
 
 #include <algorithm>
 #include <cmath>
 #include <string>
 #include <vector>
 
 #include <glm/gtc/constants.hpp>
 #include <glm/gtc/matrix_transform.hpp>
 
 namespace game::render::menu {
 
 /**
  * @brief Desenha o overlay de instruções (tabs + conteúdo + botão BACK).
  *
  * @param m MenuCtx com ctx/state/assets/layout.
  *
  * @details
  *  O layout do overlay (OL) é recalculado por tab/resolução para posicionar:
  *   - panel (quando necessário)
  *   - title
  *   - modelRect / infoRect / videoRect
  *   - navLeft/navRight
  *   - listas de cartas e back button
  */
 void drawInstructionsOverlay(const MenuCtx& m) {
     const auto OL = game::ui::instructionsOverlayLayout(m.L, m.ctx.fbW, m.ctx.fbH, m.state.instructionsTab);
     const float instrX = OL.panel.x;
     const float instrY = OL.panel.y;
     const float instrW = OL.panel.w;
     const float instrH = OL.panel.h;
 
     // -------------------------------------------------------------------------
     // (1) Fundo/Fill do overlay.
     // - CONTROLS: painel sólido para legibilidade.
     // - POWERUPS / ROGUE CARDS: sem painel grande (conteúdo “flutua” no fundo).
     // -------------------------------------------------------------------------
     if (m.state.instructionsTab == 0) {
         m.ctx.renderer.drawUIQuad(instrX, instrY, instrW, instrH, glm::vec4(0.05f, 0.05f, 0.1f, 0.98f));
     }
 
     // -------------------------------------------------------------------------
     // (2) Borda do overlay (só para CONTROLS, para manter look “painel”).
     // -------------------------------------------------------------------------
     float borderThickness = 3.0f;
     float tRgbInstr = m.ctx.time.now();
     float hueRgbInstr = std::fmod(0.56f + 0.08f * std::sin(tRgbInstr * 1.2f), 1.0f);
     glm::vec3 neonRgbInstr = ui::hsv2rgb(hueRgbInstr, 0.85f, 1.0f);
     glm::vec4 instrBorder(neonRgbInstr, 1.0f);
 
     if (m.state.instructionsTab == 0) {
         m.ctx.renderer.drawUIQuad(instrX - borderThickness, instrY - borderThickness, instrW + 2*borderThickness, borderThickness, instrBorder);
         m.ctx.renderer.drawUIQuad(instrX - borderThickness, instrY + instrH,        instrW + 2*borderThickness, borderThickness, instrBorder);
         m.ctx.renderer.drawUIQuad(instrX - borderThickness, instrY,                borderThickness, instrH,      instrBorder);
         m.ctx.renderer.drawUIQuad(instrX + instrW,          instrY,                borderThickness, instrH,      instrBorder);
     }
 
     // -------------------------------------------------------------------------
     // (3) Título do overlay (depende da tab).
     // Para POWERUPS/ROGUE CARDS, o título desce um pouco para não colidir com o header do menu.
     // -------------------------------------------------------------------------
     std::string instrTitle =
         (m.state.instructionsTab == 0) ? "CONTROLS" :
         (m.state.instructionsTab == 1) ? "POWERUPS" :
         "ROGUE CARDS";
 
     float instrTitleScale = 1.6f * m.uiS;
     float instrTitleW = m.ctx.renderer.measureUITextWidth(instrTitle, instrTitleScale);
     float instrTitleX = instrX + (instrW - instrTitleW) * 0.5f;
 
     float titleTopPad = (m.state.instructionsTab == 1 || m.state.instructionsTab == 2)
         ? (96.0f * m.uiS)
         : (42.0f * m.uiS);
 
     float instrTitleY = instrY + instrH - titleTopPad;
 
     // Pequeno backdrop só atrás do título (para ficar legível sem painel grande).
     if (m.state.instructionsTab == 1 || m.state.instructionsTab == 2) {
         float padX = 26.0f * m.uiS;
         float padY = 12.0f * m.uiS;
         float th = m.ctx.renderer.getUIFontLineHeight(instrTitleScale);
 
         m.ctx.renderer.drawUIQuad(
             instrTitleX - padX, instrTitleY - padY,
             instrTitleW + 2.0f * padX, th + 2.0f * padY,
             glm::vec4(0.05f, 0.05f, 0.1f, 0.40f)
         );
     }
 
     // Glow do título.
     glm::vec3 glowCol(0.10f, 0.35f, 0.90f);
     for (float o = 2.5f; o >= 1.0f; o -= 0.5f) {
         float a = 0.16f / o;
         m.ctx.renderer.drawUIText(instrTitleX - o, instrTitleY, instrTitle, instrTitleScale, glowCol * a);
         m.ctx.renderer.drawUIText(instrTitleX + o, instrTitleY, instrTitle, instrTitleScale, glowCol * a);
         m.ctx.renderer.drawUIText(instrTitleX, instrTitleY - o, instrTitle, instrTitleScale, glowCol * a);
         m.ctx.renderer.drawUIText(instrTitleX, instrTitleY + o, instrTitle, instrTitleScale, glowCol * a);
     }
 
     // Texto principal + underline neon.
     m.ctx.renderer.drawUIText(instrTitleX, instrTitleY, instrTitle, instrTitleScale, glm::vec3(0.2f, 0.85f, 1.0f));
 
     float barW = instrTitleW;
     float barH = 4.0f * m.uiS;
     float hueBar = std::fmod(0.56f + 0.08f * std::sin(m.ctx.time.now() * 1.2f), 1.0f);
     glm::vec3 neonBar = ui::hsv2rgb(hueBar, 0.85f, 1.0f);
     m.ctx.renderer.drawUIQuad(instrTitleX, instrTitleY - 6.0f * m.uiS, barW, barH, glm::vec4(neonBar, 1.0f));
 
     // -------------------------------------------------------------------------
     // (4) TAB: CONTROLS
     // -------------------------------------------------------------------------
     if (m.state.instructionsTab == 0) {
         // Lista simples (sem “cards” internos) com badges alinhados.
         float pad = 44.0f;
         float x = instrX + pad;
         float y = instrY + instrH - 132.0f;
 
         float lineGap = 68.0f;
         float badgeScale = 1.20f;
         float descScale = 1.10f;
 
         struct CtrlItem { std::string key; std::string desc; };
         std::vector<CtrlItem> controls = {
             {"A / D",   "Move paddle"},
             {"ARROWS",  "Move paddle"},
             {"SPACE",   "Launch ball (towards mouse)"},
             {"ESC",     "Pause / Resume"},
             {"1 / 2",   "Change camera"}
         };
 
         // Medir maior key para alinhar as descrições em coluna.
         float maxKeyW = 0.0f;
         for (const auto& it : controls) {
             maxKeyW = std::max(maxKeyW, m.ctx.renderer.measureUITextWidth(it.key, badgeScale));
         }
 
         float badgeW = maxKeyW + 38.0f;
         float descX = x + badgeW + 28.0f;
 
         for (const auto& it : controls) {
             // Badge desenha o “teclado” com estilo neon/retro.
             ui::drawKeyBadge(m.ctx.renderer, x, y - 18.0f, it.key, badgeScale, m.ctx.time.now());
 
             // Texto descritivo ao lado.
             m.ctx.renderer.drawUIText(descX, y, it.desc, descScale, glm::vec3(0.86f, 0.94f, 1.0f));
 
             y -= lineGap;
         }
     }
 
     // -------------------------------------------------------------------------
     // (5) TAB: POWERUPS (inspector: esquerda = modelo 3D, direita = info + gif)
     // -------------------------------------------------------------------------
     if (m.state.instructionsTab == 1) {
         struct Entry {
             const char* name;
             const char* desc;
             const engine::Mesh* mesh;
             glm::vec3 tint;
             float scale;
             int videoIdx; // index into GameAssets::powerupVideoFrames
         };
 
         // Lista de entries (ordem = ordem de navegação no inspector).
         std::vector<Entry> entries = {
             {"EXPAND",      "Makes the paddle wider for a short time.",              &m.assets.expand,    glm::vec3(0.35f, 1.0f, 0.35f), 1.05f, 0},
             {"EXTRA BALL",  "Spawns 3 additional balls.",                             &m.assets.extraBall, glm::vec3(0.35f, 0.85f, 1.0f),  1.00f, 1},
             {"EXTRA LIFE",  "Adds +1 life.",                                          &m.assets.extraLife, glm::vec3(1.0f, 0.25f, 0.25f),  1.05f, 2},
             {"FIREBALL",    "Turns the ball into a one-shot fireball with AoE.",     &m.assets.fireball,  glm::vec3(1.0f, 0.55f, 0.10f),  1.05f, 3},
             {"SLOW",        "Slows paddle movement for a short time.",               &m.assets.slow,      glm::vec3(0.75f, 0.55f, 0.20f), 1.05f, 4},
             {"SHIELD",      "A barrier behind the paddle saves balls temporarily.",  &m.assets.shield,    glm::vec3(0.25f, 1.0f, 1.0f),  1.10f, 5},
             {"REVERSE",     "Inverts left/right controls temporarily.",              &m.assets.skull,     glm::vec3(1.0f, 0.20f, 0.90f), 1.05f, 6},
             {"TINY",        "Shrinks the paddle for a short time.",                  &m.assets.minus,     glm::vec3(1.0f, 0.95f, 0.25f), 1.00f, 7},
         };
 
         // Clamp do índice actual (evita crash se for fora do range).
         int idx = m.state.powerupInspectIndex;
         if (entries.empty()) idx = 0;
         else {
             if (idx < 0) idx = 0;
             if (idx >= (int)entries.size()) idx = (int)entries.size() - 1;
         }
         const Entry& e = entries[idx];
 
         // ---------------------------------------------------------------------
         // Esquerda: viewport do modelo 3D.
         // ---------------------------------------------------------------------
         {
             const auto& R = OL.modelRect;
 
             // Ponto base do preview e escala (faz ocupar metade esquerda).
             float cx = R.x + R.w * 0.5f;
             float cy = R.y + R.h * 0.60f;
             float s = std::min(R.w, R.h) * 0.42f * e.scale;
 
             // Rotação: “spin” automático + input do utilizador (yaw/pitch).
             float spin = m.ctx.time.now() * 0.35f;
             float yaw = m.state.powerupInspectYaw + spin;
             float pitch = m.state.powerupInspectPitch;
 
             glm::mat4 M(1.0f);
             M = glm::translate(M, glm::vec3(cx, cy, 0.0f));
             M = glm::rotate(M, yaw, glm::vec3(0, 1, 0));
             M = glm::rotate(M, pitch, glm::vec3(1, 0, 0));
             M = glm::scale(M, glm::vec3(s, s, s));
 
             // Caso especial: “TINY” usa mesh minus, mas queremos mais “barra” (plank).
             if (std::string(e.name) == "TINY") {
                 M = glm::scale(M, glm::vec3(2.35f, 0.22f, 0.60f));
             }
 
             // UI pass normalmente não tem depth -> meshes podem parecer “inside-out”.
             // Solução: depth test on só durante este draw + scissor para recortar ao rect.
             m.ctx.renderer.uiSetScissor(true, R.x, R.y, R.w, R.h);
             m.ctx.renderer.uiSetDepthTest(true, true);
 
             m.ctx.renderer.drawMesh(*e.mesh, M, e.tint);
 
             m.ctx.renderer.uiSetDepthTest(false, false);
             m.ctx.renderer.uiSetScissor(false);
         }
 
         // ---------------------------------------------------------------------
         // Direita: widget com info (nome, desc, video e navegação).
         // ---------------------------------------------------------------------
         {
             const auto& R = OL.infoRect;
 
             // Fundo do widget (card readable).
             m.ctx.renderer.drawUIQuad(R.x, R.y, R.w, R.h, glm::vec4(0.06f, 0.06f, 0.10f, 0.92f));
 
             // Header: nome + linha decorativa com a tint do powerup.
             float x = R.x + 16.0f * m.uiS;
             float y = R.y + R.h - 58.0f * m.uiS;
             float titleScale = 1.55f * m.uiS;
 
             m.ctx.renderer.drawUIText(x, y, e.name, titleScale, glm::vec3(1.0f, 1.0f, 1.0f));
             m.ctx.renderer.drawUIQuad(
                 x, y - 10.0f * m.uiS,
                 std::min(R.w - 28.0f * m.uiS, 340.0f * m.uiS),
                 3.0f * m.uiS,
                 glm::vec4(e.tint, 1.0f)
             );
 
             // Descrição wrapped.
             float descScale = 1.10f * m.uiS;
             float descY = y - 62.0f * m.uiS;
             ui::drawWrappedText(
                 m.ctx.renderer,
                 x, descY,
                 R.w - 28.0f * m.uiS,
                 e.desc,
                 descScale,
                 glm::vec4(0.86f, 0.94f, 1.0f, 0.92f),
                 6.0f * m.uiS
             );
 
             // -----------------------------------------------------------------
             // “Video” preview: toca frames (gif) se existir; senão placeholder.
             // -----------------------------------------------------------------
             const auto& V = OL.videoRect;
 
             // Base do rect do vídeo.
             m.ctx.renderer.drawUIQuad(V.x, V.y, V.w, V.h, glm::vec4(0.02f, 0.02f, 0.04f, 0.90f));
 
             // Borda discreta
             float bt = 2.0f;
             m.ctx.renderer.drawUIQuad(V.x - bt, V.y - bt, V.w + 2*bt, bt, glm::vec4(1,1,1,0.12f));
             m.ctx.renderer.drawUIQuad(V.x - bt, V.y + V.h, V.w + 2*bt, bt, glm::vec4(1,1,1,0.12f));
             m.ctx.renderer.drawUIQuad(V.x - bt, V.y, bt, V.h, glm::vec4(1,1,1,0.12f));
             m.ctx.renderer.drawUIQuad(V.x + V.w, V.y, bt, V.h, glm::vec4(1,1,1,0.12f));
 
             // Procurar frames “pré-extraídos” (GameAssets::powerupVideo).
             const auto& gif = m.assets.powerupVideo(e.videoIdx);
 
             if (!gif.empty()) {
                 // Frame actual baseado no tempo (loop interno do gif).
                 const auto& fr = gif.frameAtTime(m.ctx.time.now());
                 m.ctx.renderer.drawUIQuad(V.x, V.y, V.w, V.h, glm::vec4(1,1,1,1), fr.id);
             } else {
                 // Placeholder: indica se falta path ou se está “a carregar”.
                 std::string ph = m.assets.powerupVideoPaths[e.videoIdx].empty()
                     ? "VIDEO (gif not set)"
                     : "VIDEO (loading...)";
 
                 float phS = 0.72f * m.uiS;
                 float phW = m.ctx.renderer.measureUITextWidth(ph, phS);
                 float phH = m.ctx.renderer.getUIFontLineHeight(phS);
                 m.ctx.renderer.drawUIText(
                     V.x + (V.w - phW) * 0.5f,
                     V.y + (V.h - phH) * 0.5f,
                     ph, phS,
                     glm::vec4(1,1,1,0.55f)
                 );
 
                 // Hint para a pasta esperada.
                 std::string hint = "assets/video/*.gif";
                 float hs = 0.55f * m.uiS;
                 float hw = m.ctx.renderer.measureUITextWidth(hint, hs);
                 m.ctx.renderer.drawUIText(
                     V.x + (V.w - hw) * 0.5f,
                     V.y + 16.0f * m.uiS,
                     hint, hs,
                     glm::vec4(1,1,1,0.45f)
                 );
             }
 
             // -----------------------------------------------------------------
             // Borda RGB animada no widget direito (pedido).
             // -----------------------------------------------------------------
             {
                 float bt2 = 3.0f * m.uiS;
                 float hue2 = std::fmod(0.56f + 0.08f * std::sin(m.ctx.time.now() * 1.2f), 1.0f);
                 glm::vec3 neon2 = ui::hsv2rgb(hue2, 0.85f, 1.0f);
                 glm::vec4 b2(neon2, 0.95f);
 
                 m.ctx.renderer.drawUIQuad(R.x - bt2, R.y - bt2, R.w + 2*bt2, bt2, b2);
                 m.ctx.renderer.drawUIQuad(R.x - bt2, R.y + R.h, R.w + 2*bt2, bt2, b2);
                 m.ctx.renderer.drawUIQuad(R.x - bt2, R.y, bt2, R.h, b2);
                 m.ctx.renderer.drawUIQuad(R.x + R.w, R.y, bt2, R.h, b2);
             }
 
             // -----------------------------------------------------------------
             // Setas de navegação (prev/next).
             // -----------------------------------------------------------------
             auto drawNav = [&](const game::ui::Rect& B, const std::string& label, bool hovered) {
                 glm::vec3 base(0.10f, 0.10f, 0.16f);
                 glm::vec3 col = base * (hovered ? 1.25f : 1.0f);
                 float sh = hovered ? 6.0f : 3.0f;
 
                 m.ctx.renderer.drawUIQuad(B.x + sh, B.y - sh, B.w, B.h, glm::vec4(0,0,0, hovered ? 0.55f : 0.45f));
                 m.ctx.renderer.drawUIQuad(B.x, B.y, B.w, B.h, glm::vec4(col, 0.95f));
 
                 float tS = 1.15f * m.uiS;
                 float tw = m.ctx.renderer.measureUITextWidth(label, tS);
                 float th = m.ctx.renderer.getUIFontLineHeight(tS);
                 m.ctx.renderer.drawUIText(B.x + (B.w - tw) * 0.5f, B.y + (B.h - th) * 0.5f, label, tS, glm::vec4(1,1,1,1));
             };
 
             drawNav(OL.navLeft,  "<", m.state.hoveredPowerupNav == 0);
             drawNav(OL.navRight, ">", m.state.hoveredPowerupNav == 1);
 
             // Indicador “i / total”.
             std::string ind = std::to_string(idx + 1) + " / " + std::to_string((int)entries.size());
             float indS = 0.70f * m.uiS;
             float indW = m.ctx.renderer.measureUITextWidth(ind, indS);
             m.ctx.renderer.drawUIText(
                 R.x + (R.w - indW) * 0.5f,
                 OL.navLeft.y + 16.0f * m.uiS,
                 ind, indS,
                 glm::vec4(1,1,1,0.70f)
             );
         }
     }
 
     // -------------------------------------------------------------------------
     // (6) TAB: ROGUE CARDS (3 colunas scrolláveis + inspector grande)
     // -------------------------------------------------------------------------
     if (m.state.instructionsTab == 2) {
         // Agrupar ids por categoria (para 3 colunas).
         std::vector<game::rogue::RogueCardId> powerups;
         std::vector<game::rogue::RogueCardId> modifiers;
         std::vector<game::rogue::RogueCardId> ops;
 
         for (auto id : game::rogue::allCardIds()) {
             const auto& def = game::rogue::cardDef(id);
             if (def.isOp) ops.push_back(id);
             else if (game::rogue::isPowerupCard(id)) powerups.push_back(id);
             else modifiers.push_back(id);
         }
 
         // Região total de conteúdo (união de modelRect + infoRect).
         float contentX = OL.modelRect.x;
         float contentY = OL.modelRect.y;
         float contentW = (OL.infoRect.x + OL.infoRect.w) - contentX;
         float contentH = OL.modelRect.h;
 
         // Layout de 3 colunas.
         float gap = 22.0f * m.uiS;
         float colW = (contentW - 2.0f * gap) / 3.0f;
         float colH = contentH;
 
         float colX0 = contentX;
         float colX1 = contentX + colW + gap;
         float colX2 = contentX + (colW + gap) * 2.0f;
 
         // Cabeçalho de cada coluna.
         auto drawColHeader = [&](float x, const char* title, glm::vec3 tint) {
             float s = 1.05f * m.uiS;
             float tw = m.ctx.renderer.measureUITextWidth(title, s);
             m.ctx.renderer.drawUIText(x + (colW - tw) * 0.5f, contentY + colH - 34.0f * m.uiS, title, s, tint);
         };
 
         drawColHeader(colX0, "POWERUPS",   glm::vec3(0.35f, 0.85f, 1.0f));
         drawColHeader(colX1, "MODIFIERS",  glm::vec3(0.85f, 0.35f, 1.0f));
         drawColHeader(colX2, "OP",         glm::vec3(1.0f, 0.80f, 0.15f));
 
         // Base indices: mapeiam cada item (por hover) para um índice global (1D).
         int base0 = 0;
         int base1 = base0 + (int)powerups.size();
         int base2 = base1 + (int)modifiers.size();
 
         /**
          * @brief Desenha uma lista scrollável dentro de uma coluna.
          *
          * @param x        X da coluna.
          * @param scrollPx Scroll actual (em px) para esta coluna.
          * @param ids      Lista de cards (IDs) a desenhar.
          * @param baseIndex Offset para index global (hoveredRogueCardsItem).
          *
          * @details
          *  - Calcula viewTop/viewBottom e clampa scroll para não “passar” do fim.
          *  - Activa scissor para cortar items fora do viewport.
          *  - Cada item:
          *     * background (bg)
          *     * border neon (RGB se hovered/selected, senão “accent” brilhante)
          *     * nome com fitScaleToWidth
          */
         auto drawListScrollable = [&](float x, float scrollPx,
                                      const std::vector<game::rogue::RogueCardId>& ids, int baseIndex) {
             float padX = 10.0f * m.uiS;
             float itemH = 54.0f * m.uiS;
             float itemGap = 10.0f * m.uiS;
             float step = itemH + itemGap;
 
             // Viewport vertical da coluna.
             float viewTopY = contentY + colH - 80.0f * m.uiS;
             float viewBottomY = contentY + 30.0f * m.uiS;
             float viewH = std::max(1.0f, viewTopY - viewBottomY);
 
             // Altura total do conteúdo (N items com gaps).
             float totalH = std::max(0.0f, (float)ids.size() * step - itemGap);
 
             // Max scroll = totalH - viewH (se conteúdo maior que viewport).
             float maxScroll = std::max(0.0f, totalH - viewH);
 
             // Clamp do scroll.
             if (scrollPx < 0.0f) scrollPx = 0.0f;
             if (scrollPx > maxScroll) scrollPx = maxScroll;
 
             // Scissor para recortar ao viewport.
             float bt = 3.0f * m.uiS;
             m.ctx.renderer.uiSetScissor(true, x - bt, viewBottomY - bt, colW + 2.0f * bt, viewH + 2.0f * bt);
 
             // y começa no topo (viewTopY) e move-se para baixo item a item.
             // scrollPx adiciona “offset” (itens sobem quando scroll aumenta).
             float y = viewTopY + scrollPx;
 
             for (size_t i = 0; i < ids.size(); ++i) {
                 // Skip rápido se ainda estamos muito acima do viewport.
                 if ((y - itemH) > (viewTopY + itemH)) { y -= step; continue; }
                 if (y < viewBottomY) break;
 
                 int idx = baseIndex + (int)i;
 
                 bool hovered = (m.state.hoveredRogueCardsItem == idx) && !m.state.rogueCardsInspectOpen;
                 bool selected = (m.state.rogueCardsSelected == ids[i]);
 
                 glm::vec3 acc = game::rogue::cardAccent(ids[i]);
 
                 // “Neon accent” = accent mais brilhante (clamp para não saturar em excesso).
                 glm::vec3 neonAcc = glm::clamp(
                     acc * 1.55f + glm::vec3(0.08f),
                     glm::vec3(0.0f),
                     glm::vec3(1.0f)
                 );
 
                 // Background do item (mais opaco em hover).
                 glm::vec4 bg(0.06f, 0.06f, 0.10f, hovered ? 0.96f : 0.88f);
                 m.ctx.renderer.drawUIQuad(x, y - itemH, colW, itemH, bg);
 
                 // Borda: hovered/selected usa neon RGB global, senão usa neonAcc.
                 float hue = std::fmod(0.56f + 0.08f * std::sin(m.ctx.time.now() * 1.2f), 1.0f);
                 glm::vec3 neon = ui::hsv2rgb(hue, 0.85f, 1.0f);
                 glm::vec4 border = (hovered || selected)
                     ? glm::vec4(neon, 1.0f)
                     : glm::vec4(neonAcc, 0.92f);
 
                 m.ctx.renderer.drawUIQuad(x - bt, y - itemH - bt, colW + 2 * bt, bt, border);
                 m.ctx.renderer.drawUIQuad(x - bt, y,              colW + 2 * bt, bt, border);
                 m.ctx.renderer.drawUIQuad(x - bt, y - itemH,      bt, itemH,      border);
                 m.ctx.renderer.drawUIQuad(x + colW, y - itemH,    bt, itemH,      border);
 
                 // Nome: escala ajustada para caber (sem cortar).
                 const auto& def = game::rogue::cardDef(ids[i]);
                 std::string nm = def.name;
 
                 float nameScale = ui::fitScaleToWidth(m.ctx.renderer, nm, 0.90f * m.uiS, colW - 2.0f * padX);
                 float th = m.ctx.renderer.getUIFontLineHeight(nameScale);
 
                 m.ctx.renderer.drawUIText(
                     x + padX,
                     (y - itemH) + (itemH - th) * 0.5f,
                     nm, nameScale,
                     glm::vec3(1.0f)
                 );
 
                 y -= step;
             }
 
             // Desliga scissor no fim (importante para não recortar o resto do menu).
             m.ctx.renderer.uiSetScissor(false);
         };
 
         // Desenhar as 3 listas com scroll independente.
         drawListScrollable(colX0, m.state.rogueCardsScrollPowerups, powerups, base0);
         drawListScrollable(colX1, m.state.rogueCardsScrollModifiers, modifiers, base1);
         drawListScrollable(colX2, m.state.rogueCardsScrollOp, ops, base2);
 
         // Inspector grande (modal) por cima, se aberto.
         if (m.state.rogueCardsInspectOpen) {
             // Dim fullscreen.
             m.ctx.renderer.drawUIQuad(0, 0, (float)m.ctx.fbW, (float)m.ctx.fbH, glm::vec4(0, 0, 0, 0.82f));
 
             // Tamanho base e escala adaptativa ao ecrã.
             float cardWBase = 420.0f;
             float cardHBase = 900.0f;
 
             float s = std::min((float)m.ctx.fbW / (cardWBase + 120.0f), (float)m.ctx.fbH / (cardHBase + 160.0f));
             s = std::max(0.65f, std::min(1.45f, s));
 
             float cardW = cardWBase * s;
             float cardH = cardHBase * s;
 
             float cx = ((float)m.ctx.fbW - cardW) * 0.5f;
             float cy = ((float)m.ctx.fbH - cardH) * 0.5f;
 
             game::ui::Rect r{cx, cy, cardW, cardH};
 
             // Card seleccionado (id vem do state).
             auto id = m.state.rogueCardsSelected;
             const auto& def = game::rogue::cardDef(id);
             glm::vec3 acc = game::rogue::cardAccent(id);
 
             // Sombra + corpo.
             m.ctx.renderer.drawUIQuad(r.x + 14.0f, r.y - 14.0f, r.w, r.h, glm::vec4(0, 0, 0, 0.66f));
             m.ctx.renderer.drawUIQuad(r.x, r.y, r.w, r.h, glm::vec4(0.08f, 0.08f, 0.14f, 0.98f));
 
             // Borda com accent sólido (não RGB, para combinar com a identidade do card).
             float bt = 5.0f;
             m.ctx.renderer.drawUIQuad(r.x - bt, r.y - bt, r.w + 2 * bt, bt, glm::vec4(acc, 1.0f));
             m.ctx.renderer.drawUIQuad(r.x - bt, r.y + r.h, r.w + 2 * bt, bt, glm::vec4(acc, 1.0f));
             m.ctx.renderer.drawUIQuad(r.x - bt, r.y, bt, r.h, glm::vec4(acc, 1.0f));
             m.ctx.renderer.drawUIQuad(r.x + r.w, r.y, bt, r.h, glm::vec4(acc, 1.0f));
 
             // Nome (centrado no topo do card).
             std::string nm = def.name;
             float nameScale = ui::fitScaleToWidth(m.ctx.renderer, nm, 1.85f, r.w - 70.0f);
             float nw = m.ctx.renderer.measureUITextWidth(nm, nameScale);
             float nh = m.ctx.renderer.getUIFontLineHeight(nameScale);
             float nx = r.x + (r.w - nw) * 0.5f;
             float ny = r.y + r.h - nh - 34.0f;
 
             m.ctx.renderer.drawUIText(nx, ny, nm, nameScale, glm::vec3(1, 1, 1));
 
             // Descrição wrapped (coluna central).
             std::string desc = def.shortDesc;
             float pad = 40.0f;
             float dScale = 1.25f;
 
             ui::drawWrappedText(
                 m.ctx.renderer,
                 r.x + pad,
                 ny - 150.0f,
                 r.w - 2.0f * pad,
                 desc,
                 dScale,
                 glm::vec4(0.93f, 0.97f, 1.0f, 0.95f),
                 12.0f
             );
 
             // Hint de fecho.
             std::string hint = "CLICK OUTSIDE TO CLOSE";
             float hS = 0.62f;
             float hw = m.ctx.renderer.measureUITextWidth(hint, hS);
             m.ctx.renderer.drawUIText(r.x + (r.w - hw) * 0.5f, r.y + 26.0f, hint, hS, glm::vec3(0.8f, 0.9f, 1.0f));
         }
     }
 
     // -------------------------------------------------------------------------
     // (7) Botão BACK (bottom-left) — substitui o antigo X.
     // -------------------------------------------------------------------------
     {
         bool backHover = m.state.hoveredCloseButton;
 
         float uiS = m.L.uiScale;
         float bx = OL.backBtn.x, by = OL.backBtn.y, bw = OL.backBtn.w, bh = OL.backBtn.h;
 
         // Sombra em hover.
         float sh = (backHover ? 6.0f : 3.0f) * uiS;
         m.ctx.renderer.drawUIQuad(bx + sh, by - sh, bw, bh, glm::vec4(0,0,0, backHover ? 0.55f : 0.45f));
 
         // Corpo do botão.
         glm::vec3 base(0.10f, 0.10f, 0.16f);
         glm::vec3 col = base * (backHover ? 1.25f : 1.0f);
         m.ctx.renderer.drawUIQuad(bx, by, bw, bh, glm::vec4(col, 0.95f));
 
         // Borda neon subtil (mesma vibração do menu).
         float bt = 2.5f * uiS;
         float hue = std::fmod(0.56f + 0.08f * std::sin(m.ctx.time.now() * 1.2f), 1.0f);
         glm::vec3 neon = ui::hsv2rgb(hue, 0.85f, 1.0f);
         glm::vec4 bcol(neon, backHover ? 1.0f : 0.75f);
 
         m.ctx.renderer.drawUIQuad(bx - bt, by - bt, bw + 2*bt, bt, bcol);
         m.ctx.renderer.drawUIQuad(bx - bt, by + bh, bw + 2*bt, bt, bcol);
         m.ctx.renderer.drawUIQuad(bx - bt, by, bt, bh, bcol);
         m.ctx.renderer.drawUIQuad(bx + bw, by, bt, bh, bcol);
 
         // Texto "< BACK" centrado.
         std::string back = "< BACK";
         float bs = (bh / 56.0f) * (0.78f * uiS);
         float tw = m.ctx.renderer.measureUITextWidth(back, bs);
         float th = m.ctx.renderer.getUIFontLineHeight(bs);
 
         float tx = bx + (bw - tw) * 0.5f - 6.0f * uiS;
         float ty = by + (bh - th) * 0.5f;
 
         // Sombra pequena no texto.
         m.ctx.renderer.drawUIText(tx + 1.0f * uiS, ty - 1.0f * uiS, back, bs, glm::vec4(0,0,0,0.55f));
         m.ctx.renderer.drawUIText(tx, ty, back, bs, glm::vec4(1,1,1,1));
     }
 }
 
 } // namespace game::render::menu
 