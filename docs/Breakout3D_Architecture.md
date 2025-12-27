# Arquitetura do Projeto Breakout 3D

> Objetivo: ter um projeto **escálavel**, **performante**, **fácil de manter** e **fácil de perceber**, com separação clara entre:
> - **Engine / OpenGL / Infraestrutura**
> - **Lógica de Jogo (Breakout)**

---

## 1. Estrutura de pastas e ficheiros (IMPLEMENTADO)

```text
Breakout3D/Breakout3D/
  Makefile

  external/
    stb_image.h
    (glfw, glad, glm typically installed system-wide)

  assets/
    shaders/
    models/
    textures/

  include/
    engine/
      Window.hpp
      Input.hpp
      Time.hpp
      Shader.hpp
      Texture.hpp
      Mesh.hpp
      Renderer.hpp
    
    game/
      Game.hpp
      GameState.hpp
      GameConfig.hpp
      GameAssets.hpp

      entities/
        Ball.hpp
        Brick.hpp
        Paddle.hpp
        PowerUp.hpp

      systems/
        InitSystem.hpp
        InputSystem.hpp
        PhysicsSystem.hpp
        CollisionSystem.hpp
        PowerUpSystem.hpp

  src/
    main.cpp

    engine/
      Window.cpp
      Input.cpp
      Time.cpp
      Shader.cpp
      Texture.cpp
      Mesh.cpp
      Renderer.cpp

    game/
      Game.cpp
      GameState.cpp
      GameConfig.cpp
      GameAssets.cpp

      systems/
        InitSystem.cpp
        InputSystem.cpp
        PhysicsSystem.cpp
        CollisionSystem.cpp
        PowerUpSystem.cpp
```

### Ideia principal

- Tudo o que é **OpenGL, janela, assets genéricos** vive em `engine/`.
- Tudo o que é **Breakout (regras, entidades, níveis)** vive em `game/`.
- `main.cpp` é o **maestro** que cria engine + jogo e corre o loop.
- **Sistemas** separam a lógica do jogo em módulos independentes e testáveis.

---

## 2. Dependências entre módulos

Regra de ouro:

- `engine/` **NÃO** conhece `game/`.
- `game/` **PODE** usar tipos da engine (ex: tempo, input bruto, interface de renderer).
- `main.cpp` junta as duas coisas.
- **Sistemas** dentro de `game/` são independentes uns dos outros (quando possível).

Diagrama simplificado:

```text
external (GLFW, GLAD, GLM, stb)
        ↓
      engine/
        ↓
       game/
        ├── systems/  (lógica modular)
        ├── entities/ (estruturas de dados)
        └── Game.cpp  (orquestrador)
        ↓
     main.cpp
```

Isto garante que a engine é reutilizável noutros projetos.

---

## 3. Módulo `engine/` (infraestrutura / OpenGL)

### 3.1. Window

**`Window.hpp` / `Window.cpp`**

Responsável por:
- Criar e destruir a janela + contexto OpenGL (GLFW).
- `create(width, height, title)`
- `shouldClose()`
- `pollEvents()`
- `getFramebufferSize()`
- `swapBuffers()`
- `requestClose()`

Não sabe nada sobre o jogo, só janela e eventos.

---

### 3.2. Input (raw)

**`Input.hpp` / `Input.cpp`**

Responsável por:
- Ler **teclas/rato** diretamente da GLFW.
- Guardar o estado de cada tecla/botão.
- Funções do tipo:
  - `keyDown(key)` / `keyPressed(key)` (edge detection)
  - `mousePosFbPx()` - posição do mouse
  - `mousePressed(button)`

Aqui as teclas ainda são "A, D, Space, Escape...", não "moveLeft" ou "launchBall".  
A conversão para ações de jogo é feita em `game/systems/InputSystem`.

---

### 3.3. Time

**`Time.hpp` / `Time.cpp`**

Responsável por:
- Gerir tempo do jogo:
  - `delta()` - tempo desde o último frame
  - `now()` - tempo absoluto desde o início
- Atualizado uma vez por frame no `main.cpp` via `tick()`.

---

### 3.4. Shader

**`Shader.hpp` / `Shader.cpp`**

Wrapper para programas OpenGL:
- Carrega de `assets/shaders/*.vert` / `*.frag`.
- Compila e faz link.
- Tem helpers:
  - `use()`
  - `setMat4(name, mat)`
  - `setVec3(name, vec)`
  - `setFloat(name, v)`  
  etc.

---

### 3.5. Texture

**`Texture.hpp` / `Texture.cpp`**

