// AnimatedTexture.cpp
#include "engine/AnimatedTexture.hpp"

#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <vector>

// A implementação do stb_image está em Texture.cpp (STB_IMAGE_IMPLEMENTATION).
#include "external/stb_image.h"

namespace engine {

/**
 * @file AnimatedTexture.cpp
 * @brief Carregamento e gestão de texturas animadas (GIF) como sequência de Texture2D.
 *
 * Ideia base:
 * - O GIF é descodificado (CPU) via stb_image.
 * - Cada frame é carregada para a GPU como uma textura 2D independente.
 * - O consumidor pode pedir "qual é a frame" para um tempo t (segundos),
 *   respeitando os delays do GIF quando disponíveis.
 *
 * Nota: isto é pensado para UI/preview (ex.: menus), não para dezenas de GIFs grandes ao mesmo tempo.
 */

// Cria uma Texture2D OpenGL a partir de um buffer RGBA (w*h*4).
static Texture2D makeTextureFromRGBA(const unsigned char* rgba, int w, int h) {
    Texture2D t;
    t.w = w;
    t.h = h;
    t.channels = 4;

    glGenTextures(1, &t.id);
    glBindTexture(GL_TEXTURE_2D, t.id);

    // Upload directo do RGBA para a GPU.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);

    // Para GIFs/atlas UI, filtro linear e clamp evitam "bordas" ao interpolar.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
    return t;
}

// Inverte o buffer RGBA no eixo Y (linha 0 <-> linha h-1).
// Útil para alinhar a convenção de coordenadas com as tuas texturas normais (flipY=true).
static void flipRgbaY(std::vector<unsigned char>& buf, int w, int h) {
    if (w <= 0 || h <= 1) return;

    const int stride = w * 4;
    std::vector<unsigned char> tmp((size_t)stride);

    for (int y = 0; y < h / 2; ++y) {
        unsigned char* a = buf.data() + (size_t)y * stride;
        unsigned char* b = buf.data() + (size_t)(h - 1 - y) * stride;

        std::memcpy(tmp.data(), a, (size_t)stride);
        std::memcpy(a, b, (size_t)stride);
        std::memcpy(b, tmp.data(), (size_t)stride);
    }
}

void AnimatedTexture2D::destroy() {
    // Liberta todas as texturas de frame na GPU e limpa metadata.
    for (auto& f : frames) f.destroy();
    frames.clear();
    delaysMs.clear();
}

const Texture2D& AnimatedTexture2D::frameAtTime(float tSec) const {
    // Devolve uma textura “vazia” se não houver frames.
    // (Evita references inválidas; o caller deve tratar id=0.)
    static Texture2D dummy{};
    if (frames.empty()) return dummy;

    // Se não houver delays válidos, assume 100ms por frame (fallback simples).
    if (delaysMs.empty() || delaysMs.size() != frames.size()) {
        int idx = (int)std::floor(tSec / 0.10f) % (int)frames.size();
        return frames[idx];
    }

    // Soma total dos delays (garantindo mínimo 1ms para não haver frames “zero”).
    int totalMs = 0;
    for (int d : delaysMs) totalMs += std::max(1, d);
    if (totalMs <= 0) return frames[0];

    // Tempo actual (ms), repetindo no ciclo do GIF (loop).
    int tMs = (int)std::floor(std::max(0.0f, tSec) * 1000.0f);
    int m = tMs % totalMs;

    // Procura a frame onde o acumulado ultrapassa m.
    int acc = 0;
    for (size_t i = 0; i < frames.size(); ++i) {
        acc += std::max(1, delaysMs[i]);
        if (m < acc) return frames[i];
    }

    // Segurança: se por arredondamentos não encontrou, devolve a última.
    return frames.back();
}

AnimatedTexture2D AnimatedTexture2D::loadGifFromFile(const std::string& path, bool flipY, int maxFrames, int frameStep) {
    AnimatedTexture2D anim;

    // Lê o ficheiro inteiro para memória (stbi_load_gif_from_memory trabalha assim).
    std::ifstream f(path, std::ios::binary);
    if (!f) return anim;

    f.seekg(0, std::ios::end);
    std::streamoff len = f.tellg();
    f.seekg(0, std::ios::beg);
    if (len <= 0) return anim;

    std::vector<unsigned char> bytes((size_t)len);
    f.read((char*)bytes.data(), len);
    if (!f) return anim;

    // Descoding do GIF via stb_image (saída em RGBA 4 canais).
    int* delays = nullptr;
    int w = 0, h = 0, framesN = 0, comp = 0;
    stbi_uc* data = stbi_load_gif_from_memory(
        bytes.data(), (int)bytes.size(),
        &delays, &w, &h, &framesN, &comp,
        4
    );

    if (!data || w <= 0 || h <= 0 || framesN <= 0) {
        if (data) stbi_image_free(data);
        if (delays) stbi_image_free(delays);
        return anim;
    }

    // Normalização dos parâmetros para evitar valores inválidos.
    frameStep = std::max(1, frameStep);
    maxFrames = std::max(1, maxFrames);

    // Reserva para reduzir realocações.
    anim.frames.reserve((size_t)std::min(framesN, maxFrames));
    anim.delaysMs.reserve((size_t)std::min(framesN, maxFrames));

    // Cada frame é um bloco RGBA contíguo.
    const size_t frameBytes = (size_t)w * (size_t)h * 4;

    // Upload parcial (frameStep) e com limite (maxFrames) para poupar memória/tempo.
    int uploaded = 0;
    for (int i = 0; i < framesN && uploaded < maxFrames; i += frameStep) {
        const unsigned char* src = data + (size_t)i * frameBytes;

        // Fazemos cópia para poder flipY sem mexer no buffer global do stb.
        std::vector<unsigned char> buf(src, src + frameBytes);
        if (flipY) flipRgbaY(buf, w, h);

        anim.frames.push_back(makeTextureFromRGBA(buf.data(), w, h));
        anim.delaysMs.push_back(delays ? delays[i] : 100);

        uploaded++;
    }

    // Limpeza da memória alocada pelo stb.
    stbi_image_free(data);
    if (delays) stbi_image_free(delays);

    return anim;
}

} // namespace engine
