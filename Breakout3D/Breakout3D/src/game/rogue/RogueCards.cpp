/**
 * @file RogueCards.cpp
 * @brief Sistema de cartas do modo Rogue:
 *  - Catálogo (defs) + cores/abbrevs
 *  - Pools por run (normais vs OP)
 *  - Deal de offers com gating por wave
 *  - Aplicação de efeitos quando o jogador escolhe uma carta
 *
 * Notas rápidas:
 *  - "offer" não remove cartas do pool; só a carta PICKED é removida.
 *  - As cartas Powerup adicionam ao deck de drops E dão 1 uso imediato.
 *  - As OP são mais raras e são gated para não rebentar o early-game.
 */
 #include "game/rogue/RogueCards.hpp"

 #include "game/systems/PowerUpSystem.hpp"
 
 #include <algorithm>
 #include <cstdlib>
 
 namespace game::rogue {
 
 // ---------- Card metadata ----------
 // kDefs é o "catálogo" de cartas: id + nome + descrição curta + flag isOp.
 // NOTA: cardDef() faz lookup linear (pool pequeno, ok).
 static RogueCardDef kDefs[] = {
     // Powerup cards
     {RogueCardId::PU_EXPAND, "EXPAND", "Adds EXPAND to your deck.\nGrants it once now.", false},
     {RogueCardId::PU_EXTRA_BALL, "EXTRA BALL", "Adds EXTRA_BALL to your deck.\nGrants it once now.", false},
     {RogueCardId::PU_EXTRA_LIFE, "EXTRA LIFE", "Adds EXTRA_LIFE to your deck.\nGrants it once now.", false},
     {RogueCardId::PU_FIREBALL, "FIREBALL", "Adds FIREBALL to your deck.\nGrants it once now.", false},
     {RogueCardId::PU_SHIELD, "SHIELD", "Adds SHIELD to your deck.\nGrants it once now.", false},
     {RogueCardId::PU_SLOW, "SLOW", "Adds SLOW to your deck.\nGrants it once now.", false},
     {RogueCardId::PU_REVERSE, "REVERSE", "Adds REVERSE to your deck.\nGrants it once now.", false},
     {RogueCardId::PU_TINY, "TINY", "Adds TINY to your deck.\nGrants it once now.", false},
 
     // Normal modifiers (trade-offs)
     {RogueCardId::MOD_WIDE_PADDLE_SLOW, "WIDE PADDLE / SLOW", "+Paddle width.\n-Paddle speed.", false},
     {RogueCardId::MOD_WIDE_PADDLE_LIGHT_BALL, "WIDE PADDLE / LIGHT BALL", "+Paddle width.\n-Slower balls.", false},
     {RogueCardId::MOD_FAST_PADDLE_TINY_PADDLE, "FAST PADDLE / TINY PADDLE", "+Paddle speed.\n-Smaller paddle.", false},
     {RogueCardId::MOD_FAST_BALL_SLOW_PADDLE, "FAST BALL / SLOW PADDLE", "+Ball speed.\n-Slower paddle.", false},
     {RogueCardId::MOD_LUCKY_DROPS_BRITTLE, "LUCKY DROPS / BRITTLE", "+Drop chance.\n-Lose 1 life now.", false},
     {RogueCardId::MOD_LUCKY_DROPS_SLOW_BALL, "LUCKY DROPS / SLOW BALL", "+Drop chance.\n-Slower balls.", false},
     {RogueCardId::MOD_BONUS_LIFE_TAX, "BONUS LIFE / TAX", "+1 life now.\n-Bank penalty.", false},
     {RogueCardId::MOD_GLASS_CANNON, "GLASS CANNON", "+Drop chance + ball speed.\n+Harsher life loss.", false},
     {RogueCardId::MOD_STREAK_GREED, "STREAK GREED", "+Brick points.\n-Faster bank commit.", false},
     {RogueCardId::MOD_SAFE_BANKER, "SAFE BANKER", "+Safer banking.\n-Lower brick points.", false},
     {RogueCardId::MOD_SHIELD_LONG_SLOW, "SHIELD LONG / SLOW", "+Longer shields.\n-Slower paddle.", false},
     {RogueCardId::MOD_FIREBALL_WIDE_SLOW, "FIREBALL WIDE / SLOW", "+Bigger explosions.\n-Slower paddle.", false},
 
     // More normal modifiers (environment-ish)
     {RogueCardId::MOD_WIND_RANDOM, "WIND: CHAOTIC", "Balls are pushed left or right randomly.\n+Drops, -paddle speed.", false},
     {RogueCardId::MOD_CENTERED_ARENA, "CENTERED ARENA", "You can't hug the walls.\n+Points, -drops.", false},
     {RogueCardId::MOD_STICKY_PADDLE, "STICKY PADDLE", "Balls stick to the paddle on hit.\n-Slower balls.", false},
     {RogueCardId::MOD_SCORE_FARM, "SCORE FARM", "Higher brick points.\nLower drop chance.", false},
     {RogueCardId::MOD_CURSE_ENGINE, "CURSE ENGINE", "Higher drop chance.\nAdds curses into your deck.", false},
     {RogueCardId::MOD_SHOCK_ABSORB, "SHOCK ABSORB", "Banking is safer.\n-Slower paddle.", false},
     {RogueCardId::MOD_RUSH_MODE, "RUSH MODE", "Faster balls + more drops.\nHarsher life loss.", false},
     {RogueCardId::MOD_FRAIL_PADDLE, "FRAIL PADDLE", "Smaller paddle.\n+Drops + speed.", false},
     // Promoted to OP (no downside): fewer rows per wave is a big pacing lever.
     {RogueCardId::MOD_ROW_BARGAIN, "ROW CONTROL", "Fewer rows are added per wave.", true},
 
     // OP (no downsides)
     {RogueCardId::OP_FIREBALL_MASTERY, "FIREBALL MASTERY", "Fireball is empowered.\nBigger explosions.", true},
     {RogueCardId::OP_SHIELD_GENERATOR, "SHIELD GENERATOR", "Shield is empowered.\nLonger duration.", true},
     {RogueCardId::OP_LUCK_ENGINE, "LUCK ENGINE", "Massive drop chance boost.", true},
     {RogueCardId::OP_TITAN_PADDLE, "TITAN PADDLE", "Massive base paddle width.", true},
     {RogueCardId::OP_OVERDRIVE, "OVERDRIVE", "Huge paddle + ball speed boost.", true},
     {RogueCardId::OP_THREE_BALL_START, "THREE BALL START", "Spawns extra balls now.\nImproves multiball.", true},
     {RogueCardId::OP_HEART_STOCKPILE, "HEART STOCKPILE", "+2 lives now.", true},
     {RogueCardId::OP_BANKER_S_UPSIDE, "BANKER'S UPSIDE", "Safer banking + more points.", true},
     {RogueCardId::OP_PIERCE_TRAINING, "PIERCE TRAINING", "Bricks take +1 damage.\n(Not one-shot.)", true},
     {RogueCardId::OP_ROW_CONTROL, "ROW CONTROL", "Reduces rows added per wave.", true},
 };
 
 const RogueCardDef& cardDef(RogueCardId id) {
     for (const auto& d : kDefs) {
         if (d.id == id) return d;
     }
     // fallback seguro (nunca devia acontecer se enum + catálogo estiverem alinhados)
     return kDefs[0];
 }
 
 glm::vec3 cardAccent(RogueCardId id) {
     // Linguagem de cor:
     // - OP = dourado
     // - Powerups = azul
     // - Trade-offs/modifiers = roxo
     const auto& d = cardDef(id);
     if (d.isOp) return glm::vec3(1.0f, 0.80f, 0.15f);
 
     if (isPowerupCard(id)) {
         return glm::vec3(0.25f, 0.75f, 0.95f);
     }
 
     return glm::vec3(0.75f, 0.25f, 0.85f);
 }
 
 std::string cardAbbrev(RogueCardId id) {
     // Badge curto para HUD/menus (2-3 chars).
     switch (id) {
         case RogueCardId::PU_EXTRA_LIFE: return "HP";
         case RogueCardId::PU_EXTRA_BALL: return "EX";
         case RogueCardId::PU_EXPAND: return "XP";
         case RogueCardId::PU_FIREBALL: return "FB";
         case RogueCardId::PU_SHIELD: return "SH";
         case RogueCardId::PU_SLOW: return "SL";
         case RogueCardId::PU_REVERSE: return "RV";
         case RogueCardId::PU_TINY: return "TN";
         case RogueCardId::OP_FIREBALL_MASTERY: return "OPF";
         case RogueCardId::OP_SHIELD_GENERATOR: return "OPS";
         case RogueCardId::OP_LUCK_ENGINE: return "LCK";
         case RogueCardId::OP_TITAN_PADDLE: return "TIT";
         case RogueCardId::OP_OVERDRIVE: return "OVR";
         case RogueCardId::OP_THREE_BALL_START: return "3B";
         case RogueCardId::OP_HEART_STOCKPILE: return "HRT";
         case RogueCardId::OP_BANKER_S_UPSIDE: return "BNK";
         case RogueCardId::OP_PIERCE_TRAINING: return "PRC";
         case RogueCardId::OP_ROW_CONTROL: return "ROW";
         case RogueCardId::MOD_ROW_BARGAIN: return "ROW";
         default: return "MOD";
     }
 }
 
 const std::vector<RogueCardId>& allCardIds() {
     // Cache para UI: lista dos ids em ordem do catálogo.
     static std::vector<RogueCardId> ids = []() {
         std::vector<RogueCardId> out;
         out.reserve(sizeof(kDefs) / sizeof(kDefs[0]));
         for (const auto& d : kDefs) out.push_back(d.id);
         return out;
     }();
     return ids;
 }
 
 bool isPowerupCard(RogueCardId id) {
     switch (id) {
         case RogueCardId::PU_EXPAND:
         case RogueCardId::PU_EXTRA_BALL:
         case RogueCardId::PU_EXTRA_LIFE:
         case RogueCardId::PU_FIREBALL:
         case RogueCardId::PU_SHIELD:
         case RogueCardId::PU_SLOW:
         case RogueCardId::PU_REVERSE:
         case RogueCardId::PU_TINY:
             return true;
         default:
             return false;
     }
 }
 
 // ---------- Pools / dealing ----------
 // pushAll() monta o pool (normal ou OP) a partir do catálogo.
 static void pushAll(std::vector<RogueCardId>& out, bool wantOp) {
     out.clear();
     for (const auto& d : kDefs) {
         if (d.isOp == wantOp) out.push_back(d.id);
     }
 }
 
 void initRunPools(GameState& state) {
     // Estado de UI/flow do draft
     state.rogueChosen.clear();
     state.rogueDropDeck.clear();
     state.rogueOfferCount = 0;
     state.hoveredRogueCard = -1;
     state.hoveredRogueCardPickButton = -1;
 
     // Reset de modificadores do Rogue (ficam aplicados ao longo da run)
     state.rogueDropChanceMult = 1.0f;
     state.rogueBasePaddleScaleX = 1.0f;
     state.roguePaddleSpeedMult = 1.0f;
     state.rogueBallSpeedMult = 1.0f;
     state.rogueBrickPointsMult = 1.0f;
     state.rogueBankIdleMult = 1.0f;
     state.rogueBrickDamageBonus = 0;
     state.rogueFireballRadiusMult = 1.0f;
     state.rogueShieldDurationMult = 1.0f;
     state.rogueLifeLossPenaltyBonus = 0;
     state.rogueRowsPerWaveDelta = 0;
     state.rogueWindX = 0.0f;
     state.roguePaddleClampMarginX = 0.0f;
     state.rogueStickyPaddle = false;
     state.rogueRandomWindActive = false;
     state.rogueRandomWindTimer = 0.0f;
 
     // Pools por run (cartas disponíveis para oferecer)
     pushAll(state.rogueRemainingNormal, false);
     pushAll(state.rogueRemainingOp, true);
 }
 
 // Gating: evita cartas fortes cedo demais (balance/pacing).
 static bool isEligibleForWave(RogueCardId id, bool opPack, int waveProgress) {
     // waveProgress = wave já "limpa" (1..N)
     waveProgress = std::max(1, waveProgress);
 
     if (opPack) {
         // OP packs não aparecem cedo; multiball OP ainda mais tarde.
         if (id == RogueCardId::OP_THREE_BALL_START) return waveProgress >= 9;
         return waveProgress >= 3;
     }
 
     // Normal packs: trava power spikes cedo
     if (id == RogueCardId::PU_EXTRA_BALL) return waveProgress >= 4;
     if (id == RogueCardId::PU_FIREBALL) return waveProgress >= 6;
 
     // Penalidades/score/bank: não meter no draft inicial wave-1
     if (id == RogueCardId::MOD_BONUS_LIFE_TAX) return waveProgress >= 2;
     if (id == RogueCardId::MOD_RUSH_MODE) return waveProgress >= 5;
     if (id == RogueCardId::MOD_GLASS_CANNON) return waveProgress >= 5;
     if (id == RogueCardId::MOD_CURSE_ENGINE) return waveProgress >= 3;
 
     return true;
 }
 
 void dealOffer(GameState& state, int count, bool opPack, int waveProgress) {
     auto& pool = opPack ? state.rogueRemainingOp : state.rogueRemainingNormal;
 
     state.rogueOfferCount = 0;
     state.hoveredRogueCard = -1;
     state.hoveredRogueCardPickButton = -1;
 
     // Oferta:
     // - NÃO remove do pool só por aparecer.
     // - Garante cartas únicas dentro da oferta (partial shuffle).
     std::vector<RogueCardId> eligible;
     eligible.reserve(pool.size());
     for (auto id : pool) {
         if (isEligibleForWave(id, opPack, waveProgress)) eligible.push_back(id);
     }
 
     int n = std::min(count, (int)eligible.size());
     if (n <= 0) return;
 
     std::vector<RogueCardId> tmp = eligible;
     for (int i = 0; i < n; ++i) {
         int j = i + (rand() % (int)(tmp.size() - i));
         std::swap(tmp[(size_t)i], tmp[(size_t)j]);
 
         state.rogueOffer[i] = tmp[(size_t)i];
         state.rogueOfferCount++;
     }
 }
 
 void startInitialDraft(GameState& state) {
     // Draft inicial: 3 rondas de 3 cartas, antes de começar a jogar.
     state.rogueStartingDraftRoundsLeft = 3;
     dealOffer(state, 3, /*opPack=*/false, /*waveProgress=*/1);
     state.mode = GameMode::ROGUE_CARDS;
 }
 
 // ---------- Effects ----------
 // Add ao deck de drops, sem duplicados.
 static void addDrop(GameState& state, PowerUpType t) {
     if (std::find(state.rogueDropDeck.begin(), state.rogueDropDeck.end(), t) == state.rogueDropDeck.end()) {
         state.rogueDropDeck.push_back(t);
     }
 }
 
 // Guarda histórico de picks (para HUD/inspector/debug).
 static void markChosen(GameState& state, RogueCardId id) {
     state.rogueChosen.push_back(id);
 }
 
 // Powerup card: adiciona ao deck + dá o powerup agora (efeito imediato).
 static void applyPowerupCard(GameState& state, const GameConfig& cfg, PowerUpType t) {
     addDrop(state, t);
     PowerUpSystem::applyPowerUpEffect(state, cfg, t);
 }
 
 void applyPickedCard(GameState& state, const GameConfig& cfg, RogueCardId picked) {
     // Regista escolhido
     markChosen(state, picked);
 
     // Remove do pool para nunca mais voltar a aparecer nesta run.
     {
         const auto& def = cardDef(picked);
         auto& pool = def.isOp ? state.rogueRemainingOp : state.rogueRemainingNormal;
         pool.erase(std::remove(pool.begin(), pool.end(), picked), pool.end());
     }
 
     // Aplica efeitos (mutadores que ficam “sempre on” nesta run + grants imediatos)
     switch (picked) {
         // Powerup cards
         case RogueCardId::PU_EXPAND: applyPowerupCard(state, cfg, PowerUpType::EXPAND); break;
         case RogueCardId::PU_EXTRA_BALL: applyPowerupCard(state, cfg, PowerUpType::EXTRA_BALL); break;
         case RogueCardId::PU_EXTRA_LIFE: applyPowerupCard(state, cfg, PowerUpType::EXTRA_LIFE); break;
         case RogueCardId::PU_FIREBALL: applyPowerupCard(state, cfg, PowerUpType::FIREBALL); break;
         case RogueCardId::PU_SHIELD: applyPowerupCard(state, cfg, PowerUpType::SHIELD); break;
         case RogueCardId::PU_SLOW: applyPowerupCard(state, cfg, PowerUpType::SLOW); break;
         case RogueCardId::PU_REVERSE: applyPowerupCard(state, cfg, PowerUpType::REVERSE); break;
         case RogueCardId::PU_TINY: applyPowerupCard(state, cfg, PowerUpType::TINY); break;
 
         // Normal modifiers (trade-offs)
         case RogueCardId::MOD_WIDE_PADDLE_SLOW:
             state.rogueBasePaddleScaleX *= 1.35f;
             state.roguePaddleSpeedMult *= 0.88f;
             break;
         case RogueCardId::MOD_WIDE_PADDLE_LIGHT_BALL:
             state.rogueBasePaddleScaleX *= 1.30f;
             state.rogueBallSpeedMult *= 0.90f;
             break;
         case RogueCardId::MOD_FAST_PADDLE_TINY_PADDLE:
             state.roguePaddleSpeedMult *= 1.25f;
             state.rogueBasePaddleScaleX *= 0.82f;
             break;
         case RogueCardId::MOD_FAST_BALL_SLOW_PADDLE:
             state.rogueBallSpeedMult *= 1.18f;
             state.roguePaddleSpeedMult *= 0.85f;
             break;
         case RogueCardId::MOD_LUCKY_DROPS_BRITTLE:
             state.rogueDropChanceMult *= 1.22f;
             state.lives = std::max(0, state.lives - 1);
             break;
         case RogueCardId::MOD_LUCKY_DROPS_SLOW_BALL:
             state.rogueDropChanceMult *= 1.25f;
             state.rogueBallSpeedMult *= 0.88f;
             break;
         case RogueCardId::MOD_BONUS_LIFE_TAX:
             state.lives += 1;
             // Penalidade de bank (usa o mesmo sistema do Endless)
             state.endlessStreakPoints -= 300;
             state.endlessStreakNegPoints += 300;
             state.endlessStreakIdleTimer = 0.0f;
             state.endlessStreakBanking = false;
             state.endlessStreakBankTimer = 0.0f;
             break;
         case RogueCardId::MOD_GLASS_CANNON:
             state.rogueDropChanceMult *= 1.18f;
             state.rogueBallSpeedMult *= 1.12f;
             state.rogueLifeLossPenaltyBonus += 140;
             break;
         case RogueCardId::MOD_STREAK_GREED:
             state.rogueBrickPointsMult *= 1.18f;
             state.rogueBankIdleMult *= 0.75f; // commits mais rápido
             break;
         case RogueCardId::MOD_SAFE_BANKER:
             state.rogueBrickPointsMult *= 0.88f;
             state.rogueBankIdleMult *= 1.35f; // banking mais “safe”
             break;
         case RogueCardId::MOD_SHIELD_LONG_SLOW:
             state.rogueShieldDurationMult *= 1.35f;
             state.roguePaddleSpeedMult *= 0.90f;
             break;
         case RogueCardId::MOD_FIREBALL_WIDE_SLOW:
             state.rogueFireballRadiusMult *= 1.35f;
             state.roguePaddleSpeedMult *= 0.90f;
             break;
 
         // Environment / rules
         case RogueCardId::MOD_WIND_RANDOM:
             state.rogueRandomWindActive = true;
             state.rogueRandomWindTimer = 0.0f;
             state.rogueDropChanceMult *= 1.08f;
             state.roguePaddleSpeedMult *= 0.92f;
             break;
         case RogueCardId::MOD_CENTERED_ARENA:
             state.roguePaddleClampMarginX += 3.25f;
             state.rogueBrickPointsMult *= 1.15f;
             state.rogueDropChanceMult *= 0.85f;
             break;
         case RogueCardId::MOD_STICKY_PADDLE:
             state.rogueStickyPaddle = true;
             state.rogueBallSpeedMult *= 0.88f;
             break;
         case RogueCardId::MOD_SCORE_FARM:
             state.rogueBrickPointsMult *= 1.28f;
             state.rogueDropChanceMult *= 0.78f;
             break;
         case RogueCardId::MOD_CURSE_ENGINE:
             state.rogueDropChanceMult *= 1.20f;
             // “injetar” curses no deck de drops
             addDrop(state, PowerUpType::SLOW);
             addDrop(state, PowerUpType::REVERSE);
             addDrop(state, PowerUpType::TINY);
             break;
         case RogueCardId::MOD_SHOCK_ABSORB:
             state.rogueBankIdleMult *= 1.55f;
             state.roguePaddleSpeedMult *= 0.92f;
             break;
         case RogueCardId::MOD_RUSH_MODE:
             state.rogueBallSpeedMult *= 1.18f;
             state.rogueDropChanceMult *= 1.10f;
             state.rogueLifeLossPenaltyBonus += 220;
             break;
         case RogueCardId::MOD_FRAIL_PADDLE:
             state.rogueBasePaddleScaleX *= 0.80f;
             state.rogueDropChanceMult *= 1.18f;
             state.roguePaddleSpeedMult *= 1.12f;
             break;
         case RogueCardId::MOD_ROW_BARGAIN:
             // Menos rows por wave = pacing mais controlado
             state.rogueRowsPerWaveDelta -= 1;
             break;
 
         // OP (sem downsides)
         case RogueCardId::OP_FIREBALL_MASTERY:
             addDrop(state, PowerUpType::FIREBALL);
             state.rogueFireballRadiusMult *= 1.55f;
             state.rogueDropChanceMult *= 1.08f;
             break;
         case RogueCardId::OP_SHIELD_GENERATOR:
             addDrop(state, PowerUpType::SHIELD);
             state.rogueShieldDurationMult *= 1.55f;
             state.rogueDropChanceMult *= 1.06f;
             break;
         case RogueCardId::OP_LUCK_ENGINE:
             state.rogueDropChanceMult *= 1.35f;
             break;
         case RogueCardId::OP_TITAN_PADDLE:
             state.rogueBasePaddleScaleX *= 1.70f;
             break;
         case RogueCardId::OP_OVERDRIVE:
             state.roguePaddleSpeedMult *= 1.35f;
             state.rogueBallSpeedMult *= 1.25f;
             break;
         case RogueCardId::OP_THREE_BALL_START:
             addDrop(state, PowerUpType::EXTRA_BALL);
             PowerUpSystem::applyPowerUpEffect(state, cfg, PowerUpType::EXTRA_BALL);
             break;
         case RogueCardId::OP_HEART_STOCKPILE:
             state.lives += 2;
             break;
         case RogueCardId::OP_BANKER_S_UPSIDE:
             state.rogueBrickPointsMult *= 1.20f;
             state.rogueBankIdleMult *= 1.45f;
             break;
         case RogueCardId::OP_PIERCE_TRAINING:
             state.rogueBrickDamageBonus += 1;
             break;
         case RogueCardId::OP_ROW_CONTROL:
             state.rogueRowsPerWaveDelta -= 1;
             break;
     }
 
     // ---- Flow do draft inicial ----
     if (state.rogueStartingDraftRoundsLeft > 0) {
         state.rogueStartingDraftRoundsLeft--;
         if (state.rogueStartingDraftRoundsLeft > 0) {
             // Ainda no draft inicial -> mantém waveProgress=1
             dealOffer(state, 3, /*opPack=*/false, /*waveProgress=*/1);
             state.mode = GameMode::ROGUE_CARDS;
             return;
         }
     }
 
     // Sai do overlay de cartas e volta ao jogo
     state.mode = GameMode::PLAYING;
 }
 
 float effectiveDropChance(const GameState& state, const GameConfig& cfg) {
     // Base Rogue: reduz a frequência global (mesmo com deck a crescer).
     // Cap para evitar runaway com multiplicadores.
     const float rogueBase = 0.62f;
     float c = cfg.powerUpChance * rogueBase * state.rogueDropChanceMult;
     if (c < 0.0f) c = 0.0f;
     if (c > 0.68f) c = 0.68f;
     return c;
 }
 
 float basePaddleScaleX(const GameState& state) { return state.rogueBasePaddleScaleX; }
 float paddleSpeedMult(const GameState& state) { return state.roguePaddleSpeedMult; }
 float ballSpeedMult(const GameState& state) { return state.rogueBallSpeedMult; }
 
 } // namespace game::rogue
 