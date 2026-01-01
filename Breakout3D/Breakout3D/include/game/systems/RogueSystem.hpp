// RogueSystem.hpp
#pragma once

#include "game/GameState.hpp"
#include "game/GameConfig.hpp"

namespace game {

/**
 * @file RogueSystem.hpp
 * @brief Helpers do modo Rogue: progressão por waves + ofertas de cartas + curva de dificuldade.
 *
 * Para já, o Rogue é essencialmente:
 * - uma run com waves (rooms) que avançam com progresso/tempo;
 * - ofertas de cartas no início e a cada N waves limpas;
 * - regras próprias de vitória (ex.: terminar após X waves);
 * - curva de bricks (hp) e quantidade de rows inseridas por wave.
 */
class RogueSystem {
public:
    /// Começa uma run Rogue (reset de variáveis específicas, wave=1, draft inicial, etc.).
    static void startRun(GameState& state);

    /// Prepara/gera as opções de cartas para mostrar ao jogador (estado->rogueOffer).
    static void setCardOptions(GameState& state);

    /**
     * @brief Chamado quando o jogador escolhe uma carta.
     * @details Normalmente aplica o efeito (modificadores/deck) e fecha o overlay de escolha.
     */
    static void onCardPicked(GameState& state, PowerUpType chosen);

    /// @return true se esta wave, ao ser limpa, deve disparar uma oferta de cartas.
    static bool shouldOfferCardsAfterClearingWave(const GameState& state, int clearedWave);

    /// @return true se a run deve terminar como vitória após limpar esta wave.
    static bool shouldWinAfterClearingWave(const GameState& state, int clearedWave);

    // ---- Curva de dificuldade / bricks ----

    /**
     * @brief Decide o HP de um brick para uma wave.
     * @param isFrontHalf Pode ser usado para diferenciar “metade da frente” vs “metade de trás” da grelha.
     */
    static int pickBrickHpForWave(int wave, bool isFrontHalf);

    // ---- Regras de progressão por wave ----

    /// Quantos bricks (ou progresso equivalente) são necessários para avançar a wave.
    static int bricksRequiredForWave(int wave);

    /// Tempo mínimo antes de permitir avançar (evita waves demasiado rápidas).
    static float minTimeForWave(int wave);

    /// Limite de tempo da wave (se aplicável) antes de forçar avanço/penalização.
    static float timeLimitForWave(int wave);

    /// Hook chamado quando a wave avança (actualiza contadores, prepara próxima wave, etc.).
    static void onWaveAdvanced(GameState& state);

    /// Insere rows (linhas) no “campo” de bricks para a wave actual.
    static void spawnWaveRows(GameState& state, const GameConfig& cfg, int rowsToInsert);

    /// Calcula quantas rows inserir nesta wave (pode depender do estado e da dificuldade).
    static int rowsToInsertForWave(const GameState& state, int wave);

    // ---- Best score (persistência) ----

    /// Carrega o melhor score do Rogue (separado do Endless).
    static int loadRogueBestScore();

    /// Se o score actual bater recorde, actualiza/persiste o best do Rogue.
    static void maybeUpdateRogueBest(GameState& state);
};

} // namespace game
