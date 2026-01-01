// AudioSystem.cpp
/**
 * @file AudioSystem.cpp
 * @brief Sistema de áudio do Breakout3D baseado em miniaudio (SFX one-shot, loops de SFX, música com crossfade e stingers).
 *
 * Visão geral
 * -----------
 * Este módulo faz 4 coisas principais:
 *  1) Inicializa o backend de áudio (com foco em WSL/WSLg + PulseAudio).
 *  2) Descobre ficheiros .wav em assets/audio e cria “grupos” por ID (random variants).
 *  3) Reproduz:
 *      - SFX one-shot (pool de voices)
 *      - SFX loops persistentes com fade in/out (map)
 *      - Música em loop com crossfade entre 2 slots (A/B)
 *      - Stingers (one-shot, “musical punctuation”)
 *  4) Faz “live volume updates” (se o utilizador mexe nos sliders, reescala o que já está a tocar).
 *
 * Convenções de assets / IDs
 * --------------------------
 * Raiz: assets/audio
 *
 *  - SFX: assets/audio/sfx/**\/*.wav
 *      ID = caminho relativo dentro de sfx/, sem extensão, e com sufixo _NN opcional removido.
 *      Ex: assets/audio/sfx/game/hit_wall_01.wav  -> id "game/hit_wall"
 *          assets/audio/sfx/ui/ui_confirm.wav     -> id "ui/ui_confirm"
 *
 *  - Music loops: assets/audio/music/*.wav
 *      ID = nome do ficheiro sem extensão (strip _NN)
 *      Ex: assets/audio/music/music_menu_loop.wav -> id "music_menu_loop"
 *
 *  - Stingers musicais: assets/audio/stingers_music/*.wav
 *      ID = nome do ficheiro sem extensão (strip _NN)
 *      Ex: assets/audio/stingers_music/stinger_game_over.wav -> id "stinger_game_over"
 *
 * Notas importantes:
 *  - Os stingers são one-shot (não são “SFX”, mas são reproduzidos com o mesmo mecanismo de voice pool).
 *  - Os loops de SFX são “persistentes” e geridos por ID (para ligar/desligar sem spam).
 *
 * WSL/WSLg
 * --------
 * Para evitar o clássico “cannot find card 0” em ALSA dentro de WSL,
 * o init dá prioridade a PulseAudio e, em modo WSL, usa *apenas* Pulse.
 * Em Linux normal: PulseAudio -> ALSA -> Null.
 */
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
 
 #define MINIAUDIO_IMPLEMENTATION
 #include "external/miniaudio.h"
 
 namespace fs = std::filesystem;
 
 namespace game {
 
 struct AudioSystem::Impl {
     // Backend/context e engine do miniaudio
     ma_context context{};
     ma_engine engine{};
     bool contextValid = false;
 
     // ------------------------------------------------------------
     // Música com crossfade (2 slots)
     // ------------------------------------------------------------
     ma_sound musicA{};
     ma_sound musicB{};
     bool musicAValid = false;
     bool musicBValid = false;
     bool musicUseA = true; // indica qual slot é o “novo/ativo” na lógica de fade
 
     float musicTarget = 1.0f;   // volume alvo (já com master+music e sliders)
     float musicFadeT = 0.0f;    // tempo decorrido no fade
     float musicFadeDur = 0.0f;  // duração total do fade
     float musicStartA = 0.0f;   // volume inicial do slot A no início do fade
     float musicStartB = 0.0f;   // volume inicial do slot B no início do fade
 
     // Pesos normalizados (volume/ganho do bus) para suportar sliders a ir a 0 e voltar
     float musicWeightA = 0.0f;
     float musicWeightB = 0.0f;
 
     // ------------------------------------------------------------
     // Loops de SFX persistentes (por ID)
     // ------------------------------------------------------------
     struct LoopSfx {
         ma_sound sound{};
         bool valid = false;
 
         // “Estado desejado”: ligado/desligado
         bool targetEnabled = false;
 
         // Fade/anim
         float target = 0.0f;
         float start = 0.0f;
         float t = 0.0f;
         float dur = 0.0f;
 
         // Alpha normalizado 0..1 relativo ao sfx bus (para reescala com sliders)
         float fadeAlpha = 0.0f;
     };
     std::unordered_map<std::string, LoopSfx> loops;
 
     // ------------------------------------------------------------
     // One-shots (pool de voices)
     // ------------------------------------------------------------
     struct Voice {
         ma_sound s{};
         bool valid = false;
     };
     std::vector<Voice> voices;
 
     // Últimos gains do bus (linear) – usados para reescalar sons já a tocar
     float lastMusicGain = 1.0f;
     float lastSfxGain = 1.0f;
     float lastStingerGain = 1.0f;
 };
 
 static fs::path executableDir() {
 #if defined(__linux__)
     // /proc/self/exe funciona no Linux e em WSL.
     char buf[4096];
     ssize_t n = ::readlink("/proc/self/exe", buf, sizeof(buf) - 1);
     if (n > 0) {
         buf[n] = '\0';
         fs::path p(buf);
         return p.parent_path();
     }
     return fs::current_path();
 #elif defined(__APPLE__)
     return fs::current_path();
 #else
     return fs::current_path();
 #endif
 }
 
 static bool isWSL() {
 #if defined(__linux__)
     // Heurística: “microsoft”/“wsl” no osrelease, ou env var WSL_DISTRO_NAME.
     std::ifstream f("/proc/sys/kernel/osrelease");
     std::string s;
     if (f && std::getline(f, s)) {
         std::string low = s;
         std::transform(low.begin(), low.end(), low.begin(),
                        [](unsigned char c){ return (char)std::tolower(c); });
         if (low.find("microsoft") != std::string::npos) return true;
         if (low.find("wsl") != std::string::npos) return true;
     }
     const char* wsl = std::getenv("WSL_DISTRO_NAME");
     if (wsl && *wsl) return true;
     return false;
 #else
     return false;
 #endif
 }
 
 static std::string stripVariantSuffix(const std::string& nameNoExt) {
     // Remove _NN (NN dígitos) no final, para suportar variantes random:
     // hit_wall_01 -> hit_wall
     if (nameNoExt.size() >= 3 && nameNoExt[nameNoExt.size() - 3] == '_') {
         char a = nameNoExt[nameNoExt.size() - 2];
         char b = nameNoExt[nameNoExt.size() - 1];
         if (a >= '0' && a <= '9' && b >= '0' && b <= '9') {
             return nameNoExt.substr(0, nameNoExt.size() - 3);
         }
     }
     return nameNoExt;
 }
 
 // ------------------------------------------------------------
 // Ctor/Dtor
 // ------------------------------------------------------------
 AudioSystem::AudioSystem() {
     std::random_device rd;
     m_rng.seed(rd());
 }
 
 AudioSystem::~AudioSystem() {
     shutdown();
 }
 
 // ------------------------------------------------------------
 // Helpers básicos
 // ------------------------------------------------------------
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
 
 // Ganhos (linear) por bus: master + categoria + sliders.
 // Nota: isto é o “ganho base” que depois é multiplicado por “weights” (música) ou “fadeAlpha” (loops).
 float AudioSystem::musicBusGain() const { return dbToLinear(m_masterDb + m_musicDb) * (m_masterVol * m_musicVol); }
 float AudioSystem::sfxBusGain() const { return dbToLinear(m_masterDb + m_sfxDb) * (m_masterVol * m_sfxVol); }
 float AudioSystem::stingerBusGain() const { return dbToLinear(m_masterDb + m_stingerDb) * (m_masterVol * m_stingerVol); }
 
 // Encontra um grupo (map id -> lista de ficheiros). Devolve nullptr se não existir/vazio.
 const AudioSystem::FileList* AudioSystem::findGroup(
     const std::unordered_map<std::string, FileList>& groups,
     const std::string& id
 ) const {
     auto it = groups.find(id);
     if (it == groups.end()) return nullptr;
     if (it->second.empty()) return nullptr;
     return &it->second;
 }
 
 // Escolhe um ficheiro aleatório dentro de um grupo
 const std::string* AudioSystem::chooseRandomFile(const FileList& files) {
     if (files.empty()) return nullptr;
     std::uniform_int_distribution<size_t> dist(0, files.size() - 1);
     return &files[dist(m_rng)];
 }
 
 // ------------------------------------------------------------
 // init(): resolve root, init miniaudio, scan folders, build groups
 // ------------------------------------------------------------
 bool AudioSystem::init(const std::string& audioRoot) {
     shutdown();
 
     // Resolver “audio root” de forma robusta (CWD pode variar consoante IDE/launcher).
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
 
     // Se não encontrar, desliga áudio e deixa o jogo continuar “silencioso”.
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
 
     // Backends: WSL -> Pulse ONLY; Linux normal -> Pulse -> ALSA -> Null.
     const bool wsl = isWSL();
     ma_backend backendsWSL[] = { ma_backend_pulseaudio };
     ma_backend backendsLinux[] = { ma_backend_pulseaudio, ma_backend_alsa, ma_backend_null };
     ma_backend* backends = wsl ? backendsWSL : backendsLinux;
     ma_uint32 backendCount = wsl
         ? (ma_uint32)(sizeof(backendsWSL) / sizeof(backendsWSL[0]))
         : (ma_uint32)(sizeof(backendsLinux) / sizeof(backendsLinux[0]));
 
     // Context init
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
 
     // Engine init (usa o contexto para manter a seleção de backend “presa”)
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
     (void)ma_engine_start(&m_impl->engine);
 
     // Pool de one-shots: muitos porque brick breaks e UI podem ser densos.
     m_impl->voices.resize(96);
 
     // Helpers de scan
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
             std::transform(ext.begin(), ext.end(), ext.begin(),
                            [](unsigned char c){ return (char)std::tolower(c); });
             if (ext != ".wav") continue;
             onFile(p);
         }
     };
 
     // SFX scan: id = path dentro de sfx/, sem ext, strip _NN no último componente
     {
         fs::path sfxRoot = fs::path(m_root) / "sfx";
         scanDir(sfxRoot, [&](const fs::path& p) {
             fs::path rel = fs::relative(p, sfxRoot);
             std::string relNoExt = normalizeSlashes(rel.replace_extension("").string());
 
             fs::path relPath(relNoExt);
             std::string fname = relPath.filename().string();
             std::string base = stripVariantSuffix(fname);
 
             fs::path idPath = relPath.parent_path() / base;
             addFileToGroups(m_sfxGroups,
                             normalizeSlashes(idPath.string()),
                             normalizeSlashes(p.string()));
         });
     }
 
     // Music scan: id = stem (strip _NN)
     {
         fs::path musicRoot = fs::path(m_root) / "music";
         scanDir(musicRoot, [&](const fs::path& p) {
             std::string id = stripVariantSuffix(p.stem().string());
             addFileToGroups(m_musicGroups, id, normalizeSlashes(p.string()));
         });
     }
 
     // Stingers music scan: id = stem (strip _NN)
     {
         fs::path stRoot = fs::path(m_root) / "stingers_music";
         scanDir(stRoot, [&](const fs::path& p) {
             std::string id = stripVariantSuffix(p.stem().string());
             addFileToGroups(m_stingerGroups, id, normalizeSlashes(p.string()));
         });
     }
 
     // Se não houver nada, desliga áudio.
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
 
     // “Baseline” de ganhos (para reescala em runtime)
     m_impl->lastMusicGain = musicBusGain();
     m_impl->lastSfxGain = sfxBusGain();
     m_impl->lastStingerGain = stingerBusGain();
     m_impl->musicWeightA = 0.0f;
     m_impl->musicWeightB = 0.0f;
     return true;
 }
 
 // ------------------------------------------------------------
 // shutdown(): liberta tudo (loops, voices, music, engine/context)
 // ------------------------------------------------------------
 void AudioSystem::shutdown() {
     m_enabled = false;
     if (!m_impl) return;
 
     // Loops
     for (auto& [_, L] : m_impl->loops) {
         if (L.valid) {
             ma_sound_stop(&L.sound);
             ma_sound_uninit(&L.sound);
             L.valid = false;
         }
     }
     m_impl->loops.clear();
 
     // Voices one-shot
     for (auto& v : m_impl->voices) {
         if (v.valid) {
             ma_sound_stop(&v.s);
             ma_sound_uninit(&v.s);
             v.valid = false;
         }
     }
 
     // Música
     if (m_impl->musicAValid) { ma_sound_stop(&m_impl->musicA); ma_sound_uninit(&m_impl->musicA); }
     if (m_impl->musicBValid) { ma_sound_stop(&m_impl->musicB); ma_sound_uninit(&m_impl->musicB); }
     m_impl->musicAValid = m_impl->musicBValid = false;
 
     ma_engine_uninit(&m_impl->engine);
     if (m_impl->contextValid) ma_context_uninit(&m_impl->context);
     delete m_impl;
     m_impl = nullptr;
 }
 
 // ------------------------------------------------------------
 // Volume setters (0..1)
 // ------------------------------------------------------------
 static float clamp01(float v) { return std::max(0.0f, std::min(1.0f, v)); }
 
 void AudioSystem::setMasterVolume(float v)  { m_masterVol = clamp01(v); }
 void AudioSystem::setSfxVolume(float v)     { m_sfxVol = clamp01(v); }
 void AudioSystem::setMusicVolume(float v)   { m_musicVol = clamp01(v); }
 void AudioSystem::setStingerVolume(float v) { m_stingerVol = clamp01(v); }
 
 // ------------------------------------------------------------
 // One-shot playback (voice pool)
 // ------------------------------------------------------------
 void AudioSystem::playOneShotFile(const std::string& filePath, float linearGain) {
     if (!m_enabled || !m_impl) return;
 
     // Procura voice livre; se não houver, “steal” a primeira.
     Impl::Voice* voice = nullptr;
     for (auto& v : m_impl->voices) {
         if (!v.valid) { voice = &v; break; }
         if (!ma_sound_is_playing(&v.s)) { voice = &v; break; }
     }
     if (!voice && !m_impl->voices.empty()) voice = &m_impl->voices[0];
     if (!voice) return;
 
     // Se a voice já tinha um som, limpa.
     if (voice->valid) {
         ma_sound_stop(&voice->s);
         ma_sound_uninit(&voice->s);
         voice->valid = false;
     }
 
     // Carrega do ficheiro e toca
     ma_result r = ma_sound_init_from_file(&m_impl->engine, filePath.c_str(), 0, nullptr, nullptr, &voice->s);
     if (r != MA_SUCCESS) return;
 
     voice->valid = true;
     ma_sound_set_volume(&voice->s, linearGain);
     ma_sound_start(&voice->s);
 }
 
 // API pública: playSfx/playStinger escolhem random file do group e aplicam db + sliders.
 void AudioSystem::playSfx(const std::string& id, float db) {
     if (!m_enabled) return;
     const FileList* grp = findGroup(m_sfxGroups, id);
     if (!grp) return;
     const std::string* file = chooseRandomFile(*grp);
     if (!file) return;
 
     float gain = dbToLinear(m_masterDb + m_sfxDb + db) * (m_masterVol * m_sfxVol);
     playOneShotFile(*file, gain);
 }
 
 void AudioSystem::playStinger(const std::string& id, float db) {
     if (!m_enabled) return;
     const FileList* grp = findGroup(m_stingerGroups, id);
     if (!grp) return;
     const std::string* file = chooseRandomFile(*grp);
     if (!file) return;
 
     float gain = dbToLinear(m_masterDb + m_stingerDb + db) * (m_masterVol * m_stingerVol);
     playOneShotFile(*file, gain);
 }
 
 // ------------------------------------------------------------
 // Música em loop com crossfade (2 slots)
 // ------------------------------------------------------------
 void AudioSystem::startOrSwapLoopMusic(const std::string& filePath, float fadeSeconds) {
     if (!m_enabled || !m_impl) return;
 
     // Target = ganho do bus de música (master+music + sliders)
     m_impl->musicTarget = musicBusGain();
     m_impl->musicFadeT = 0.0f;
     m_impl->musicFadeDur = std::max(0.0f, fadeSeconds);
 
     // “Novo” slot é o inativo
     bool useA = !m_impl->musicUseA;
     ma_sound* newS = useA ? &m_impl->musicA : &m_impl->musicB;
     bool& newValid = useA ? m_impl->musicAValid : m_impl->musicBValid;
     ma_sound* oldS = m_impl->musicUseA ? &m_impl->musicA : &m_impl->musicB;
     bool oldValid = m_impl->musicUseA ? m_impl->musicAValid : m_impl->musicBValid;
 
     // Limpa slot novo se já existia
     if (newValid) {
         ma_sound_stop(newS);
         ma_sound_uninit(newS);
         newValid = false;
     }
 
     // Carrega o novo loop
     ma_result r = ma_sound_init_from_file(&m_impl->engine, filePath.c_str(), 0, nullptr, nullptr, newS);
     if (r != MA_SUCCESS) return;
 
     newValid = true;
     ma_sound_set_looping(newS, MA_TRUE);
     ma_sound_set_volume(newS, 0.0f);
     ma_sound_start(newS);
 
     // Captura volumes atuais para o início do fade
     m_impl->musicStartA = (m_impl->musicAValid) ? ma_sound_get_volume(&m_impl->musicA) : 0.0f;
     m_impl->musicStartB = (m_impl->musicBValid) ? ma_sound_get_volume(&m_impl->musicB) : 0.0f;
 
     // Marca slot ativo
     m_impl->musicUseA = useA;
 
     // Se não havia “old” ou fade é 0, faz snap.
     if (!oldValid || m_impl->musicFadeDur <= 1e-4f) {
         ma_sound_set_volume(newS, m_impl->musicTarget);
 
         // Atualiza pesos (para reescala depois)
         if (m_impl->musicTarget > 1e-8f) {
             m_impl->musicWeightA = m_impl->musicAValid ? (ma_sound_get_volume(&m_impl->musicA) / m_impl->musicTarget) : 0.0f;
             m_impl->musicWeightB = m_impl->musicBValid ? (ma_sound_get_volume(&m_impl->musicB) / m_impl->musicTarget) : 0.0f;
         }
 
         // Mata o velho se existia
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
     float s = u * u * (3.0f - 2.0f * u); // smoothstep
 
     bool newIsA = m_impl->musicUseA;
 
     // Crossfade: new -> target; old -> 0
     if (newIsA) {
         if (m_impl->musicAValid) ma_sound_set_volume(&m_impl->musicA, (1.0f - s) * m_impl->musicStartA + s * m_impl->musicTarget);
         if (m_impl->musicBValid) ma_sound_set_volume(&m_impl->musicB, (1.0f - s) * m_impl->musicStartB + s * 0.0f);
     } else {
         if (m_impl->musicBValid) ma_sound_set_volume(&m_impl->musicB, (1.0f - s) * m_impl->musicStartB + s * m_impl->musicTarget);
         if (m_impl->musicAValid) ma_sound_set_volume(&m_impl->musicA, (1.0f - s) * m_impl->musicStartA + s * 0.0f);
     }
 
     // No fim, liberta o slot que ficou a 0 (poupa recursos)
     if (u >= 1.0f - 1e-4f) {
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
 
     // “Stop” é um fade para target=0.
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
 
 // ------------------------------------------------------------
 // Loops de SFX (por ID) com fade
 // ------------------------------------------------------------
 void AudioSystem::ensureLoopSfx(const std::string& id, const std::string& filePath, bool enabled, float fadeSeconds) {
     if (!m_enabled || !m_impl) return;
     auto& L = m_impl->loops[id];
 
     // Se ainda não existe e queremos ligar, inicializa e começa a tocar em volume 0.
     if (!L.valid) {
         if (!enabled) return; // não criar loop só para desligar
         ma_result r = ma_sound_init_from_file(&m_impl->engine, filePath.c_str(), 0, nullptr, nullptr, &L.sound);
         if (r != MA_SUCCESS) return;
         L.valid = true;
         ma_sound_set_looping(&L.sound, MA_TRUE);
         ma_sound_set_volume(&L.sound, 0.0f);
         ma_sound_start(&L.sound);
     }
 
     // Define alvo: ligado -> sfxBusGain; desligado -> 0
     float target = enabled ? sfxBusGain() : 0.0f;
     L.targetEnabled = enabled;
     L.start = ma_sound_get_volume(&L.sound);
     L.target = target;
     L.t = 0.0f;
     L.dur = std::max(0.0f, fadeSeconds);
 
     // Fade “instantâneo”
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
 
         // Se acabou e o alvo era off, liberta e remove do map
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
 
     // Se estamos a desligar um loop que nem existe, não fazer nada.
     if (!enabled) {
         auto it = m_impl->loops.find(id);
         if (it == m_impl->loops.end()) return;
 
         // IMPORTANTE: aqui passas filePath vazio porque já existe.
         // (A função ensureLoopSfx usa o som já inicializado; não vai recarregar se L.valid==true.)
         ensureLoopSfx(id, /*filePath=*/"", false, fadeSeconds);
         return;
     }
 
     // Ligar: escolher random file do grupo e garantir loop.
     const FileList* grp = findGroup(m_sfxGroups, id);
     if (!grp) return;
     const std::string* file = chooseRandomFile(*grp);
     if (!file) return;
     ensureLoopSfx(id, *file, true, fadeSeconds);
 }
 
 // ------------------------------------------------------------
 // update(): volume live + fades (music e loops)
 // ------------------------------------------------------------
 void AudioSystem::update(float dt) {
     if (!m_enabled) return;
 
     // Se sliders mudaram, reescala sons atuais sem “saltos”.
     if (m_impl) {
         float newMusic = musicBusGain();
         float newSfx = sfxBusGain();
         float newSt = stingerBusGain();
 
         // === MUSIC ===
         // Se havia baseline, recalcula weights atuais a partir do volume real/lastMusicGain.
         if (m_impl->lastMusicGain > 1e-8f) {
             if (m_impl->musicAValid) m_impl->musicWeightA = ma_sound_get_volume(&m_impl->musicA) / m_impl->lastMusicGain;
             if (m_impl->musicBValid) m_impl->musicWeightB = ma_sound_get_volume(&m_impl->musicB) / m_impl->lastMusicGain;
         }
 
         // Atualiza target (para o fade continuar a apontar para o ganho certo)
         m_impl->musicTarget = newMusic;
 
         // Aplica ganho novo com base nos weights (funciona mesmo se o user passou por 0)
         if (m_impl->musicAValid) ma_sound_set_volume(&m_impl->musicA, m_impl->musicWeightA * newMusic);
         if (m_impl->musicBValid) ma_sound_set_volume(&m_impl->musicB, m_impl->musicWeightB * newMusic);
 
         // === LOOP SFX ===
         // Atualiza fadeAlpha (quanto “ligado” está) relativo ao lastSfxGain.
         if (m_impl->lastSfxGain > 1e-8f) {
             for (auto& [_, L] : m_impl->loops) {
                 if (!L.valid) continue;
                 float cur = ma_sound_get_volume(&L.sound);
                 L.fadeAlpha = std::clamp(cur / m_impl->lastSfxGain, 0.0f, 1.0f);
             }
         }
 
         // Reescala loops e atualiza target se estiverem a “fadar para on”
         for (auto& [_, L] : m_impl->loops) {
             if (!L.valid) continue;
             float v = L.fadeAlpha * newSfx;
             ma_sound_set_volume(&L.sound, v);
             if (L.targetEnabled) {
                 L.target = newSfx; // mantém a intenção de “ligado”, mesmo com slider diferente
             }
         }
 
         // Stingers são one-shot -> não dá para reescalar depois de start.
         (void)newSt;
 
         m_impl->lastMusicGain = newMusic;
         m_impl->lastSfxGain = newSfx;
         m_impl->lastStingerGain = newSt;
     }
 
     // Executa fades (usa musicTarget e L.target)
     updateMusicFade(dt);
     updateLoopSfxFade(dt);
 
     // Depois de processar fades, refresca weights/alpha com o ganho atual, para a próxima mexida no slider ser suave.
     if (m_impl) {
         float mg = m_impl->lastMusicGain;
         if (mg > 1e-8f) {
             if (m_impl->musicAValid) m_impl->musicWeightA = ma_sound_get_volume(&m_impl->musicA) / mg;
             if (m_impl->musicBValid) m_impl->musicWeightB = ma_sound_get_volume(&m_impl->musicB) / mg;
         }
         float sg = m_impl->lastSfxGain;
         if (sg > 1e-8f) {
             for (auto& [_, L] : m_impl->loops) {
                 if (!L.valid) continue;
                 float cur = ma_sound_get_volume(&L.sound);
                 L.fadeAlpha = std::clamp(cur / sg, 0.0f, 1.0f);
             }
         }
     }
 }
 
 } // namespace game
 