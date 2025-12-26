#include "game/Game.hpp"
#include "engine/Input.hpp"
#include "game/GameAssets.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <glm/gtc/matrix_transform.hpp>

static float clampf(float v, float a, float b) { return std::max(a, std::min(v, b)); }

static bool sphereAabbXZ(const glm::vec3& c, float r, const glm::vec3& bpos, const glm::vec3& bsize) {
    float hx = bsize.x * 0.5f;
    float hz = bsize.z * 0.5f;

    float minX = bpos.x - hx, maxX = bpos.x + hx;
    float minZ = bpos.z - hz, maxZ = bpos.z + hz;

    float cx = clampf(c.x, minX, maxX);
    float cz = clampf(c.z, minZ, maxZ);

    float dx = c.x - cx;
    float dz = c.z - cz;
    return (dx * dx + dz * dz) <= (r * r);
}

namespace game {

static bool anyBricksAlive(const GameState& s) {
    for (const auto& b : s.bricks) if (b.alive) return true;
    return false;
}

static void resetBallToPaddle(Ball& b, const glm::vec3& paddlePos, const GameConfig& cfg) {
    b.attached = true;
    b.vel = glm::vec3(0.0f);
    b.pos = paddlePos + glm::vec3(
        0.0f, 0.0f,
        -(cfg.paddleSize.z * 0.5f + cfg.ballRadius + 0.15f)
    );
}

static void spawnPowerUp(GameState& s, const glm::vec3& pos, float chance) {
    float r = (float)rand() / (float)RAND_MAX;
    if (r > chance) return;

    PowerUp p;
    p.pos = pos;
    // Lift slightly off the ground initially
    p.pos.y = 0.4f;

    int r2 = rand() % 100;
    if (r2 < 2)        p.type = PowerUpType::EXTRA_LIFE; // 2% (Rarer)
    else if (r2 < 35)  p.type = PowerUpType::EXPAND;     // 33%
    else if (r2 < 68)  p.type = PowerUpType::SLOW;       // 33%
    else               p.type = PowerUpType::EXTRA_BALL; // 32%
    
    s.powerups.push_back(p);
}


static bool pointInRectPx(float px, float py, float x, float y, float w, float h) {
    return (px >= x && px <= x + w && py >= y && py <= y + h);
}

Game::Game(engine::Window& window, engine::Time& time, engine::Renderer& renderer, GameAssets& assets)
: m_window(window), m_time(time), m_renderer(renderer), m_assets(assets) {}

void Game::generateBricks(int waveNumber) {
    m_state.bricks.clear();
    
    const int cols = 12;
    int rows = 9;  // Default for normal mode

    // Adjust difficulty for endless mode
    if (waveNumber > 0) {
        // Endless mode: increase difficulty each wave
        rows = 9 + (waveNumber / 2);               // Add rows every 2 waves (max 14 at wave 10+)
    }

    glm::vec3 brickSize(2.95f, 0.7f, 1.30f); 
    float gapX = 0.04f;
    float gapZ = 0.03f;

    float startZ = m_cfg.arenaMinZ + 0.85f;
    float totalW = cols * brickSize.x + (cols - 1) * gapX;
    float leftX  = -totalW * 0.5f + brickSize.x * 0.5f;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            Brick b;
            b.size = brickSize;
            b.alive = true;

            b.pos.x = leftX + c * (brickSize.x + gapX);
            b.pos.y = 0.0f;
            b.pos.z = startZ + r * (brickSize.z + gapZ);

            // For normal mode: 2 roxo, 2 azul, 2 amarelo, 3 verde
            // For endless mode: all bricks have scaled HP based on wave
            if (waveNumber == 0) {
                // Normal mode: fixed distribution
                if (r <= 1)      b.maxHp = b.hp = 4;
                else if (r <= 3) b.maxHp = b.hp = 3;
                else if (r <= 5) b.maxHp = b.hp = 2;
                else             b.maxHp = b.hp = 1;
            } else {
                // Endless mode: wave-based HP
                // Distribute maxHp across rows (harder at top)
                int hpBonus = waveNumber / 5;  // +1 HP every 5 waves
                if (r < rows / 3)           b.maxHp = b.hp = std::min(6, 4 + hpBonus);
                else if (r < 2 * rows / 3) b.maxHp = b.hp = std::min(6, 3 + hpBonus);
                else if (r < rows - 2)     b.maxHp = b.hp = std::min(6, 2 + hpBonus);
                else                        b.maxHp = b.hp = std::min(6, 1 + hpBonus);
            }

            m_state.bricks.push_back(b);
        }
    }
}

