#include "game/Game.hpp"

#include "game/systems/InitSystem.hpp"

#include <algorithm>
#include <cmath>

namespace game {

void Game::updateTimers(const engine::Input& input, float dt) {
#ifdef BREAKOUT3D_DEBUG
    // Debug key for endless mode
    if (m_state.gameType == GameType::ENDLESS && input.keyPressed(engine::Key::K3)) {
        InitSystem::spawnIncrementalBricks(m_state, m_cfg, 12, m_state.wave);
        m_state.pendingSpawnBricks = 0;
        m_state.endlessSpawnCooldown = 0.5f;
        m_state.endlessAutoTimer = 0.0f;
    }
#endif

    // Update timers
    if (m_state.brickHitCooldown > 0.0f)
        m_state.brickHitCooldown = std::max(0.0f, m_state.brickHitCooldown - dt);
    if (m_state.endlessSpawnCooldown > 0.0f)
        m_state.endlessSpawnCooldown = std::max(0.0f, m_state.endlessSpawnCooldown - dt);
    if (m_state.gameType == GameType::ENDLESS) {
        m_state.endlessAutoTimer += dt;
        if (m_state.mode == GameMode::PLAYING) {
            m_state.endlessElapsedTime += dt;
        }
    }
    if (m_state.gameType == GameType::ROGUE) {
        if (m_state.rogueWaveCooldown > 0.0f) {
            m_state.rogueWaveCooldown = std::max(0.0f, m_state.rogueWaveCooldown - dt);
        }
        if (m_state.rogueRowSpawnTimer > 0.0f) {
            m_state.rogueRowSpawnTimer = std::max(0.0f, m_state.rogueRowSpawnTimer - dt);
        }
        if (m_state.mode == GameMode::PLAYING) {
            m_state.rogueWaveTimer += dt;
        }
    }
    if (m_state.expandTimer > 0.0f) m_state.expandTimer = std::max(0.0f, m_state.expandTimer - dt);
    if (m_state.slowTimer > 0.0f) m_state.slowTimer = std::max(0.0f, m_state.slowTimer - dt);
    if (m_state.fireballTimer > 0.0f) m_state.fireballTimer = std::max(0.0f, m_state.fireballTimer - dt);
    if (m_state.shieldTimer > 0.0f) m_state.shieldTimer = std::max(0.0f, m_state.shieldTimer - dt);
    if (m_state.reverseTimer > 0.0f) m_state.reverseTimer = std::max(0.0f, m_state.reverseTimer - dt);
    if (m_state.tinyTimer > 0.0f) m_state.tinyTimer = std::max(0.0f, m_state.tinyTimer - dt);

    // Fireball explosion FX timers
    if (!m_state.fireballExplosions.empty()) {
        for (size_t i = 0; i < m_state.fireballExplosions.size(); ) {
            m_state.fireballExplosions[i].t += dt;
            if (m_state.fireballExplosions[i].t >= m_cfg.fireballExplosionFxDuration) {
                m_state.fireballExplosions.erase(m_state.fireballExplosions.begin() + i);
            } else {
                ++i;
            }
        }
    }

    // Fireball shake timer
    if (m_state.fireballShakeTimer > 0.0f) {
        m_state.fireballShakeTimer = std::max(0.0f, m_state.fireballShakeTimer - dt);
    }

    // Fireball shard simulation (cheap debris)
    if (!m_state.fireballShards.empty()) {
        const float g = 12.0f;
        for (size_t i = 0; i < m_state.fireballShards.size(); ) {
            auto& s = m_state.fireballShards[i];
            s.t += dt;
            // simple drag on XZ
            float drag = m_cfg.fireballShardDrag;
            s.vel.x *= std::exp(-drag * dt);
            s.vel.z *= std::exp(-drag * dt);
            s.vel.y -= g * dt;
            s.pos += s.vel * dt;
            if (s.t >= m_cfg.fireballShardLife || s.pos.y < -0.25f) {
                m_state.fireballShards.erase(m_state.fireballShards.begin() + i);
            } else {
                ++i;
            }
        }
    }

    // Score popup timers (penalties, etc.)
    if (!m_state.scorePopups.empty()) {
        // Keep longer so penalty popups never feel "instant vanish".
        const float popupDur = 2.10f;
        for (size_t i = 0; i < m_state.scorePopups.size(); ) {
            m_state.scorePopups[i].t += dt;
            if (m_state.scorePopups[i].t >= popupDur) {
                m_state.scorePopups.erase(m_state.scorePopups.begin() + i);
            } else {
                ++i;
            }
        }
    }
}

} // namespace game


