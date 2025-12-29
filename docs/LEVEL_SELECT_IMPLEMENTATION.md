# Level Select Screen Implementation

## Overview
Menu de seleção de níveis (level select) para o modo LEVELS, permitindo escolher qual dos **20 níveis** jogar em vez de progressão linear automática. Atualizado para suportar sistema de 20 níveis com dificuldade progressiva.

## Características Implementadas

### 1. **Menu de Seleção de Níveis**
- **Acesso**: Ao clicar em PLAY no modo LEVELS, abre o menu de seleção
- **Layout**: Grade de 4 linhas × 5 colunas (**20 níveis totais**)
- **Design**: Inspirado no screenshot fornecido pelo utilizador

### 2. **Sistema de Desbloqueio**
- Apenas o **nível 1** está desbloqueado inicialmente
- Completar um nível **desbloqueia o próximo**
- `levelsBestLevel` rastreia o nível mais alto alcançado
- Níveis bloqueados aparecem escurecidos e não são clicáveis
- **Dificuldade Progressiva**:
  - Níveis 1-10: Dificuldade variável (HP 1-4)
  - Níveis 11-15: Dificuldade intermediária (HP mínimo 2)
  - Níveis 16-20: Dificuldade máxima (HP mínimo 3, grids 12×7-9)

### 3. **Sistema de Estrelas**
- **3 estrelas**: Completar nível com 3 vidas restantes
- **2 estrelas**: Completar nível com 2 vidas restantes
- **1 estrela**: Completar nível com 1 vida restante
- As estrelas são exibidas embaixo do número do nível
- Estrelas salvas permanentemente (melhor desempenho)

### 4. **Interação**
- **Hover**: Botão de nível ilumina quando o rato passa por cima
- **Click**: Clicar num nível desbloqueado inicia esse nível
- **BACK**: Botão no canto inferior esquerdo volta ao menu de modos de jogo

### 5. **Progressão de Nível**
- Ao completar um nível, **retorna automaticamente ao level select**
- Não avança automaticamente para o próximo nível
- Permite rejogar níveis já completados para melhorar as estrelas
- GAME OVER também retorna ao level select

## Alterações Técnicas

### Ficheiros Modificados

#### 1. **GameState.hpp**
```cpp
enum class MenuScreen {
    MAIN,
    PLAY_MODES,
    LEVEL_SELECT,  // NOVO
    INSTRUCTIONS,
    OPTIONS
};

// Novos campos de estado
int levelsCompletedStars[20] = {0}; // 0-3 estrelas por nível (agora 20 níveis)
int hoveredLevelButton = -1; // -1 = nenhum, 0-19 = índice do nível
```

#### 2. **MenuRender.cpp**
- Novo renderizador para `MenuScreen::LEVEL_SELECT`
- Grade de **20 botões** de nível (4 linhas × 5 colunas) com:
  - Número do nível (grande e centralizado)
  - 3 estrelas embaixo (preenchidas conforme desempenho)
  - Cor diferente para níveis bloqueados vs desbloqueados
  - Efeito de hover (brilho e sombra aumentados)
- **Panel Position**: fbH × 0.40f (synchronized with InputSystem)
- **Button Size**: 110×110 pixels
- **Button Spacing**: 25 pixels
- **Panel Dimensions**: 800×650 pixels
- Título "SELECT LEVEL" em cyan no topo

#### 3. **InputSystem.cpp**
- **Botão LEVELS**: Agora navega para `LEVEL_SELECT` em vez de iniciar o jogo
- **Level Select Screen**:
  - Detecção de hover para cada botão de nível
  - Detecção de clique com verificação de desbloqueio
  - Botão BACK retorna a `PLAY_MODES`
- **Panel Position**: fbH × 0.40f (synchronized with MenuRender)
- **Hit Detection Formula**: `screenY = fbH - y - btnSize`
- Cálculo de posição da grade idêntico ao rendering (20 botões em grid 4×5)

#### 4. **Game.cpp**
- **Level Complete**: 
  - Calcula estrelas baseado em vidas restantes
  - Atualiza `levelsCompletedStars[levelIdx]` (guarda melhor)
  - Incrementa `levelsBestLevel` para desbloquear próximo
  - Retorna a `LEVEL_SELECT` em vez de avançar automaticamente
