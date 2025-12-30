#include "game/render/menu/MenuRenderParts.hpp"

#include "game/render/ui/UIHelpers.hpp"
#include "game/ui/OverlayLayout.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace game::render::menu {

static float clampf(float v, float a, float b) { return std::max(a, std::min(v, b)); }

static std::string fmtPct(float v01) {
    v01 = clampf(v01, 0.0f, 1.0f);
    int p = (int)std::round(v01 * 100.0f);
    return std::to_string(p) + "%";
}

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

    // Label
    float labelScale = 0.70f * s;
    float labelY = sl.track.y + sl.track.h + 18.0f * s;
    m.ctx.renderer.drawUIText(sl.track.x, labelY, label, labelScale, hovered ? glm::vec3(1,1,1) : glm::vec3(0.85f, 0.85f, 0.92f));

    // Value (right aligned)
    float valScale = 0.62f * s;
    float valW = m.ctx.renderer.measureUITextWidth(valueText, valScale);
    m.ctx.renderer.drawUIText(sl.track.x + sl.track.w - valW, labelY + 2.0f * s, valueText, valScale, glm::vec3(0.75f, 0.90f, 1.0f));

    // Track background
    float shadow = 3.0f * s;
    m.ctx.renderer.drawUIQuad(sl.track.x + shadow, sl.track.y - shadow, sl.track.w, sl.track.h, glm::vec4(0,0,0,0.55f));
    m.ctx.renderer.drawUIQuad(sl.track.x, sl.track.y, sl.track.w, sl.track.h, glm::vec4(0.10f, 0.10f, 0.16f, 1.0f));

    // Fill (neon-ish)
    float fillW = sl.track.w * norm;
    glm::vec3 fillCol = hovered ? glm::vec3(0.25f, 0.85f, 1.0f) : glm::vec3(0.20f, 0.70f, 0.95f);
    m.ctx.renderer.drawUIQuad(sl.track.x, sl.track.y, fillW, sl.track.h, glm::vec4(fillCol, 0.90f));

    // Knob
    float knobX = sl.track.x + fillW - sl.knob.w * 0.5f;
    knobX = clampf(knobX, sl.track.x - sl.knob.w * 0.5f, sl.track.x + sl.track.w - sl.knob.w * 0.5f);
    float knobY = sl.knob.y;
    glm::vec4 kShadow(0,0,0, hovered ? 0.70f : 0.55f);
    m.ctx.renderer.drawUIQuad(knobX + 2.0f * s, knobY - 2.0f * s, sl.knob.w, sl.knob.h, kShadow);
    m.ctx.renderer.drawUIQuad(knobX, knobY, sl.knob.w, sl.knob.h, glm::vec4(1,1,1, hovered ? 1.0f : 0.92f));
    m.ctx.renderer.drawUIQuad(knobX + 2.0f * s, knobY + 2.0f * s, sl.knob.w - 4.0f * s, sl.knob.h - 4.0f * s,
                              glm::vec4(0.06f, 0.06f, 0.10f, 1.0f));
}

void drawSoundScreen(const MenuCtx& m) {
    float s = m.uiS;
    const auto Ls = game::ui::soundSettingsLayout(m.L, m.ctx.fbW, m.ctx.fbH);

    // Title (anchored to the SOUND panel, not the default menu panel)
    std::string title = "SOUND SETTINGS";
    float tScale = 0.90f * s;
    float tW = m.ctx.renderer.measureUITextWidth(title, tScale);
    float tX = Ls.panel.x + (Ls.panel.w - tW) * 0.5f;
    float tY = Ls.panel.y + Ls.panel.h - 86.0f * s;
    m.ctx.renderer.drawUIText(tX, tY, title, tScale, glm::vec3(0.85f, 0.95f, 1.0f));

    // Back button (reuse drawButton styling)
    // NOTE: Options screen uses hoveredMenuButton index 2 for BACK; keep consistent.
    drawButton(m, /*btnIndex=*/2, Ls.backBtn.x, Ls.backBtn.y, Ls.backBtn.w, Ls.backBtn.h, "< BACK",
               glm::vec3(0.5f, 0.5f, 0.5f), "");

    // Sliders
    drawSlider(m, Ls.master,  m.state.audioMasterVol,  "MASTER",   fmtPct(m.state.audioMasterVol),  m.state.hoveredSoundSlider == 0);
    drawSlider(m, Ls.sfx,     m.state.audioSfxVol,     "SFX",      fmtPct(m.state.audioSfxVol),     m.state.hoveredSoundSlider == 1);
    drawSlider(m, Ls.music,   m.state.audioMusicVol,   "MUSIC",    fmtPct(m.state.audioMusicVol),   m.state.hoveredSoundSlider == 2);
    drawSlider(m, Ls.stinger, m.state.audioStingerVol, "STINGERS", fmtPct(m.state.audioStingerVol), m.state.hoveredSoundSlider == 3);

    // Helper hint
    std::string hint = "Tip: drag sliders. Keep Music lower than SFX for clarity.";
    float hScale = 0.46f * s;
    float hW = m.ctx.renderer.measureUITextWidth(hint, hScale);
    float hX = Ls.panel.x + (Ls.panel.w - hW) * 0.5f;
    // Put the hint right above the BACK button (user request).
    float hY = Ls.backBtn.y + Ls.backBtn.h + 14.0f * s;
    m.ctx.renderer.drawUIText(hX, hY, hint, hScale, glm::vec3(0.72f, 0.80f, 0.90f));
}

} // namespace game::render::menu


