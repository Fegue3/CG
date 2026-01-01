// InitSystem.hpp
#pragma once
#include "game/GameState.hpp"
#include "game/GameConfig.hpp"

namespace game {

/**
 * @file InitSystem.hpp
 * @brief Inicialização e geração de conteúdo (bricks, resets e checks simples).
 *
 * Responsabilidades:
 * - inicializar `GameState` para começar uma sessão/run
 * - gerar bricks para modos diferentes (level fixo, wave procedural, endless incremental)
 * - reset de bola/paddle após perder vida ou ao iniciar
 */
class InitSystem {
public:
    /// Inicializa o estado do jogo (valores default, vidas, score, seeds, etc.).
    static void initGame(GameState& state, const GameConfig& cfg);

    /// Gera bricks para uma wave/nível “genérico” (waveNumber pode influenciar dificuldade/padrão).
    static void generateBricks(GameState& state, const GameConfig& cfg, int waveNumber = 0);

    /// Gera bricks a partir de um layout fixo (Levels mode).
    static void generateBricksFromLevel(GameState& state, const GameConfig& cfg, int levelNumber);

    /// Endless: adiciona bricks “incrementalmente” (ex.: mais linhas ao longo do tempo/wave).
    static void spawnIncrementalBricks(GameState& state, const GameConfig& cfg, int count, int waveNumber);

    /// Reposiciona bola e paddle para o estado inicial (tipicamente bola attached ao paddle).
    static void resetBallAndPaddle(GameState& state, const GameConfig& cfg);

    /// @return true se ainda existirem bricks vivos (para detectar wave clear).
    static bool anyBricksAlive(const GameState& state);
};

} // namespace game
