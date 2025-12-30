#include "game/AudioSystem.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <iostream>
#include <vector>
#if defined(__linux__)
#include <unistd.h>
#endif

// miniaudio implementation (single translation unit)
// WSL2/WSLg typically exposes audio via PulseAudio. Prefer it over ALSA to avoid "cannot find card 0".
// (We still keep ALSA enabled as a fallback.)
#define MINIAUDIO_IMPLEMENTATION
#include "external/miniaudio.h"

namespace fs = std::filesystem;

namespace game {

struct AudioSystem::Impl {
    ma_context context{};
    ma_engine engine{};
    bool contextValid = false;

    // Music crossfade (two slots)
    ma_sound musicA{};
    ma_sound musicB{};
    bool musicAValid = false;
    bool musicBValid = false;
    bool musicUseA = true;

    float musicTarget = 1.0f;
    float musicFadeT = 0.0f;
    float musicFadeDur = 0.0f;
    float musicStartA = 0.0f;
    float musicStartB = 0.0f;

    // Looping SFX (small map of persistent ma_sound)
    struct LoopSfx {
        ma_sound sound{};
        bool valid = false;
        bool targetEnabled = false;
        float target = 0.0f;
        float start = 0.0f;
        float t = 0.0f;
        float dur = 0.0f;
    };
    std::unordered_map<std::string, LoopSfx> loops;

    // One-shot pool
    struct Voice {
        ma_sound s{};
        bool valid = false;
    };
    std::vector<Voice> voices;
};

static fs::path executableDir() {
#if defined(__linux__)
    // Works in WSL too.
    // NOLINTNEXTLINE
    char buf[4096];
    // NOLINTNEXTLINE
    ssize_t n = ::readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        fs::path p(buf);
        return p.parent_path();
    }
    return fs::current_path();
#elif defined(__APPLE__)
    // Keep it simple: if we ever need mac executable path, we can add _NSGetExecutablePath().
    return fs::current_path();
#else
    return fs::current_path();
#endif
}

static bool isWSL() {
#if defined(__linux__)
    // WSL kernels usually contain "microsoft" in osrelease.
    std::ifstream f("/proc/sys/kernel/osrelease");
    std::string s;
    if (f && std::getline(f, s)) {
        std::string low = s;
        std::transform(low.begin(), low.end(), low.begin(), [](unsigned char c){ return (char)std::tolower(c); });
        if (low.find("microsoft") != std::string::npos) return true;
        if (low.find("wsl") != std::string::npos) return true;
    }
    // Fallback: common env var.
    const char* wsl = std::getenv("WSL_DISTRO_NAME");
    if (wsl && *wsl) return true;
    return false;
#else
    return false;
#endif
}

static std::string stripVariantSuffix(const std::string& nameNoExt) {
    // Removes trailing _NN where NN are digits (e.g. hit_wall_01 -> hit_wall).
    if (nameNoExt.size() >= 3 && nameNoExt[nameNoExt.size() - 3] == '_') {
        char a = nameNoExt[nameNoExt.size() - 2];
        char b = nameNoExt[nameNoExt.size() - 1];
        if (a >= '0' && a <= '9' && b >= '0' && b <= '9') {
            return nameNoExt.substr(0, nameNoExt.size() - 3);
        }
    }
    return nameNoExt;
}

AudioSystem::AudioSystem() {
    std::random_device rd;
    m_rng.seed(rd());
}

AudioSystem::~AudioSystem() {
    shutdown();
}

bool AudioSystem::endsWith(const std::string& s, const std::string& suf) {
    if (s.size() < suf.size()) return false;
    return std::equal(suf.rbegin(), suf.rend(), s.rbegin());
}

std::string AudioSystem::normalizeSlashes(std::string p) const {
    for (char& c : p) {
        if (c == '\\') c = '/';
    }
    return p;
}

float AudioSystem::dbToLinear(float db) const {
    return std::pow(10.0f, db / 20.0f);
}

const AudioSystem::FileList* AudioSystem::findGroup(
    const std::unordered_map<std::string, FileList>& groups,
    const std::string& id
) const {
    auto it = groups.find(id);
    if (it == groups.end()) return nullptr;
    if (it->second.empty()) return nullptr;
    return &it->second;
}

const std::string* AudioSystem::chooseRandomFile(const FileList& files) {
    if (files.empty()) return nullptr;
    std::uniform_int_distribution<size_t> dist(0, files.size() - 1);
    return &files[dist(m_rng)];
}

