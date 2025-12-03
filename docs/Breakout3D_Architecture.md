# Arquitetura do Projeto Breakout 3D

> Objetivo: ter um projeto **escálavel**, **performante**, **fácil de manter** e **fácil de perceber**, com separação clara entre:
> - **Engine / OpenGL / Infraestrutura**
> - **Lógica de Jogo (Breakout)**

---

## 1. Estrutura de pastas e ficheiros

```text
Breakout3D/
  Makefile

  external/
    glfw/
    glad/
    glm/
    stb_image.h

  assets/
    shaders/
      basic_phong.vert
      basic_phong.frag
      ui_color.frag
    models/
      paddle.obj
      brick.obj
      wall.obj
      ball.obj
    textures/
      paddle_diffuse.png
      brick_diffuse.png
      wall_diffuse.png
      ball_diffuse.png
    levels/
      level01.txt
      level02.txt

  include/
    engine/
      Window.hpp
      Input.hpp
      Time.hpp
      Camera.hpp
      Shader.hpp
      Texture.hpp
      Mesh.hpp
      Renderer.hpp
      ResourceManager.hpp
    game/
      Game.hpp
      GameState.hpp
      GameConfig.hpp
      InputMapping.hpp

      entities/
        Paddle.hpp
        Ball.hpp
        Brick.hpp
        Wall.hpp
        PowerUp.hpp

      systems/
        InitSystem.hpp
        InputSystem.hpp
        PhysicsSystem.hpp
        CollisionSystem.hpp
        LevelSystem.hpp
        GameStateSystem.hpp

  src/
    main.cpp

    engine/
      Window.cpp
      Input.cpp
      Time.cpp
      Camera.cpp
      Shader.cpp
      Texture.cpp
      Mesh.cpp
      Renderer.cpp
      ResourceManager.cpp

    game/
      Game.cpp
      GameState.cpp
      GameConfig.cpp
      InputMapping.cpp

      entities/
        Paddle.cpp
        Ball.cpp
        Brick.cpp
        Wall.cpp
        PowerUp.cpp

      systems/
        InitSystem.cpp
        InputSystem.cpp
        PhysicsSystem.cpp
        CollisionSystem.cpp
        LevelSystem.cpp
        GameStateSystem.cpp
```

### Ideia principal

- Tudo o que é **OpenGL, janela, assets genéricos** vive em `engine/`.
- Tudo o que é **Breakout (regras, entidades, níveis)** vive em `game/`.
- `main.cpp` é o **maestro** que cria engine + jogo e corre o loop.

---

## 2. Dependências entre módulos

Regra de ouro:

- `engine/` **NÃO** conhece `game/`.
- `game/` **PODE** usar tipos da engine (ex: tempo, input bruto, interface de renderer).
- `main.cpp` junta as duas coisas.

Diagrama simplificado:

