/**
 * @file ScoreHud.cpp
 * @brief Desenha o HUD de pontuação e “progressão” no topo do ecrã (Score/Wave/Level/Best),
 *        incluindo animações de “bank” (streak) e popups temporários de pontos.
 *
 * @details
 *  Esta função decide automaticamente o que mostrar consoante o GameType:
 *   - ROGUE: mostra "WAVE X" + score no topo-centro (sem best score). Também usa o UI de streak/bank
 *            e popups (igual ao Endless). Durante o overlay de escolha de cartas (ROGUE_CARDS),
 *            NÃO desenha nada para não competir com o título do overlay.
 *   - LEVELS: mostra "LEVEL N / 20" no topo-centro.
 *   - ENDLESS: mostra best score (linha superior) + score (linha inferior), e o UI de streak/bank e popups.
 *   - NORMAL/OUTROS: não desenha score (foi removido “per request”).
 *
 *  Convenções de UI usadas aqui:
 *   - Coordenadas em pixels com origem em baixo-esquerda (Renderer::beginUI usa ortho 0..fbW,0..fbH).
 *   - Topo do ecrã = fbH - topMargin.
 *   - Para legibilidade, usa-se outline (4 passes) e alpha ligeiramente reduzida em texto secundário.
 *
 *  Sobre “Split bank display”:
 *   - Quando a streak está a “bankar” (endlessStreakBanking), desenha-se:
 *       +XXX (branco) à direita do score
 *       -XXX (vermelho) à esquerda do score
 *   - Há uma animação curta (bankAnim) que move o texto e reduz alpha (ease in/out).
 *
 *  Sobre “Score popups”:
 *   - Lista state.scorePopups contém entradas temporárias (pts, t).
 *   - Negativos: aparecem à esquerda do score, em vermelho, com stacking vertical.
 *   - Positivos: aparecem por baixo/perto do score, em verde, com stacking vertical.
 *   - Cada popup tem duração e easing próprios (negativos seguem bankAnim; positivos usam popupDur).
 */

 #include "game/render/ui/HudParts.hpp"

 #include "game/GameState.hpp"
 #include "game/render/ui/UIHelpers.hpp"
 
 #include <algorithm>
 #include <cmath>
 #include <string>
 
 #include <glm/glm.hpp>
 
 namespace game::render::hud {
 
 /**
  * @brief Desenha o HUD de score/wave/level/best e os efeitos de “banking” + popups.
  *
  * @param ctx   Contexto de render UI (framebuffer size, renderer, tempo, etc.).
  * @param state Estado actual do jogo (pontuação, wave, tipo de jogo, animações).
  *
  * @note
  *  A posição base do HUD é “top-center”. Elementos secundários (bank, popups)
  *  posicionam-se relativamente ao bounding box do score.
  */
 void drawScoreAndWaveHud(const RenderContext& ctx, const GameState& state) {
     // -------------------------------------------------------------------------
     // (1) ROGUE MODE
     // -------------------------------------------------------------------------
     if (state.gameType == GameType::ROGUE) {
         // Durante a escolha de cartas, o slot topo-centro fica reservado para o título.
         if (state.mode == GameMode::ROGUE_CARDS) return;
 
         // Rogue: wave + score (sem best-score).
         std::string waveStr = "WAVE " + std::to_string(std::max(1, state.wave));
         std::string scoreStr = std::to_string(state.score);
 
         // Escalas (wave mais pequeno; score maior).
         float waveScale = 1.30f;
         float scoreScale = 2.15f;
 
         // Métricas de fonte (altura em px para layout vertical).
         float waveH = ctx.renderer.getUIFontLineHeight(waveScale);
         float scoreH = ctx.renderer.getUIFontLineHeight(scoreScale);
 
         // Larguras para centrar texto.
         float waveW = ctx.renderer.measureUITextWidth(waveStr, waveScale);
         float scoreW = ctx.renderer.measureUITextWidth(scoreStr, scoreScale);
 
         float cx = (float)ctx.fbW * 0.5f;
         float waveX = cx - waveW * 0.5f;
         float scoreX = cx - scoreW * 0.5f;
 
         // Layout vertical no topo (margem + gap).
         const float topMargin = 18.0f;
         const float lineGap = 8.0f;
 
         float yTop = (float)ctx.fbH - topMargin;
         float waveY = yTop - waveH;
         float scoreY = waveY - lineGap - scoreH;
 
         // ---------------------------------------------------------------------
         // Wave: branco com outline (sem RGB), para ser mais “informativo” que “show-off”.
         // ---------------------------------------------------------------------
         {
             glm::vec3 outline(0.02f, 0.02f, 0.06f);
             ctx.renderer.drawUIText(waveX - 2.0f, waveY, waveStr, waveScale, outline);
             ctx.renderer.drawUIText(waveX + 2.0f, waveY, waveStr, waveScale, outline);
             ctx.renderer.drawUIText(waveX, waveY - 2.0f, waveStr, waveScale, outline);
             ctx.renderer.drawUIText(waveX, waveY + 2.0f, waveStr, waveScale, outline);
 
             // Texto principal com alpha ligeiramente reduzida (fica mais “HUD-ish”).
             ctx.renderer.drawUIText(waveX, waveY, waveStr, waveScale, glm::vec4(1, 1, 1, 0.90f));
         }
 
         // Score: branco sólido.
         ctx.renderer.drawUIText(scoreX, scoreY, scoreStr, scoreScale, glm::vec4(1, 1, 1, 1.0f));
 
         // ---------------------------------------------------------------------
         // Split bank display (igual ao Endless):
         // - Anima com ease e offset vertical quando está a “bankar”.
         // - + pontos (branco) à direita, - pontos (vermelho) à esquerda.
         // ---------------------------------------------------------------------
         {
             const float addScale = 1.45f; // escala dos “+XXX/-XXX”
             const float bankAnim = 0.55f; // duração da animação
 
             // t = progresso 0..1 (só se banking estiver activo)
             float t = state.endlessStreakBanking
                 ? std::min(1.0f, std::max(0.0f, state.endlessStreakBankTimer / bankAnim))
                 : 0.0f;
 
             // smoothstep: acelera e desacelera suavemente
             float ease = t * t * (3.0f - 2.0f * t);
 
             // sobe ligeiramente com o tempo (efeito “a dissipar”)
             float yOffset = ease * 22.0f;
 
             // vai perdendo alpha durante o banking
             float alpha = 1.0f - 0.65f * ease;
 
             // baseY: ancorado perto do score
             float baseY = scoreY - scoreH * 0.65f + yOffset;
 
             // + pontos: lado direito (perto do final do score)
             if (state.endlessStreakPosPoints > 0) {
                 std::string s = "+" + std::to_string(state.endlessStreakPosPoints);
                 float w = ctx.renderer.measureUITextWidth(s, addScale);
 
                 // scoreX + scoreW = lado direito do score; deslocamos um pouco para “encaixar”
                 float x = scoreX + scoreW - w * 0.25f;
                 ctx.renderer.drawUIText(x, baseY, s, addScale, glm::vec4(1, 1, 1, alpha));
             }
 
             // - pontos: lado esquerdo (simétrico)
             if (state.endlessStreakNegPoints > 0) {
                 std::string s = "-" + std::to_string(state.endlessStreakNegPoints);
                 float w = ctx.renderer.measureUITextWidth(s, addScale);
 
                 float x = scoreX - w * 0.75f;
                 ctx.renderer.drawUIText(x, baseY, s, addScale, glm::vec4(1.0f, 0.15f, 0.15f, alpha));
             }
         }
 
         // ---------------------------------------------------------------------
         // Score popups (igual ao Endless):
         // - negativos: à esquerda, vermelho, sobem e desvanecem (stack vertical)
         // - positivos: por baixo do score, verde, sobem e desvanecem (stack vertical)
         //
         // Nota: calculamos larguras máximas por “lado” para alinhar bonitinho.
         // ---------------------------------------------------------------------
         if (!state.scorePopups.empty()) {
             const float popupDur = 2.10f;
             float scale = 1.35f;
 
             float negMaxW = 0.0f;
             float posMaxW = 0.0f;
 
             // Primeiro passe: medir larguras máximas para alinhar colunas.
             for (const auto& sp : state.scorePopups) {
                 std::string s = (sp.pts >= 0) ? ("+" + std::to_string(sp.pts)) : std::to_string(sp.pts);
                 float w = ctx.renderer.measureUITextWidth(s, scale);
                 if (sp.pts < 0) negMaxW = std::max(negMaxW, w);
                 else            posMaxW = std::max(posMaxW, w);
             }
 
             float negX = scoreX - negMaxW * 0.75f;
             float posX = scoreX + (scoreW - posMaxW) * 0.5f;
 
             float yBase = scoreY - scoreH * 0.65f;
 
             int negIdx = 0;
             int posIdx = 0;
 
             for (const auto& sp : state.scorePopups) {
                 std::string s = (sp.pts >= 0) ? ("+" + std::to_string(sp.pts)) : std::to_string(sp.pts);
 
                 if (sp.pts < 0) {
                     // Negativos: seguem o mesmo envelope do bank (rápido e “punchy”).
                     const float bankAnim = 0.55f;
                     float t = std::min(1.0f, std::max(0.0f, sp.t / bankAnim));
                     float ease = t * t * (3.0f - 2.0f * t);
 
                     float yOffset = ease * 22.0f;
                     float a = 1.0f - 0.65f * ease;
 
                     // stacking vertical para não sobrepor múltiplos eventos negativos
                     float y = yBase + yOffset + (float)negIdx * 20.0f;
 
                     ctx.renderer.drawUIText(negX, y, s, scale, glm::vec4(1.0f, 0.15f, 0.15f, a));
                     negIdx++;
                 }
                 else if (sp.pts > 0) {
                     // Positivos: duram mais (popupDur), e movem-se um pouco mais.
                     float u = std::min(1.0f, std::max(0.0f, sp.t / popupDur));
                     float ease = u * u * (3.0f - 2.0f * u);
                     float a = 1.0f - 0.75f * ease;
 
                     float y = (scoreY - scoreH * 1.25f) + ease * 26.0f + (float)posIdx * 20.0f;
 
                     ctx.renderer.drawUIText(posX, y, s, scale, glm::vec4(0.35f, 1.0f, 0.35f, a));
                     posIdx++;
                 }
             }
         }
 
         return;
     }
 
     // -------------------------------------------------------------------------
     // (2) LEVELS MODE
     // -------------------------------------------------------------------------
     if (state.gameType == GameType::LEVELS) {
         // Mostra o nível actual. (Assume-se 20 levels; se mudares, ajusta o literal.)
         std::string levelStr = "LEVEL " + std::to_string(state.currentLevel) + " / 20";
 
         float levelScale = 1.60f;
         float levelH = ctx.renderer.getUIFontLineHeight(levelScale);
         float levelW = ctx.renderer.measureUITextWidth(levelStr, levelScale);
 
         float cx = (float)ctx.fbW * 0.5f;
         float levelX = cx - levelW * 0.5f;
 
         const float topMargin = 18.0f;
         float levelY = (float)ctx.fbH - topMargin - levelH;
 
         // Outline para legibilidade (especialmente em backgrounds claros).
         glm::vec3 outline(0.02f, 0.02f, 0.06f);
         ctx.renderer.drawUIText(levelX - 2.0f, levelY, levelStr, levelScale, outline);
         ctx.renderer.drawUIText(levelX + 2.0f, levelY, levelStr, levelScale, outline);
         ctx.renderer.drawUIText(levelX, levelY - 2.0f, levelStr, levelScale, outline);
         ctx.renderer.drawUIText(levelX, levelY + 2.0f, levelStr, levelScale, outline);
 
         // Cor “cyan” para diferenciar de score (mais “progress UI”).
         ctx.renderer.drawUIText(levelX, levelY, levelStr, levelScale, glm::vec4(0.20f, 0.75f, 0.85f, 1.0f));
         return;
     }
 
     // -------------------------------------------------------------------------
     // (3) NORMAL/OUTROS (sem score)
     // -------------------------------------------------------------------------
     if (state.gameType != GameType::ENDLESS) {
         // Normal mode: score HUD removido.
         return;
     }
 
     // -------------------------------------------------------------------------
     // (4) ENDLESS MODE
     // -------------------------------------------------------------------------
     std::string bestStr = std::to_string(state.endlessBestScore);
     std::string scoreStr = std::to_string(state.score);
 
     float bestScale = 1.30f;   // best score (mais discreto)
     float scoreScale = 2.35f;  // score (principal)
 
     float bestH = ctx.renderer.getUIFontLineHeight(bestScale);
     float scoreH = ctx.renderer.getUIFontLineHeight(scoreScale);
 
     float bestTotalW = ctx.renderer.measureUITextWidth(bestStr, bestScale);
     float scoreTotalW = ctx.renderer.measureUITextWidth(scoreStr, scoreScale);
 
     float bestX = (float)ctx.fbW * 0.5f - bestTotalW * 0.5f;
     float scoreX = (float)ctx.fbW * 0.5f - scoreTotalW * 0.5f;
 
     const float topMargin = 18.0f;
     const float lineGap = 8.0f;
 
     float bestY = (float)ctx.fbH - topMargin - bestH;
     float scoreY = bestY - lineGap - scoreH;
 
     // Best score: alpha menor para não competir com o score actual.
     ctx.renderer.drawUIText(bestX, bestY, bestStr, bestScale, glm::vec4(1.0f, 1.0f, 1.0f, 0.75f));
     ctx.renderer.drawUIText(scoreX, scoreY, scoreStr, scoreScale, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
 
     // -------------------------------------------------------------------------
     // Split bank display (gains/penalties) — ver comentário na versão Rogue.
     // Aqui usamos scoreTotalW para o lado direito (porque o score string mudou).
     // -------------------------------------------------------------------------
     {
         const float addScale = 1.45f;
         const float bankAnim = 0.55f;
 
         float t = state.endlessStreakBanking
             ? std::min(1.0f, std::max(0.0f, state.endlessStreakBankTimer / bankAnim))
             : 0.0f;
 
         float ease = t * t * (3.0f - 2.0f * t);
         float yOffset = ease * 22.0f;
         float alpha = 1.0f - 0.65f * ease;
 
         float baseY = scoreY - scoreH * 0.65f + yOffset;
 
         if (state.endlessStreakPosPoints > 0) {
             std::string s = "+" + std::to_string(state.endlessStreakPosPoints);
             float w = ctx.renderer.measureUITextWidth(s, addScale);
 
             float x = scoreX + scoreTotalW - w * 0.25f;
             ctx.renderer.drawUIText(x, baseY, s, addScale, glm::vec4(1.0f, 1.0f, 1.0f, alpha));
         }
         if (state.endlessStreakNegPoints > 0) {
             std::string s = "-" + std::to_string(state.endlessStreakNegPoints);
             float w = ctx.renderer.measureUITextWidth(s, addScale);
 
             float x = scoreX - w * 0.75f;
             ctx.renderer.drawUIText(x, baseY, s, addScale, glm::vec4(1.0f, 0.15f, 0.15f, alpha));
         }
     }
 
     // -------------------------------------------------------------------------
     // Score popups (neg/pos) — ver comentário na versão Rogue.
     // -------------------------------------------------------------------------
     if (!state.scorePopups.empty()) {
         const float popupDur = 2.10f;
         float scale = 1.35f;
 
         float negMaxW = 0.0f;
         float posMaxW = 0.0f;
 
         for (const auto& sp : state.scorePopups) {
             std::string s = (sp.pts >= 0) ? ("+" + std::to_string(sp.pts)) : std::to_string(sp.pts);
             float w = ctx.renderer.measureUITextWidth(s, scale);
             if (sp.pts < 0) negMaxW = std::max(negMaxW, w);
             else            posMaxW = std::max(posMaxW, w);
         }
 
         float negX = scoreX - negMaxW * 0.75f;
         float posX = scoreX + (scoreTotalW - posMaxW) * 0.5f;
 
         float yBase = scoreY - scoreH * 0.65f;
         int negIdx = 0;
         int posIdx = 0;
 
         for (const auto& sp : state.scorePopups) {
             std::string s = (sp.pts >= 0) ? ("+" + std::to_string(sp.pts)) : std::to_string(sp.pts);
 
             if (sp.pts < 0) {
                 const float bankAnim = 0.55f;
 
                 float t = std::min(1.0f, std::max(0.0f, sp.t / bankAnim));
                 float ease = t * t * (3.0f - 2.0f * t);
 
                 float yOffset = ease * 22.0f;
                 float a = 1.0f - 0.65f * ease;
 
                 float y = yBase + yOffset + (float)negIdx * 20.0f;
                 ctx.renderer.drawUIText(negX, y, s, scale, glm::vec4(1.0f, 0.15f, 0.15f, a));
                 negIdx++;
             }
             else if (sp.pts > 0) {
                 float u = std::min(1.0f, std::max(0.0f, sp.t / popupDur));
                 float ease = u * u * (3.0f - 2.0f * u);
                 float a = 1.0f - 0.75f * ease;
 
                 float y = (scoreY - scoreH * 1.25f) + ease * 26.0f + (float)posIdx * 20.0f;
                 ctx.renderer.drawUIText(posX, y, s, scale, glm::vec4(0.35f, 1.0f, 0.35f, a));
                 posIdx++;
             }
         }
     }
 }
 
 } // namespace game::render::hud
 