// RogueCardId.hpp
#pragma once

namespace game::rogue {

/**
 * @file RogueCardId.hpp
 * @brief IDs das cartas do modo Rogue (únicas por run, sem repetição).
 *
 * Estrutura:
 * - PU_*  : cartas que adicionam power-ups ao “drop deck” e também dão 1 uso imediato.
 * - MOD_* : modificadores (normalmente com trade-offs).
 * - OP_*  : cartas fortes sem downsides (tipicamente aparecem em packs especiais).
 *
 * @note
 * Os efeitos exactos (multiplicadores/valores) são aplicados em `applyPickedCard()` e vivem no state/modifiers.
 */
enum class RogueCardId : int {
    // --- Cartas de power-up (entram no deck de drops + activam uma vez de imediato) ---

    /// Adiciona EXPAND ao deck e aplica já (paddle maior).
    PU_EXPAND,

    /// Adiciona EXTRA_BALL ao deck e aplica já (spawn de bola extra).
    PU_EXTRA_BALL,

    /// Adiciona EXTRA_LIFE ao deck e aplica já (+1 vida).
    PU_EXTRA_LIFE,

    /// Adiciona FIREBALL ao deck e aplica já (bola especial one-shot com AoE).
    PU_FIREBALL,

    /// Adiciona SHIELD ao deck e aplica já (barreira temporária atrás do paddle).
    PU_SHIELD,

    /// Adiciona SLOW ao deck e aplica já (abranda bola(s) / ritmo).
    PU_SLOW,

    /// Adiciona REVERSE ao deck e aplica já (maldição: inverte controlos).
    PU_REVERSE,

    /// Adiciona TINY ao deck e aplica já (maldição: paddle encolhe).
    PU_TINY,

    // --- Cartas normais (maioria com trade-offs) ---

    /// Paddle mais largo, mas mais lento.
    MOD_WIDE_PADDLE_SLOW,

    /// Paddle mais largo, mas bola(s) mais lenta(s).
    MOD_WIDE_PADDLE_LIGHT_BALL,

    /// Paddle mais rápido, mas mais pequeno.
    MOD_FAST_PADDLE_TINY_PADDLE,

    /// Bola(s) mais rápida(s), mas paddle mais lento.
    MOD_FAST_BALL_SLOW_PADDLE,

    /// Mais drops, mas perdes 1 vida imediatamente (mais frágil).
    MOD_LUCKY_DROPS_BRITTLE,

    /// Mais drops, mas bola(s) mais lenta(s).
    MOD_LUCKY_DROPS_SLOW_BALL,

    /// Ganha 1 vida já, mas levas penalização no score (bank/score tax).
    MOD_BONUS_LIFE_TAX,

    /// “Glass cannon”: mais velocidade e drops, mas penalização de perder vida é mais severa.
    MOD_GLASS_CANNON,

    /// Mais ganho de streak, mas “commit” mais cedo quando estás idle (mais arriscado).
    MOD_STREAK_GREED,

    /// Mais tempo seguro antes do commit do bank, mas menos pontos por brick (mult menor).
    MOD_SAFE_BANKER,

    /// Shield dura mais, mas paddle mais lento.
    MOD_SHIELD_LONG_SLOW,

    /// Fireball com raio maior, mas paddle mais lento.
    MOD_FIREBALL_WIDE_SLOW,

    // --- Mais cartas normais (ambiente + trade-offs criativos) ---

    /// “Wind”: perturba direcção da bola left/right (geralmente mais caos; trade-off no controlo/paddle).
    MOD_WIND_RANDOM,

    /// Arena centrada: movimento do paddle clampado ao centro (mais pontos, mas menos drops).
    MOD_CENTERED_ARENA,

    /// Paddle “sticky”: bolas colam no paddle ao bater (mais controlo, mas bola mais lenta).
    MOD_STICKY_PADDLE,

    /// “Farm” de score: mais pontos por brick, mas menos chance de drops.
    MOD_SCORE_FARM,

    /// Motor de maldições: aumenta drops e mete curses no deck.
    MOD_CURSE_ENGINE,

    /// Absorção/segurança extra (ex.: banking mais seguro), mas paddle mais lento.
    MOD_SHOCK_ABSORB,

    /// Modo rush: bola mais rápida + mais drops, mas perder vida custa mais.
    MOD_RUSH_MODE,

    /// Paddle frágil: paddle menor, mas mais drops e/ou paddle mais rápido.
    MOD_FRAIL_PADDLE,

    /// Barganha de rows: menos filas por wave, mas menos pontos por brick.
    MOD_ROW_BARGAIN,

    // --- OP (sem downsides) ---

    /// Fireball sempre no deck + raio maior + mais “shards”/explosão mais forte.
    OP_FIREBALL_MASTERY,

    /// Shield sempre no deck + duração maior.
    OP_SHIELD_GENERATOR,

    /// “Luck engine”: multiplicador grande de chance de drops.
    OP_LUCK_ENGINE,

    /// “Titan paddle”: aumenta base do tamanho do paddle.
    OP_TITAN_PADDLE,

    /// “Overdrive”: multiplica bastante velocidade do paddle e da bola.
    OP_OVERDRIVE,

    /// Começa com 3 bolas e ainda mete EXTRA_BALL no deck.
    OP_THREE_BALL_START,

    /// +2 vidas imediatamente.
    OP_HEART_STOCKPILE,

    /// Banking mais seguro + multiplicador de pontos bonus (upside sem downside).
    OP_BANKER_S_UPSIDE,

    /// “Pierce training”: bricks levam +1 dano (não é one-shot).
    OP_PIERCE_TRAINING,

    /// “Row control”: menos filas adicionadas por wave (alívio global).
    OP_ROW_CONTROL,
};

} // namespace game::rogue
