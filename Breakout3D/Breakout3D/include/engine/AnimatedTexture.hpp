// AnimatedTexture.hpp
#pragma once

#include "engine/Texture.hpp"
#include <string>
#include <vector>

namespace engine {

/**
 * @file AnimatedTexture.hpp
 * @brief Textura animada 2D (ex.: GIF) carregada como frames na GPU.
 *
 * Notas:
 * - Cada frame é um `Texture2D` (upload separado).
 * - `delaysMs` guarda delays por frame (ms); se vazio, assume 100ms.
 * - Usa `maxFrames`/`frameStep` para reduzir VRAM/tempo de load.
 * - Requer contexto OpenGL activo para upload/destroy.
 */
struct AnimatedTexture2D {
    std::vector<Texture2D> frames;
    std::vector<int> delaysMs; // se vazio, assume 100ms por frame

    /// Liberta as texturas de todos os frames e limpa os vetores.
    void destroy();

    /// @return true se não existir qualquer frame carregado.
    bool empty() const { return frames.empty(); }

    /**
     * @brief Selecciona o frame correspondente ao tempo decorrido.
     * @param tSec Tempo em segundos (ex.: tempo acumulado do jogo).
     * @return Referência para a textura do frame actual.
     */
    const Texture2D& frameAtTime(float tSec) const;

    /**
     * @brief Carrega um GIF e envia frames para a GPU.
     * @param path Caminho do ficheiro.
     * @param flipY Inverte verticalmente para consistência com `Texture2D::loadFromFile`.
     * @param maxFrames Limite máximo de frames carregados.
     * @param frameStep Carrega 1 em cada N frames (2 = metade dos frames).
     */
    static AnimatedTexture2D loadGifFromFile(const std::string& path, bool flipY = true, int maxFrames = 120, int frameStep = 2);
};

} // namespace engine
