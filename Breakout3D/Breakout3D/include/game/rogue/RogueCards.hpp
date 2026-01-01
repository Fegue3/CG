// RogueCards.hpp
#pragma once

#include "game/GameConfig.hpp"
#include "game/GameState.hpp"
#include "game/rogue/RogueCardId.hpp"

#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace game::rogue {

/**
 * @file RogueCards.hpp
 * @brief Definições e regras do sistema de cartas do modo Rogue (draft, pools e aplicação).
 *
 * Aqui ficam:
 * - Metadata das cartas (nome, descrição curta, se é OP) para UI.
 * - Regras de run: inicialização de pools, draft inicial e offers por wave.
 * - Aplicação de uma pick (efeitos, registo de picks e alterações ao drop deck/modificadores).
 * - Helpers para calcular valores efectivos (drop chance, scales e multipliers).
 */

struct RogueCardDef {
    RogueCardId id;
    const char* name;
    const char* shortDesc;
    bool isOp;
};

// -------- Metadata / UI --------

/// @return Definição (nome/descrição/isOp) para uma carta.
const RogueCardDef& cardDef(RogueCardId id);

/// @return Cor/acento usada no UI (ex.: por tipo/raridade).
glm::vec3 cardAccent(RogueCardId id);

/// @return Abreviação curta (ex.: para badges/mini-cards).
std::string cardAbbrev(RogueCardId id);

/// Lista com todos os IDs (para browsing no menu). Estável durante o runtime.
const std::vector<RogueCardId>& allCardIds();

/// @return true se for uma carta PU_* (contribui para o drop deck).
bool isPowerupCard(RogueCardId id);

// -------- Ciclo de vida da run --------

/// Prepara pools de cartas (remaining pools) e reseta modificadores da run.
void initRunPools(GameState& state);

/// Draft inicial (wave 1): normalmente várias rondas de escolha com 3 cartas.
void startInitialDraft(GameState& state);

// -------- Regras de “dealing” --------

/**
 * @brief Coloca até `count` cartas únicas em `state.rogueOffer`, removendo-as dos pools restantes.
 * @param opPack Se true, puxa do pool OP; caso contrário, do pool normal.
 * @param waveProgress Pode influenciar pesos/regras (ex.: bloquear certas cartas cedo/tarde).
 */
void dealOffer(GameState& state, int count, bool opPack, int waveProgress);

// -------- Aplicar uma pick --------

/**
 * @brief Aplica os efeitos da carta escolhida e regista a pick (sem repetição).
 *
 * Efeitos típicos:
 * - Ajustar multiplicadores/modificadores no `GameState` (paddle/ball speed, drops, banking, etc.).
 * - Se for PU_*, adicionar ao deck de drops e também aplicar um “grant” imediato.
 */
void applyPickedCard(GameState& state, const GameConfig& cfg, RogueCardId picked);

// -------- Valores efectivos (derivados do estado) --------

/// Chance final de drop no Rogue (ex.: cfg.powerUpChance * multiplier do estado).
float effectiveDropChance(const GameState& state, const GameConfig& cfg);

/// Escala base do paddle no eixo X (aplicada em física/render/colisões).
float basePaddleScaleX(const GameState& state);

/// Multiplicador de velocidade do paddle (aplicado no movimento/controle).
float paddleSpeedMult(const GameState& state);

/// Multiplicador de velocidade da bola para lançamentos/spawns novos.
float ballSpeedMult(const GameState& state);

} // namespace game::rogue
