// PowerUpSystem.hpp
#pragma once
#include "game/GameState.hpp"
#include "game/GameConfig.hpp"
#include <glm/glm.hpp>

namespace game {

/**
 * @file PowerUpSystem.hpp
 * @brief Spawns e lógica de power-ups (queda, pickup e aplicação de efeitos).
 *
 * Responsabilidades:
 * - criar um power-up (quando um brick parte / evento de drop)
 * - atualizar movimento/queda e detetar pickup pelo paddle
 * - aplicar o efeito no `GameState` (e, se necessário, com base no `GameConfig`)
 */
class PowerUpSystem {
public:
    /// Tenta spawnar um power-up na posição dada (com probabilidade `chance`).
    static void spawnPowerUp(GameState& state, const glm::vec3& pos, float chance);

    /// Actualiza power-ups activos: queda, colisão com paddle, expiração e efeitos.
    static void updatePowerUps(GameState& state, const GameConfig& cfg, float dt);

    /// Aplica o efeito de um tipo de power-up (quando apanhado).
    static void applyPowerUpEffect(GameState& state, const GameConfig& cfg, PowerUpType type);
};

} // namespace game
