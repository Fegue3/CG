/**
 * @file MenuRenderPlayModes.cpp
 * @brief Render do ecrã PLAY_MODES: 4 “cards” (NORMAL/ENDLESS/ROGUE/LEVELS) com hover scaling,
 *        título, descrição, features (bullets) e botão PLAY/SOON.
 *
 * @details
 *  Este ecrã é “card-based” (tipo seleção de modos) e tem 2 requisitos importantes:
 *   1) Hover deve escalar o *card real* (não usar backplates invisíveis).
 *   2) O card hovered deve ser desenhado por último para poder sobrepor o botão BACK do rodapé.
 *
 *  Organização:
 *   - drawModeCard(): lambda que desenha um card específico, dada a estrutura do layout e conteúdo.
 *   - Primeiro desenha todos os cards que NÃO estão em hover.
 *   - Depois desenha o botão BACK.
 *   - No fim desenha o card hovered (se existir).
 *
 *  Dependências do GameState:
 *   - state.hoveredMenuButton:
 *       usado para saber qual card está em hover (0..3) e se o BACK está em hover (4).
 *   - state.hoveredPlayModeButton:
 *       hover específico no botão PLAY dentro do card (para a borda RGB no botão).
 */

 #include "game/render/menu/MenuRenderParts.hpp"

 #include "game/render/ui/UIHelpers.hpp"
 
 #include <algorithm>
 #include <string>
 #include <vector>
 
 namespace game::render::menu {
 
 void drawPlayModesScreen(const MenuCtx& m) {
     /**
      * @brief Desenha um card de modo de jogo (NORMAL/ENDLESS/ROGUE/LEVELS).
      *
      * @param idx        índice do card (0..3) — usado para hoveredPlayModeButton.
      * @param c          layout do card (rect do card + rect do botão play).
      * @param modeTitle  título do modo (“NORMAL”, “ENDLESS”, etc.).
      * @param desc       descrição curta.
      * @param features   lista de bullets.
      * @param accent     cor de “identidade” do card (borda + highlights).
      * @param enabled    se false, mostra “SOON” e cores mais neutras.
      * @param hovered    se true, aplica scaling, sombras maiores e borda RGB.
      */
     auto drawModeCard = [&](int idx,
                             const game::ui::MenuLayout::ModeCard& c,
                             const std::string& modeTitle,
                             const std::string& desc,
                             const std::vector<std::string>& features,
                             const glm::vec3& accent,
                             bool enabled,
                             bool hovered) {
 
         // ---------------------------------------------------------------------
         // (1) Hover scaling (aplica ao rect do card, centrado).
         // ---------------------------------------------------------------------
         float pop = hovered ? 1.10f : 1.0f;
 
         float cx = c.card.x + c.card.w * 0.5f;
         float cy = c.card.y + c.card.h * 0.5f;
 
         float w = c.card.w * pop;
         float h = c.card.h * pop;
 
         float cardX = cx - w * 0.5f;
         float cardY = cy - h * 0.5f;
 
         // ---------------------------------------------------------------------
         // (2) Cores de borda:
         // - hovered: RGB neon animado
         // - normal: accent sólido
         // ---------------------------------------------------------------------
         float hueRgb = std::fmod(0.56f + 0.08f * std::sin(m.ctx.time.now() * 1.2f), 1.0f);
         glm::vec3 neonRgb = ui::hsv2rgb(hueRgb, 0.85f, 1.0f);
         glm::vec4 borderCol = hovered ? glm::vec4(neonRgb, 1.0f) : glm::vec4(accent, 1.0f);
 
         // ---------------------------------------------------------------------
         // (3) Sombra do card (muito maior em hover).
         // ---------------------------------------------------------------------
         float sh = (hovered ? 20.0f : 8.0f) * m.uiS;
         m.ctx.renderer.drawUIQuad(cardX + sh, cardY - sh, w, h,
                                  glm::vec4(0, 0, 0, hovered ? 0.66f : 0.55f));
 
         // Corpo do card.
         glm::vec4 body(0.08f, 0.08f, 0.14f, 0.98f);
         m.ctx.renderer.drawUIQuad(cardX, cardY, w, h, body);
 
         // ---------------------------------------------------------------------
         // (4) Outline única (sem camadas extra).
         // ---------------------------------------------------------------------
         float bt = (hovered ? 6.0f : 3.0f) * m.uiS;
         m.ctx.renderer.drawUIQuad(cardX - bt, cardY - bt, w + 2*bt, bt, borderCol);
         m.ctx.renderer.drawUIQuad(cardX - bt, cardY + h,  w + 2*bt, bt, borderCol);
         m.ctx.renderer.drawUIQuad(cardX - bt, cardY,      bt, h,    borderCol);
         m.ctx.renderer.drawUIQuad(cardX + w,  cardY,      bt, h,    borderCol);
 
         // ---------------------------------------------------------------------
         // (5) Título do card (centrado no topo).
         // ---------------------------------------------------------------------
         float tScale = 1.75f * m.uiS;
         float tW = m.ctx.renderer.measureUITextWidth(modeTitle, tScale);
         float tH = m.ctx.renderer.getUIFontLineHeight(tScale);
 
         float tX = cardX + (w - tW) * 0.5f;
         float tY = cardY + h - tH - 22.0f * m.uiS;
 
         // Sombra e texto principal.
         m.ctx.renderer.drawUIText(tX + 1.0f * m.uiS, tY - 1.0f * m.uiS,
                                  modeTitle, tScale, glm::vec4(0,0,0,0.65f));
         m.ctx.renderer.drawUIText(tX, tY, modeTitle, tScale, glm::vec4(1,1,1,1));
 
         // ---------------------------------------------------------------------
         // (6) Descrição (wrapped) com “big gap” do título (pedido).
         // ---------------------------------------------------------------------
         float padX = 34.0f * m.uiS;
         float maxW = w - padX * 2.0f;
 
         float dScale = 0.86f * m.uiS;
         glm::vec4 dCol = enabled
             ? glm::vec4(0.82f, 0.92f, 1.0f, 0.90f)
             : glm::vec4(0.80f, 0.80f, 0.84f, 0.85f);
 
         float dTop = tY - 120.0f * m.uiS;
         ui::drawWrappedText(m.ctx.renderer, cardX + padX, dTop, maxW, desc, dScale, dCol, 7.0f * m.uiS);
 
         // Para posicionar FEATURES depois da descrição, calcula quantas linhas a descrição ocupa.
         float lhDesc = m.ctx.renderer.getUIFontLineHeight(dScale);
         auto dLines = ui::wrapText(m.ctx.renderer, desc, dScale, maxW);
 
         float yCursor = dTop - (float)dLines.size() * (lhDesc + 7.0f * m.uiS) - 18.0f * m.uiS;
 
         // ---------------------------------------------------------------------
         // (7) Bloco “FEATURES” (título pequeno + bullets).
         // ---------------------------------------------------------------------
         {
             std::string hdr = "FEATURES";
             float hs = 0.62f * m.uiS;
             glm::vec4 hcol(accent, enabled ? 0.95f : 0.70f);
 
             m.ctx.renderer.drawUIText(cardX + padX, yCursor, hdr, hs, hcol);
 
             float hh = m.ctx.renderer.getUIFontLineHeight(hs);
             yCursor -= hh + 10.0f * m.uiS;
         }
 
         float fScale = 0.80f * m.uiS;
         float lh = m.ctx.renderer.getUIFontLineHeight(fScale);
         float lineGap = 6.0f * m.uiS;
 
         glm::vec4 fCol = enabled
             ? glm::vec4(0.92f, 0.96f, 1.0f, 0.88f)
             : glm::vec4(0.86f, 0.86f, 0.90f, 0.80f);
 
         glm::vec4 dotCol(accent, enabled ? 0.95f : 0.70f);
 
         float dot = 6.0f * m.uiS;
         float bulletGap = 18.0f * m.uiS;
 
         // “minY”: impede bullets de invadirem o botão PLAY.
         const float minY = c.playBtn.y + c.playBtn.h + 22.0f * m.uiS;
 
         for (const auto& ft : features) {
             if (yCursor < minY) break;
 
             // Bullet (quadrado)
             m.ctx.renderer.drawUIQuad(cardX + padX, yCursor + lh * 0.35f, dot, dot, dotCol);
 
             float textX = cardX + padX + dot + bulletGap;
             float textW = maxW - (dot + bulletGap);
 
             ui::drawWrappedText(m.ctx.renderer, textX, yCursor, textW, ft, fScale, fCol, lineGap);
 
             auto fl = ui::wrapText(m.ctx.renderer, ft, fScale, textW);
             yCursor -= (float)fl.size() * (lh + lineGap) + 12.0f * m.uiS;
         }
 
         // ---------------------------------------------------------------------
         // (8) Botão PLAY/SOON (no layout do card, não escala com pop).
         // ---------------------------------------------------------------------
         glm::vec3 base = enabled ? accent : glm::vec3(0.38f, 0.38f, 0.40f);
         glm::vec3 btnCol = base * (hovered ? 1.25f : 1.0f);
 
         float btnShadow = hovered ? 5.0f * m.uiS : 3.0f * m.uiS;
         m.ctx.renderer.drawUIQuad(c.playBtn.x + btnShadow, c.playBtn.y - btnShadow,
                                  c.playBtn.w, c.playBtn.h,
                                  glm::vec4(0,0,0, hovered ? 0.55f : 0.45f));
         m.ctx.renderer.drawUIQuad(c.playBtn.x, c.playBtn.y, c.playBtn.w, c.playBtn.h, glm::vec4(btnCol, 1.0f));
 
         // Borda RGB apenas quando o hover é em cima do botão PLAY (hoveredPlayModeButton).
         bool playHovered = (m.state.hoveredPlayModeButton == idx);
         if (playHovered) {
             float bt2 = 3.0f * m.uiS;
             float hueRgb2 = std::fmod(0.56f + 0.08f * std::sin(m.ctx.time.now() * 1.2f), 1.0f);
             glm::vec3 neonRgb2 = ui::hsv2rgb(hueRgb2, 0.85f, 1.0f);
             glm::vec4 borderCol2(neonRgb2, 1.0f);
 
             m.ctx.renderer.drawUIQuad(c.playBtn.x - bt2, c.playBtn.y - bt2, c.playBtn.w + 2 * bt2, bt2, borderCol2);
             m.ctx.renderer.drawUIQuad(c.playBtn.x - bt2, c.playBtn.y + c.playBtn.h, c.playBtn.w + 2 * bt2, bt2, borderCol2);
             m.ctx.renderer.drawUIQuad(c.playBtn.x - bt2, c.playBtn.y, bt2, c.playBtn.h, borderCol2);
             m.ctx.renderer.drawUIQuad(c.playBtn.x + c.playBtn.w, c.playBtn.y, bt2, c.playBtn.h, borderCol2);
         }
 
         // Texto do botão: PLAY (enabled) ou SOON (disabled)
         std::string lbl = enabled ? "PLAY" : "SOON";
         float pScale = 1.28f * m.uiS;
 
         float pW = m.ctx.renderer.measureUITextWidth(lbl, pScale);
         float pH = m.ctx.renderer.getUIFontLineHeight(pScale);
 
         float pX = c.playBtn.x + (c.playBtn.w - pW) * 0.5f;
         float pY = c.playBtn.y + (c.playBtn.h - pH) * 0.5f;
 
         m.ctx.renderer.drawUIText(pX + 1.0f * m.uiS, pY - 1.0f * m.uiS, lbl, pScale, glm::vec4(0,0,0,0.55f));
         m.ctx.renderer.drawUIText(pX, pY, lbl, pScale, glm::vec4(1,1,1,1));
     };
 
     // -------------------------------------------------------------------------
     // (A) Descobrir qual card está em hover (0..3). Caso contrário, -1.
     // -------------------------------------------------------------------------
     const int hoveredIdx =
         (m.state.hoveredMenuButton >= 0 && m.state.hoveredMenuButton <= 3)
             ? m.state.hoveredMenuButton
             : -1;
 
     // -------------------------------------------------------------------------
     // (B) Desenhar cards não-hovered primeiro (ordem fixa).
     // -------------------------------------------------------------------------
     if (hoveredIdx != 0) drawModeCard(0, m.L.normal, "NORMAL",
         "A classic run: clear the wall, keep your lives, and learn the feel of the paddle + ball.",
         {"Win by destroying every brick.", "Balanced power-ups and curses.", "Great for quick matches and learning."},
         glm::vec3(0.25f, 0.85f, 0.25f), true, false);
 
     if (hoveredIdx != 1) drawModeCard(1, m.L.endless, "ENDLESS",
         "No finish line. Survive the pressure as new rows spawn and your score keeps climbing.",
         {"Infinite waves with rising intensity.", "Score streak banking (risk vs. safety).", "Perfect for high-score chasing."},
         glm::vec3(0.95f, 0.55f, 0.15f), true, false);
 
     if (hoveredIdx != 2) drawModeCard(2, m.L.rogue, "ROGUE",
         "Build a run by choosing upgrades and taking risks — every decision shapes the next round.",
         {"Draft reward cards after wave milestones.", "No random drops — choices define your run.", "Wave-based progression (prototype)."},
         glm::vec3(0.75f, 0.25f, 0.85f), true, false);
 
     if (hoveredIdx != 3) drawModeCard(3, m.L.levels, "LEVELS",
         "A curated set of 20 stages with unique brick layouts.",
         {"Hand-crafted brick patterns.", "20 progressive levels.", "Win by completing all levels."},
         glm::vec3(0.20f, 0.75f, 0.85f), true, false);
 
     // -------------------------------------------------------------------------
     // (C) BACK button (rodapé): desenha antes do hovered card.
     // Hover detectado por hoveredMenuButton == 4.
     // -------------------------------------------------------------------------
     bool backHover = (m.state.hoveredMenuButton == 4);
     float bx = m.L.backBtn.x, by = m.L.backBtn.y, bw = m.L.backBtn.w, bh = m.L.backBtn.h;
 
     float sh = (backHover ? 6.0f : 3.0f) * m.uiS;
     m.ctx.renderer.drawUIQuad(bx + sh, by - sh, bw, bh, glm::vec4(0,0,0, backHover ? 0.55f : 0.45f));
 
     glm::vec3 base(0.10f, 0.10f, 0.16f);
     glm::vec3 col = base * (backHover ? 1.25f : 1.0f);
     m.ctx.renderer.drawUIQuad(bx, by, bw, bh, glm::vec4(col, 0.95f));
 
     // Borda neon subtil.
     float bt = 2.5f * m.uiS;
     float hue = std::fmod(0.56f + 0.08f * std::sin(m.ctx.time.now() * 1.2f), 1.0f);
     glm::vec3 neon = ui::hsv2rgb(hue, 0.85f, 1.0f);
     glm::vec4 bcol(neon, backHover ? 1.0f : 0.75f);
 
     m.ctx.renderer.drawUIQuad(bx - bt, by - bt, bw + 2*bt, bt, bcol);
     m.ctx.renderer.drawUIQuad(bx - bt, by + bh, bw + 2*bt, bt, bcol);
     m.ctx.renderer.drawUIQuad(bx - bt, by, bt, bh, bcol);
     m.ctx.renderer.drawUIQuad(bx + bw, by, bt, bh, bcol);
 
     // Texto do BACK (ligeiramente puxado para a esquerda).
     std::string back = "< BACK";
     float bs = (bh / 56.0f) * (0.78f * m.uiS);
     float tw = m.ctx.renderer.measureUITextWidth(back, bs);
     float th = m.ctx.renderer.getUIFontLineHeight(bs);
     float tx = bx + (bw - tw) * 0.5f - 6.0f * m.uiS;
     float ty = by + (bh - th) * 0.5f;
 
     m.ctx.renderer.drawUIText(tx + 1.0f * m.uiS, ty - 1.0f * m.uiS, back, bs, glm::vec4(0,0,0,0.55f));
     m.ctx.renderer.drawUIText(tx, ty, back, bs, glm::vec4(1,1,1,1));
 
     // -------------------------------------------------------------------------
     // (D) Desenhar o card hovered por último (para sobrepor o footer).
     // -------------------------------------------------------------------------
     if (hoveredIdx == 0) drawModeCard(0, m.L.normal, "NORMAL",
         "A classic run: clear the wall, keep your lives, and learn the feel of the paddle + ball.",
         {"Win by destroying every brick.", "Balanced power-ups and curses.", "Great for quick matches and learning."},
         glm::vec3(0.25f, 0.85f, 0.25f), true, true);
 
     if (hoveredIdx == 1) drawModeCard(1, m.L.endless, "ENDLESS",
         "No finish line. Survive the pressure as new rows spawn and your score keeps climbing.",
         {"Infinite waves with rising intensity.", "Score streak banking (risk vs. safety).", "Perfect for high-score chasing."},
         glm::vec3(0.95f, 0.55f, 0.15f), true, true);
 
     if (hoveredIdx == 2) drawModeCard(2, m.L.rogue, "ROGUE",
         "Build a run by choosing upgrades and taking risks — every decision shapes the next round.",
         {"Draft reward cards after wave milestones.", "No random drops — choices define your run.", "Wave-based progression (prototype)."},
         glm::vec3(0.75f, 0.25f, 0.85f), true, true);
 
     if (hoveredIdx == 3) drawModeCard(3, m.L.levels, "LEVELS",
         "A curated set of 20 stages with unique brick layouts.",
         {"Hand-crafted brick patterns.", "20 progressive levels.", "Win by completing all levels."},
         glm::vec3(0.20f, 0.75f, 0.85f), true, true);
 }
 
 } // namespace game::render::menu
 