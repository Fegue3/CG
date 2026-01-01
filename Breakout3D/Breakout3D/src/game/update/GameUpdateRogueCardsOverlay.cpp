// GameUpdateRogueCardsOverlay.cpp
/**
 * @file GameUpdateRogueCardsOverlay.cpp
 * @brief Overlay de escolha de cartas no modo Rogue (ROGUE_CARDS).
 *
 * Responsabilidades:
 *  - Só corre quando mode == ROGUE_CARDS.
 *  - Calcula layout (rogueCardOverlay).
 *  - Atualiza hover:
 *      - hoveredRogueCard (qual carta)
 *      - hoveredRogueCardPickButton (hover no botão PICK dentro da carta)
 *  - Clique:
 *      - aplica carta (applyPickedCard)
 *      - toca SFX + stinger
 *      - se sair do overlay para PLAYING, inicia timer para spawn gradual de rows.
 *  - Hover SFX sem spam (comparação com prev).
 *
 * Nota:
 *  - Enquanto o overlay está aberto, desliga loops longos (warning loop).
 */
 #include "game/Game.hpp"

 #include "game/ui/RogueCardLayout.hpp"
 #include "game/rogue/RogueCards.hpp"
 
 namespace game {
 
 bool Game::updateRogueCardsOverlay(const engine::Input& input) {
     if (m_state.mode != GameMode::ROGUE_CARDS) return false;
 
     auto [fbW, fbH] = m_window.getFramebufferSize();
     auto [px, py_raw] = input.mousePosFbPx();
     float py = (float)fbH - py_raw;
     bool click = input.mousePressed(engine::MouseButton::Left);
 
     const auto L = game::ui::rogueCardOverlay(fbW, fbH);
 
     // Reset hover
     m_state.hoveredRogueCard = -1;
     m_state.hoveredRogueCardPickButton = -1;
 
     // Hover na carta (A/B/C) conforme offerCount
     if (m_state.rogueOfferCount > 0 && L.cardA.contains(px, py)) m_state.hoveredRogueCard = 0;
     else if (m_state.rogueOfferCount > 1 && L.cardB.contains(px, py)) m_state.hoveredRogueCard = 1;
     else if (m_state.rogueOfferCount > 2 && L.cardC.contains(px, py)) m_state.hoveredRogueCard = 2;
 
     // Hover específico no botão PICK dentro da carta
     auto checkPickButtonHover = [&](const game::ui::Rect& cardRect, int cardIndex) {
         float pbW = cardRect.w * 0.56f;
         float pbH = 76.0f;
         float pbX = cardRect.x + (cardRect.w - pbW) * 0.5f;
         float pbY = cardRect.y + 26.0f;
         game::ui::Rect pickBtn{pbX, pbY, pbW, pbH};
         if (pickBtn.contains(px, py)) {
             m_state.hoveredRogueCardPickButton = cardIndex;
         }
     };
 
     if (m_state.hoveredRogueCard == 0) checkPickButtonHover(L.cardA, 0);
     else if (m_state.hoveredRogueCard == 1) checkPickButtonHover(L.cardB, 1);
     else if (m_state.hoveredRogueCard == 2) checkPickButtonHover(L.cardC, 2);
 
     // Clique: escolher carta
     if (click) {
         if (m_state.hoveredRogueCard >= 0 && m_state.hoveredRogueCard < m_state.rogueOfferCount) {
             m_audio.playSfx("rogue/rogue_card_pick", -2.0f);
             m_audio.playStinger("stinger_card_pick", +2.0f);
 
             game::rogue::applyPickedCard(m_state, m_cfg, m_state.rogueOffer[m_state.hoveredRogueCard]);
 
             // Se o draft terminou e voltámos ao gameplay, iniciar cadência de spawn das rows.
             if (m_state.mode == GameMode::PLAYING) {
                 m_state.rogueRowSpawnTimer = 0.15f;
             }
             return true;
         }
     }
 
     // Hover SFX (sem spam)
     if (m_state.hoveredRogueCard != m_prevHoveredRogueCard ||
         m_state.hoveredRogueCardPickButton != m_prevHoveredRoguePickBtn) {
         if (m_state.hoveredRogueCard >= 0 || m_state.hoveredRogueCardPickButton >= 0) {
             m_audio.playSfx("rogue/rogue_card_hover", -7.5f);
         }
     }
 
     // Prev trackers (early return)
     m_prevMode = m_state.mode;
     m_prevGameType = m_state.gameType;
     m_prevHoveredRogueCard = m_state.hoveredRogueCard;
     m_prevHoveredRoguePickBtn = m_state.hoveredRogueCardPickButton;
 
     // Overlay UI: desligar loop longo do danger warning.
     m_audio.setSfxLoopEnabled("endless/endless_warning_loop", false, 0.12f);
     return true;
 }
 
 } // namespace game
 