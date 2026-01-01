// InputSystem.hpp
#pragma once
#include "game/GameState.hpp"
#include "game/GameConfig.hpp"
#include "engine/Input.hpp"
#include "engine/Window.hpp"
#include <glm/glm.hpp>

namespace game {

/**
 * @file InputSystem.hpp
 * @brief Input do jogo e do menu (movimento, launch, pause, cliques UI).
 *
 * Ideia:
 * - `handleMenuInput` lida com navegação/clicks e mudanças de ecrã/estado.
 * - `handleGameInput` lida com gameplay (paddle, lançar bola, pause, etc.).
 */
class InputSystem {
public:
    /**
     * @brief Processa input quando o menu está activo.
     * @return true se houve uma acção (ex.: mudou opção, clicou botão, fechou menu).
     */
    static bool handleMenuInput(GameState& state, const engine::Input& input, engine::Window& window);

    /**
     * @brief Processa input durante o gameplay.
     *
     * Normalmente:
     * - mover paddle (dir = -1/0/+1 a partir de teclas)
     * - lançar bola quando attached
     * - alternar pause / overlays
     *
     * @param dt usado para movimentos smooth / repeat rate (se aplicável).
     */
    static void handleGameInput(GameState& state, const engine::Input& input, const GameConfig& cfg, engine::Window& window, float dt);

private:
    /// Helper simples para hit-test de UI em pixels (ex.: cliques nos botões).
    static bool pointInRectPx(float px, float py, float x, float y, float w, float h);
};

} // namespace game