Responsável por:
- Carregar texturas de `assets/textures/` usando `stb_image.h`.
- Criar textura OpenGL (glTexImage2D, etc.).
- Fazer bind/unbind.
- Guardar `id` da textura OpenGL.

---

### 3.6. Mesh

**`Mesh.hpp` / `Mesh.cpp`**

Responsável por:
- Guardar geometria (VAO, VBO, EBO).
- Carregar `.obj` simples de `assets/models/`, incluindo suporte para materiais (`.mtl`).
- Métodos para desenhar a mesh.

Não sabe nada de "bricks" ou "paddles"; só sabe que desenha uma mesh.

---

### 3.7. Renderer

**`Renderer.hpp` / `Renderer.cpp`**

Responsável por:
- Configurar estado global de OpenGL:
  - depth test, culling, clear color…
- Iniciar frame:
  - `beginFrame(width, height)` → `glClear(...)`
- Terminar frame:
  - `endFrame()` → prepara para swap buffers
- Fornecer funções de desenho de alto nível:
  - `drawMesh(mesh, position, scale, tint)` - desenha mesh 3D
  - `drawMesh(mesh, modelMatrix, tint)` - desenha com matriz custom
  - `beginUI(width, height)` - inicia modo UI (ortho)
  - `drawUIQuad(...)` - desenha quad UI
  - `drawUIText(...)` - desenha texto UI
  - `drawBackground(textureId)` - desenha background
  - `setCamera(view, projection, position)` - define câmera 3D

O `Renderer` **não sabe** o que é "score", "vidas" ou "GameState".  
Só recebe transformações (modelMatrix) e info de material.

---

## 4. Módulo `game/` (lógica do Breakout)

### 4.1. GameConfig

**`GameConfig.hpp` / `GameConfig.cpp`**

Responsável por guardar as **regras do jogo**:
- Velocidades (paddle, bola).
- Limites da arena (minX, maxX, minZ, maxZ).
- Tamanhos (paddleSize, ballRadius).
- Parâmetros de power-ups (chance, duração, velocidades).

Serve de referência para os sistemas.

---

### 4.2. GameState

**`GameState.hpp` / `GameState.cpp`**

Responsável por guardar o **estado mutável** do jogo:

- `enum class GameMode { MENU, PLAYING, PAUSED, GAME_OVER, WIN };`
- `enum class GameType { NORMAL, ENDLESS };`
- `GameMode mode;`
- `int score;`
- `int lives;`
- `int wave;` (para endless mode)
- `glm::vec3 paddlePos;`
- `std::vector<Ball> balls;`
  - `std::vector<Brick> bricks;`
- `std::vector<PowerUp> powerups;`
- Timers (expandTimer, slowTimer, brickHitCooldown, etc.)
- UI state (cameraMode, currentBg, showInstructions, etc.)

É o "snapshot" completo do mundo numa frame.

---

### 4.3. GameAssets

**`GameAssets.hpp` / `GameAssets.cpp`**

Responsável por:
- Carregar todos os modelos (meshes) e texturas do jogo.
- Armazenar referências para meshes de bricks, paddle, ball, power-ups, etc.
- `loadAll()` - carrega todos os assets
- `destroy()` - liberta recursos

---

### 4.4. Game

**`Game.hpp` / `Game.cpp`**

Classe principal da camada de jogo:

- Guarda:
  - `GameState m_state;`
  - `GameConfig m_cfg;`
  - referências para:
    - `engine::Window`
    - `engine::Time`
    - `engine::Renderer`
    - `GameAssets`
- Métodos públicos:
  - `init()` - inicializa o jogo
  - `update(const engine::Input& input)` - atualiza lógica do jogo
  - `render()` - desenha o jogo

**Internamente, `Game` delega para os sistemas:**
- `InitSystem::initGame()` - inicialização
- `InputSystem::handleMenuInput()` / `handleGameInput()` - input
- `PhysicsSystem::updatePaddle()` / `updateBalls()` - física
- `CollisionSystem::handleWorldCollisions()` / `handlePaddleCollision()` / `handleBrickCollisions()` - colisões
- `PowerUpSystem::updatePowerUps()` - power-ups

---

### 4.5. Entidades (`game/entities/`)

Cada ficheiro representa um tipo de objeto no mundo.  
São basicamente **estruturas de dados**; não têm lógica pesada (toda a lógica está nos sistemas).

#### Ball

**`Ball.hpp`** (header-only)

- `glm::vec3 pos` - posição
- `glm::vec3 vel` - velocidade
- `bool attached` - se está presa ao paddle

