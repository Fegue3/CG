#pragma once

namespace game::rogue {

// Rogue cards are unique per run (no repeats).
// Split into Normal cards (often with trade-offs) and OP cards (no downsides; shown every 3 waves).
enum class RogueCardId : int {
    // --- Powerup cards (add powerup to the drop deck; also grant once immediately) ---
    PU_EXPAND,
    PU_EXTRA_BALL,
    PU_EXTRA_LIFE,
    PU_FIREBALL,
    PU_SHIELD,
    PU_SLOW,
    PU_REVERSE,
    PU_TINY,

    // --- Normal modifier cards (mostly trade-offs) ---
    MOD_WIDE_PADDLE_SLOW,          // +paddle width, -paddle speed
    MOD_WIDE_PADDLE_LIGHT_BALL,    // +paddle width, -ball speed
    MOD_FAST_PADDLE_TINY_PADDLE,   // +paddle speed, -paddle width
    MOD_FAST_BALL_SLOW_PADDLE,     // +ball speed, -paddle speed
    MOD_LUCKY_DROPS_BRITTLE,       // +drop chance, -lives (lose 1 now)
    MOD_LUCKY_DROPS_SLOW_BALL,     // +drop chance, -ball speed
    MOD_BONUS_LIFE_TAX,            // +1 life now, -score (bank penalty)
    MOD_GLASS_CANNON,              // +ball speed, +drop chance, -life loss penalty increases
    MOD_STREAK_GREED,              // +streak gain, -streak idle commit faster (riskier)
    MOD_SAFE_BANKER,               // +streak idle time (safer), -brick points (mult)
    MOD_SHIELD_LONG_SLOW,          // +shield duration mult, -paddle speed
    MOD_FIREBALL_WIDE_SLOW,        // +fireball radius mult, -paddle speed

    // --- More normal modifier cards (environment + creative trade-offs) ---
    MOD_WIND_RANDOM,               // wind randomizes ball direction left/right (+drops, -paddle speed)
    MOD_CENTERED_ARENA,            // paddle movement clamped to center (+points, -drops)
    MOD_STICKY_PADDLE,             // balls stick on paddle hit (+control, -ball speed)
    MOD_SCORE_FARM,                // +brick points, -drop chance
    MOD_CURSE_ENGINE,              // +drop chance, adds curses into drop deck
    MOD_SHOCK_ABSORB,              // safer banking, -paddle speed
    MOD_RUSH_MODE,                 // +ball speed +drops, harsher life loss
    MOD_FRAIL_PADDLE,              // smaller paddle, +drops +paddle speed
    MOD_ROW_BARGAIN,               // fewer rows per wave, -brick points

    // --- OP cards (no downsides) ---
    OP_FIREBALL_MASTERY,           // fireball always in deck + bigger radius + more shards
    OP_SHIELD_GENERATOR,           // shield always in deck + longer duration
    OP_LUCK_ENGINE,                // big drop chance mult
    OP_TITAN_PADDLE,               // big base paddle width
    OP_OVERDRIVE,                  // big paddle+ball speed mult
    OP_THREE_BALL_START,           // spawn extra balls now and add EXTRA_BALL to deck
    OP_HEART_STOCKPILE,            // +2 lives now
    OP_BANKER_S_UPSIDE,            // streak bank commits slower (safer) + bonus points mult
    OP_PIERCE_TRAINING,            // fireball-like: bricks take +1 dmg (not one-shot)
    OP_ROW_CONTROL,                // fewer rows added each wave (global relief)
};

} // namespace game::rogue


