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

void drawInstructionsOverlay(const MenuCtx& m) {
    const auto OL = game::ui::instructionsOverlayLayout(m.L, m.ctx.fbW, m.ctx.fbH, m.state.instructionsTab);
    const float instrX = OL.panel.x;
    const float instrY = OL.panel.y;
    const float instrW = OL.panel.w;
    const float instrH = OL.panel.h;

    // Overlay fill:
    // - Controls: solid panel for readability.
    // - Powerups/Rogue Cards: NO big background panel (content floats on background).
    if (m.state.instructionsTab == 0) {
        m.ctx.renderer.drawUIQuad(instrX, instrY, instrW, instrH, glm::vec4(0.05f, 0.05f, 0.1f, 0.98f));
    }

    // Instructions border — keep for Controls, but for Powerups we avoid a big "panel box"
    // so the left model area feels truly on the background.
    float borderThickness = 3.0f;
    float tRgbInstr = m.ctx.time.now();
    float hueRgbInstr = std::fmod(0.56f + 0.08f * std::sin(tRgbInstr * 1.2f), 1.0f);
    glm::vec3 neonRgbInstr = ui::hsv2rgb(hueRgbInstr, 0.85f, 1.0f);
    glm::vec4 instrBorder(neonRgbInstr, 1.0f);
    if (m.state.instructionsTab == 0) {
        m.ctx.renderer.drawUIQuad(instrX - borderThickness, instrY - borderThickness, instrW + 2*borderThickness, borderThickness, instrBorder);
        m.ctx.renderer.drawUIQuad(instrX - borderThickness, instrY + instrH, instrW + 2*borderThickness, borderThickness, instrBorder);
        m.ctx.renderer.drawUIQuad(instrX - borderThickness, instrY, borderThickness, instrH, instrBorder);
        m.ctx.renderer.drawUIQuad(instrX + instrW, instrY, borderThickness, instrH, instrBorder);
    }

    // Title with glow + underline (tab-aware)
    std::string instrTitle =
        (m.state.instructionsTab == 0) ? "CONTROLS" :
        (m.state.instructionsTab == 1) ? "POWERUPS" :
        "ROGUE CARDS";
    float instrTitleScale = 1.6f * m.uiS;
    float instrTitleW = m.ctx.renderer.measureUITextWidth(instrTitle, instrTitleScale);
    float instrTitleX = instrX + (instrW - instrTitleW) * 0.5f;
    // Powerups/RogueCards title should sit lower so it doesn't crowd the big "BREAKOUT 3D" header.
    float titleTopPad = (m.state.instructionsTab == 1 || m.state.instructionsTab == 2) ? (96.0f * m.uiS) : (42.0f * m.uiS);
    float instrTitleY = instrY + instrH - titleTopPad;
    // In Powerups/Rogue Cards (no panel fill), add a tiny backdrop behind the title only (keeps it readable).
    if (m.state.instructionsTab == 1 || m.state.instructionsTab == 2) {
        float padX = 26.0f * m.uiS;
        float padY = 12.0f * m.uiS;
        float th = m.ctx.renderer.getUIFontLineHeight(instrTitleScale);
        m.ctx.renderer.drawUIQuad(instrTitleX - padX, instrTitleY - padY, instrTitleW + 2.0f * padX, th + 2.0f * padY,
                               glm::vec4(0.05f, 0.05f, 0.1f, 0.40f));
    }
    glm::vec3 glowCol(0.10f, 0.35f, 0.90f);
    for (float o = 2.5f; o >= 1.0f; o -= 0.5f) {
        float a = 0.16f / o;
        m.ctx.renderer.drawUIText(instrTitleX - o, instrTitleY, instrTitle, instrTitleScale, glowCol * a);
        m.ctx.renderer.drawUIText(instrTitleX + o, instrTitleY, instrTitle, instrTitleScale, glowCol * a);
        m.ctx.renderer.drawUIText(instrTitleX, instrTitleY - o, instrTitle, instrTitleScale, glowCol * a);
        m.ctx.renderer.drawUIText(instrTitleX, instrTitleY + o, instrTitle, instrTitleScale, glowCol * a);
    }
    m.ctx.renderer.drawUIText(instrTitleX, instrTitleY, instrTitle, instrTitleScale, glm::vec3(0.2f, 0.85f, 1.0f));
    // Underline neon bar
    float barW = instrTitleW;
    float barH = 4.0f * m.uiS;
    float hueBar = std::fmod(0.56f + 0.08f * std::sin(m.ctx.time.now() * 1.2f), 1.0f);
    glm::vec3 neonBar = ui::hsv2rgb(hueBar, 0.85f, 1.0f);
    m.ctx.renderer.drawUIQuad(instrTitleX, instrTitleY - 6.0f * m.uiS, barW, barH, glm::vec4(neonBar, 1.0f));

    if (m.state.instructionsTab == 0) {
        // Controls list (single panel; no inner "card", no duplicate title, no tip)
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

        // Align descriptions (consistent key column width)
        float maxKeyW = 0.0f;
        for (const auto& it : controls) {
            maxKeyW = std::max(maxKeyW, m.ctx.renderer.measureUITextWidth(it.key, badgeScale));
        }
        float badgeW = maxKeyW + 38.0f;
        float descX = x + badgeW + 28.0f;

        for (const auto& it : controls) {
            ui::drawKeyBadge(m.ctx.renderer, x, y - 18.0f, it.key, badgeScale, m.ctx.time.now());
            m.ctx.renderer.drawUIText(descX, y, it.desc, descScale, glm::vec3(0.86f, 0.94f, 1.0f));
            y -= lineGap;
        }
    }

    if (m.state.instructionsTab == 1) {
        // --- POWERUP INSPECTOR (LET HIM COOK) ---
        struct Entry {
            const char* name;
            const char* desc;
            const engine::Mesh* mesh;
            glm::vec3 tint;
            float scale;
            int videoIdx; // index into GameAssets::powerupVideoFrames
        };

        std::vector<Entry> entries = {
            {"EXPAND",      "Makes the paddle wider for a short time.",              &m.assets.expand,    glm::vec3(0.35f, 1.0f, 0.35f), 1.05f, 0},
            {"EXTRA BALL",  "Spawns 3 additional balls.",                             &m.assets.extraBall, glm::vec3(0.35f, 0.85f, 1.0f),  1.00f, 1},
            {"EXTRA LIFE",  "Adds +1 life.",                                          &m.assets.extraLife, glm::vec3(1.0f, 0.25f, 0.25f),  1.05f, 2},
            {"FIREBALL",    "Turns the ball into a one-shot fireball with AoE.",     &m.assets.fireball,  glm::vec3(1.0f, 0.55f, 0.10f),  1.05f, 3},
            {"SLOW",        "Slows paddle movement for a short time.",               &m.assets.slow,      glm::vec3(0.75f, 0.55f, 0.20f), 1.05f, 4},
            {"SHIELD",      "A barrier behind the paddle saves balls temporarily.",  &m.assets.shield,    glm::vec3(0.25f, 1.0f, 1.0f),  1.10f, 5},
            // Curses (no SKULL powerup in the actual game; SKULL mesh is used as the REVERSE icon)
            {"REVERSE",     "Inverts left/right controls temporarily.",              &m.assets.skull,     glm::vec3(1.0f, 0.20f, 0.90f), 1.05f, 6},
            {"TINY",        "Shrinks the paddle for a short time.",                  &m.assets.minus,     glm::vec3(1.0f, 0.95f, 0.25f), 1.00f, 7},
        };

        int idx = m.state.powerupInspectIndex;
        if (entries.empty()) idx = 0;
        else {
            if (idx < 0) idx = 0;
            if (idx >= (int)entries.size()) idx = (int)entries.size() - 1;
        }
        const Entry& e = entries[idx];

        // Left: model viewport
        {
            const auto& R = OL.modelRect;
            // Model (slow spin + user rotation) — rendered "on the background" (no inner box),
            // clipped to the left half so it feels like the PLAY screen card style.
            float cx = R.x + R.w * 0.5f;
            float cy = R.y + R.h * 0.60f;
            // Make preview models much bigger so they occupy the left half.
            float s = std::min(R.w, R.h) * 0.42f * e.scale;

            float spin = m.ctx.time.now() * 0.35f;
            float yaw = m.state.powerupInspectYaw + spin;
            float pitch = m.state.powerupInspectPitch;

            glm::mat4 M(1.0f);
            M = glm::translate(M, glm::vec3(cx, cy, 0.0f));
            M = glm::rotate(M, yaw, glm::vec3(0, 1, 0));
            M = glm::rotate(M, pitch, glm::vec3(1, 0, 0));
            M = glm::scale(M, glm::vec3(s, s, s));

            // Match in-game "TINY looks like a plank": sculpt minus into a bar (see `WorldRender.cpp`).
            if (std::string(e.name) == "TINY") {
                M = glm::scale(M, glm::vec3(2.35f, 0.22f, 0.60f));
            }

            // Important: UI pass disables depth test, which makes complex meshes look "inside-out".
            // Enable depth + clip to the model rect just for this draw.
            m.ctx.renderer.uiSetScissor(true, R.x, R.y, R.w, R.h);
            m.ctx.renderer.uiSetDepthTest(true, true);
            m.ctx.renderer.drawMesh(*e.mesh, M, e.tint);
            m.ctx.renderer.uiSetDepthTest(false, false);
            m.ctx.renderer.uiSetScissor(false);
        }

        // Right: info widget (name, description, video placeholder, arrows)
        {
            const auto& R = OL.infoRect;
            // Keep this as the "second widget" (readable card).
            m.ctx.renderer.drawUIQuad(R.x, R.y, R.w, R.h, glm::vec4(0.06f, 0.06f, 0.10f, 0.92f));

            // Header
            float x = R.x + 16.0f * m.uiS;
            float y = R.y + R.h - 58.0f * m.uiS;
            float titleScale = 1.55f * m.uiS;
            m.ctx.renderer.drawUIText(x, y, e.name, titleScale, glm::vec3(1.0f, 1.0f, 1.0f));
            m.ctx.renderer.drawUIQuad(x, y - 10.0f * m.uiS, std::min(R.w - 28.0f * m.uiS, 340.0f * m.uiS), 3.0f * m.uiS, glm::vec4(e.tint, 1.0f));

            // Description (wrapped)
            float descScale = 1.10f * m.uiS;
            float descY = y - 62.0f * m.uiS;
            ui::drawWrappedText(m.ctx.renderer, x, descY, R.w - 28.0f * m.uiS, e.desc, descScale, glm::vec4(0.86f, 0.94f, 1.0f, 0.92f), 6.0f * m.uiS);

            // Video placeholder
            const auto& V = OL.videoRect;
            // "Video" preview: if extracted frame textures exist, play them; otherwise show placeholder.
            m.ctx.renderer.drawUIQuad(V.x, V.y, V.w, V.h, glm::vec4(0.02f, 0.02f, 0.04f, 0.90f));
            float bt = 2.0f;
            m.ctx.renderer.drawUIQuad(V.x - bt, V.y - bt, V.w + 2*bt, bt, glm::vec4(1,1,1,0.12f));
            m.ctx.renderer.drawUIQuad(V.x - bt, V.y + V.h, V.w + 2*bt, bt, glm::vec4(1,1,1,0.12f));
            m.ctx.renderer.drawUIQuad(V.x - bt, V.y, bt, V.h, glm::vec4(1,1,1,0.12f));
            m.ctx.renderer.drawUIQuad(V.x + V.w, V.y, bt, V.h, glm::vec4(1,1,1,0.12f));

            const auto& gif = m.assets.powerupVideo(e.videoIdx);
            if (!gif.empty()) {
                const auto& fr = gif.frameAtTime(m.ctx.time.now());
                m.ctx.renderer.drawUIQuad(V.x, V.y, V.w, V.h, glm::vec4(1,1,1,1), fr.id);
            } else {
                std::string ph = m.assets.powerupVideoPaths[e.videoIdx].empty()
                    ? "VIDEO (gif not set)"
                    : "VIDEO (loading...)";
                float phS = 0.72f * m.uiS;
                float phW = m.ctx.renderer.measureUITextWidth(ph, phS);
                float phH = m.ctx.renderer.getUIFontLineHeight(phS);
                m.ctx.renderer.drawUIText(V.x + (V.w - phW) * 0.5f, V.y + (V.h - phH) * 0.5f, ph, phS, glm::vec4(1,1,1,0.55f));
                std::string hint = "assets/video/*.gif";
                float hs = 0.55f * m.uiS;
                float hw = m.ctx.renderer.measureUITextWidth(hint, hs);
                m.ctx.renderer.drawUIText(V.x + (V.w - hw) * 0.5f, V.y + 16.0f * m.uiS, hint, hs, glm::vec4(1,1,1,0.45f));
            }

            // RGB border on the right widget (requested)
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

            // Nav arrows
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

            drawNav(OL.navLeft, "<", m.state.hoveredPowerupNav == 0);
            drawNav(OL.navRight, ">", m.state.hoveredPowerupNav == 1);

            // Index indicator
            std::string ind = std::to_string(idx + 1) + " / " + std::to_string((int)entries.size());
            float indS = 0.70f * m.uiS;
            float indW = m.ctx.renderer.measureUITextWidth(ind, indS);
            m.ctx.renderer.drawUIText(R.x + (R.w - indW) * 0.5f, OL.navLeft.y + 16.0f * m.uiS, ind, indS, glm::vec4(1,1,1,0.70f));
        }
    }

    if (m.state.instructionsTab == 2) {
        // --- ROGUE CARDS BROWSER ---
        // Group all cards into Powerups / Modifiers / OP.
        std::vector<game::rogue::RogueCardId> powerups;
        std::vector<game::rogue::RogueCardId> modifiers;
        std::vector<game::rogue::RogueCardId> ops;
        for (auto id : game::rogue::allCardIds()) {
            const auto& def = game::rogue::cardDef(id);
            if (def.isOp) ops.push_back(id);
            else if (game::rogue::isPowerupCard(id)) powerups.push_back(id);
            else modifiers.push_back(id);
        }

        // Content region = union of model+info rects.
        float contentX = OL.modelRect.x;
        float contentY = OL.modelRect.y;
        float contentW = (OL.infoRect.x + OL.infoRect.w) - contentX;
        float contentH = OL.modelRect.h;

        float gap = 22.0f * m.uiS;
        float colW = (contentW - 2.0f * gap) / 3.0f;
        float colH = contentH;
        float colX0 = contentX;
        float colX1 = contentX + colW + gap;
        float colX2 = contentX + (colW + gap) * 2.0f;

        auto drawColHeader = [&](float x, const char* title, glm::vec3 tint) {
            float s = 1.05f * m.uiS;
            float tw = m.ctx.renderer.measureUITextWidth(title, s);
            m.ctx.renderer.drawUIText(x + (colW - tw) * 0.5f, contentY + colH - 34.0f * m.uiS, title, s, tint);
        };

        drawColHeader(colX0, "POWERUPS", glm::vec3(0.35f, 0.85f, 1.0f));
        drawColHeader(colX1, "MODIFIERS", glm::vec3(0.85f, 0.35f, 1.0f));
        drawColHeader(colX2, "OP", glm::vec3(1.0f, 0.80f, 0.15f));

        int base0 = 0;
        int base1 = base0 + (int)powerups.size();
        int base2 = base1 + (int)modifiers.size();

        auto drawListScrollable = [&](float x, float scrollPx,
                                     const std::vector<game::rogue::RogueCardId>& ids, int baseIndex) {
            float padX = 10.0f * m.uiS;
            float itemH = 54.0f * m.uiS;
            float itemGap = 10.0f * m.uiS;
            float step = itemH + itemGap;
            float viewTopY = contentY + colH - 80.0f * m.uiS;
            float viewBottomY = contentY + 30.0f * m.uiS;
            float viewH = std::max(1.0f, viewTopY - viewBottomY);
            float totalH = std::max(0.0f, (float)ids.size() * step - itemGap);
            float maxScroll = std::max(0.0f, totalH - viewH);
            if (scrollPx < 0.0f) scrollPx = 0.0f;
            if (scrollPx > maxScroll) scrollPx = maxScroll;

            float bt = 3.0f * m.uiS;
            m.ctx.renderer.uiSetScissor(true, x - bt, viewBottomY - bt, colW + 2.0f * bt, viewH + 2.0f * bt);

            float y = viewTopY + scrollPx;
            for (size_t i = 0; i < ids.size(); ++i) {
                if ((y - itemH) > (viewTopY + itemH)) { y -= step; continue; }
                if (y < viewBottomY) break;

                int idx = baseIndex + (int)i;
                bool hovered = (m.state.hoveredRogueCardsItem == idx) && !m.state.rogueCardsInspectOpen;
                bool selected = (m.state.rogueCardsSelected == ids[i]);
                glm::vec3 acc = game::rogue::cardAccent(ids[i]);
                glm::vec3 neonAcc = glm::clamp(acc * 1.55f + glm::vec3(0.08f), glm::vec3(0.0f), glm::vec3(1.0f));

                glm::vec4 bg(0.06f, 0.06f, 0.10f, hovered ? 0.96f : 0.88f);
                m.ctx.renderer.drawUIQuad(x, y - itemH, colW, itemH, bg);

                float hue = std::fmod(0.56f + 0.08f * std::sin(m.ctx.time.now() * 1.2f), 1.0f);
                glm::vec3 neon = ui::hsv2rgb(hue, 0.85f, 1.0f);
                glm::vec4 border = (hovered || selected)
                    ? glm::vec4(neon, 1.0f)
                    : glm::vec4(neonAcc, 0.92f);
                m.ctx.renderer.drawUIQuad(x - bt, y - itemH - bt, colW + 2 * bt, bt, border);
                m.ctx.renderer.drawUIQuad(x - bt, y, colW + 2 * bt, bt, border);
                m.ctx.renderer.drawUIQuad(x - bt, y - itemH, bt, itemH, border);
                m.ctx.renderer.drawUIQuad(x + colW, y - itemH, bt, itemH, border);

                const auto& def = game::rogue::cardDef(ids[i]);
                std::string nm = def.name;
                float nameScale = ui::fitScaleToWidth(m.ctx.renderer, nm, 0.90f * m.uiS, colW - 2.0f * padX);
                float th = m.ctx.renderer.getUIFontLineHeight(nameScale);
                m.ctx.renderer.drawUIText(x + padX, (y - itemH) + (itemH - th) * 0.5f, nm, nameScale, glm::vec3(1.0f));

                y -= step;
            }

            m.ctx.renderer.uiSetScissor(false);
        };

        drawListScrollable(colX0, m.state.rogueCardsScrollPowerups, powerups, base0);
        drawListScrollable(colX1, m.state.rogueCardsScrollModifiers, modifiers, base1);
        drawListScrollable(colX2, m.state.rogueCardsScrollOp, ops, base2);

        if (m.state.rogueCardsInspectOpen) {
            m.ctx.renderer.drawUIQuad(0, 0, (float)m.ctx.fbW, (float)m.ctx.fbH, glm::vec4(0, 0, 0, 0.82f));

            // Big centered card (menu inspector)
            float cardWBase = 420.0f;
            float cardHBase = 900.0f;
            float s = std::min((float)m.ctx.fbW / (cardWBase + 120.0f), (float)m.ctx.fbH / (cardHBase + 160.0f));
            s = std::max(0.65f, std::min(1.45f, s));
            float cardW = cardWBase * s;
            float cardH = cardHBase * s;
            float cx = ((float)m.ctx.fbW - cardW) * 0.5f;
            float cy = ((float)m.ctx.fbH - cardH) * 0.5f;

            game::ui::Rect r{cx, cy, cardW, cardH};
            auto id = m.state.rogueCardsSelected;
            const auto& def = game::rogue::cardDef(id);
            glm::vec3 acc = game::rogue::cardAccent(id);

            m.ctx.renderer.drawUIQuad(r.x + 14.0f, r.y - 14.0f, r.w, r.h, glm::vec4(0, 0, 0, 0.66f));
            m.ctx.renderer.drawUIQuad(r.x, r.y, r.w, r.h, glm::vec4(0.08f, 0.08f, 0.14f, 0.98f));
            float bt = 5.0f;
            m.ctx.renderer.drawUIQuad(r.x - bt, r.y - bt, r.w + 2 * bt, bt, glm::vec4(acc, 1.0f));
            m.ctx.renderer.drawUIQuad(r.x - bt, r.y + r.h, r.w + 2 * bt, bt, glm::vec4(acc, 1.0f));
            m.ctx.renderer.drawUIQuad(r.x - bt, r.y, bt, r.h, glm::vec4(acc, 1.0f));
            m.ctx.renderer.drawUIQuad(r.x + r.w, r.y, bt, r.h, glm::vec4(acc, 1.0f));

            std::string nm = def.name;
            float nameScale = ui::fitScaleToWidth(m.ctx.renderer, nm, 1.85f, r.w - 70.0f);
            float nw = m.ctx.renderer.measureUITextWidth(nm, nameScale);
            float nh = m.ctx.renderer.getUIFontLineHeight(nameScale);
            float nx = r.x + (r.w - nw) * 0.5f;
            float ny = r.y + r.h - nh - 34.0f;
            m.ctx.renderer.drawUIText(nx, ny, nm, nameScale, glm::vec3(1, 1, 1));

            std::string desc = def.shortDesc;
            float pad = 40.0f;
            float dScale = 1.25f;
            ui::drawWrappedText(m.ctx.renderer, r.x + pad, ny - 150.0f, r.w - 2.0f * pad, desc, dScale,
                                glm::vec4(0.93f, 0.97f, 1.0f, 0.95f), 12.0f);

            std::string hint = "CLICK OUTSIDE TO CLOSE";
            float hS = 0.62f;
            float hw = m.ctx.renderer.measureUITextWidth(hint, hS);
            m.ctx.renderer.drawUIText(r.x + (r.w - hw) * 0.5f, r.y + 26.0f, hint, hS, glm::vec3(0.8f, 0.9f, 1.0f));
        }
    }

    // BACK button (bottom-left; replaces the old X)
    {
        // Match the PLAY_MODES back button style.
        bool backHover = m.state.hoveredCloseButton;
        float uiS = m.L.uiScale;
        float bx = OL.backBtn.x, by = OL.backBtn.y, bw = OL.backBtn.w, bh = OL.backBtn.h;

        float sh = (backHover ? 6.0f : 3.0f) * uiS;
        m.ctx.renderer.drawUIQuad(bx + sh, by - sh, bw, bh, glm::vec4(0,0,0, backHover ? 0.55f : 0.45f));
        glm::vec3 base(0.10f, 0.10f, 0.16f);
        glm::vec3 col = base * (backHover ? 1.25f : 1.0f);
        m.ctx.renderer.drawUIQuad(bx, by, bw, bh, glm::vec4(col, 0.95f));

        // Neon border (subtle, matches menu vibe)
        float bt = 2.5f * uiS;
        float hue = std::fmod(0.56f + 0.08f * std::sin(m.ctx.time.now() * 1.2f), 1.0f);
        glm::vec3 neon = ui::hsv2rgb(hue, 0.85f, 1.0f);
        glm::vec4 bcol(neon, backHover ? 1.0f : 0.75f);
        m.ctx.renderer.drawUIQuad(bx - bt, by - bt, bw + 2*bt, bt, bcol);
        m.ctx.renderer.drawUIQuad(bx - bt, by + bh, bw + 2*bt, bt, bcol);
        m.ctx.renderer.drawUIQuad(bx - bt, by, bt, bh, bcol);
        m.ctx.renderer.drawUIQuad(bx + bw, by, bt, bh, bcol);

        std::string back = "< BACK";
        float bs = (bh / 56.0f) * (0.78f * uiS);
        float tw = m.ctx.renderer.measureUITextWidth(back, bs);
        float th = m.ctx.renderer.getUIFontLineHeight(bs);
        float tx = bx + (bw - tw) * 0.5f - 6.0f * uiS;
        float ty = by + (bh - th) * 0.5f;
        m.ctx.renderer.drawUIText(tx + 1.0f * uiS, ty - 1.0f * uiS, back, bs, glm::vec4(0,0,0,0.55f));
        m.ctx.renderer.drawUIText(tx, ty, back, bs, glm::vec4(1,1,1,1));
    }
}

} // namespace game::render::menu