void Game::spawnIncrementalBricks(int count, int waveNumber) {
    // Spawn new bricks by inserting rows at the top, pushing existing bricks down
    glm::vec3 brickSize(2.95f, 0.7f, 1.30f);
    float gapX = 0.04f;
    float gapZ = 0.03f;

    const int cols = 12;
    float totalW = cols * brickSize.x + (cols - 1) * gapX;
    float leftX = -totalW * 0.5f + brickSize.x * 0.5f;
    float stepZ = brickSize.z + gapZ;
    float topZ = m_cfg.arenaMinZ + 0.85f;  // reference for row 0

    // Determine how many full rows to insert at the top
    int rowsToInsert = (count + cols - 1) / cols; // ceil(count/cols)

    // Push all existing alive bricks down by rowsToInsert
    float push = rowsToInsert * stepZ;
    for (auto& br : m_state.bricks) {
        if (!br.alive) continue;
        br.pos.z += push;
    }

    int totalRowsBefore = m_state.endlessRowsSpawned;

    for (int i = 0; i < count; ++i) {
        Brick b;
        b.size = brickSize;
        b.alive = true;

        int col = i % cols;
        int rowLocal = (i / cols); // new rows start at 0 at the top
        int rowGlobal = totalRowsBefore + rowLocal;

        b.pos.x = leftX + col * (brickSize.x + gapX);
        b.pos.y = 0.0f;
        b.pos.z = topZ + rowLocal * stepZ;  // place below the current top

        // Difficulty based on cumulative rows spawned in endless
        int hp = 1 + (rowGlobal / 20); // every 20 rows increases HP by 1
        b.maxHp = b.hp = std::min(6, hp);

        m_state.bricks.push_back(b);
    }

    m_state.endlessRowsSpawned = totalRowsBefore + rowsToInsert;

    // Spawn executed; no HUD ping
}

void Game::init() {
    srand(static_cast<unsigned int>(time(nullptr)));
    m_state.mode = GameMode::PLAYING;
    m_state.lives = 3;
    m_state.bricksDestroyedThisWave = 0;
    m_state.endlessRowsSpawned = 0;
    m_state.score = 0;
    
    // For endless mode, wave is already set before calling init()
    // For normal mode, reset wave to 1
    if (m_state.gameType == GameType::NORMAL) {
        m_state.wave = 1;
    }

    m_state.paddlePos = glm::vec3(
        0.0f, 0.0f,
        m_cfg.arenaMaxZ - (m_cfg.paddleSize.z * 0.5f) - 0.25f
    );

    m_state.balls.clear();
    Ball firstBall;
    resetBallToPaddle(firstBall, m_state.paddlePos, m_cfg);
    m_state.balls.push_back(firstBall);

    m_state.powerups.clear();
    m_state.expandTimer = 0.0f;
    m_state.slowTimer = 0.0f;

    m_state.brickHitCooldown = 0.0f;

    // Generate bricks (0 for normal, waveNumber for endless)
    int waveToGenerate = (m_state.gameType == GameType::ENDLESS) ? m_state.wave : 0;
    int rowsInitial = 9;
    if (m_state.gameType == GameType::ENDLESS) {
        rowsInitial = 9 + (m_state.wave / 2);
    }
    generateBricks(waveToGenerate);
    if (m_state.gameType == GameType::ENDLESS) {
        m_state.endlessRowsSpawned = rowsInitial;
    }
}