#### Brick

**`Brick.hpp`** (header-only)

- `glm::vec3 pos` - posição
- `glm::vec3 size` - tamanho
- `bool alive` - se está vivo
- `int hp` - pontos de vida atuais
- `int maxHp` - pontos de vida máximos (1-6)

#### Paddle

**`Paddle.hpp`** (header-only)

- `glm::vec3 pos` - posição
- (tamanho e velocidade vêm de `GameConfig`)

#### PowerUp

**`PowerUp.hpp`** (header-only)

- `PowerUpType type` - tipo (EXPAND, EXTRA_BALL, SLOW, EXTRA_LIFE)
- `glm::vec3 pos` - posição
- `bool alive` - se está ativo
- `enum class PowerUpType` - definido aqui

---

### 4.6. Sistemas (`game/systems/`)

Cada sistema tem uma **responsabilidade clara** e mexe em partes do `GameState`.  
Todos os métodos são **estáticos** - não mantêm estado próprio.

#### InitSystem

**`InitSystem.hpp` / `InitSystem.cpp`**

Responsável por:
- `initGame(GameState& state, const GameConfig& cfg)` - inicializa jogo completo
- `generateBricks(GameState& state, const GameConfig& cfg, int waveNumber)` - gera bricks
- `spawnIncrementalBricks(...)` - spawna bricks adicionais (endless mode)
- `resetBallAndPaddle(...)` - reseta bola e paddle
- `anyBricksAlive(const GameState& state)` - verifica se há bricks vivos

Cuida da criação inicial do mundo e reset após perda de vida.

---

#### InputSystem

**`InputSystem.hpp` / `InputSystem.cpp`**

Responsável por:
- `handleMenuInput(GameState& state, const engine::Input& input, engine::Window& window)` - processa input do menu
- `handleGameInput(GameState& state, const engine::Input& input, const GameConfig& cfg, engine::Window& window, float dt)` - processa input do jogo

Funções:
- Em `MENU`: detecta cliques nos botões, muda modo, fecha jogo
- Em `PLAYING`: move paddle, lança bola, pausa, muda câmera, seleciona background
- Em `PAUSED`: resume jogo
- Em `GAME_OVER`/`WIN`: botões de restart/menu

---

#### PhysicsSystem

**`PhysicsSystem.hpp` / `PhysicsSystem.cpp`**

Responsável por:
- `updatePaddle(GameState& state, const GameConfig& cfg, float dir, float dt)` - atualiza posição do paddle
- `updateBalls(GameState& state, const GameConfig& cfg, float dt)` - atualiza posições das bolas
- `resetBallToPaddle(Ball& ball, const glm::vec3& paddlePos, const GameConfig& cfg)` - reseta bola para paddle

Tarefas:
- Atualizar posição de bolas (movimento baseado em velocidade)
- Aplicar clamps (paddle não sai do campo)
- Remover bolas que saíram do campo
- **NÃO trata colisões**; só integração de física.

---

#### CollisionSystem

**`CollisionSystem.hpp` / `CollisionSystem.cpp`**

Responsável por **TODAS as colisões**:

- `handleWorldCollisions(Ball& ball, const GameConfig& cfg)` - colisões com paredes
  - bola vs paredes (MinX, MaxX, MinZ) → reflete direção
- `handlePaddleCollision(Ball& ball, const glm::vec3& paddlePos, const glm::vec3& paddleSize, const GameConfig& cfg)` - colisão com paddle
  - bola vs paddle → muda direção com base no ponto de impacto
  - calcula ângulo de reflexão baseado na posição do impacto
- `handleBrickCollisions(Ball& ball, GameState& state, const GameConfig& cfg)` - colisões com bricks
  - bola vs bricks → reflete velocidade, remove/atualiza bricks, aumenta score
  - spawna power-ups quando brick é destruído
  - atualiza contadores para endless mode

Aqui vive a matemática de colisão (esfera–AABB em XZ, etc.).

---

#### PowerUpSystem

**`PowerUpSystem.hpp` / `PowerUpSystem.cpp`**

Responsável por:
- `spawnPowerUp(GameState& state, const glm::vec3& pos, float chance)` - spawna power-up aleatório
- `updatePowerUps(GameState& state, const GameConfig& cfg, float dt)` - atualiza power-ups
  - move power-ups para baixo (física)
  - detecta colisão com paddle
  - aplica efeitos quando coletado
- `applyPowerUpEffect(GameState& state, const GameConfig& cfg, PowerUpType type)` - aplica efeito do power-up
  - EXTRA_LIFE: adiciona vida
  - EXTRA_BALL: spawna 3 novas bolas
  - SLOW: ativa slow timer
  - EXPAND: ativa expand timer