bool AudioSystem::init(const std::string& audioRoot) {
    shutdown();

    // Resolve audio root robustly (IDE/launcher may not set CWD to the executable folder).
    // Typical layouts:
    // - run from project dir: "./breakout3d" -> assets/audio exists in CWD
    // - run from repo root: "Breakout3D/Breakout3D/breakout3d" -> assets/audio exists next to the binary
    fs::path rootCandidate = fs::path(normalizeSlashes(audioRoot));
    std::vector<fs::path> candidates;
    candidates.push_back(rootCandidate);
    candidates.push_back(fs::current_path() / rootCandidate);
    candidates.push_back(executableDir() / rootCandidate);
    candidates.push_back(executableDir() / "assets/audio");

    fs::path chosen;
    for (const auto& c : candidates) {
        std::error_code ec;
        if (fs::exists(c, ec) && fs::is_directory(c, ec)) {
            chosen = c;
            break;
        }
    }

    if (chosen.empty()) {
        std::cerr << "[Audio] Audio root not found. Tried:\n";
        for (const auto& c : candidates) {
            std::cerr << "  - " << c.string() << "\n";
        }
        std::cerr << "[Audio] Audio disabled.\n";
        return false;
    }

    m_root = normalizeSlashes(chosen.string());
    m_enabled = false;
    m_impl = new Impl();

    // Init engine with explicit backend priority:
    // - On WSL: PulseAudio ONLY (WSLg exposes Pulse; if Pulse fails, we want a loud error, not silent Null)
    // - On normal Linux: PulseAudio -> ALSA -> NULL
    const bool wsl = isWSL();
    ma_backend backendsWSL[] = { ma_backend_pulseaudio };
    ma_backend backendsLinux[] = { ma_backend_pulseaudio, ma_backend_alsa, ma_backend_null };
    ma_backend* backends = wsl ? backendsWSL : backendsLinux;
    ma_uint32 backendCount = wsl
        ? (ma_uint32)(sizeof(backendsWSL) / sizeof(backendsWSL[0]))
        : (ma_uint32)(sizeof(backendsLinux) / sizeof(backendsLinux[0]));

    ma_context_config ctxCfg = ma_context_config_init();
    ma_result r = ma_context_init(backends, backendCount, &ctxCfg, &m_impl->context);
    if (r != MA_SUCCESS) {
        std::cerr << "[Audio] ma_context_init failed (" << r << "). Audio disabled.\n";
        if (wsl) {
            std::cerr << "[Audio] WSL detected. PulseAudio is required. Check:\n";
            std::cerr << "  - $PULSE_SERVER (should be unix:/mnt/wslg/PulseServer)\n";
            std::cerr << "  - `pactl info` works\n";
            std::cerr << "  - libpulse client libs installed (e.g. libpulse0)\n";
        }
        delete m_impl;
        m_impl = nullptr;
        return false;
    }
    m_impl->contextValid = true;

    // Let the engine create/manage its internal device, but constrain backend selection via our context.
    ma_engine_config cfg = ma_engine_config_init();
    cfg.pContext = &m_impl->context;
    r = ma_engine_init(&cfg, &m_impl->engine);
    if (r != MA_SUCCESS) {
        std::cerr << "[Audio] ma_engine_init failed (" << r << "). Audio disabled.\n";
        ma_context_uninit(&m_impl->context);
        delete m_impl;
        m_impl = nullptr;
        return false;
    }
    // Make sure the engine is started.
    (void)ma_engine_start(&m_impl->engine);

    {
        ma_device* dev = ma_engine_get_device(&m_impl->engine);
        if (dev && dev->pContext) {
#ifdef BREAKOUT3D_DEBUG
            std::cerr << "[Audio] Backend: " << ma_get_backend_name(dev->pContext->backend)
                      << (wsl ? " (WSL mode)" : "") << "\n";
#endif
        }
    }

    // One-shot voices (dense brick breaks + UI + stingers)
    m_impl->voices.resize(96);

    // Discover files
    auto addFileToGroups = [&](std::unordered_map<std::string, FileList>& groups,
                               const std::string& id,
                               const std::string& fullPath) {
        groups[id].push_back(fullPath);
    };

    auto scanDir = [&](const fs::path& dir, auto&& onFile) {
        if (!fs::exists(dir)) return;
        for (auto const& entry : fs::recursive_directory_iterator(dir)) {
            if (!entry.is_regular_file()) continue;
            fs::path p = entry.path();
            std::string ext = p.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return (char)std::tolower(c); });
            if (ext != ".wav") continue;
            onFile(p);
        }
    };

    // SFX: id = relative path under sfx/, no ext, strip _NN suffix
    {
        fs::path sfxRoot = fs::path(m_root) / "sfx";
        scanDir(sfxRoot, [&](const fs::path& p) {
            fs::path rel = fs::relative(p, sfxRoot);
            std::string relNoExt = normalizeSlashes(rel.replace_extension("").string());
            // strip variant suffix on the final component only
            fs::path relPath(relNoExt);
            std::string fname = relPath.filename().string();
            std::string base = stripVariantSuffix(fname);
            fs::path idPath = relPath.parent_path() / base;
            addFileToGroups(m_sfxGroups, normalizeSlashes(idPath.string()), normalizeSlashes(p.string()));
        });
    }

    // Music: id = filename no ext, strip _NN suffix (rare), include legacy folder too
    {
        fs::path musicRoot = fs::path(m_root) / "music";
        scanDir(musicRoot, [&](const fs::path& p) {
            std::string id = stripVariantSuffix(p.stem().string());
            addFileToGroups(m_musicGroups, id, normalizeSlashes(p.string()));
        });
    }

    // Stingers music: id = filename no ext, strip _NN suffix
    {
        fs::path stRoot = fs::path(m_root) / "stingers_music";
        scanDir(stRoot, [&](const fs::path& p) {
            std::string id = stripVariantSuffix(p.stem().string());
            addFileToGroups(m_stingerGroups, id, normalizeSlashes(p.string()));
        });
    }

