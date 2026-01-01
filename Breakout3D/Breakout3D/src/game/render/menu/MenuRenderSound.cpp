/**
 * @file MenuRenderSound.cpp
 * @brief Render do ecrã "Sound Settings" no menu (título + sliders + botão back).
 *
 * Este ecrã usa o layout calculado em `game::ui::soundSettingsLayout(...)` e desenha:
 * - Título do painel SOUND
 * - Botão "< BACK" (reutiliza o estilo de `drawButton`)
 * - 4 sliders (master/sfx/music/stingers) com label + percentagem + knob
 * - Hint final acima do botão
 *
 * Notas:
 * - O slider recebe um valor normalizado [0..1] e converte para pixels via `fillW`.
 * - O hover muda brilho/contraste e dá feedback visual (fill mais vivo + knob mais claro).
 */
 #include "game/render/menu/MenuRenderParts.hpp"

 #include "game/render/ui/UIHelpers.hpp"
 #include "game/ui/OverlayLayout.hpp"
 
 #include <algorithm>
 #include <cmath>
 #include <string>
 
 namespace game::render::menu {
 
 /// Clamp simples para floats (evita dependências extra).
 static float clampf(float v, float a, float b) { return std::max(a, std::min(v, b)); }
 
 /// Converte um valor [0..1] em string de percentagem ("0%".."100%").
 static std::string fmtPct(float v01) {
     v01 = clampf(v01, 0.0f, 1.0f);
     int p = (int)std::round(v01 * 100.0f);
     return std::to_string(p) + "%";
 }
 
 /**
  * @brief Desenha um slider horizontal com label e valor.
  *
  * O layout do slider (track + knob) vem do layout do UI (SoundSettingsLayout::Slider).
  * Aqui só fazemos o render:
  * - label (em cima)
  * - valueText (alinhado à direita)
  * - track (barra)
  * - fill (parte preenchida)
  * - knob (manípulo)
  */
 static void drawSlider(
     const MenuCtx& m,
     const game::ui::SoundSettingsLayout::Slider& sl,
     float norm,
     const std::string& label,
     const std::string& valueText,
     bool hovered
 ) {
     float s = m.uiS;
     norm = clampf(norm, 0.0f, 1.0f);
 
     // --- Texto: label + valor ---
     float labelScale = 0.70f * s;
     float labelY = sl.track.y + sl.track.h + 18.0f * s;
     m.ctx.renderer.drawUIText(
         sl.track.x, labelY, label, labelScale,
         hovered ? glm::vec3(1,1,1) : glm::vec3(0.85f, 0.85f, 0.92f)
     );
 
     float valScale = 0.62f * s;
     float valW = m.ctx.renderer.measureUITextWidth(valueText, valScale);
     m.ctx.renderer.drawUIText(
         sl.track.x + sl.track.w - valW, labelY + 2.0f * s,
         valueText, valScale,
         glm::vec3(0.75f, 0.90f, 1.0f)
     );
 
     // --- Track (fundo da barra) ---
     float shadow = 3.0f * s;
     m.ctx.renderer.drawUIQuad(
         sl.track.x + shadow, sl.track.y - shadow,
         sl.track.w, sl.track.h,
         glm::vec4(0,0,0,0.55f)
     );
     m.ctx.renderer.drawUIQuad(
         sl.track.x, sl.track.y,
         sl.track.w, sl.track.h,
         glm::vec4(0.10f, 0.10f, 0.16f, 1.0f)
     );
 
     // --- Fill (parte preenchida) ---
     float fillW = sl.track.w * norm;
     glm::vec3 fillCol = hovered ? glm::vec3(0.25f, 0.85f, 1.0f)
                                 : glm::vec3(0.20f, 0.70f, 0.95f);
     m.ctx.renderer.drawUIQuad(
         sl.track.x, sl.track.y,
         fillW, sl.track.h,
         glm::vec4(fillCol, 0.90f)
     );
 
     // --- Knob (manípulo) ---
     // Centro do knob “segue” o fim do fill, mas clamp para não sair fora do track.
     float knobX = sl.track.x + fillW - sl.knob.w * 0.5f;
     knobX = clampf(knobX,
                    sl.track.x - sl.knob.w * 0.5f,
                    sl.track.x + sl.track.w - sl.knob.w * 0.5f);
 
     float knobY = sl.knob.y;
 
     glm::vec4 kShadow(0,0,0, hovered ? 0.70f : 0.55f);
     m.ctx.renderer.drawUIQuad(knobX + 2.0f * s, knobY - 2.0f * s, sl.knob.w, sl.knob.h, kShadow);
     m.ctx.renderer.drawUIQuad(knobX, knobY, sl.knob.w, sl.knob.h,
                               glm::vec4(1,1,1, hovered ? 1.0f : 0.92f));
     // “Inset” escuro para dar profundidade (parece knob real, não só um rect branco).
     m.ctx.renderer.drawUIQuad(knobX + 2.0f * s, knobY + 2.0f * s,
                               sl.knob.w - 4.0f * s, sl.knob.h - 4.0f * s,
                               glm::vec4(0.06f, 0.06f, 0.10f, 1.0f));
 }
 
 void drawSoundScreen(const MenuCtx& m) {
     float s = m.uiS;
 
     // Layout específico do painel SOUND (mais alto).
     const auto Ls = game::ui::soundSettingsLayout(m.L, m.ctx.fbW, m.ctx.fbH);
 
     // --- Título ---
     std::string title = "SOUND SETTINGS";
     float tScale = 0.90f * s;
     float tW = m.ctx.renderer.measureUITextWidth(title, tScale);
     float tX = Ls.panel.x + (Ls.panel.w - tW) * 0.5f;
     float tY = Ls.panel.y + Ls.panel.h - 86.0f * s;
     m.ctx.renderer.drawUIText(tX, tY, title, tScale, glm::vec3(0.85f, 0.95f, 1.0f));
 
     // --- Botão Back ---
     // Index 2 = BACK dentro do ecrã SOUND (mantém coerência com Options screen).
     drawButton(m, /*btnIndex=*/2,
                Ls.backBtn.x, Ls.backBtn.y, Ls.backBtn.w, Ls.backBtn.h,
                "< BACK",
                glm::vec3(0.5f, 0.5f, 0.5f),
                "");
 
     // --- Sliders ---
     // Cada um lê o valor do GameState (0..1) e mostra percentagem.
     drawSlider(m, Ls.master,  m.state.audioMasterVol,  "MASTER",   fmtPct(m.state.audioMasterVol),  m.state.hoveredSoundSlider == 0);
     drawSlider(m, Ls.sfx,     m.state.audioSfxVol,     "SFX",      fmtPct(m.state.audioSfxVol),     m.state.hoveredSoundSlider == 1);
     drawSlider(m, Ls.music,   m.state.audioMusicVol,   "MUSIC",    fmtPct(m.state.audioMusicVol),   m.state.hoveredSoundSlider == 2);
     drawSlider(m, Ls.stinger, m.state.audioStingerVol, "STINGERS", fmtPct(m.state.audioStingerVol), m.state.hoveredSoundSlider == 3);
 
     // --- Hint ---
     std::string hint = "Tip: drag sliders. Keep Music lower than SFX for clarity.";
     float hScale = 0.46f * s;
     float hW = m.ctx.renderer.measureUITextWidth(hint, hScale);
     float hX = Ls.panel.x + (Ls.panel.w - hW) * 0.5f;
     float hY = Ls.backBtn.y + Ls.backBtn.h + 14.0f * s; // acima do botão
     m.ctx.renderer.drawUIText(hX, hY, hint, hScale, glm::vec3(0.72f, 0.80f, 0.90f));
 }
 
 } // namespace game::render::menu
 