#pragma once
#include <glm/glm.hpp>

namespace game::effects {

/**
 * @file WinFinisher.hpp
 * @brief “Win finisher” cinematográfico: parâmetros de câmara + overlay (flash/ring) calculados por tempo.
 *
 * @details
 * Este header expõe funções **puramente matemáticas** (sem render) para:
 * - aplicar um “punch” cinematográfico à câmara (shake/roll/FOV) durante o finisher;
 * - gerar valores de overlay (flash + ring) e projectar um ponto-âncora do mundo para o ecrã.
 *
 * Convenções:
 * - `finisherCineTimeSec`: tempo normalizado/cinemático (pode ser clamped/curvado, depende do caller).
 * - `finisherRealTimeSec`: tempo real (útil para ruído/shake consistente e independente do cine time).
 * - Coordenadas de overlay são em **pixels do framebuffer** (fbW/fbH).
 */

struct WinFinisherCamera {
    /// Posição final da câmara (world space) após efeitos do finisher.
    glm::vec3 camPos{0.0f};

    /// Target final da câmara (world space) após efeitos do finisher.
    glm::vec3 camTarget{0.0f};

    /// Campo de visão final (graus) após “punch” de FOV.
    float fovDeg = 45.0f;

    /// Roll final da câmara (radianos) para dar impacto/estilo.
    float rollRad = 0.0f;
};

struct WinFinisherOverlay {
    /**
     * @brief Centro do overlay em pixels do framebuffer.
     *
     * @details
     * Normalmente corresponde à projecção do `anchorWorldPos` (quando `anchorValid=true`).
     * Se o anchor for inválido, o .cpp pode optar por centrar no ecrã (ou manter valores neutros).
     */
    glm::vec2 centerPx{0.0f};

    /// Alpha do flash (0..1). Tipicamente um impulso rápido no início do finisher.
    float flashAlpha = 0.0f;

    /// Alpha do “ring” (0..1). Usado para um anel expansivo ao redor do centro.
    float ringAlpha = 0.0f;

    /// Raio do ring em pixels.
    float ringRadiusPx = 0.0f;

    /// Espessura do ring em pixels.
    float ringThicknessPx = 0.0f;
};

/**
 * @brief Calcula a câmara do finisher (shake/roll/FOV) a partir de um estado base.
 *
 * @param baseCamPos Posição base da câmara (world space) antes do finisher.
 * @param baseCamTarget Target base da câmara (world space) antes do finisher.
 * @param baseFovDeg FOV base em graus.
 * @param arenaBaseScale Escala “de referência” do arena/cena (para normalizar amplitudes).
 * @param finisherCineTimeSec Tempo cinematográfico (segundos) para envelopes/curvas do efeito.
 * @param finisherRealTimeSec Tempo real (segundos), típico para ruído/shake contínuo.
 *
 * @return Parâmetros finais de câmara a usar no render do frame.
 *
 * @note
 * Esta função não lê nem escreve estado global; é intencionalmente determinística
 * para um dado conjunto de inputs (bom para tuning e debugging).
 */
WinFinisherCamera computeWinFinisherCamera(
    const glm::vec3& baseCamPos,
    const glm::vec3& baseCamTarget,
    float baseFovDeg,
    float arenaBaseScale,
    float finisherCineTimeSec,
    float finisherRealTimeSec
);

/**
 * @brief Calcula valores de overlay (flash/ring) e projeta um “anchor” do mundo para o ecrã.
 *
 * @param fbW Largura do framebuffer em pixels.
 * @param fbH Altura do framebuffer em pixels.
 * @param V Matriz View final usada para renderizar a cena neste frame.
 * @param P Matriz Projection final usada para renderizar a cena neste frame.
 * @param anchorWorldPos Posição no mundo que serve de centro visual (ex.: ponto de impacto).
 * @param finisherCineTimeSec Tempo cinematográfico (segundos) para envelopes/curvas do overlay.
 * @param anchorValid Se false, evita projecção e permite fallback (ex.: centro do ecrã).
 *
 * @return Parâmetros de overlay em pixels do framebuffer.
 *
 * @warning
 * Assume que `V` e `P` correspondem exactamente à câmara usada no mundo;
 * caso contrário, o overlay pode “descolar” do ponto real.
 */
WinFinisherOverlay computeWinFinisherOverlay(
    int fbW, int fbH,
    const glm::mat4& V,
    const glm::mat4& P,
    const glm::vec3& anchorWorldPos,
    float finisherCineTimeSec,
    bool anchorValid
);

} // namespace game::effects