#ifdef BREAKOUT3D_DEBUG
    std::cerr << "[Audio] Root: " << m_root << "\n";
    std::cerr << "[Audio] Groups: sfx=" << m_sfxGroups.size()
              << " music=" << m_musicGroups.size()
              << " stingers=" << m_stingerGroups.size() << "\n";
#endif

    if (m_sfxGroups.empty() && m_musicGroups.empty() && m_stingerGroups.empty()) {
        std::cerr << "[Audio] No audio files found under: " << m_root << "\n";
        std::cerr << "[Audio] Audio disabled.\n";
        ma_engine_uninit(&m_impl->engine);
        if (m_impl->contextValid) ma_context_uninit(&m_impl->context);
        delete m_impl;
        m_impl = nullptr;
        return false;
    }

    m_enabled = true;
    return true;
}

void AudioSystem::shutdown() {
    m_enabled = false;
    if (!m_impl) return;

    // Stop/uninit loops
    for (auto& [_, L] : m_impl->loops) {
        if (L.valid) {
            ma_sound_stop(&L.sound);
            ma_sound_uninit(&L.sound);
            L.valid = false;
        }
    }
    m_impl->loops.clear();

    // Uninit voices
    for (auto& v : m_impl->voices) {
        if (v.valid) {
            ma_sound_stop(&v.s);
            ma_sound_uninit(&v.s);
            v.valid = false;
        }
    }

    // Uninit music
    if (m_impl->musicAValid) { ma_sound_stop(&m_impl->musicA); ma_sound_uninit(&m_impl->musicA); }
    if (m_impl->musicBValid) { ma_sound_stop(&m_impl->musicB); ma_sound_uninit(&m_impl->musicB); }
    m_impl->musicAValid = m_impl->musicBValid = false;

    ma_engine_uninit(&m_impl->engine);
    if (m_impl->contextValid) ma_context_uninit(&m_impl->context);
    delete m_impl;
    m_impl = nullptr;
}

void AudioSystem::setMasterDb(float db)  { m_masterDb = db; }
void AudioSystem::setSfxDb(float db)     { m_sfxDb = db; }
void AudioSystem::setMusicDb(float db)   { m_musicDb = db; }
void AudioSystem::setStingerDb(float db) { m_stingerDb = db; }

void AudioSystem::playOneShotFile(const std::string& filePath, float linearGain) {
    if (!m_enabled || !m_impl) return;

    // Find a free voice or steal the first one.
    Impl::Voice* voice = nullptr;
    for (auto& v : m_impl->voices) {
        if (!v.valid) { voice = &v; break; }
        if (!ma_sound_is_playing(&v.s)) { voice = &v; break; }
    }
    if (!voice && !m_impl->voices.empty()) voice = &m_impl->voices[0];
    if (!voice) return;

    if (voice->valid) {
        ma_sound_stop(&voice->s);
        ma_sound_uninit(&voice->s);
        voice->valid = false;
    }

    ma_result r = ma_sound_init_from_file(&m_impl->engine, filePath.c_str(), 0, nullptr, nullptr, &voice->s);
    if (r != MA_SUCCESS) return;

    voice->valid = true;
    ma_sound_set_volume(&voice->s, linearGain);
    ma_sound_start(&voice->s);
}

