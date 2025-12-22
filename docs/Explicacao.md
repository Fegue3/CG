# Breakout3D 🎮🧱

Projeto académico de um **jogo Breakout em 3D**, desenvolvido em **C++ com OpenGL**, seguindo uma arquitetura modular e escalável que separa claramente:

- **Engine / Infraestrutura gráfica**
- **Lógica de Jogo (Breakout)**

---

## 🎯 Estado atual do projeto

O projeto encontra-se funcional e inclui:

- Janela OpenGL com suporte a fullscreen
- Câmara 3D com perspetiva
- Paddle controlada pelo utilizador
- Bola com movimento e física básica
- Colisão bola ↔ paredes
- Colisão bola ↔ paddle (ângulo dependente do impacto)
- Grelha de bricks destrutíveis
- Colisão bola ↔ bricks

O jogo já se comporta como um **Breakout 3D jogável**.

---

## 📁 Estrutura do Projeto

```
Breakout3D/
│
├── Makefile
├── README.md
│
├── external/
│   ├── glfw/
│   ├── glad/
│   ├── glm/
│   └── stb_image.h
│
├── assets/
│   ├── shaders/
│   ├── models/
│   ├── textures/
│   └── levels/
│
├── include/
│   ├── engine/
│   └── game/
│       └── entities/
│
├── src/
│   ├── engine/
│   └── game/
│
└── breakout3d
```

---

## 🧱 Arquitetura

### engine/
Camada genérica e reutilizável:
- Janela (GLFW)
- OpenGL
- Shaders
- Meshes
- Renderer
- Câmara

Não contém qualquer lógica específica do jogo.

### game/
Camada específica do Breakout:
- Estado do jogo (bola, paddle, bricks)
- Configuração da arena
- Lógica de colisões
- Regras do jogo

Não comunica diretamente com OpenGL.

### main.cpp
Responsável por:
- Inicializar a engine
- Criar o jogo
- Executar o loop principal

---

## 🕹️ Controlos

| Tecla | Ação |
|------|------|
| A / ← | Mover paddle para a esquerda |
| D / → | Mover paddle para a direita |
| SPACE | Lançar a bola |
| F11 | Fullscreen |
| ESC | Sair |

---

## ⚙️ Compilação e Execução (Linux / WSL)

```bash
make
./breakout3d
```

Limpar:
```bash
make clean
```

---

## 🚀 Próximos passos

- Sistema de pontuação
- Vidas
- Níveis a partir de ficheiros
- Power-ups
- Efeitos visuais e som

---

## 👨‍💻 Notas finais

Este projeto foi desenvolvido com foco em:
- Organização de código
- Separação de responsabilidades
- Facilidade de evolução
- Clareza para avaliação académica
