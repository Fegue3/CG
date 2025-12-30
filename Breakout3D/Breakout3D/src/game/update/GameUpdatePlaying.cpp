#include "game/Game.hpp"

#include "game/systems/PhysicsSystem.hpp"
#include "game/systems/CollisionSystem.hpp"
#include "game/systems/InitSystem.hpp"
#include "game/systems/PowerUpSystem.hpp"
#include "game/systems/RogueSystem.hpp"
#include "game/rogue/RogueCards.hpp"

#include <glm/glm.hpp>

#include <algorithm>

namespace game {

void Game::updatePlayingFrame(
    const engine::Input& /*input*/,
    float dt,
    GameMode modeBefore,
    GameType typeBefore,
    int livesBefore,
    int waveBefore,
    int endlessRowsBefore,
    bool dangerBefore,
    int streakBefore,
    bool hadPowerupBefore
) {
    auto rogueMusicForWave = [&](int wave) -> std::string {
        wave = std::max(1, wave);
        if (wave >= m_state.rogueMaxWaves) return "music_rogue_boss_loop";
        if (wave >= 7) return "music_rogue_act3_loop";
        if (wave >= 4) return "music_rogue_act2_loop";
        return "music_rogue_act1_loop";
    };

    // Update physics
    PhysicsSystem::updateBalls(m_state, m_cfg, dt);

    // Calculate current paddle size for collisions
    glm::vec3 currentPaddleSize = m_cfg.paddleSize;
    if (m_state.gameType == GameType::ROGUE) currentPaddleSize.x *= game::rogue::basePaddleScaleX(m_state);
    if (m_state.expandTimer > 0.0f) currentPaddleSize.x *= m_cfg.expandScaleFactor;
    if (m_state.tinyTimer > 0.0f) currentPaddleSize.x *= m_cfg.tinyScaleFactor;

    // Handle collisions for each ball
    for (auto& ball : m_state.balls) {
        if (ball.attached) continue;

        // World collisions (walls)
        if (CollisionSystem::handleWorldCollisions(ball, m_cfg)) {
            m_audio.playSfx("game/hit_wall", -7.0f);
        }

        // Paddle collision
        if (CollisionSystem::handlePaddleCollision(ball, m_state, m_state.paddlePos, currentPaddleSize, m_cfg)) {
            m_audio.playSfx("game/hit_paddle", -4.5f);
        }

        // Brick collisions
        auto br = CollisionSystem::handleBrickCollisions(ball, m_state, m_cfg);
        if (br.hit) {
            if (br.fireball) {
                m_audio.playSfx("bricks/brick_break", -2.0f);
            } else if (br.broke) {
                m_audio.playSfx("bricks/brick_break", -3.0f);
            } else if (br.cracked) {
                m_audio.playSfx("bricks/brick_crack", -6.0f);
            } else {
                m_audio.playSfx("bricks/hit_brick", -8.0f);
            }
        }
    }

    // Remove balls that were killed by one-shot effects (e.g., Fireball).
    if (!m_state.balls.empty()) {
        m_state.balls.erase(
            std::remove_if(
                m_state.balls.begin(),
                m_state.balls.end(),
                [](const Ball& b) { return !b.alive; }
            ),
            m_state.balls.end()
        );
    }

    // If a one-shot fireball was consumed, respawn a normal attached ball
    // without costing a life.
    if (m_state.pendingRespawnAfterFireball && m_state.balls.empty() && !m_state.winFinisherActive) {
        Ball b;
        b.isFireball = false;
        PhysicsSystem::resetBallToPaddle(b, m_state.paddlePos, m_cfg);
        m_state.balls.push_back(b);
        m_state.pendingRespawnAfterFireball = false;
    }

    // NOTE: Rogue wave progression is handled later (quota/time), not by clearing all bricks.

    // Endless/Rogue score streak logic:
    // Points accumulate into `endlessStreakPoints` as bricks are destroyed.
    // If no new points arrive for ~2.5s, we play a short "banking" animation,
    // then commit the streak into the main score.
    if ((m_state.gameType == GameType::ENDLESS || m_state.gameType == GameType::ROGUE) && m_state.mode == GameMode::PLAYING) {
        float idleToBank = 2.5f;   // ~2-3 seconds
        if (m_state.gameType == GameType::ROGUE) {
            idleToBank *= std::max(0.35f, m_state.rogueBankIdleMult);
        }
        const float bankAnim = 0.55f;    // short float-up

        if (m_state.endlessStreakPoints != 0) {
            if (!m_state.endlessStreakBanking) {
                m_state.endlessStreakIdleTimer += dt;
                if (m_state.endlessStreakIdleTimer >= idleToBank) {
                    m_state.endlessStreakBanking = true;
                    m_state.endlessStreakBankTimer = 0.0f;
                }
            } else {
                m_state.endlessStreakBankTimer += dt;
                if (m_state.endlessStreakBankTimer >= bankAnim) {
                    commitEndlessStreak(m_state);
                    maybeUpdateEndlessBest(m_state);
                    RogueSystem::maybeUpdateRogueBest(m_state);
                }
            }
        }
    }

    // Rogue: wave progression is NOT "clear all bricks".
    // We advance when either:
    // - enough bricks were broken this wave (quota), OR
    // - enough time has elapsed (time limit),
    // gated by a small cooldown to prevent double-advances.
    if (m_state.gameType == GameType::ROGUE && m_state.mode == GameMode::PLAYING && m_state.rogueWaveCooldown <= 0.0f) {
        int required = RogueSystem::bricksRequiredForWave(m_state.wave);
        float minT = RogueSystem::minTimeForWave(m_state.wave);
        float limit = RogueSystem::timeLimitForWave(m_state.wave);
        bool quotaMet = (m_state.rogueWaveTimer >= minT) && (m_state.rogueBricksBrokenThisWave >= required);
        bool timeMet = (m_state.rogueWaveTimer >= limit);

        if (quotaMet || timeMet) {
            int clearedWave = std::max(1, m_state.wave);
            // Make sure banked points count before we move on.
            commitEndlessStreak(m_state);
            RogueSystem::maybeUpdateRogueBest(m_state);

            if (RogueSystem::shouldWinAfterClearingWave(m_state, clearedWave)) {
                m_state.mode = GameMode::WIN;
                m_state.balls.clear();
                m_state.powerups.clear();
                // Immediate results music for Rogue win (returns early).
                setMusic("music_results_loop", 0.25f);
                m_audio.playStinger("stinger_run_complete", +2.5f);
                return;
            }

            m_state.wave = clearedWave + 1;
            RogueSystem::onWaveAdvanced(m_state);

            // Next wave: queue rows to insert. They will spawn AFTER the player picks a card,
            // gradually, not all at once.
            m_state.roguePendingRowsToSpawn = RogueSystem::rowsToInsertForWave(m_state, m_state.wave);

            if (RogueSystem::shouldOfferCardsAfterClearingWave(m_state, clearedWave)) {
                // OP pack on the START of waves 3/6/9/... (i.e. after clearing 2/5/8/...)
                int nextWave = std::max(1, clearedWave + 1);
                bool opPack = (nextWave % 3) == 0 && nextWave >= 3;
                game::rogue::dealOffer(m_state, 3, opPack, nextWave);
                m_state.mode = GameMode::ROGUE_CARDS;
                m_state.hoveredRogueCard = -1;
                m_state.hoveredRogueCardPickButton = -1;
            }
            return;
        }
    }

    // Rogue: after a card pick, insert queued rows gradually (one row per interval).
    if (m_state.gameType == GameType::ROGUE && m_state.mode == GameMode::PLAYING && m_state.roguePendingRowsToSpawn > 0) {
        if (m_state.rogueRowSpawnTimer <= 0.0f) {
            RogueSystem::spawnWaveRows(m_state, m_cfg, /*rowsToInsert=*/1);
            m_state.roguePendingRowsToSpawn--;
            m_state.rogueRowSpawnTimer = m_state.rogueRowSpawnInterval;
        }
    }

    // Perform queued brick spawns (endless mode)
    if (m_state.gameType == GameType::ENDLESS && m_state.pendingSpawnBricks > 0) {
        InitSystem::spawnIncrementalBricks(m_state, m_cfg, m_state.pendingSpawnBricks, m_state.wave);
        m_state.pendingSpawnBricks = 0;
    }

    // Endless mode: time-based pressure (rows spawn over time, but ramps up gently).
    if (m_state.gameType == GameType::ENDLESS && m_state.mode == GameMode::PLAYING && m_state.endlessSpawnCooldown <= 0.0f) {
        const float t = m_state.endlessElapsedTime;

        // Grace period: let the player stabilize first.
        const float grace = 60.0f;
        if (t < grace) {
            m_state.endlessAutoTimer = 0.0f;
        } else {
            // After grace, interval slowly decreases over time:
            // ~26s (just after grace) down to ~12s by ~10 minutes, clamped.
            float u = std::min(1.0f, (t - grace) / 540.0f); // 0..1 over 9 minutes
            float interval = 26.0f - (14.0f * u);
            interval = std::max(12.0f, interval);

            // Avoid stacking multiple rows at once.
            if (m_state.pendingSpawnBricks < 12 && m_state.endlessAutoTimer >= interval) {
                m_state.pendingSpawnBricks += 12;
                m_state.endlessSpawnCooldown = 0.50f;
                m_state.endlessAutoTimer = 0.0f;
            }
        }
    }

    // Endless mode: check for lose condition (brick reached paddle)
    if (m_state.gameType == GameType::ENDLESS) {
        float limitZ = m_state.paddlePos.z - 0.5f;
        float warningThresholdZ = limitZ - (1.33f * 3.0f);

        float maxZFound = -20.0f;
        bool anyBricksAliveInEndless = false;

        for (const auto& br : m_state.bricks) {
            if (!br.alive) continue;
            anyBricksAliveInEndless = true;
            float brMaxZ = br.pos.z + br.size.z * 0.5f;
            if (brMaxZ > maxZFound) maxZFound = brMaxZ;

            if (brMaxZ >= limitZ) {
                // About to lose: commit any pending streak so it counts.
                commitEndlessStreak(m_state);
                maybeUpdateEndlessBest(m_state);
                m_state.mode = GameMode::GAME_OVER;
                m_state.balls.clear();
                break;
            }
        }

        if (m_state.mode != GameMode::GAME_OVER) {
            m_state.endlessDangerActive = (anyBricksAliveInEndless && maxZFound >= warningThresholdZ);
            if (m_state.endlessDangerActive) {
                m_state.endlessDangerTimer += dt;
                m_state.endlessDangerMaxZ = maxZFound;
            } else {
                m_state.endlessDangerTimer = 0.0f;
                m_state.endlessDangerMaxZ = -20.0f;
            }
        }
    }

    // Rogue mode: danger warning + brick-reached-paddle lose condition (same visual language as Endless).
    if (m_state.gameType == GameType::ROGUE) {
        float limitZ = m_state.paddlePos.z - 0.5f;
        float warningThresholdZ = limitZ - (1.33f * 3.0f);

        float maxZFound = -20.0f;
        bool anyAlive = false;

        for (const auto& br : m_state.bricks) {
            if (!br.alive) continue;
            anyAlive = true;
            float brMaxZ = br.pos.z + br.size.z * 0.5f;
            if (brMaxZ > maxZFound) maxZFound = brMaxZ;

            if (brMaxZ >= limitZ) {
                // About to lose: commit any pending streak so it counts.
                commitEndlessStreak(m_state);
                RogueSystem::maybeUpdateRogueBest(m_state);
                m_state.mode = GameMode::GAME_OVER;
                m_state.balls.clear();
                break;
            }
        }

        if (m_state.mode != GameMode::GAME_OVER) {
            m_state.endlessDangerActive = (anyAlive && maxZFound >= warningThresholdZ);
            if (m_state.endlessDangerActive) {
                m_state.endlessDangerTimer += dt;
                m_state.endlessDangerMaxZ = maxZFound;
            } else {
                m_state.endlessDangerTimer = 0.0f;
                m_state.endlessDangerMaxZ = -20.0f;
            }
        }
    }

    // Update power-ups
    PowerUpSystem::updatePowerUps(m_state, m_cfg, dt);

    // Powerup drop loop gate + pickup SFX (systems push types into GameState buffers)
    bool hasPowerupNow = !m_state.powerups.empty();
    if (hasPowerupNow != hadPowerupBefore) {
        if (hasPowerupNow) m_audio.playSfx("powerups/powerup_drop", -4.0f);
    }
    if (!m_state.audioSpawnedPowerups.empty()) {
        // Drop loop already sells the event; keep this silent to avoid spam.
        m_state.audioSpawnedPowerups.clear();
    }
    if (!m_state.audioPickedPowerups.empty()) {
        for (auto t : m_state.audioPickedPowerups) {
            // Slight flavor per type
            if (t == PowerUpType::EXPAND) m_audio.playSfx("powerups/powerup_expand_on", -3.0f);
            else if (t == PowerUpType::SLOW) m_audio.playSfx("powerups/powerup_slow_on", -3.0f);
            else if (t == PowerUpType::EXTRA_BALL) m_audio.playSfx("powerups/powerup_multiball_spawn", -2.0f);
            else m_audio.playSfx("powerups/powerup_pickup", -3.0f);
        }
        m_state.audioPickedPowerups.clear();
    }

    // Handle lives loss
    if (!m_state.winFinisherActive && m_state.balls.empty()) {
        m_state.lives--;
        m_audio.playSfx("game/life_lost", -1.0f);
        // Losing a heart = -points (but Fireball one-shot respawn does NOT reach this path).
        int lossPenalty = m_cfg.lifeLossPenalty;
        if (m_state.gameType == GameType::ROGUE) {
            lossPenalty += m_state.rogueLifeLossPenaltyBonus;
            if (lossPenalty < 0) lossPenalty = 0;
        }
        if (lossPenalty > 0) {
            if (m_state.gameType == GameType::ENDLESS) {
                // Same "bank" logic as the white streak popup:
                // deduct into the bank now, commit later.
                m_state.endlessStreakPoints -= lossPenalty;
                m_state.endlessStreakNegPoints += lossPenalty;
                m_state.endlessStreakIdleTimer = 0.0f;
                m_state.endlessStreakBanking = false;
                m_state.endlessStreakBankTimer = 0.0f;
            } else if (m_state.gameType == GameType::ROGUE) {
                m_state.endlessStreakPoints -= lossPenalty;
                m_state.endlessStreakNegPoints += lossPenalty;
                m_state.endlessStreakIdleTimer = 0.0f;
                m_state.endlessStreakBanking = false;
                m_state.endlessStreakBankTimer = 0.0f;
            } else {
                m_state.score = std::max(0, m_state.score - lossPenalty);
                // Normal mode: don't show score popups.
            }
        }
        if (m_state.lives > 0) {
            Ball b;
            PhysicsSystem::resetBallToPaddle(b, m_state.paddlePos, m_cfg);
            m_state.balls.push_back(b);
        } else {
            if (m_state.gameType == GameType::ENDLESS) {
                commitEndlessStreak(m_state);
                maybeUpdateEndlessBest(m_state);
            }
            if (m_state.gameType == GameType::ROGUE) {
                commitEndlessStreak(m_state);
                RogueSystem::maybeUpdateRogueBest(m_state);
            }
            m_state.mode = InitSystem::anyBricksAlive(m_state) ? GameMode::GAME_OVER : GameMode::WIN;
        }
    }

    // Life gain (powerup / card effects)
    if (m_state.lives > livesBefore) {
        m_audio.playSfx("game/life_gain");
        m_audio.playStinger("stinger_extra_life", +3.0f);
    }

    // Endless / Rogue danger warning loop (tension)
    if ((m_state.gameType == GameType::ENDLESS || m_state.gameType == GameType::ROGUE) && m_state.mode == GameMode::PLAYING) {
        if (m_state.endlessDangerActive && !dangerBefore) {
            m_audio.playSfx("endless/endless_danger_start", -1.0f);
            m_audio.setSfxLoopEnabled("endless/endless_warning_loop", true, 0.20f);
        } else if (!m_state.endlessDangerActive && dangerBefore) {
            m_audio.playSfx("endless/endless_warning_end", -2.0f);
            m_audio.setSfxLoopEnabled("endless/endless_warning_loop", false, 0.20f);
        }
    } else {
        // Only disable loops once when leaving PLAYING. Repeated fade resets can cause occasional audio weirdness.
        if (modeBefore == GameMode::PLAYING && m_state.mode != GameMode::PLAYING) {
            m_audio.setSfxLoopEnabled("endless/endless_warning_loop", false, 0.12f);
        }
    }

    // Rows inserted (Endless / Rogue pacing punctuation)
    if (m_state.gameType == GameType::ENDLESS && m_state.endlessRowsSpawned > endlessRowsBefore) {
        m_audio.playSfx("endless/endless_column_spawn", -3.0f);
    }

    // Banking commit sound (when bank resets to 0 after being non-zero)
    if ((m_state.gameType == GameType::ENDLESS || m_state.gameType == GameType::ROGUE) &&
        m_state.endlessStreakPoints == 0 && streakBefore != 0) {
        m_audio.playSfx("endless/endless_streak_bank", -3.0f);
    }

    // --- Music director (simple, fun, no layer system) ---
    // Enter/exit PLAYING
    if (modeBefore != m_state.mode || typeBefore != m_state.gameType) {
        if (m_state.mode == GameMode::PLAYING) {
            if (m_state.gameType == GameType::ENDLESS) {
                setMusic("music_endless_loop", 0.25f);
                m_endlessMusicTier = 0;
            } else if (m_state.gameType == GameType::ROGUE) {
                setMusic(rogueMusicForWave(m_state.wave), 0.25f);
            } else {
                setMusic("music_gameplay_loop", 0.25f);
            }
        }
    }

    // Immediate GAME OVER music switch (some game-over paths change mode after the overlay handler has already been skipped).
    if (modeBefore == GameMode::PLAYING && m_state.mode == GameMode::GAME_OVER) {
        setMusic("music_results_loop", 0.25f);
        m_audio.playSfx("stingers/stinger_game_over", -1.0f);
        m_audio.playStinger("stinger_game_over", +3.0f);
    }
    // Returning from PAUSED to PLAYING: restore correct loop
    if (modeBefore == GameMode::PAUSED && m_state.mode == GameMode::PLAYING) {
        if (m_state.gameType == GameType::ENDLESS) {
            int tier = 0;
            // Faster hype ramp (Endless runs often don't last long enough to reach old thresholds).
            if (m_state.endlessRowsSpawned >= 10) tier = 1;
            if (m_state.endlessRowsSpawned >= 16) tier = 2;
            m_endlessMusicTier = tier;
            if (tier == 1) setMusic("music_endless_mid_loop", 0.18f);
            else if (tier == 2) setMusic("music_endless_high_loop", 0.18f);
            else setMusic("music_endless_loop", 0.18f);
        } else if (m_state.gameType == GameType::ROGUE) {
            int wave = std::max(1, m_state.wave);
            if (wave >= m_state.rogueMaxWaves) setMusic("music_rogue_boss_loop", 0.18f);
            else if (wave >= 7) setMusic("music_rogue_act3_loop", 0.18f);
            else if (wave >= 4) setMusic("music_rogue_act2_loop", 0.18f);
            else setMusic("music_rogue_act1_loop", 0.18f);
        } else {
            setMusic("music_gameplay_loop", 0.18f);
        }
    }

    // Endless hype tiers (no layer system): switch loops at milestones
    if (m_state.gameType == GameType::ENDLESS && m_state.mode == GameMode::PLAYING) {
        int tier = 0;
        // Faster hype ramp (Endless runs often don't last long enough to reach old thresholds).
        if (m_state.endlessRowsSpawned >= 10) tier = 1;
        if (m_state.endlessRowsSpawned >= 16) tier = 2;
        if (tier != m_endlessMusicTier) {
            m_endlessMusicTier = tier;
            if (tier == 1) setMusic("music_endless_mid_loop", 0.35f);
            else if (tier == 2) setMusic("music_endless_high_loop", 0.35f);
            m_audio.playSfx("endless/endless_difficulty_up", -2.0f);
            m_audio.playStinger("stinger_milestone", +3.0f);
        }
    }

    // Rogue: wave/act punctuation + act music
    if (m_state.gameType == GameType::ROGUE) {
        // If wave changed (advance), do punctuation and update act music immediately.
        if (m_state.wave != waveBefore) {
            m_audio.playSfx("rogue/rogue_wave_clear", -1.5f);
            m_audio.playStinger("stinger_wave_clear", +2.0f);
            setMusic(rogueMusicForWave(m_state.wave), 0.35f);
        } else if (m_state.mode == GameMode::PLAYING) {
            // Keep the correct act loop selected even if we advanced without a card-pick transition.
            setMusic(rogueMusicForWave(m_state.wave), 0.12f);
        }

        // When exiting cards back to gameplay: wave start stingers + boss/elite punctuations.
        if (m_prevMode == GameMode::ROGUE_CARDS && m_state.mode == GameMode::PLAYING) {
            m_audio.playSfx("rogue/rogue_wave_start", -2.0f);
            m_audio.playStinger("stinger_wave_start", +2.0f);

            int wave = std::max(1, m_state.wave);
            if (wave >= m_state.rogueMaxWaves) {
                // Ensure boss loop is locked in.
                setMusic("music_rogue_boss_loop", 0.35f);
                m_audio.playSfx("rogue/rogue_boss_spawn", -2.0f);
                m_audio.playStinger("stinger_boss_spawn", +3.0f);
            } else if (wave % 3 == 0) {
                // Elite stinger on wave 3/6/9 starts (fun punctuation)
                m_audio.playSfx("rogue/rogue_elite_spawn", -2.0f);
                m_audio.playStinger("stinger_elite_spawn", +3.0f);
            }
        }
    }

    // Update prev trackers for next frame (non-early-return path)
    m_prevMode = m_state.mode;
    m_prevGameType = m_state.gameType;
    m_prevMenuScreen = m_state.currentMenuScreen;
    m_prevHoveredMenuButton = m_state.hoveredMenuButton;
    m_prevHoveredPlayModeButton = m_state.hoveredPlayModeButton;
    m_prevShowInstructions = m_state.showInstructions;
    m_prevInstructionsTab = m_state.instructionsTab;
    m_prevHoveredCloseButton = m_state.hoveredCloseButton ? 1 : 0;
    m_prevHoveredPowerupNav = m_state.hoveredPowerupNav;
    m_prevHoveredRogueCard = m_state.hoveredRogueCard;
    m_prevHoveredRoguePickBtn = m_state.hoveredRogueCardPickButton;
    m_prevLives = m_state.lives;
    m_prevWave = m_state.wave;
    m_prevEndlessRowsSpawned = m_state.endlessRowsSpawned;
    m_prevEndlessDangerActive = m_state.endlessDangerActive;
    m_prevStreakPoints = m_state.endlessStreakPoints;
}

} // namespace game


