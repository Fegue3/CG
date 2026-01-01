/**
 * @file MenuRenderBase.cpp
 * @brief Funções base de render do menu: criação de contexto (MenuCtx), fundo “retro” animado,
 *        título com neon RGB, painel principal e botões (com hover e subtítulo).
 *
 * @details
 *  Este ficheiro concentra peças reutilizáveis do menu que são usadas por vários ecrãs:
 *   - makeMenuCtx(): “empacota” referências (ctx/state/assets/layout) e pré-calcula medidas/posições
 *     usadas repetidamente (painel e botões), para evitar repetir contas noutros renders.
 *   - drawRetroBackground(): desenha o fundo animado (textura com scroll + overlay escuro),
 *     scanlines e partículas (estrelas) pseudo-aleatórias determinísticas.
 *   - drawTitle(): desenha o título “BREAKOUT 3D” com glow e gradiente RGB por letra.
 *   - drawMainPanelIfNeeded(): desenha o painel “default” por trás dos ecrãs MAIN/OPTIONS/INSTRUCTIONS/SOUND.
 *     Alguns ecrãs (PLAY_MODES, LEVEL_SELECT) desenham painéis próprios.
 *   - drawButton(): desenha um botão com sombra, “fake gradient”, borda e label + subtítulo.
 *
 *  Convenções:
 *   - UI em píxeis (0..fbW, 0..fbH). Origem em baixo-esquerda.
 *   - Todas as escalas são multiplicadas por uiS (m.uiS) que vem do layout, para adaptar a resoluções.
 *   - Efeitos “retro” (scanlines/vignette/particles) são intencionais para manter vibe arcade.
 */

 #include "game/render/menu/MenuRenderParts.hpp"

 #include "game/GameAssets.hpp"
 #include "game/render/ui/UIHelpers.hpp"
 
 #include <algorithm>
 #include <cmath>
 
 namespace game::render::menu {
 
 /**
  * @brief Constrói um MenuCtx a partir do RenderContext, GameState e GameAssets.
  *
  * @param ctx    Contexto de render (framebuffer, renderer, tempo).
  * @param state  Estado do jogo (menuLayout, hover states, screen actual).
  * @param assets Assets do jogo (texturas/meshes usados no menu).
  *
  * @return MenuCtx preenchido com referências e valores “cacheados” do layout.
  *
  * @details
  *  O MenuCtx serve para:
  *   - evitar passar dezenas de parâmetros para cada função de render;
  *   - evitar recalcular posições/tamanhos de elementos a cada chamada.
  */
 MenuCtx makeMenuCtx(const RenderContext& ctx, const GameState& state, const GameAssets& assets) {
     MenuCtx m{ctx, state, assets, state.menuLayout};
 
     // Escala global de UI (define o “tamanho” do menu consoante resolução/aspect).
     m.uiS = m.L.uiScale;
 
     // Painel base do menu (a maior parte dos ecrãs usa este rect).
     m.panelW = m.L.panelW;
     m.panelH = m.L.panelH;
     m.panelX = m.L.panelX;
     m.panelY = m.L.panelY;
 
     // Botões principais (posição X é comum; Y varia por botão).
     m.btnW = m.L.btn1.w;
     m.btnH = m.L.btn1.h;
     m.btnX = m.L.btn1.x;
     m.btn1Y = m.L.btn1.y;
     m.btn2Y = m.L.btn2.y;
     m.btn3Y = m.L.btn3.y;
     m.btn4Y = m.L.btn4.y;
 
     return m;
 }
 
 /**
  * @brief Desenha o fundo animado “retro” do menu: scroll de textura, scanlines, vignette e partículas.
  *
  * @param m MenuCtx com renderer, fb size, tempo e assets.
  *
  * @details
  *  Camadas (por ordem):
  *   1) Textura de background com scroll vertical lento (desenhada 2x para “loop”).
  *   2) Overlay escuro para aumentar contraste dos botões/painéis.
  *   3) Scanlines horizontais (efeito CRT).
  *   4) Vignette simples (escurece top/bottom).
  *   5) Partículas/estrelas (pontos a cair) com pulso de brilho.
  *
  *  As partículas são determinísticas (seed a partir do índice) para evitar “random jitter”
  *  e manter um visual estável.
  */
 void drawRetroBackground(const MenuCtx& m) {
     // === RETRO ANIMATED BACKGROUND ===
 
     // -------------------------------------------------------------------------
     // (1) Textura de fundo com scroll vertical lento.
     // -------------------------------------------------------------------------
     float scrollSpeed = 0.05f;
 
     // O *100.0f é só para trazer o valor para uma escala “visível” em pixels por segundo.
     // fmod mantém o valor dentro de [0..fbH) para dar loop perfeito.
     float scrollY = std::fmod(m.ctx.time.now() * scrollSpeed * 100.0f, (float)m.ctx.fbH);
 
     // Desenha a textura 2x (uma em cima da outra) para o loop ser “seamless”.
     m.ctx.renderer.drawUIQuad(
         0, scrollY,
         (float)m.ctx.fbW, (float)m.ctx.fbH,
         glm::vec4(1, 1, 1, 0.15f),
         m.assets.backgroundTexs[0].id
     );
     m.ctx.renderer.drawUIQuad(
         0, scrollY - m.ctx.fbH,
         (float)m.ctx.fbW, (float)m.ctx.fbH,
         glm::vec4(1, 1, 1, 0.15f),
         m.assets.backgroundTexs[0].id
     );
 
     // -------------------------------------------------------------------------
     // (2) Overlay escuro: dá contraste ao UI por cima.
     // -------------------------------------------------------------------------
     m.ctx.renderer.drawUIQuad(
         0, 0,
         (float)m.ctx.fbW, (float)m.ctx.fbH,
         glm::vec4(0.0f, 0.0f, 0.0f, 0.7f)
     );
 
     // -------------------------------------------------------------------------
     // (3) Scanlines: linhas horizontais alternadas.
     // -------------------------------------------------------------------------
     float scanLineSpacing = 4.0f;
     float scanLineAlpha = 0.08f;
 
     // step = spacing*2 para ficar “linha / gap / linha / gap”.
     for (float y = 0; y < m.ctx.fbH; y += scanLineSpacing * 2) {
         m.ctx.renderer.drawUIQuad(
             0, y,
             (float)m.ctx.fbW, scanLineSpacing,
             glm::vec4(0, 0, 0, scanLineAlpha)
         );
     }
 
     // -------------------------------------------------------------------------
     // (4) Vignette simples: escurece top e bottom (não é radial, é “bar”).
     // -------------------------------------------------------------------------
     float vignetteSize = m.ctx.fbW * 0.3f;
 
     // Top
     m.ctx.renderer.drawUIQuad(
         0, 0,
         (float)m.ctx.fbW, vignetteSize,
         glm::vec4(0.0f, 0.0f, 0.0f, 0.5f)
     );
 
     // Bottom
     m.ctx.renderer.drawUIQuad(
         0, m.ctx.fbH - vignetteSize,
         (float)m.ctx.fbW, vignetteSize,
         glm::vec4(0.0f, 0.0f, 0.0f, 0.5f)
     );
 
     // -------------------------------------------------------------------------
     // (5) Partículas/estrelas: pontos “a cair” com pulso de brilho.
     // -------------------------------------------------------------------------
     int numParticles = 100;
 
     for (int i = 0; i < numParticles; i++) {
         // Seed determinística por índice (não depende de rand()).
         float seed = (float)i * 123.456f;
 
         // x fixo por partícula (distribuição pseudo-rand no [0..fbW]).
         float x = std::fmod(seed * 0.314f, 1.0f) * m.ctx.fbW;
 
         // Cada partícula tem speed diferente (15..40).
         float speed = 15.0f + std::fmod(seed * 0.271f, 1.0f) * 25.0f;
 
         // Offset temporal faz as partículas “rodarem” em loop vertical.
         float yOffset = std::fmod(
             m.ctx.time.now() * speed + seed * 100.0f,
             (float)m.ctx.fbH + 100.0f
         ) - 50.0f;
 
         // Inverte para cair de cima para baixo (porque UI y cresce para cima).
         float y = m.ctx.fbH - yOffset;
 
         // Tamanho (2..5) e pulso de brilho.
         float size = 2.0f + std::fmod(seed * 0.421f, 1.0f) * 3.0f;
         float pulse = 0.5f + 0.5f * std::sin(m.ctx.time.now() * 2.0f + seed);
         float alpha = 0.3f + 0.4f * pulse;
 
         // Cor “azulada” com variação via pulse (vibe neon).
         glm::vec3 starColor(0.2f + 0.8f * pulse, 0.6f + 0.4f * pulse, 1.0f);
 
         // Nota: starColor*alpha + alpha é um bocado redundante (o RGB já vai “dim”),
         // mas mantém o visual “soft” sem precisar de lerp adicional.
         m.ctx.renderer.drawUIQuad(
             x, y,
             size, size,
             glm::vec4(starColor * alpha, alpha)
         );
     }
 }
 
 /**
  * @brief Desenha o título principal do menu (“BREAKOUT 3D”) com glow e gradiente neon por letra.
  *
  * @param m MenuCtx com layout (posição/escala) e renderer.
  *
  * @details
  *  Técnica:
  *   - Glow: vários draws deslocados (offsets) com alpha inversamente proporcional.
  *   - Outline: 4 passes (esq/dir/cima/baixo) para legibilidade.
  *   - Cor final: HSV->RGB por letra, com um pequeno “wiggle” no hue ao longo do tempo.
  */
 void drawTitle(const MenuCtx& m) {
     std::string title = "BREAKOUT 3D";
     float titleScale = m.L.titleScale;
     float titleX = m.L.titleX;
     float titleY = m.L.titleY;
 
     // -------------------------------------------------------------------------
     // Glow: desenhar o texto várias vezes com offsets e alpha baixo.
     // -------------------------------------------------------------------------
     glm::vec3 glowColor(0.10f, 0.35f, 0.90f);
     for (float offset = 3.0f; offset >= 1.0f; offset -= 0.5f) {
         float alpha = 0.18f / offset;
         m.ctx.renderer.drawUIText(titleX - offset, titleY, title, titleScale, glowColor * alpha);
         m.ctx.renderer.drawUIText(titleX + offset, titleY, title, titleScale, glowColor * alpha);
         m.ctx.renderer.drawUIText(titleX, titleY - offset, title, titleScale, glowColor * alpha);
         m.ctx.renderer.drawUIText(titleX, titleY + offset, title, titleScale, glowColor * alpha);
     }
 
     // -------------------------------------------------------------------------
     // Cor por letra: gradiente RGB com animação suave (tempo).
     // -------------------------------------------------------------------------
     int n = (int)title.size();
     for (int i = 0; i < n; ++i) {
         float prefixW = m.ctx.renderer.measureUITextWidth(title.substr(0, i), titleScale);
         float cx = titleX + prefixW;
         float cy = titleY;
         std::string ch = title.substr(i, 1);
 
         // Outline fino para contraste sobre backgrounds claros.
         glm::vec3 outlineCol(0.02f, 0.02f, 0.06f);
         m.ctx.renderer.drawUIText(cx - 2.0f, cy, ch, titleScale, outlineCol);
         m.ctx.renderer.drawUIText(cx + 2.0f, cy, ch, titleScale, outlineCol);
         m.ctx.renderer.drawUIText(cx, cy - 2.0f, ch, titleScale, outlineCol);
         m.ctx.renderer.drawUIText(cx, cy + 2.0f, ch, titleScale, outlineCol);
 
         // Hue base ~0.56 (azul), varre até ~+0.35 no fim da palavra (mais verde/ciano),
         // e um wiggle temporal para “viver”.
         float t = m.ctx.time.now();
         float hue = std::fmod(
             0.56f
             + (float)i / (float)std::max(1, n - 1) * 0.35f
             + 0.08f * std::sin(t * 1.2f + i * 0.3f),
             1.0f
         );
 
         glm::vec3 col = ui::hsv2rgb(hue, 0.85f, 1.0f);
         m.ctx.renderer.drawUIText(cx, cy, ch, titleScale, col);
     }
 }
 
 /**
  * @brief Desenha o painel “default” atrás do menu (para ecrãs que não têm painel custom).
  *
  * @param m MenuCtx.
  *
  * @details
  *  Regras:
  *   - Se showInstructions estiver activo, não desenhar o painel por trás (evita bleed-through).
  *   - PLAY_MODES e LEVEL_SELECT têm painéis próprios (layout diferente).
  *   - SOUND tem painel custom mais alto (soundSettingsLayout).
  *
  *  Visual:
  *   - sombra + corpo do painel
  *   - borda neon RGB animada (mesma linguagem visual do título)
  */
 void drawMainPanelIfNeeded(const MenuCtx& m) {
     // Não desenhar por trás do overlay de instruções.
     if (m.state.showInstructions) return;
 
     // Estes ecrãs fazem rendering custom (não usam o painel base).
     if (m.state.currentMenuScreen == MenuScreen::PLAY_MODES) return;
     if (m.state.currentMenuScreen == MenuScreen::LEVEL_SELECT) return;
 
     // -------------------------------------------------------------------------
     // Escolher rect do painel:
     // - default = m.panel*
     // - SOUND = soundSettingsLayout (taller)
     // -------------------------------------------------------------------------
     float panelX = m.panelX;
     float panelY = m.panelY;
     float panelW = m.panelW;
     float panelH = m.panelH;
 
     if (m.state.currentMenuScreen == MenuScreen::SOUND) {
         const auto SL = game::ui::soundSettingsLayout(m.L, m.ctx.fbW, m.ctx.fbH);
         panelX = SL.panel.x;
         panelY = SL.panel.y;
         panelW = SL.panel.w;
         panelH = SL.panel.h;
     }
 
     // -------------------------------------------------------------------------
     // Sombra do painel (puxa UI para a frente).
     // -------------------------------------------------------------------------
     float shadowOffset = 6.0f * m.uiS;
     m.ctx.renderer.drawUIQuad(
         panelX + shadowOffset, panelY - shadowOffset,
         panelW, panelH,
         glm::vec4(0.0f, 0.0f, 0.0f, 0.5f)
     );
 
     // Corpo do painel (azul escuro)
     m.ctx.renderer.drawUIQuad(
         panelX, panelY,
         panelW, panelH,
         glm::vec4(0.08f, 0.08f, 0.14f, 0.98f)
     );
 
     // -------------------------------------------------------------------------
     // Borda neon RGB animada.
     // -------------------------------------------------------------------------
     float borderThickness = 3.0f * m.uiS;
     float tRgb = m.ctx.time.now();
     float hueRgb = std::fmod(0.56f + 0.08f * std::sin(tRgb * 1.2f), 1.0f);
     glm::vec3 neonRgb = ui::hsv2rgb(hueRgb, 0.85f, 1.0f);
     glm::vec4 borderColor(neonRgb, 1.0f);
 
     m.ctx.renderer.drawUIQuad(panelX - borderThickness, panelY - borderThickness, panelW + 2 * borderThickness, borderThickness, borderColor);
     m.ctx.renderer.drawUIQuad(panelX - borderThickness, panelY + panelH,        panelW + 2 * borderThickness, borderThickness, borderColor);
     m.ctx.renderer.drawUIQuad(panelX - borderThickness, panelY,                borderThickness, panelH,       borderColor);
     m.ctx.renderer.drawUIQuad(panelX + panelW,          panelY,                borderThickness, panelH,       borderColor);
 }
 
 /**
  * @brief Desenha um botão do menu com hover: sombra, fundo (com “fake gradient”), borda, label e subtítulo.
  *
  * @param m         MenuCtx.
  * @param btnIndex  Índice do botão (para comparar com state.hoveredMenuButton).
  * @param bx        X do botão.
  * @param by        Y do botão.
  * @param bw        Largura do botão.
  * @param bh        Altura do botão.
  * @param label     Texto principal do botão.
  * @param baseColor Cor base do botão.
  * @param subtitle  Texto secundário (opcional). Se existir, o label é puxado um pouco para cima.
  *
  * @details
  *  - Hover aumenta brilho e sombra (dá feedback imediato).
  *  - “Fake gradient”: desenha-se um segundo quad na metade de cima com alpha baixo e cor mais escura.
  *  - Borda: branca quando hovered, preta/transparente quando não.
  */
 void drawButton(
     const MenuCtx& m,
     int btnIndex,
     float bx, float by, float bw, float bh,
     const std::string& label,
     const glm::vec3& baseColor,
     const std::string& subtitle
 ) {
     bool hovered = (m.state.hoveredMenuButton == btnIndex);
     float hoverBrightness = hovered ? 1.3f : 1.0f;
 
     // -------------------------------------------------------------------------
     // (1) Sombra do botão (maior e mais opaca em hover).
     // -------------------------------------------------------------------------
     float btnShadowOffset = (hovered ? 5.0f : 3.0f) * m.uiS;
     m.ctx.renderer.drawUIQuad(
         bx + btnShadowOffset, by - btnShadowOffset,
         bw, bh,
         glm::vec4(0.0f, 0.0f, 0.0f, hovered ? 0.6f : 0.5f)
     );
 
     // -------------------------------------------------------------------------
     // (2) Fundo do botão + “fake gradient”.
     // -------------------------------------------------------------------------
     glm::vec3 color = baseColor * hoverBrightness;
 
     // Corpo principal
     m.ctx.renderer.drawUIQuad(
         bx, by,
         bw, bh,
         glm::vec4(color.r, color.g, color.b, 1.0f)
     );
 
     // Meio superior com alpha baixo e cor mais escura para sugerir gradiente/luz.
     m.ctx.renderer.drawUIQuad(
         bx, by + bh * 0.5f,
         bw, bh * 0.5f,
         glm::vec4(color.r * 0.7f, color.g * 0.7f, color.b * 0.7f, 0.3f)
     );
 
     // -------------------------------------------------------------------------
     // (3) Borda do botão: em hover fica mais grossa e branca.
     // -------------------------------------------------------------------------
     float btnBorderThickness = (hovered ? 3.0f : 2.0f) * m.uiS;
     glm::vec4 btnBorder = hovered
         ? glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)
         : glm::vec4(0.0f, 0.0f, 0.0f, 0.4f);
 
     m.ctx.renderer.drawUIQuad(bx, by, bw, btnBorderThickness, btnBorder);
     m.ctx.renderer.drawUIQuad(bx, by + bh - btnBorderThickness, bw, btnBorderThickness, btnBorder);
     m.ctx.renderer.drawUIQuad(bx, by, btnBorderThickness, bh, btnBorder);
     m.ctx.renderer.drawUIQuad(bx + bw - btnBorderThickness, by, btnBorderThickness, bh, btnBorder);
 
     // -------------------------------------------------------------------------
     // (4) Label: centrado. Se existir subtítulo, sobe ligeiramente.
     // -------------------------------------------------------------------------
     float labelScale = 1.0f * m.uiS;
     float labelW = m.ctx.renderer.measureUITextWidth(label, labelScale);
     float labelH = m.ctx.renderer.getUIFontLineHeight(labelScale);
 
     float labelX = bx + (bw - labelW) * 0.5f;
     float labelY = by + (bh - labelH) * 0.5f + (subtitle.empty() ? 0.0f : 8.0f * m.uiS);
 
     // Sombra do label: mais forte em hover para reforçar contraste.
     float shadowStrength = hovered ? 0.8f : 0.6f;
     m.ctx.renderer.drawUIText(
         labelX + 1.0f * m.uiS, labelY - 1.0f * m.uiS,
         label, labelScale,
         glm::vec3(0, 0, 0) * shadowStrength
     );
 
     m.ctx.renderer.drawUIText(labelX, labelY, label, labelScale, glm::vec3(1, 1, 1));
 
     // -------------------------------------------------------------------------
     // (5) Subtítulo: opcional, mais pequeno. Fica abaixo do label.
     // -------------------------------------------------------------------------
     if (!subtitle.empty()) {
         float subScale = 0.50f * m.uiS;
         float subW = m.ctx.renderer.measureUITextWidth(subtitle, subScale);
 
         float subX = bx + (bw - subW) * 0.5f;
         float subY = labelY - labelH * 0.5f - 4.0f * m.uiS;
 
         glm::vec3 subColor = hovered
             ? glm::vec3(1.0f, 1.0f, 1.0f)
             : glm::vec3(0.8f, 0.8f, 0.9f);
 
         m.ctx.renderer.drawUIText(subX, subY, subtitle, subScale, subColor);
     }
 }
 
 } // namespace game::render::menu
 