void AudioSystem::playSfx(const std::string& id, float db) {
    if (!m_enabled) return;
    const FileList* grp = findGroup(m_sfxGroups, id);
    if (!grp) return;
    const std::string* file = chooseRandomFile(*grp);
    if (!file) return;

    float gain = dbToLinear(m_masterDb + m_sfxDb + db);
    playOneShotFile(*file, gain);
}

void AudioSystem::playStinger(const std::string& id, float db) {
    if (!m_enabled) return;
    const FileList* grp = findGroup(m_stingerGroups, id);
    if (!grp) return;
    const std::string* file = chooseRandomFile(*grp);
    if (!file) return;

    float gain = dbToLinear(m_masterDb + m_stingerDb + db);
    playOneShotFile(*file, gain);
}

void AudioSystem::startOrSwapLoopMusic(const std::string& filePath, float fadeSeconds) {
    if (!m_enabled || !m_impl) return;

    // Decide target gain
    m_impl->musicTarget = dbToLinear(m_masterDb + m_musicDb);
    m_impl->musicFadeT = 0.0f;
    m_impl->musicFadeDur = std::max(0.0f, fadeSeconds);

    // Pick inactive slot
    bool useA = !m_impl->musicUseA;
    ma_sound* newS = useA ? &m_impl->musicA : &m_impl->musicB;
    bool& newValid = useA ? m_impl->musicAValid : m_impl->musicBValid;
    ma_sound* oldS = m_impl->musicUseA ? &m_impl->musicA : &m_impl->musicB;
    bool oldValid = m_impl->musicUseA ? m_impl->musicAValid : m_impl->musicBValid;

    if (newValid) {
        ma_sound_stop(newS);
        ma_sound_uninit(newS);
        newValid = false;
    }

    ma_result r = ma_sound_init_from_file(&m_impl->engine, filePath.c_str(), 0, nullptr, nullptr, newS);
    if (r != MA_SUCCESS) return;

    newValid = true;
    ma_sound_set_looping(newS, MA_TRUE);
    ma_sound_set_volume(newS, 0.0f);
    ma_sound_start(newS);

    // Start fade from current old volume (if any)
    m_impl->musicStartA = (m_impl->musicAValid) ? ma_sound_get_volume(&m_impl->musicA) : 0.0f;
    m_impl->musicStartB = (m_impl->musicBValid) ? ma_sound_get_volume(&m_impl->musicB) : 0.0f;

    m_impl->musicUseA = useA;

    // If no old track, snap up instantly.
    if (!oldValid || m_impl->musicFadeDur <= 1e-4f) {
        ma_sound_set_volume(newS, m_impl->musicTarget);
        if (oldValid) {
            ma_sound_set_volume(oldS, 0.0f);
            ma_sound_stop(oldS);
        }
    }
}

void AudioSystem::updateMusicFade(float dt) {
    if (!m_enabled || !m_impl) return;
    if (m_impl->musicFadeDur <= 1e-4f) return;
    if (!m_impl->musicAValid && !m_impl->musicBValid) return;

    m_impl->musicFadeT += dt;
    float u = std::clamp(m_impl->musicFadeT / m_impl->musicFadeDur, 0.0f, 1.0f);
    // smoothstep
    float s = u * u * (3.0f - 2.0f * u);

    // Determine which is "new"
    bool newIsA = m_impl->musicUseA;
    if (newIsA) {
        if (m_impl->musicAValid) ma_sound_set_volume(&m_impl->musicA, (1.0f - s) * m_impl->musicStartA + s * m_impl->musicTarget);
        if (m_impl->musicBValid) ma_sound_set_volume(&m_impl->musicB, (1.0f - s) * m_impl->musicStartB + s * 0.0f);
    } else {
        if (m_impl->musicBValid) ma_sound_set_volume(&m_impl->musicB, (1.0f - s) * m_impl->musicStartB + s * m_impl->musicTarget);
        if (m_impl->musicAValid) ma_sound_set_volume(&m_impl->musicA, (1.0f - s) * m_impl->musicStartA + s * 0.0f);
    }

    if (u >= 1.0f - 1e-4f) {
        // Stop the faded-out one to save resources.
        if (newIsA) {
            if (m_impl->musicBValid) { ma_sound_stop(&m_impl->musicB); ma_sound_uninit(&m_impl->musicB); m_impl->musicBValid = false; }
        } else {
            if (m_impl->musicAValid) { ma_sound_stop(&m_impl->musicA); ma_sound_uninit(&m_impl->musicA); m_impl->musicAValid = false; }
        }
        m_impl->musicFadeDur = 0.0f;
    }
}

