/**
 * @file OverlayLayout.cpp
 * @brief Cálculo do layout base do menu (título + painel principal + botões) e sub-layouts:
 *        - PLAY MODES (4 colunas)
 *        - SOUND SETTINGS (painel + sliders)
 *
 * Notas gerais:
 *  - Coordenadas são y-up (consistente com o renderer UI do projeto).
 *  - fitScaleToWidth() usa métricas reais da fonte via Renderer para evitar overflow.
 *  - uiScale (sPanel / sGrid) tenta “crescer” em ecrãs grandes mas mantém limites para não rebentar.
 */
 #include "game/ui/OverlayLayout.hpp"
 #include "engine/Renderer.hpp"
 
 #include <algorithm>
 #include <cmath>
 #include <string>
 
 namespace game::ui {
 
 /// Ajusta uma escala de texto para caber num maxWidth (em px), usando measureUITextWidth().
 static float fitScaleToWidth(engine::Renderer& renderer, const std::string& text, float desiredScale, float maxWidthPx) {
     float s = desiredScale;
     if (maxWidthPx <= 1.0f) return s;
 
     float w = renderer.measureUITextWidth(text, s);
     if (w > maxWidthPx && w > 1.0f) {
         s *= (maxWidthPx / w);
     }
     return s;
 }
 
 MenuLayout calculateMenuLayout(engine::Renderer& renderer, int fbW, int fbH) {
     MenuLayout L;
 
     // Margens e distâncias principais
     const float topMargin = 28.0f;
     const float bottomMargin = 24.0f;
     const float titlePanelGap = 26.0f;
 
     // ------------------------------------------------------------------
     // 1) TÍTULO
     // ------------------------------------------------------------------
     const std::string title = "BREAKOUT 3D";
 
     // Escala base “grande”, mas limitada pela largura do ecrã
     L.titleScale = fitScaleToWidth(renderer, title, 12.0f, (float)fbW * 0.92f);
 
     // Garante espaço vertical: [title][gap][panel]
     float maxTitleH = (float)fbH - topMargin - bottomMargin - titlePanelGap - L.panelH;
     if (maxTitleH > 8.0f) {
         float th = renderer.getUIFontLineHeight(L.titleScale);
         if (th > maxTitleH && th > 1.0f) {
             L.titleScale *= (maxTitleH / th);
         }
     }
 
     L.titleW = renderer.measureUITextWidth(title, L.titleScale);
     L.titleH = renderer.getUIFontLineHeight(L.titleScale);
     L.titleX = ((float)fbW - L.titleW) * 0.5f;
     L.titleY = (float)fbH - topMargin - L.titleH;
 
     // ------------------------------------------------------------------
     // 2) UI SCALE (base)
     // ------------------------------------------------------------------
     // Referência: tuning ~1280x900
     const float refW = 1280.0f;
     const float refH = 900.0f;
     float s = std::min((float)fbW / refW, (float)fbH / refH);
     s = std::max(0.85f, std::min(1.60f, s));
 
     // Espaço disponível abaixo do título
     float maxY   = L.titleY - titlePanelGap;              // y-up
     float availH = std::max(1.0f, maxY - bottomMargin);   // bottomMargin..maxY
 
     // ------------------------------------------------------------------
     // 3) MAIN MENU PANEL (botões PLAY/INSTRUCTIONS/OPTIONS/EXIT)
     // ------------------------------------------------------------------
     const float basePanelW = 500.0f;
     const float basePanelH = 480.0f;
 
     // sPanel: tenta encaixar o painel dentro de availH
     float sPanel = std::min(s, availH / basePanelH);
     sPanel = std::max(0.70f, std::min(1.60f, sPanel));
     L.uiScale = sPanel;
 
     L.panelW = basePanelW * sPanel;
     L.panelH = basePanelH * sPanel;
     L.panelX = ((float)fbW - L.panelW) * 0.5f;
 
     // Composição: painel ligeiramente abaixo do centro (para ficar “debaixo” do título)
     float desiredCenterY = (float)fbH * 0.42f;
     L.panelY = desiredCenterY - L.panelH * 0.5f;
 
     // Clamp a margens
     L.panelY = std::max(bottomMargin, std::min(L.panelY, (float)fbH - bottomMargin - L.panelH));
 
     // Evita sobrepor ao título
     if (L.panelY + L.panelH > maxY) {
         L.panelY = maxY - L.panelH;
         L.panelY = std::max(bottomMargin, L.panelY);
     }
 
     // Botões (mesmas proporções do tuning original)
     const float btnW = 200.0f * sPanel;
     const float btnH = 70.0f * sPanel;
     const float btnX = L.panelX + (L.panelW - btnW) * 0.5f;
 
     const float btn1Y = L.panelY + 360.0f * sPanel;
     const float btn2Y = L.panelY + 250.0f * sPanel;
     const float btn3Y = L.panelY + 140.0f * sPanel;
     const float btn4Y = L.panelY + 30.0f  * sPanel;
 
     L.btn1 = Rect{btnX, btn1Y, btnW, btnH};
     L.btn2 = Rect{btnX, btn2Y, btnW, btnH};
     L.btn3 = Rect{btnX, btn3Y, btnW, btnH};
     L.btn4 = Rect{btnX, btn4Y, btnW, btnH};
 
     // Badge “4” (one-brick test)
     const float badgeW = 48.0f * sPanel;
     const float badgeH = 48.0f * sPanel;
     const float badgeX = L.panelX + L.panelW - badgeW - 15.0f * sPanel;
     const float badgeY = L.panelY + 15.0f * sPanel;
     L.testBadge = Rect{badgeX, badgeY, badgeW, badgeH};
 
     // ------------------------------------------------------------------
     // 4) PLAY MODES: 4 colunas (NORMAL | ENDLESS | ROGUE | LEVELS)
     // ------------------------------------------------------------------
     // Cards muito mais altos (“mini-brochures”), com gaps adaptativos.
     const float baseCardW = 380.0f;
     const float baseCardH = 820.0f;
     const float baseGapXMin = 44.0f;
 
     const float baseGridH = baseCardH;
 
     // Margens laterais maiores (full screen aesthetics)
     const float sideMargin = 84.0f;
     float availW = std::max(1.0f, (float)fbW - 2.0f * sideMargin);
 
     float baseGridW = baseCardW * 4.0f + baseGapXMin * 3.0f;
 
     // Reserva de espaço para footer BACK
     const float baseBackH = 72.0f;
     const float baseBackGap = 18.0f;
     float availHGrid = std::max(1.0f, availH - (baseBackH + baseBackGap));
 
     float sGrid = s;
     sGrid = std::min(sGrid, availHGrid / baseGridH);
     sGrid = std::min(sGrid, availW / baseGridW);
     sGrid = std::max(0.60f, std::min(1.70f, sGrid));
 
     float cardWBase = baseCardW * sGrid;
     float cardH     = baseCardH * sGrid;
 
     // Gap mínimo (scaled)
     float minGapX = baseGapXMin * sGrid;
 
     // Distribuir largura extra:
     // - 65% aumenta cardW
     // - restante aumenta gapX
     float remaining = availW - (cardWBase * 4.0f + minGapX * 3.0f);
     if (remaining < 0.0f) remaining = 0.0f;
 
     float extraCardPer = std::min((remaining * 0.65f) / 4.0f, 140.0f * sGrid);
     float cardW = cardWBase + extraCardPer;
 
     float remainingAfterCards = std::max(0.0f, availW - (cardW * 4.0f + minGapX * 3.0f));
     float gapX = minGapX + (remainingAfterCards / 3.0f);
     gapX = std::min(gapX, 220.0f * sGrid);
 
     float gridW = cardW * 4.0f + gapX * 3.0f;
     float gridH = cardH;
     float gridX = ((float)fbW - gridW) * 0.5f;
 
     // Posição do grid: abaixo do título, ligeiramente mais baixo que o centro
     float centerY = (bottomMargin + maxY) * 0.5f;
     centerY -= 24.0f * sGrid;
 
     float gridY = centerY - gridH * 0.5f;
 
     // Footer para BACK (garante que cards não colidem com o botão)
     float footerY = bottomMargin + (baseBackH + baseBackGap) * sGrid;
     gridY = std::max(footerY, std::min(gridY, maxY - gridH));
 
     // Helper: cria um card + PLAY button interno
     auto makeCard = [&](float x, float y) -> MenuLayout::ModeCard {
         MenuLayout::ModeCard c;
         c.card = Rect{x, y, cardW, cardH};
 
         float pbW = cardW * 0.46f;
         float pbH = 72.0f * sGrid;
         float pbX = x + (cardW - pbW) * 0.5f;
         float pbY = y + 26.0f * sGrid;
 
         c.playBtn = Rect{pbX, pbY, pbW, pbH};
         return c;
     };
 
     float y  = gridY;
     float x0 = gridX;
     float x1 = gridX + (cardW + gapX) * 1.0f;
     float x2 = gridX + (cardW + gapX) * 2.0f;
     float x3 = gridX + (cardW + gapX) * 3.0f;
 
     L.normal  = makeCard(x0, y);
     L.endless = makeCard(x1, y);
     L.rogue   = makeCard(x2, y);
     L.levels  = makeCard(x3, y);
 
     // BACK: footer bottom-left (não encostado ao canto)
     float backW = 260.0f * sGrid;
     float backH = baseBackH * sGrid;
     float backX = sideMargin;
     float backY = bottomMargin;
     L.backBtn = Rect{backX, backY, backW, backH};
 
     return L;
 }
 
 SoundSettingsLayout soundSettingsLayout(const MenuLayout& menu, int fbW, int fbH) {
     (void)fbW;
 
     SoundSettingsLayout L;
     float s = menu.uiScale;
 
     // Painel mais alto que o normal para sliders terem espaço
     float extraH = 140.0f * s;
     float panelW = menu.panelW;
     float panelH = menu.panelH + extraH;
     float panelX = menu.panelX;
 
     // Alinha com o menu standard, mas expande para cima/baixo igualmente (extraH/2)
     float panelY = menu.panelY - extraH * 0.5f;
 
     // Clamp ao ecrã com margem
     panelY = std::max(24.0f, std::min(panelY, (float)fbH - 24.0f - panelH));
 
     L.panel = Rect{panelX, panelY, panelW, panelH};
 
     // BACK dentro do painel (top-left)
     float backW = 120.0f * s;
     float backH = 50.0f * s;
     float backX = L.panel.x + 20.0f * s;
     float backY = L.panel.y + 15.0f * s;
     L.backBtn = Rect{backX, backY, backW, backH};
 
     // Sliders (tracks)
     float trackW = L.panel.w * 0.74f;
     float trackH = 14.0f * s;
     float trackX = L.panel.x + (L.panel.w - trackW) * 0.5f;
 
     // Rows: de cima para baixo
     float topY  = L.panel.y + L.panel.h - 170.0f * s;
     float rowGap = 112.0f * s;
 
     auto makeSlider = [&](int row) -> SoundSettingsLayout::Slider {
         float y = topY - row * rowGap;
 
         SoundSettingsLayout::Slider sl;
         sl.track = Rect{trackX, y, trackW, trackH};
 
         // Knob mais alto que a track (para “pegar” melhor com o rato)
         float knobW = 18.0f * s;
         float knobH = 34.0f * s;
         sl.knob = Rect{trackX, y - (knobH - trackH) * 0.5f, knobW, knobH};
 
         return sl;
     };
 
     L.master  = makeSlider(0);
     L.sfx     = makeSlider(1);
     L.music   = makeSlider(2);
     L.stinger = makeSlider(3);
 
     return L;
 }
 
 } // namespace game::ui
 