- **GAME OVER/WIN**:
  - Botão "Restart" no modo LEVELS retorna ao level select
  - Botão "Menu" no modo LEVELS retorna ao level select

### Estruturas de Dados

#### Estado de Nível
```cpp
currentLevel          // 1-20: nível atual sendo jogado (agora 20 níveis)
levelsBestLevel       // 1-20: nível mais alto desbloqueado
levelsCompletedStars[20] // Array de estrelas (0-3) para cada nível (agora 20)
hoveredLevelButton    // -1 ou 0-19: qual botão tem hover
```

## Fluxo de Utilizador

### Iniciar Nível
1. Menu Principal → PLAY
2. Selecionar modo LEVELS
3. **Level Select Screen** aparece
4. Clicar no nível 1 (único desbloqueado inicialmente)
5. Jogo começa nesse nível

### Completar Nível
1. Destruir todos os bricks
2. Estrelas calculadas (1-3 baseado em vidas)
3. Próximo nível desbloqueado
4. **Retorna automaticamente ao Level Select**
5. Próximo nível agora está iluminado e clicável

### Rejogar Nível
- Pode clicar em qualquer nível já desbloqueado
- Útil para melhorar estrelas (ex: conseguir 3 estrelas em vez de 1)
- Progresso (desbloqueio) não é perdido

### Game Over
- Perder todas as vidas retorna ao level select
- Pode tentar o nível novamente ou escolher outro desbloqueado

## Elementos Visuais

### Botão de Nível Desbloqueado
- **Cor de fundo**: Azul/cyan (0.15, 0.50, 0.65)
- **Hover**: Azul brilhante (0.25, 0.90, 1.0)
- **Borda**: Cyan neon (0.20, 0.75, 0.85)
- **Número**: Branco grande
- **Estrelas**: Amarelo dourado (preenchidas) ou cinza (vazias)

### Botão de Nível Bloqueado
- **Cor de fundo**: Cinza escuro (0.12, 0.12, 0.15)
- **Alpha**: 50% transparência
- **Borda**: Cinza (0.25, 0.25, 0.30)
- **Número**: Cinza desbotado
- **Sem estrelas**

### Layout da Grade
```
LEVEL 1    LEVEL 2    LEVEL 3    LEVEL 4    LEVEL 5
☆☆☆        ☆☆☆        ☆☆☆        ☆☆☆        ☆☆☆

LEVEL 6    LEVEL 7    LEVEL 8    LEVEL 9    LEVEL 10
☆☆☆        ☆☆☆        ☆☆☆        ☆☆☆        ☆☆☆

LEVEL 11   LEVEL 12   LEVEL 13   LEVEL 14   LEVEL 15
☆☆☆        ☆☆☆        ☆☆☆        ☆☆☆        ☆☆☆

LEVEL 16   LEVEL 17   LEVEL 18   LEVEL 19   LEVEL 20
☆☆☆        ☆☆☆        ☆☆☆        ☆☆☆        ☆☆☆
```

## Benefícios

### Para o Jogador
- ✅ **Escolha livre**: Jogar níveis em qualquer ordem (desde que desbloqueados)
- ✅ **Replay value**: Melhorar estrelas em níveis antigos
- ✅ **Menos frustração**: Não perder progresso se falhar um nível
- ✅ **Objetivos claros**: Ver quais níveis têm 3 estrelas e quais não

### Para o Jogo
- ✅ **Progressão não-linear**: Mais flexível que sequencial forçado
- ✅ **Retenção**: Jogadores voltam para completar estrelas
- ✅ **UX moderna**: Padrão esperado em jogos de níveis
- ✅ **Debug facilitado**: Testers podem ir direto a níveis específicos

## Próximas Melhorias (Opcionais)

1. **Persistência**: Salvar estrelas e progresso em disco
2. **Animações**: Transição suave entre level select e gameplay
3. **Preview**: Mostrar thumbnail do layout quando hover sobre nível
4. **Tempo/Score**: Estrelas baseadas em tempo ou score além de vidas
5. **Achievements**: "3 estrelas em todos os níveis", etc.
6. **Dificuldades**: Fácil/Normal/Difícil para cada nível

---

**Status**: ✅ Totalmente implementado e funcional
**Data**: 29 de Dezembro de 2025
**Compatibilidade**: Testado e compilado sem erros