```text
external (GLFW, GLAD, GLM, stb)
        ↓
      engine/
        ↓
       game/
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
- `getSize()`

Não sabe nada sobre o jogo, só janela e eventos.

---

### 3.2. Input (raw)

**`Input.hpp` / `Input.cpp`**

Responsável por:

- Ler **teclas/rato** diretamente da GLFW.
- Guardar o estado de cada tecla/botão.
- Funções do tipo:
  - `isKeyDown(key)`
  - `isKeyPressed(key)` (edge)
  - `getMousePos()`

Aqui as teclas ainda são “W, A, S, D, Espaço…”, não “moveLeft” ou “launchBall”.  
A conversão para ações de jogo é feita em `game/InputMapping`.

---

### 3.3. Time

**`Time.hpp` / `Time.cpp`**

Responsável por:

- Gerir tempo do jogo:
  - `float getDeltaTime()`
  - `float getTime()`
- Atualizado uma vez por frame no `main.cpp`.

---

### 3.4. Camera

**`Camera.hpp` / `Camera.cpp`**

Responsável por:

- Guardar:
  - posição
  - target
  - up
  - fov / near / far
- Gerar:
  - `glm::mat4 getViewMatrix()`
  - `glm::mat4 getProjectionMatrix(aspectRatio)`

A lógica de onde a câmara deve olhar (paddle, centro do campo, etc.) pode ser controlada pelo jogo, mas o cálculo de matrizes vive aqui.

---

### 3.5. Shader

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

### 3.6. Texture

**`Texture.hpp` / `Texture.cpp`**

Responsável por:

- Carregar texturas de `assets/textures/` usando `stb_image.h`.
- Criar textura OpenGL (glTexImage2D, etc.).
- Fazer bind/unbind.

---

### 3.7. Mesh

**`Mesh.hpp` / `Mesh.cpp`**

Responsável por:

- Guardar geometria (VAO, VBO, EBO).
- Gerar formas básicas:
  - cubo, esfera, plano…
- Carregar `.obj` simples de `assets/models/`.

Não sabe nada de “bricks” ou “paddles”; só sabe que desenha uma mesh.

---

### 3.8. ResourceManager

**`ResourceManager.hpp` / `ResourceManager.cpp`**

Responsável por:

- Ser um “repositório” de:
  - shaders
  - meshes
  - textures
- Interface típica:
  - `loadShader(name, vertPath, fragPath)`
  - `getShader(name)`
  - `loadTexture(name, path)`
  - `getTexture(name)`
  - `createMeshCube(name)`
  - `getMesh(name)`

É aqui que o `Renderer` vai buscar recursos para desenhar.

---

### 3.9. Renderer

**`Renderer.hpp` / `Renderer.cpp`**

Responsável por:

- Configurar estado global de OpenGL:
  - depth test, culling, clear color…
- Iniciar frame:
  - `beginFrame()` → `glClear(...)`
- Terminar frame:
  - `endFrame()` → `swap buffers`
- Fornecer funções de desenho de alto nível:
  - `drawMesh(mesh, transform, materialInfo)`
- Opcional: definir uma interface `IRenderer` que o jogo pode usar sem conhecer OpenGL diretamente.

O `Renderer` **não sabe** o que é “score”, “vidas” ou “GameState”.  
Só recebe transformações (modelMatrix) e info de material.

---

## 4. Módulo `game/` (lógica do Breakout)

### 4.1. GameConfig

**`GameConfig.hpp` / `GameConfig.cpp`**

Responsável por guardar as **regras do jogo**:

- Velocidades (paddle, bola).
- Número de vidas.
- Limites da arena (minX, maxX, etc.).
- Pontuação por tipo de brick.
- Parâmetros de física (bónus, atrito, etc.).

Serve de referência para os sistemas.

---

### 4.2. GameState

**`GameState.hpp` / `GameState.cpp`**

Responsável por guardar o **estado mutável** do jogo:

- `enum class GameMode { MENU, PLAYING, PAUSED, LEVEL_COMPLETE, GAME_OVER };`
- `GameMode mode;`
- `int score;`
- `int lives;`
- `int currentLevel;`
- `float timeSinceLevelStart;`
- Entidades:
  - `Paddle paddle;`
  - `Ball ball;`
  - `std::vector<Brick> bricks;`
  - `std::vector<Wall> walls;`
  - `std::vector<PowerUp> powerUps;` (opcional)

É o “snapshot” completo do mundo numa frame.

---

### 4.3. InputMapping

**`InputMapping.hpp` / `InputMapping.cpp`**

Responsável por converter input bruto da engine para ações de jogo.

- Define `struct GameInput`:
  - `bool moveLeft;`
  - `bool moveRight;`
  - `bool launch;`
  - `bool pause;`
  - `bool restart;`
  - `bool nextLevel;`
- Função:
  - `GameInput mapInput(const engine::Input& rawInput);`

Separar isto ajuda a mudar controlos sem tocar na lógica do jogo.

---

### 4.4. Game

**`Game.hpp` / `Game.cpp`**

Classe principal da camada de jogo:

- Guarda:
  - `GameState state;`
  - `GameConfig config;`
  - `GameInput currentInput;`
  - referências para:
    - `engine::Renderer`
    - `engine::Input`
    - `engine::Time`
- Métodos públicos:
  - `void init();`
  - `void update();`
  - `void render();`

Internamente, `Game` usa os “sistemas” para fazer o trabalho: InitSystem, InputSystem, PhysicsSystem, etc.

---

### 4.5. Entidades (`game/entities/`)

Cada ficheiro representa um tipo de objeto no mundo.  
São basicamente estruturas de dados + helpers; não têm lógica pesada.

#### Paddle

**`Paddle.hpp` / `Paddle.cpp`**

- posição, tamanho, velocidade
- maxSpeed, etc.

#### Ball

**`Ball.hpp` / `Ball.cpp`**

- posição, velocidade, raio
- `bool attachedToPaddle`

#### Brick

**`Brick.hpp` / `Brick.cpp`**

- posição, tamanho, hitPoints
- `int scoreValue`
- tipo (normal, duro, indestrutível, powerup)

#### Wall

**`Wall.hpp` / `Wall.cpp`**

- posição, tamanho
- tipo (left, right, top, back, floor)

#### PowerUp (opcional)

**`PowerUp.hpp` / `PowerUp.cpp`**

- tipo (extra life, bigger paddle, etc.)
- duração
- posição, velocidade (a cair)

---

### 4.6. Sistemas (`game/systems/`)

Cada sistema tem uma responsabilidade clara e mexe em partes do `GameState`.

#### InitSystem

**`InitSystem.hpp` / `InitSystem.cpp`**

Responsável por:

- `void initGame(GameState& state, const GameConfig& config);`
- `void loadLevel(GameState& state, int levelIndex);`
- `void resetBallAndPaddle(GameState& state, const GameConfig& config);`

Cuida da criação inicial do mundo e reset após perda de vida.

---

#### InputSystem

**`InputSystem.hpp` / `InputSystem.cpp`**

Responsável por:

- `void handleInput(GameState& state, const GameConfig& config, const GameInput& input, float dt);`

Funções:

- Em `MENU`:
  - start → `mode = PLAYING`.
- Em `PLAYING`:
  - move paddle com base em `moveLeft`/`moveRight`.
  - se bola presa e `launch` → soltar bola.
  - `pause` → `mode = PAUSED`.
- Em `PAUSED`:
  - `pause` → voltar a `PLAYING`.
- Em `GAME_OVER`:
  - `restart` → `InitSystem::initGame(...)`.
- Em `LEVEL_COMPLETE`:
  - `nextLevel` → `loadLevel(...)`.

---

#### PhysicsSystem

**`PhysicsSystem.hpp` / `PhysicsSystem.cpp`**

Responsável por:

- `void updatePhysics(GameState& state, float dt);`

Tarefas:

- Atualizar posição de:
  - bola
  - paddle
  - powerups
- Aplicar:
  - clamps (paddle não sai do campo)
  - limites de velocidade (se quiseres)
- NÃO trata colisões; só integração.

---

#### CollisionSystem

**`CollisionSystem.hpp` / `CollisionSystem.cpp`**

Responsável por TODAS as colisões:

- `void handleWorldCollisions(GameState& state, const GameConfig& config);`
  - bola vs paredes / fundo → reflete direção, marca perda de vida.
- `void handlePaddleCollision(GameState& state, const GameConfig& config);`
  - bola vs paddle → muda direção com base no ponto de impacto.
- `void handleBrickCollisions(GameState& state, const GameConfig& config);`
  - bola vs bricks → reflete velocidade, remove/atualiza bricks, aumenta score.
- `void handlePowerUpCollisions(GameState& state, const GameConfig& config);`
  - powerups vs paddle → aplicar efeitos.

Aqui vive a matemática de colisão (esfera–AABB, etc.).

---

#### LevelSystem

**`LevelSystem.hpp` / `LevelSystem.cpp`**

Responsável por:

- `bool isLevelCleared(const GameState& state);`
- `void advanceLevel(GameState& state, const GameConfig& config);`

Decide quando todos os bricks importantes acabaram e chama `InitSystem::loadLevel` para o próximo nível.

---

#### GameStateSystem

**`GameStateSystem.hpp` / `GameStateSystem.cpp`**

Responsável por regras globais:

- `void updateGameState(GameState& state, const GameConfig& config);`

Tarefas típicas:

- Se perdeu vida:
  - reduzir `lives`.
  - se `lives > 0` → reset ball+paddle.
  - senão → `mode = GAME_OVER`.
- Se nível completo:
  - `mode = LEVEL_COMPLETE`.

---

## 5. `main.cpp` – o maestro

`src/main.cpp` faz a cola de tudo:

1. Cria objetos da engine:
   - `Window window;`
   - `Input input;`
   - `Time time;`
   - `Renderer renderer;`
   - `ResourceManager resources;`
2. Inicializa:
   - `window.create(...)`
   - `renderer.init(...)`
3. Cria o jogo:
   - `Game game(renderer, input, time);`
   - `game.init();`
4. Loop principal:

```cpp
while (!window.shouldClose()) {
    time.updateDelta();
    window.pollEvents();

    game.update();   // lógica do jogo (sistemas)
    game.render();   // pede ao renderer para desenhar o GameState atual
}
```

5. Cleanup e sair.

---

## 6. Fluxo por frame (visão de alto nível)

1. **Engine**
   - atualiza tempo (`deltaTime`)
   - lê input bruto (teclas/rato)
2. **Game**
   - mapeia input → `GameInput`
   - `InputSystem` → atualiza intenções (mover paddle, lançar bola, pausar)
   - `PhysicsSystem` → move entidades
   - `CollisionSystem` → resolve colisões (bola/paddle/bricks/paredes)
   - `LevelSystem` / `GameStateSystem` → vidas, score, avanço de nível
3. **Renderer**
   - recebe o `GameState`
   - desenha:
     - paredes / chão
     - paddle
     - bola
     - bricks
     - powerups (se houver)
     - HUD (vidas, score)

---

## 7. Benefícios desta arquitetura

- **Escalabilidade**  
  Podes adicionar features (powerups, mais níveis, modos de jogo) mexendo apenas em `game/`.

- **Performance**  
  Otimizações gráficas ficam em `engine/Renderer`, sem tocar na lógica.

- **Usabilidade / Manutenção**  
  Qualquer pessoa que abra o projeto:
  - sabe que `engine/` é infra genérica,
  - `game/` é só Breakout,
  - `main.cpp` é o ponto de entrada.

- **Clareza**  
  Cada módulo tem responsabilidade pequena e clara:
  - *Window* – janela e contexto
  - *Input* – leitura de teclas/rato
  - *GameState* – dados do mundo
  - *Systems* – regras específicas por área (input, física, colisões…)
  - *Renderer* – desenhar, nada mais.

---

Este ficheiro serve como referência para implementares aos poucos.  
Podes começar com uma versão reduzida (sem PowerUp, sem LevelSystem separado) e ir aproximando-te deste “final product” à medida que o projeto cresce.
