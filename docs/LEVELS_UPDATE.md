# LEVELS/LEVEL SELECT Updates

## O que foi feito
- Adicionado e organizado modo LEVELS com 10 layouts únicos em `include/game/LevelLayouts.hpp`.
- Criada a tela LEVEL_SELECT com painel dedicado (720x420) e grelha 2x5.
- Removidas estrelas/ratings dos botões de nível.
- Separadas dimensões de painel entre MAIN (500x480) e LEVEL_SELECT (720x420).
- Condição de render ajustada para não desenhar painel default no LEVEL_SELECT (evitou painel duplo).
- Ordem visual dos níveis corrigida para mostrar 1-5 na linha de cima e 6-10 na linha de baixo.
- Hover e clique da grelha alinhados com a nova ordem (Y invertido em InputSystem/MenuRender).
- Vitória: mostra animação de win finisher antes do overlay WIN (não avança auto).

## Ficheiros tocados (principais)
- `src/game/render/MenuRender.cpp`: painel custom, grelha 2x5, ordem visual 1-5 / 6-10, sem estrelas, skip painel default no LEVEL_SELECT.
- `src/game/systems/InputSystem.cpp`: hitboxes de hover/click do LEVEL_SELECT ajustadas para a ordem visual; usa mesmas dimensões do painel custom.
- `include/game/GameState.hpp`: enum `LEVEL_SELECT`, hover de botões de nível, progresso.
- `include/game/LevelLayouts.hpp`: 10 layouts de tijolos.
- `src/game/Game.cpp`: ao concluir nível, toca win finisher e só depois mostra overlay WIN.
- `src/game/ui/OverlayLayout.cpp`: painel MAIN restaurado para 500x480.

## Verificações
- Build: `make -B -j4` passa.
- Teste manual recomendado: abrir LEVEL_SELECT, confirmar hover/click correspondem ao número; jogar um nível e verificar animação de vitória antes do overlay.