void AudioSystem::playMusic(const std::string& groupId, float fadeSeconds) {
    if (!m_enabled) return;
    const FileList* grp = findGroup(m_musicGroups, groupId);
    if (!grp) return;
    const std::string* file = chooseRandomFile(*grp);
    if (!file) return;
    startOrSwapLoopMusic(*file, fadeSeconds);
}

void AudioSystem::stopMusic(float fadeSeconds) {
    if (!m_enabled || !m_impl) return;

    // Fade whichever is active down to 0 then stop.
    m_impl->musicTarget = 0.0f;
    m_impl->musicFadeT = 0.0f;
    m_impl->musicFadeDur = std::max(0.0f, fadeSeconds);
    m_impl->musicStartA = (m_impl->musicAValid) ? ma_sound_get_volume(&m_impl->musicA) : 0.0f;
    m_impl->musicStartB = (m_impl->musicBValid) ? ma_sound_get_volume(&m_impl->musicB) : 0.0f;

    if (m_impl->musicFadeDur <= 1e-4f) {
        if (m_impl->musicAValid) { ma_sound_stop(&m_impl->musicA); ma_sound_uninit(&m_impl->musicA); m_impl->musicAValid = false; }
        if (m_impl->musicBValid) { ma_sound_stop(&m_impl->musicB); ma_sound_uninit(&m_impl->musicB); m_impl->musicBValid = false; }
    }
}

void AudioSystem::ensureLoopSfx(const std::string& id, const std::string& filePath, bool enabled, float fadeSeconds) {
    if (!m_enabled || !m_impl) return;
    auto& L = m_impl->loops[id];

    if (!L.valid) {
        if (!enabled) return; // don't spin up a loop just to fade it out
        ma_result r = ma_sound_init_from_file(&m_impl->engine, filePath.c_str(), 0, nullptr, nullptr, &L.sound);
        if (r != MA_SUCCESS) return;
        L.valid = true;
        ma_sound_set_looping(&L.sound, MA_TRUE);
        ma_sound_set_volume(&L.sound, 0.0f);
        ma_sound_start(&L.sound);
    }

    float target = enabled ? dbToLinear(m_masterDb + m_sfxDb) : 0.0f;
    L.targetEnabled = enabled;
    L.start = ma_sound_get_volume(&L.sound);
    L.target = target;
    L.t = 0.0f;
    L.dur = std::max(0.0f, fadeSeconds);

    if (L.dur <= 1e-4f) {
        ma_sound_set_volume(&L.sound, target);
        if (!enabled) {
            ma_sound_stop(&L.sound);
            ma_sound_uninit(&L.sound);
            L.valid = false;
        }
    }
}

void AudioSystem::updateLoopSfxFade(float dt) {
    if (!m_enabled || !m_impl) return;
    for (auto it = m_impl->loops.begin(); it != m_impl->loops.end(); ) {
        auto& L = it->second;
        if (!L.valid) { it = m_impl->loops.erase(it); continue; }
        if (L.dur <= 1e-4f) { ++it; continue; }

        L.t += dt;
        float u = std::clamp(L.t / L.dur, 0.0f, 1.0f);
        float s = u * u * (3.0f - 2.0f * u);
        float v = (1.0f - s) * L.start + s * L.target;
        ma_sound_set_volume(&L.sound, v);

        if (u >= 1.0f - 1e-4f && !L.targetEnabled) {
            ma_sound_stop(&L.sound);
            ma_sound_uninit(&L.sound);
            L.valid = false;
            it = m_impl->loops.erase(it);
            continue;
        }
        ++it;
    }
}

void AudioSystem::setSfxLoopEnabled(const std::string& id, bool enabled, float fadeSeconds) {
    if (!m_enabled) return;
    if (!m_impl) return;

    // If disabling a loop that's not currently active, do nothing (avoid pointless init/start/stop churn).
    if (!enabled) {
        auto it = m_impl->loops.find(id);
        if (it == m_impl->loops.end()) return;
        // Fade out the existing loop (do NOT re-pick a random file).
        ensureLoopSfx(id, /*filePath=*/"", false, fadeSeconds);
        return;
    }

    const FileList* grp = findGroup(m_sfxGroups, id);
    if (!grp) return;
    const std::string* file = chooseRandomFile(*grp);
    if (!file) return;
    ensureLoopSfx(id, *file, true, fadeSeconds);
}

void AudioSystem::update(float dt) {
    if (!m_enabled) return;
    updateMusicFade(dt);
    updateLoopSfxFade(dt);
}

} // namespace game


