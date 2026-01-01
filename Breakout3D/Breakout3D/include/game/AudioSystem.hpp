// AudioSystem.hpp
#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace game {

/**
 * @file AudioSystem.hpp
 * @brief Sistema de áudio leve (miniaudio): descoberta de ficheiros + reprodução (SFX/Stingers/Música).
 *
 * Funciona como wrapper simples:
 * - Procura automaticamente `.wav` dentro de `assets/audio/` (por defeito).
 * - Agrupa variantes com sufixo `_01/_02/_03/...` e escolhe aleatoriamente para evitar repetição.
 * - Mantém buses separados para Música / Stingers / SFX (ganhos em dB + sliders do utilizador).
 *
 * Notas:
 * - IDs passados a play* são caminhos relativos ao tipo (sem extensão e sem `_NN`).
 * - A implementação real do miniaudio fica escondida em `Impl` (no .cpp).
 */
class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

    /// Inicializa o motor e faz scan aos ficheiros de áudio dentro de `audioRoot`.
    bool init(const std::string& audioRoot = "assets/audio");

    /// Liberta recursos do motor de áudio e limpa estado.
    void shutdown();

    /// @return true se o áudio estiver inicializado/activo.
    bool isEnabled() const { return m_enabled; }

    /// Tick do áudio (fades de música/loops, housekeeping). Chamar 1x por frame.
    void update(float dt);

    // ---- Volumes (multiplicadores lineares: 0..1+) ----
    void setMasterVolume(float v);
    void setSfxVolume(float v);
    void setMusicVolume(float v);
    void setStingerVolume(float v);

    /**
     * @brief Toca um SFX (one-shot).
     * @param id ID relativo a `assets/audio/sfx/` sem extensão e sem `_NN`.
     * @param db Ajuste em dB por chamada (útil para “empurrar” certos eventos).
     *
     * Exemplos:
     * - "game/hit_wall"
     * - "bricks/brick_break"
     * - "ui/ui_move"
     */
    void playSfx(const std::string& id, float db = 0.0f);

    /**
     * @brief Toca um stinger (one-shot curto, normalmente musical).
     * @param id ID relativo a `assets/audio/stingers_music/` sem extensão.
     */
    void playStinger(const std::string& id, float db = 0.0f);

    /**
     * @brief Inicia/troca a música em loop (escolhe aleatoriamente uma variante do grupo).
     * @param groupId Nome base do grupo (sem extensão, sem `_NN`).
     * @param fadeSeconds Crossfade/fade ao trocar música.
     */
    void playMusic(const std::string& groupId, float fadeSeconds = 0.35f);

    /// Pára a música actual com fade.
    void stopMusic(float fadeSeconds = 0.25f);

    /**
     * @brief Liga/desliga um loop utilitário tratado como SFX (no bus de SFX).
     *
     * Útil para hums/ambientes curtos ou loops de UI. Usa fade para evitar clicks.
     */
    void setSfxLoopEnabled(const std::string& id, bool enabled, float fadeSeconds = 0.12f);

private:
    // Helpers internos (normalização, grupos, conversões de ganho).
    float dbToLinear(float db) const;
    float musicBusGain() const;
    float sfxBusGain() const;
    float stingerBusGain() const;
    std::string normalizeSlashes(std::string p) const;
    static bool endsWith(const std::string& s, const std::string& suf);

    using FileList = std::vector<std::string>;

    const FileList* findGroup(const std::unordered_map<std::string, FileList>& groups, const std::string& id) const;
    const std::string* chooseRandomFile(const FileList& files);

    void playOneShotFile(const std::string& filePath, float linearGain);

    void startOrSwapLoopMusic(const std::string& filePath, float fadeSeconds);
    void updateMusicFade(float dt);

    void ensureLoopSfx(const std::string& id, const std::string& filePath, bool enabled, float fadeSeconds);
    void updateLoopSfxFade(float dt);

private:
    bool m_enabled = false;
    std::string m_root;

    // Ganhos base em dB (afinados para balanço: música mais baixa que SFX; stingers um pouco acima da música).
    float m_masterDb = 0.0f;
    float m_sfxDb = 0.0f;
    float m_musicDb = -14.0f;
    float m_stingerDb = -10.0f;

    // Sliders do utilizador (multiplicadores lineares).
    float m_masterVol = 1.0f;
    float m_sfxVol = 1.0f;
    float m_musicVol = 1.0f;
    float m_stingerVol = 1.0f;

    std::unordered_map<std::string, FileList> m_sfxGroups;
    std::unordered_map<std::string, FileList> m_musicGroups;
    std::unordered_map<std::string, FileList> m_stingerGroups;

    std::mt19937 m_rng;

    // --- armazenamento opaco do miniaudio (definido no .cpp) ---
    struct Impl;
    Impl* m_impl = nullptr;
};

} // namespace game