void Game::update(const engine::Input& input) {
    float dt = m_time.delta();

    // =========== MENU LOGIC ===========
    if (m_state.mode == GameMode::MENU) {
        auto [fbW, fbH] = m_window.getFramebufferSize();
        auto [px, py_raw] = input.mousePosFbPx();
        float py = (float)fbH - py_raw;
        bool click = input.mousePressed(engine::MouseButton::Left);

        // Menu button positions
        float panelW = 500.0f;
        float panelH = 480.0f;
        float panelX = (fbW - panelW) * 0.5f;
        float panelY = (fbH - panelH) * 0.5f;

        float btnW = 200.0f;
        float btnH = 70.0f;
        float btnX = panelX + (panelW - btnW) * 0.5f;
        
        // Button positions (from top to bottom)
        float btn1Y = panelY + 360.0f; // Normal Mode (top)
        float btn2Y = panelY + 250.0f; // Endless Mode
        float btn3Y = panelY + 140.0f; // Instructions
        float btn4Y = panelY + 30.0f;  // Exit (bottom)

        // If instructions panel is shown, only allow clicking on the panel or outside to close
        if (m_state.showInstructions) {
            float instrW = 600.0f;
            float instrH = 320.0f;
            float instrX = (fbW - instrW) * 0.5f;
            float instrY = (fbH - instrH) * 0.5f;

            if (click) {
                // Click outside the instructions panel = close it
                if (!pointInRectPx(px, py, instrX, instrY, instrW, instrH)) {
                    m_state.showInstructions = false;
                }
            }
            return;
        }

        if (click) {
            // Normal Mode button
            if (pointInRectPx(px, py, btnX, btn1Y, btnW, btnH)) {
                m_state.showInstructions = false;
                m_state.gameType = GameType::NORMAL;
                init();
                return;
            }
            // Endless Mode button
            if (pointInRectPx(px, py, btnX, btn2Y, btnW, btnH)) {
                m_state.showInstructions = false;
                m_state.gameType = GameType::ENDLESS;
                m_state.wave = 1;
                init();
                return;
            }
            // Instructions button
            if (pointInRectPx(px, py, btnX, btn3Y, btnW, btnH)) {
                m_state.showInstructions = true;
            }
            // Exit button
            if (pointInRectPx(px, py, btnX, btn4Y, btnW, btnH)) {
                m_window.requestClose();
                return;
            }
        }

        return; // Don't run game logic while in menu
    }

    // Toggle Pause with Escape
    if (input.keyPressed(engine::Key::Escape)) {
        if (m_state.mode == GameMode::PLAYING) m_state.mode = GameMode::PAUSED;
        else if (m_state.mode == GameMode::PAUSED) m_state.mode = GameMode::PLAYING;
    }

    if (m_state.mode == GameMode::PAUSED) return;

    auto [fbW, fbH] = m_window.getFramebufferSize();

    // --- BG Selector HUD Logic ---
    {
        float boxSize = 30.0f;
        float gap = 10.0f;
        float startX = fbW - (boxSize + gap) * 5.0f - 20.0f;
        float startY = 20.0f; // Top padding

        auto [mx, my_raw] = input.mousePosFbPx();
        float my = (float)fbH - my_raw; 

        if (input.mousePressed(engine::MouseButton::Left)) {
            for (int i = -1; i < 4; i++) {
                float bx = startX + (i + 1) * (boxSize + gap);
                float by = fbH - startY - boxSize;
                if (mx >= bx && mx <= bx + boxSize && my >= by && my <= by + boxSize) {
                    m_state.currentBg = i;
                }
            }
        }
    }

    if (input.keyPressed(engine::Key::K1)) m_state.cameraMode = 1;
    if (input.keyPressed(engine::Key::K2)) m_state.cameraMode = 2;
    if (m_state.gameType == GameType::ENDLESS && input.keyPressed(engine::Key::K3)) {
        // Debug: force-spawn a full top row to keep the wall tidy
        spawnIncrementalBricks(12, m_state.wave);
        m_state.pendingSpawnBricks = 0;
        m_state.endlessSpawnCooldown = 0.5f; // avoid immediate repeats
        m_state.endlessAutoTimer = 0.0f;
    }
    if (m_state.brickHitCooldown > 0.0f)
        m_state.brickHitCooldown = std::max(0.0f, m_state.brickHitCooldown - dt);
    if (m_state.endlessSpawnCooldown > 0.0f)
        m_state.endlessSpawnCooldown = std::max(0.0f, m_state.endlessSpawnCooldown - dt);
    if (m_state.gameType == GameType::ENDLESS)
        m_state.endlessAutoTimer += dt;

    // Only check for WIN in normal mode (endless never wins)
    if (m_state.mode == GameMode::PLAYING && m_state.gameType == GameType::NORMAL && !anyBricksAlive(m_state)) {
        m_state.mode = GameMode::WIN;
        m_state.balls.clear();
    }


    // ------------- GAME OVER / WIN: Click UI logic -------------
    if (m_state.mode == GameMode::GAME_OVER || m_state.mode == GameMode::WIN) {
        auto [px, py_raw]   = input.mousePosFbPx();
        float py = (float)fbH - py_raw; // Flip Y: Top-Left to Bottom-Left
        bool click      = input.mousePressed(engine::MouseButton::Left);

        float panelW = 450.0f;
        float panelH = 200.0f;
        float panelX = (fbW - panelW) * 0.5f;
        float panelY = (fbH - panelH) * 0.5f;
        
        float btnW = 140.0f;
        float btnH = 60.0f;
        float btnGap = 50.0f;
        float btnX_left = panelX + (panelW - 2*btnW - btnGap) * 0.5f;
        float btnX_right = btnX_left + btnW + btnGap;
        float btnY = panelY + 40.0f;

        if (click) {
            // Left button: Restart game immediately
            if (pointInRectPx(px, py, btnX_left, btnY, btnW, btnH)) { 
                init();  // Start new game right away
                return; 
            }
            // Right button: Back to Menu
            if (pointInRectPx(px, py, btnX_right, btnY, btnW, btnH)) { 
                m_state.mode = GameMode::MENU;
                m_state.showInstructions = false;
                return; 
            }
        }
        return;
    }

    // ----------------- PLAYING -----------------
    // Timers
    if (m_state.expandTimer > 0.0f) m_state.expandTimer = std::max(0.0f, m_state.expandTimer - dt);
    if (m_state.slowTimer > 0.0f)   m_state.slowTimer = std::max(0.0f, m_state.slowTimer - dt);

    float currentPaddleSpeed = m_cfg.paddleSpeed;
    if (m_state.slowTimer > 0.0f) currentPaddleSpeed *= m_cfg.slowSpeedFactor;

    glm::vec3 currentPaddleSize = m_cfg.paddleSize;
    if (m_state.expandTimer > 0.0f) currentPaddleSize.x *= m_cfg.expandScaleFactor;

    float dir = 0.0f;
    if (input.keyDown(engine::Key::A) || input.keyDown(engine::Key::Left))  dir -= 1.0f;
    if (input.keyDown(engine::Key::D) || input.keyDown(engine::Key::Right)) dir += 1.0f;

    m_state.paddlePos.x += dir * currentPaddleSpeed * dt;

    float paddleHalfX = currentPaddleSize.x * 0.5f;
    m_state.paddlePos.x = std::clamp(
        m_state.paddlePos.x,
        m_cfg.arenaMinX + paddleHalfX,
        m_cfg.arenaMaxX - paddleHalfX
    );

    // --- BALLS UPDATE ---
    for (size_t i = 0; i < m_state.balls.size(); ) {
        Ball& b = m_state.balls[i];

        if (b.attached) {
            resetBallToPaddle(b, m_state.paddlePos, m_cfg);
            if (input.keyDown(engine::Key::Space)) {
                b.attached = false;
                glm::vec3 d = glm::normalize(glm::vec3(0.30f, 0.0f, -1.0f));
                b.vel = d * m_cfg.ballSpeed;
            }
            i++;
            continue;
        }

        b.pos += b.vel * dt;

        // walls bounce
        if (b.pos.x - m_cfg.ballRadius < m_cfg.arenaMinX) {
            b.pos.x = m_cfg.arenaMinX + m_cfg.ballRadius;
            b.vel.x = std::abs(b.vel.x);
        }
        if (b.pos.x + m_cfg.ballRadius > m_cfg.arenaMaxX) {
            b.pos.x = m_cfg.arenaMaxX - m_cfg.ballRadius;
            b.vel.x = -std::abs(b.vel.x);
        }
        if (b.pos.z - m_cfg.ballRadius < m_cfg.arenaMinZ) {
            b.pos.z = m_cfg.arenaMinZ + m_cfg.ballRadius;
            b.vel.z = std::abs(b.vel.z);
        }

        // Paddle collision
        {
            float halfZ = currentPaddleSize.z * 0.5f;
            float minX = m_state.paddlePos.x - paddleHalfX;
            float maxX = m_state.paddlePos.x + paddleHalfX;
            float minZ = m_state.paddlePos.z - halfZ;
            float maxZ = m_state.paddlePos.z + halfZ;

            float cx = clampf(b.pos.x, minX, maxX);
            float cz = clampf(b.pos.z, minZ, maxZ);

            float dx = b.pos.x - cx;
            float dz = b.pos.z - cz;

            if (dx*dx + dz*dz <= m_cfg.ballRadius*m_cfg.ballRadius) {
                // Hitting the front face
                if (cz == minZ && b.vel.z > 0.0f) {
                    b.pos.z = minZ - m_cfg.ballRadius - 0.002f;
                    float t = (b.pos.x - m_state.paddlePos.x) / paddleHalfX;
                    t = clampf(t, -1.0f, 1.0f);
                    float ang = t * glm::radians(60.0f);
                    b.vel.x = std::sin(ang) * m_cfg.ballSpeed;
                    b.vel.z = -std::cos(ang) * m_cfg.ballSpeed;
                }
                // Hitting the sides
                else if (cx == minX || cx == maxX) {
                    b.vel.x = -b.vel.x;
                    float sideSign = (b.pos.x < cx) ? -1.0f : 1.0f;
                    b.pos.x = cx + sideSign * (m_cfg.ballRadius + 0.002f);
                }
            }
        }

        // Brick collisions
        if (m_state.brickHitCooldown <= 0.0f) {
            for (auto& br : m_state.bricks) {
                if (!br.alive) continue;
                if (sphereAabbXZ(b.pos, m_cfg.ballRadius, br.pos, br.size)) {
                    br.hp--;
                    if (br.hp <= 0) {
                        br.alive = false;
                        spawnPowerUp(m_state, br.pos, m_cfg.powerUpChance);
                        
                        // Add score based on brick HP and wave
                        int baseScore = br.maxHp * 100;  // More points for harder bricks
                        int waveBonus = (m_state.gameType == GameType::ENDLESS) ? m_state.wave * 50 : 0;
                        m_state.score += baseScore + waveBonus;
                        
                        // Count destroyed bricks for endless mode spawning
                        if (m_state.gameType == GameType::ENDLESS) {
                            m_state.bricksDestroyedThisWave++;

                            // Queue new bricks whenever threshold is reached (spawn after loop)
                            if (m_state.bricksDestroyedThisWave >= 30) { // spawn a row every 30 bricks destroyed
                                m_state.pendingSpawnBricks += 12; // add full rows
                                // Keep remainder in case multiple bricks are destroyed rapidly
                                m_state.bricksDestroyedThisWave -= 30;
                            }
                        }
                    }

                    glm::vec3 diff = b.pos - br.pos;
                    float ax = std::abs(diff.x) / (br.size.x * 0.5f);
                    float az = std::abs(diff.z) / (br.size.z * 0.5f);

                    if (ax > az) {
                        b.vel.x = -b.vel.x;
                        float sign = (diff.x >= 0.0f) ? 1.0f : -1.0f;
                        b.pos.x = br.pos.x + sign * (br.size.x * 0.5f + m_cfg.ballRadius + 0.002f);
                    } else {
                        b.vel.z = -b.vel.z;
                        float sign = (diff.z >= 0.0f) ? 1.0f : -1.0f;
                        b.pos.z = br.pos.z + sign * (br.size.z * 0.5f + m_cfg.ballRadius + 0.002f);
                    }
                    m_state.brickHitCooldown = 0.045f;
                    break;
                }
            }
        }

        // Ball lost check - fly away just a bit
        if (b.pos.z - m_cfg.ballRadius > 20.0f) {
            m_state.balls.erase(m_state.balls.begin() + i);
        } else {
            i++;
        }
    }

    // Perform any queued brick spawns now (safe — outside iteration loops)
    if (m_state.gameType == GameType::ENDLESS && m_state.pendingSpawnBricks > 0) {
        spawnIncrementalBricks(m_state.pendingSpawnBricks, m_state.wave);
        m_state.pendingSpawnBricks = 0;
    }

    // (Removed auto/fallback spawns; now spawning is strictly tied to bricks destroyed or manual debug key)

    // Lose condition: a brick reached the paddle zone
    if (m_state.gameType == GameType::ENDLESS) {
        float limitZ = m_state.paddlePos.z - 0.5f; // small margin above paddle
        for (const auto& br : m_state.bricks) {
            if (!br.alive) continue;
            if (br.pos.z + br.size.z * 0.5f >= limitZ) {
                m_state.mode = GameMode::GAME_OVER;
                m_state.balls.clear();
                break;
            }
        }
    }

    // --- POWERUPS UPDATE ---
    for (size_t i = 0; i < m_state.powerups.size(); ) {
        PowerUp& p = m_state.powerups[i];
        p.pos.z += m_cfg.powerUpDropSpeed * dt;

        // Collision with paddle
        float halfX = currentPaddleSize.x * 0.5f;
        float halfZ = currentPaddleSize.z * 0.5f;
        if (std::abs(p.pos.x - m_state.paddlePos.x) < halfX + m_cfg.ballRadius &&
            std::abs(p.pos.z - m_state.paddlePos.z) < halfZ + m_cfg.ballRadius) {
            
            // Activate
            if (p.type == PowerUpType::EXTRA_LIFE) {
                m_state.lives++;
            } else if (p.type == PowerUpType::EXTRA_BALL) {
                for (int k = 0; k < 3; k++) {
                    Ball nb;
                    nb.pos = m_state.paddlePos + glm::vec3(0,0,-0.5f);
                    float ang = glm::radians(-30.0f + k * 30.0f);
                    nb.vel = glm::vec3(std::sin(ang), 0.0f, -std::cos(ang)) * m_cfg.ballSpeed;
                    nb.attached = false;
                    m_state.balls.push_back(nb);
                }
            } else if (p.type == PowerUpType::SLOW) {
                m_state.slowTimer = m_cfg.powerUpDuration;
            } else if (p.type == PowerUpType::EXPAND) {
                m_state.expandTimer = m_cfg.powerUpDuration;
            }

            p.alive = false;
        }

        if (!p.alive || p.pos.z > 20.0f) {
            m_state.powerups.erase(m_state.powerups.begin() + i);
        } else {
            i++;
        }
    }

    // --- LIVES LOSS ---
    if (m_state.balls.empty()) {
        m_state.lives--;
        if (m_state.lives > 0) {
            Ball b;
            resetBallToPaddle(b, m_state.paddlePos, m_cfg);
            m_state.balls.push_back(b);
        } else {
            m_state.mode = anyBricksAlive(m_state) ? GameMode::GAME_OVER : GameMode::WIN;
        }
    }
}

