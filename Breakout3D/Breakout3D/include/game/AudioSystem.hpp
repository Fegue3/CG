#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace game {

// Lightweight audio wrapper around miniaudio.
// - Auto-discovers .wav files under assets/audio/
// - Groups _01/_02/_03 variants and picks randomly to avoid repetition
// - Separate mix buses for music / stingers / SFX (in dB)
class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

    bool init(const std::string& audioRoot = "assets/audio");
    void shutdown();
    bool isEnabled() const { return m_enabled; }

    void update(float dt);

    // Volume controls (linear multipliers, 0..1+)
    void setMasterVolume(float v);
    void setSfxVolume(float v);
    void setMusicVolume(float v);
    void setStingerVolume(float v);

    // One-shots (IDs are relative to category roots, without extension, and without _NN suffix)
    // Examples:
    // - "game/hit_wall"
    // - "bricks/brick_break"
    // - "ui/ui_move"
    void playSfx(const std::string& id, float db = 0.0f);

    // Stingers (assets/audio/stingers_music)
    // Example: "stinger_wave_start"
    void playStinger(const std::string& id, float db = 0.0f);

    // Music loops (assets/audio/music). Chooses randomly from all matching variants (incl. legacy folder).
    // Example group IDs:
    // - "music_menu_loop"
    // - "music_endless_loop" / "music_endless_mid_loop" / "music_endless_high_loop"
    // - "music_rogue_act1_loop" / "music_rogue_act2_loop" / "music_rogue_act3_loop" / "music_rogue_boss_loop"
    // - "music_results_loop"
    void playMusic(const std::string& groupId, float fadeSeconds = 0.35f);
    void stopMusic(float fadeSeconds = 0.25f);

    // Utility loops (treated as SFX loop on the SFX bus)
    void setSfxLoopEnabled(const std::string& id, bool enabled, float fadeSeconds = 0.12f);

private:
    float dbToLinear(float db) const;
    float musicBusGain() const;
    float sfxBusGain() const;
    float stingerBusGain() const;
    std::string normalizeSlashes(std::string p) const;
    static bool endsWith(const std::string& s, const std::string& suf);

    // SFX IDs are relative to assets/audio/sfx/ e.g. "game/hit_wall"
    // Music group IDs are filenames without extension e.g. "music_menu_loop"
    // Stinger IDs are filenames without extension e.g. "stinger_wave_start"
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

    // Mix dB (defaults tuned per your notes: music lower than SFX, stingers slightly above music)
    float m_masterDb = 0.0f;
    float m_sfxDb = 0.0f;
    float m_musicDb = -14.0f;
    float m_stingerDb = -10.0f;

    // User volume multipliers (set by UI sliders; keeps requested "volume" semantics).
    float m_masterVol = 1.0f;
    float m_sfxVol = 1.0f;
    float m_musicVol = 1.0f;
    float m_stingerVol = 1.0f;

    std::unordered_map<std::string, FileList> m_sfxGroups;
    std::unordered_map<std::string, FileList> m_musicGroups;
    std::unordered_map<std::string, FileList> m_stingerGroups;

    std::mt19937 m_rng;

    // --- miniaudio opaque storage (defined in .cpp) ---
    struct Impl;
    Impl* m_impl = nullptr;
};

} // namespace game