---

## 5. `main.cpp` – o maestro

`src/main.cpp` faz a cola de tudo:

1. Cria objetos da engine:
   - `Window window;`
   - `Input input;`
   - `Time time;`
   - `Renderer renderer;`
   - `GameAssets assets;`
2. Inicializa:
   - `window.create(1280, 720, "Breakout3D")`
   - `renderer.init()`
   - `assets.loadAll()`
3. Cria o jogo:
   - `Game game(window, time, renderer, assets);`
   - (não chama `init()` - jogo começa em MENU)
4. Loop principal:

```cpp
while (!window.shouldClose()) {
    time.tick();
    window.pollEvents();

    input.update(window);
    game.update(input);   // lógica do jogo (sistemas)
    game.render();        // pede ao renderer para desenhar
}
```

5. Cleanup e sair:
   - `assets.destroy()`
   - `renderer.shutdown()`
   - `window.destroy()`

---

## 6. Fluxo por frame (visão de alto nível)

1. **Engine** (no `main.cpp`)
   - atualiza tempo (`time.tick()`)
   - lê input bruto (`input.update(window)`)

2. **Game::update()**
   - Se `MENU`: `InputSystem::handleMenuInput()` → processa cliques
   - Se `PLAYING`:
     - `InputSystem::handleGameInput()` → move paddle, lança bola, pausa
     - Atualiza timers (power-ups, cooldowns)
     - `PhysicsSystem::updateBalls()` → move bolas
     - Para cada bola:
       - `CollisionSystem::handleWorldCollisions()` → colisões com paredes
       - `CollisionSystem::handlePaddleCollision()` → colisão com paddle
       - `CollisionSystem::handleBrickCollisions()` → colisões com bricks
     - `PowerUpSystem::updatePowerUps()` → atualiza power-ups
     - Verifica condições de win/loss
     - Spawna novos bricks (endless mode)

3. **Game::render()**
   - Se `MENU`: desenha menu UI
   - Se `PLAYING`:
     - Define câmera (modo 1 ou 2)
     - Desenha paredes, bricks, paddle, bolas, power-ups (3D pass)
     - Desenha HUD (vidas, score, wave) (UI pass)
     - Desenha overlays (pause, game over, win)

4. **Engine** (no `main.cpp`)
   - `window.swapBuffers()` → apresenta frame

---

## 7. Benefícios desta arquitetura

- **Escalabilidade**  
  Podes adicionar features (powerups, mais níveis, modos de jogo) mexendo apenas nos sistemas relevantes.

- **Performance**  
  Otimizações gráficas ficam em `engine/Renderer`, sem tocar na lógica.

- **Usabilidade / Manutenção**  
  Qualquer pessoa que abra o projeto:
  - sabe que `engine/` é infra genérica,
  - `game/` é só Breakout,
  - `systems/` contém lógica modular,
  - `main.cpp` é o ponto de entrada.

- **Clareza**  
  Cada módulo tem responsabilidade pequena e clara:
  - *Window* – janela e contexto
  - *Input* – leitura de teclas/rato
  - *GameState* – dados do mundo
  - *Systems* – regras específicas por área (input, física, colisões, power-ups)
  - *Renderer* – desenhar, nada mais

- **Testabilidade**  
  Sistemas podem ser testados independentemente, pois são funções estáticas que recebem estado.

- **Redução de Complexidade**  
  `Game.cpp` reduziu de **1085 linhas** para **~176 linhas**, delegando para sistemas especializados.

---

## 8. Status da Implementação

### ✅ Implementado

- **Engine Layer**: Completo (Window, Input, Time, Shader, Texture, Mesh, Renderer)
- **Entities**: Ball, Brick, Paddle, PowerUp
- **Systems**: InitSystem, InputSystem, PhysicsSystem, CollisionSystem, PowerUpSystem
- **Game**: GameState, GameConfig, GameAssets, Game (orquestrador)

### 🚧 Não Implementado (mas planeado)

- **InputMapping**: Abstração de input (atualmente InputSystem faz isso diretamente)
- **LevelSystem**: Sistema dedicado para progressão de níveis/waves
- **GameStateSystem**: Sistema para regras globais de estado (vidas, win/loss)
- **Camera** (como classe separada): Atualmente calculada diretamente no render

Estes podem ser adicionados futuramente conforme necessário.

---

Este ficheiro documenta a arquitetura **atualmente implementada** do projeto.