void Game::render() {
    auto [fbW, fbH] = m_window.getFramebufferSize();
    m_renderer.beginFrame(fbW, fbH);

    // =========== MENU RENDER ===========
    if (m_state.mode == GameMode::MENU) {
        // Dark background
        m_renderer.beginUI(fbW, fbH);
        m_renderer.drawUIQuad(0, 0, (float)fbW, (float)fbH, glm::vec4(0.05f, 0.05f, 0.08f, 1.0f));

        // Menu title
        std::string title = "BREAKOUT 3D";
        float titleScale = 4.0f;
        float titleW = title.size() * 14.0f * titleScale;
        float titleX = (fbW - titleW) * 0.5f;
        float titleY = fbH - 100.0f;
        m_renderer.drawUIText(titleX, titleY, title, titleScale, glm::vec3(0.2f, 0.8f, 1.0f));

        // Menu panel
        float panelW = 500.0f;
        float panelH = 480.0f;
        float panelX = (fbW - panelW) * 0.5f;
        float panelY = (fbH - panelH) * 0.5f;

        m_renderer.drawUIQuad(panelX, panelY, panelW, panelH, glm::vec4(0.08f, 0.08f, 0.12f, 0.95f));
        
        // Border
        float borderThickness = 3.0f;
        glm::vec4 borderColor(0.2f, 0.8f, 1.0f, 1.0f);
        m_renderer.drawUIQuad(panelX - borderThickness, panelY - borderThickness, panelW + 2*borderThickness, borderThickness, borderColor);
        m_renderer.drawUIQuad(panelX - borderThickness, panelY + panelH, panelW + 2*borderThickness, borderThickness, borderColor);
        m_renderer.drawUIQuad(panelX - borderThickness, panelY, borderThickness, panelH, borderColor);
        m_renderer.drawUIQuad(panelX + panelW, panelY, borderThickness, panelH, borderColor);

        // Buttons
        float btnW = 200.0f;
        float btnH = 70.0f;
        float btnX = panelX + (panelW - btnW) * 0.5f;

        // Button positions (from top to bottom)
        float btn1Y = panelY + 360.0f; // Normal Mode (top)
        float btn2Y = panelY + 250.0f; // Endless Mode
        float btn3Y = panelY + 140.0f; // Instructions
        float btn4Y = panelY + 30.0f;  // Exit (bottom)

        // Normal Mode button
        m_renderer.drawUIQuad(btnX, btn1Y, btnW, btnH, glm::vec4(0.2f, 0.8f, 0.2f, 1.0f));
        float normalLabelW = 6.0f * 14.0f;
        m_renderer.drawUIText(btnX + (btnW - normalLabelW) * 0.5f, btn1Y + (btnH - 20.0f) * 0.5f, "NORMAL", 1.0f, glm::vec3(1, 1, 1));

        // Endless Mode button
        m_renderer.drawUIQuad(btnX, btn2Y, btnW, btnH, glm::vec4(0.8f, 0.5f, 0.2f, 1.0f));
        float endlessLabelW = 7.0f * 14.0f;
        m_renderer.drawUIText(btnX + (btnW - endlessLabelW) * 0.5f, btn2Y + (btnH - 20.0f) * 0.5f, "ENDLESS", 1.0f, glm::vec3(1, 1, 1));

        // Instructions button
        m_renderer.drawUIQuad(btnX, btn3Y, btnW, btnH, glm::vec4(0.3f, 0.5f, 0.8f, 1.0f));
        float instrLabelW = 12.0f * 14.0f * 0.7f;
        m_renderer.drawUIText(btnX + (btnW - instrLabelW) * 0.5f, btn3Y + (btnH - 20.0f) * 0.5f, "INSTRUCTIONS", 0.85f, glm::vec3(1, 1, 1));

        // Exit button
        m_renderer.drawUIQuad(btnX, btn4Y, btnW, btnH, glm::vec4(0.8f, 0.2f, 0.2f, 1.0f));
        float exitLabelW = 4.0f * 14.0f;
        m_renderer.drawUIText(btnX + (btnW - exitLabelW) * 0.5f, btn4Y + (btnH - 20.0f) * 0.5f, "EXIT", 1.2f, glm::vec3(1, 1, 1));

        // Show instructions if toggled
        if (m_state.showInstructions) {
            // Instructions panel
            float instrW = 600.0f;
            float instrH = 320.0f;
            float instrX = (fbW - instrW) * 0.5f;
            float instrY = (fbH - instrH) * 0.5f;

            m_renderer.drawUIQuad(instrX, instrY, instrW, instrH, glm::vec4(0.05f, 0.05f, 0.1f, 0.98f));
            
            // Instructions border
            glm::vec4 instrBorder(0.4f, 0.6f, 0.9f, 1.0f);
            m_renderer.drawUIQuad(instrX - borderThickness, instrY - borderThickness, instrW + 2*borderThickness, borderThickness, instrBorder);
            m_renderer.drawUIQuad(instrX - borderThickness, instrY + instrH, instrW + 2*borderThickness, borderThickness, instrBorder);
            m_renderer.drawUIQuad(instrX - borderThickness, instrY, borderThickness, instrH, instrBorder);
            m_renderer.drawUIQuad(instrX + instrW, instrY, borderThickness, instrH, instrBorder);

            // Instructions title
            std::string instrTitle = "HOW TO PLAY";
            float instrTitleW = instrTitle.size() * 14.0f * 1.5f;
            m_renderer.drawUIText(instrX + (instrW - instrTitleW) * 0.5f, instrY + instrH - 50.0f, instrTitle, 1.5f, glm::vec3(0.4f, 0.8f, 1.0f));

            // Instructions text
            float textY = instrY + instrH - 100.0f;
            float textX = instrX + 20.0f;
            float lineGap = 28.0f;

            std::vector<std::string> instructions = {
                "A/D or ARROW KEYS: Move paddle",
                "SPACE: Launch ball",
                "ESC: Pause/Resume game",
                "1/2: Change camera view",
                "Destroy all bricks to win!",
                "Click here to go back"
            };

            for (const auto& line : instructions) {
                m_renderer.drawUIText(textX, textY, line, 0.65f, glm::vec3(0.8f, 0.9f, 1.0f));
                textY -= lineGap;
            }
        }

        m_renderer.endUI();
        m_window.swapBuffers();
        return;
    }

    if (m_state.currentBg != -1) {
        m_renderer.drawBackground(m_assets.backgroundTexs[m_state.currentBg].id);
    }

    // -------- 3D PASS --------
    float arenaW = (m_cfg.arenaMaxX - m_cfg.arenaMinX);
    float arenaD = (m_cfg.arenaMaxZ - m_cfg.arenaMinZ);
    float base   = std::max(arenaW, arenaD);
    float zMid   = (m_cfg.arenaMinZ + m_cfg.arenaMaxZ) * 0.5f;

    glm::vec3 camPos, camTarget;
    float fov = 45.0f;

    if (m_state.cameraMode == 1) {
        // Mode 1: Top-down centric - Slightly Zoomed Out
        camPos    = glm::vec3(0.0f, base * 1.03f, zMid + 0.5f);
        camTarget = glm::vec3(0.0f, 0.0f, zMid);
        fov       = 45.0f;
    } else {
        // Mode 2: Angled View - Ultra-tight Zoom
        camPos    = glm::vec3(0.0f, base * 0.62f, base * 0.82f);
        camTarget = glm::vec3(0.0f, 0.0f, 0.8f);
        fov       = 45.0f;
    }

    glm::mat4 V = glm::lookAt(camPos, camTarget, glm::vec3(0,1,0));
    glm::mat4 P = glm::perspective(glm::radians(fov), (float)fbW/(float)fbH, 0.1f, 300.0f);
    m_renderer.setCamera(V, P, camPos);

    // ✅ NÃO usamos tint para “dar cor” (só deixa as texturas falarem)
    glm::vec3 tint(1.0f);

    // Walls (Side rails extended ONLY towards camera)
    float sideThickness = 1.2f;
    float topThickness  = 1.2f;
    float wallHeight    = 1.0f;
    float railLen       = 50.0f;
    float railZStart    = m_cfg.arenaMinZ - topThickness;
    float railZCenter   = railZStart + railLen * 0.5f;

    // Left Rail
    m_renderer.drawMesh(m_assets.brick01,
        glm::vec3(m_cfg.arenaMinX - sideThickness*0.5f, 0.0f, railZCenter),
        glm::vec3(sideThickness, wallHeight, railLen),
        tint
    );

    // Right Rail
    m_renderer.drawMesh(m_assets.brick01,
        glm::vec3(m_cfg.arenaMaxX + sideThickness*0.5f, 0.0f, railZCenter),
        glm::vec3(sideThickness, wallHeight, railLen),
        tint
    );

    // Top Border
    m_renderer.drawMesh(m_assets.brick01,
        glm::vec3(0.0f, 0.0f, m_cfg.arenaMinZ - topThickness*0.5f),
        glm::vec3(arenaW + sideThickness*2.0f, wallHeight, topThickness),
        tint
    );

    // ✅ BRICKS: trocar para *_1hit / *_2hit / *_3hit (texturas/OBJ)
    for (const auto& b : m_state.bricks) {
        if (!b.alive) continue;

        const engine::Mesh* m = &m_assets.brick01;

        if (b.maxHp == 4) {
            if (b.hp == 4)      m = &m_assets.brick04;
            else if (b.hp == 3) m = &m_assets.brick04_1hit;
            else if (b.hp == 2) m = &m_assets.brick04_2hit;
            else                m = &m_assets.brick04_3hit;
        } else if (b.maxHp == 3) {
            if (b.hp == 3)      m = &m_assets.brick03;
            else if (b.hp == 2) m = &m_assets.brick03_1hit;
            else                m = &m_assets.brick03_2hit;
        } else if (b.maxHp == 2) {
            if (b.hp == 2)      m = &m_assets.brick02;
            else                m = &m_assets.brick02_1hit;
        } else {
            m = &m_assets.brick01;
        }

        m_renderer.drawMesh(*m, b.pos, b.size, tint);
    }

    // Paddle
    glm::vec3 currentPaddleSize = m_cfg.paddleSize;
    if (m_state.expandTimer > 0.0f) currentPaddleSize.x *= m_cfg.expandScaleFactor;
    m_renderer.drawMesh(m_assets.paddle, m_state.paddlePos, currentPaddleSize, tint);

    // Balls
    float ballD = m_cfg.ballRadius * 2.0f;
    for (const auto& b : m_state.balls) {
        m_renderer.drawMesh(m_assets.ball, b.pos, glm::vec3(ballD), tint);
    }

    // Powerups
    float powerUpSpin = m_time.now() * 2.5f;
    // 90 degrees (stand up) + ~33 degrees (tilt towards camera) = ~123
    float powerUpTilt = glm::radians(123.0f); 
    float powerUpBob  = std::sin(m_time.now() * 4.0f) * 0.25f;

    for (const auto& p : m_state.powerups) {
        const engine::Mesh* m = &m_assets.ball;

        if (p.type == PowerUpType::EXPAND)           m = &m_assets.expand;
        else if (p.type == PowerUpType::EXTRA_BALL)  m = &m_assets.extraBall;
        else if (p.type == PowerUpType::SLOW)        m = &m_assets.slow;
        else if (p.type == PowerUpType::EXTRA_LIFE)  m = &m_assets.extraLife;
        
        glm::mat4 M(1.0f);
        M = glm::translate(M, p.pos + glm::vec3(0, 0.5f + powerUpBob, 0));
        M = glm::rotate(M, powerUpTilt, glm::vec3(1, 0, 0));
        
        // Snail specific: rotate 90 degrees on Y so its side profile is visible
        float rotation = powerUpSpin;
        if (p.type == PowerUpType::SLOW) rotation += glm::radians(90.0f);
        
        M = glm::rotate(M, rotation, glm::vec3(0, 1, 0));
        
        if (p.type == PowerUpType::EXTRA_LIFE) {
            // Flip 180 on X for the powerup heart specifically
            M = glm::rotate(M, glm::radians(180.0f), glm::vec3(1, 0, 0));
        }

        M = glm::scale(M, glm::vec3(m_cfg.powerUpVisualScale));

        m_renderer.drawMesh(*m, M, tint);
    }

    // -------- UI PASS (HUD 3D em ortho) --------
    m_renderer.beginUI(fbW, fbH);

    const float padX = 22.0f;
    const float padTop = 18.0f;

    const float hs = 56.0f;
    const float gap = 12.0f;

    const float rx = glm::radians(18.0f);
    const float ry = glm::radians(-12.0f);

    int heartCount = std::max(3, m_state.lives);
    for (int i = 0; i < heartCount; ++i) {
        float xCenter = padX + hs * 0.5f + i * (hs + gap);
        float yCenter = (float)fbH - (padTop + hs * 0.5f);

        glm::mat4 M(1.0f);
        M = glm::translate(M, glm::vec3(xCenter, yCenter, 0.0f));
        M = glm::rotate(M, ry, glm::vec3(0,1,0));
        M = glm::rotate(M, rx, glm::vec3(1,0,0));
        M = glm::scale(M, glm::vec3(hs, hs, hs * 0.55f));

        glm::vec3 col = (i < m_state.lives)
            ? glm::vec3(1.0f, 0.20f, 0.25f)
            : glm::vec3(0.20f, 0.20f, 0.22f);

        m_renderer.drawMesh(m_assets.heart, M, col);
    }

    // Overlay (Pause / Game Over / Win)
    if (m_state.mode != GameMode::PLAYING) {
        float panelW = 450.0f;
        float panelH = 200.0f;
        float panelX = (fbW - panelW) * 0.5f;
        float panelY = (fbH - panelH) * 0.5f;
        
        if (m_state.mode == GameMode::PAUSED) {
            // 1. Full screen masked overlay (dimmed outside, clear inside)
            std::string msg = "PAUSED";
            float scale = 3.0f; 
            float charW = 14.0f * scale;
            float charSpacing = 4.0f * scale;
            float tw = msg.size() * charW + (msg.size() - 1) * charSpacing;
            float th = 20.0f * scale;
            
            float tx = (fbW - tw) * 0.5f;
            float ty = (fbH - th) * 0.5f; 
            float padding = 40.0f;

            // Hole should exactly match the box we draw later
            glm::vec2 maskMin(tx - padding, ty - padding);
            glm::vec2 maskMax(tx + tw + padding, ty + th + padding);
            m_renderer.drawUIQuad(0, 0, (float)fbW, (float)fbH, glm::vec4(0, 0, 0, 0.70f), true, maskMin, maskMax); // Darker dimming

            // 2. Pause box and text - Fully opaque box for maximum visibility
            m_renderer.drawUIQuad(tx - padding, ty - padding, tw + padding * 2, th + padding * 2, glm::vec4(0.06f, 0.06f, 0.06f, 1.0f));
            m_renderer.drawUIText(tx, ty, msg, scale, glm::vec3(1, 1, 1));
        } else {
            // Full screen masked overlay for Game Over / Win
            glm::vec2 maskMin(panelX, panelY);
            glm::vec2 maskMax(panelX + panelW, panelY + panelH);
            m_renderer.drawUIQuad(0, 0, (float)fbW, (float)fbH, glm::vec4(0, 0, 0, 0.80f), true, maskMin, maskMax);

            // Panel for Game Over / Win - Fully opaque
            m_renderer.drawUIQuad(panelX, panelY, panelW, panelH, glm::vec4(0.08f, 0.08f, 0.08f, 1.0f));
            
            std::string title = (m_state.mode == GameMode::GAME_OVER) ? "GAME OVER" : "WINNER!";
            float tw = title.size() * 20.0f + (title.size() - 1) * 6.0f;
            float tx = panelX + (panelW - tw * 1.5f) * 0.5f;
            m_renderer.drawUIText(tx, panelY + panelH - 60.0f, title, 1.5f, glm::vec3(1, 1, 1));

            // Buttons
            float btnW = 140.0f;
            float btnH = 60.0f;
            float btnGap = 50.0f;
            float btnX_left = panelX + (panelW - 2*btnW - btnGap) * 0.5f;
            float btnX_right = btnX_left + btnW + btnGap;
            float btnY = panelY + 40.0f;
            
            m_renderer.drawUIQuad(btnX_left, btnY, btnW, btnH, glm::vec4(0.8f, 0.2f, 0.2f, 1.0f));
            m_renderer.drawUIQuad(btnX_right, btnY, btnW, btnH, glm::vec4(0.2f, 0.8f, 0.2f, 1.0f));
            
            std::string leftLabel = "RETRY";
            std::string rightLabel = "MENU";
            
            float labelW = 14.0f;
            float labelSpacing = 4.0f;
            float ltw = leftLabel.size() * labelW + (leftLabel.size() - 1) * labelSpacing;
            float rtw = rightLabel.size() * labelW + (rightLabel.size() - 1) * labelSpacing;
            
            m_renderer.drawUIText(btnX_left + (btnW - ltw) * 0.5f,  btnY + (btnH - 20.0f) * 0.5f, leftLabel, 1.0f, glm::vec3(1, 1, 1));
            m_renderer.drawUIText(btnX_right + (btnW - rtw) * 0.5f, btnY + (btnH - 20.0f) * 0.5f, rightLabel, 1.0f, glm::vec3(1, 1, 1));
        }
    }

    // --- SCORE AND WAVE HUD ---
    {
        float padX = 22.0f;
        float padTop = 100.0f;

        // Score display
        std::string scoreStr = "Score: " + std::to_string(m_state.score);
        m_renderer.drawUIText(padX, (float)fbH - padTop, scoreStr, 1.0f, glm::vec3(1.0f, 0.8f, 0.2f));

        // Wave + Destroyed display (only in endless mode)
        if (m_state.gameType == GameType::ENDLESS) {
            std::string waveStr = "Wave: " + std::to_string(m_state.wave);
            m_renderer.drawUIText(padX, (float)fbH - padTop - 35.0f, waveStr, 1.0f, glm::vec3(0.2f, 0.8f, 1.0f));
        }

    }

    // --- BG Selector HUD ---
    {
        float boxSize = 30.0f;
        float gap = 10.0f;
        float startX = (float)fbW - (boxSize + gap) * 5.0f - 20.0f;
        float startY = (float)fbH - 20.0f - boxSize;

        glm::vec3 colors[] = {
            {0.15f, 0.15f, 0.2f}, // None (Dark Blueish)
            {0.2f, 0.4f, 0.9f}, // Blueish
            {0.6f, 0.2f, 0.9f}, // Purplish
            {0.2f, 0.8f, 0.4f}, // Greenish (Swapped)
            {0.9f, 0.2f, 0.3f}  // Redish (Swapped)
        };

        for (int i = -1; i < 4; i++) {
            float bx = startX + (i + 1) * (boxSize + gap);
            glm::vec3 col = colors[i + 1];
            
            // Draw border if selected
            if (m_state.currentBg == i) {
                float border = 2.0f;
                m_renderer.drawUIQuad(bx - border, startY - border, boxSize + border * 2.0f, boxSize + border * 2.0f, glm::vec4(1, 1, 1, 1.0f));
            } else {
                col *= 0.6f;
            }

            m_renderer.drawUIQuad(bx, startY, boxSize, boxSize, glm::vec4(col, 1.0f));

            if (i == -1) {
                // Draw "/" inside the first box
                float tx = bx + (boxSize - 10.0f) * 0.5f;
                float ty = startY + (boxSize - 16.0f) * 0.5f;
                m_renderer.drawUIText(tx, ty, "/", 0.8f, glm::vec3(1, 1, 1));
            }
        }
    }

    m_renderer.endUI();
    m_window.swapBuffers();
}

} // namespace game
