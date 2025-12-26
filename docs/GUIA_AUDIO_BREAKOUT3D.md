# Guia de Áudio — Breakout3D (SFX + Música)
_Data: 26-12-2025_

Este documento descreve, de forma detalhada, **todo o áudio do projeto**:  
- **SFX** (efeitos sonoros) — interacções, impactos, power-ups, modo endless, UI  
- **Música** (soundtracks/loops) — menu, gameplay, endless, pausa  
- **Stingers musicais** — mini-jingles para eventos (start, clear, game over, etc.)

> Nota: este guia está escrito em **PT‑PT** (Portugal).  
> A música e os SFX foram preparados para uma vibe **retro/arcade**, curta e “seca”, para encaixar no estilo Breakout.

---

## 1) Objetivos de design (o “porquê” do pack)

### 1.1 Consistência retro
Para o jogo soar “a sério” e não como um conjunto aleatório de sons:
- Sons curtos, com **ataque rápido** (resposta imediata).
- **Sem reverbs longos** (evita “lama” sonora).
- Paleta sonora coerente (beeps/zaps/impactos “arcade”).

### 1.2 Evitar repetição irritante
Os sons que repetem muito (hits) precisam de:
- **variações** (`_01`, `_02`, `_03`) e/ou
- **pitch ligeiro** aleatório (±1–3%) no runtime.

### 1.3 Prioridades de mix
- A **bola e impactos** devem ser claros (feedback do jogo).
- A **UI** deve ser audível mas **mais baixa**.
- Música deve “colar” a vibe, mas nunca tapar a jogabilidade.

---

## 2) Estrutura de pastas (onde meter tudo)

### 2.1 SFX
Colocar dentro de:

```
assets/audio/sfx/
  ui/
  game/
  bricks/
  powerups/
  endless/
  stingers/
```

### 2.2 Música (loops) + stingers musicais
Colocar dentro de:

```
assets/audio/
  music/
  stingers_music/
```

---

## 3) Convenções (nomes, formatos, volumes)

### 3.1 Formato recomendado
- **SFX**: `WAV` (rápido, sem compressão perceptível, óptimo para baixa latência)
- **Música**: idealmente `OGG` (mais leve).  
  *Se estiver em WAV, funciona na mesma; só ocupa mais espaço.*

### 3.2 Nomes de ficheiro
- `categoria_acao_variacao.ext`
- Ex.: `hit_brick_02.wav`, `ui_select_01.wav`

### 3.3 Alvos de volume (boa referência)
Estes valores são uma boa “base” para consistência; ajusta no jogo com sliders.

- **UI**: mais baixo
- **Hits/Powerups/Stingers**: médio/alto
- **Loops (warning/panic)**: baixo (para não cansar)
- **Música**: baixo-médio (depende do teu mix final)

Sugestão prática (relativa):
- Música: **-12 dB a -18 dB** (relativo ao SFX principal)
- UI: **~3 dB abaixo** dos hits
- Warning loops: **bem abaixo** do resto

---

## 4) Lista completa de áudio (o que existe e para quê)

### 4.1 UI (menu/HUD)
Pasta: `assets/audio/sfx/ui/`

| Ficheiro | Uso | Quando tocar |
|---|---|---|
| `ui_move_01.wav` | Navegar entre opções | ao mudar de botão/selector |
| `ui_move_02.wav` | Navegar (variação) | alternar com o _01 |
| `ui_select_01.wav` | Confirmar/seleccionar | “Enter/Click/Confirmar” |
| `ui_back_01.wav` | Voltar/cancelar | “Escape/Back” |
| `ui_error_01.wav` | Acção inválida | tentativa de usar opção indisponível |
| `ui_toggle_01.wav` | Toggle | ligar/desligar opção |
| `ui_tick_01.wav` | Tick/slider | mexer num slider / contador |
| `ui_pause_in.wav` | Entrar em pausa | abrir menu pausa |
| `ui_pause_out.wav` | Sair de pausa | retomar jogo |

**Boas práticas**
- UI normalmente vai numa “bus” separada (`UI`) para controlar o volume independentemente do resto.
- Evitar UI demasiado alto: “cansa” o jogador, sobretudo em menus.

---

### 4.2 Gameplay (core)
Pasta: `assets/audio/sfx/game/`

| Ficheiro | Uso | Quando tocar |
|---|---|---|
| `ball_launch_01.wav` | Lançar bola | Space/início de round |
| `hit_paddle_01.wav` | Bola bate no paddle | colisão bola ↔ paddle |
| `hit_paddle_02.wav` | variação | aleatório |
| `hit_paddle_03.wav` | variação | aleatório |
| `hit_wall_01.wav` | Bola bate na borda/parede | colisão bola ↔ parede |
| `hit_wall_02.wav` | variação | aleatório |
| `hit_wall_03.wav` | variação | aleatório |
| `life_lost_01.wav` | Perder vida / bola caiu | bola sai do campo / falha |
| `life_gain_01.wav` | Ganhar vida | extra life |

**Boas práticas**
- Colocar um **cooldown** curto (ex.: 20–60 ms) nos hits, para evitar tocar 10 hits no mesmo frame por bugs/edge-cases.
- Em multiball, limita **polifonia** (ex.: máximo 6–10 hits simultâneos).

---

### 4.3 Bricks (dano/crack/break)
Pasta: `assets/audio/sfx/bricks/`

| Ficheiro | Uso | Quando tocar |
|---|---|---|
| `hit_brick_01.wav` | Hit no brick (sem partir) | bola bate num brick com HP>1 |
| `hit_brick_02.wav` | variação | aleatório |
| `hit_brick_03.wav` | variação | aleatório |
| `brick_crack_01.wav` | Crack/damage tick | HP desce, mas brick continua |
| `brick_crack_02.wav` | crack alternativo | usar como variação/2ª opção |
| `brick_break_01.wav` | Brick parte | HP chega a 0 / brick destruído |
| `brick_break_02.wav` | variação | aleatório |
| `brick_break_03.wav` | variação | aleatório |

**Sugestão de trigger (HP 1–4)**
- Se HP **ainda não chegou a 0**: toca `brick_crack_*` ou `hit_brick_*` (dependendo do “peso” desejado).
- Se HP **chega a 0**: toca sempre `brick_break_*`.

---

### 4.4 Power-ups
Pasta: `assets/audio/sfx/powerups/`

| Ficheiro | Uso | Quando tocar |
|---|---|---|
| `powerup_drop_01.wav` | Power-up a cair (one‑shot) | quando o power-up aparece |
| `powerup_drop_loop.wav` | Power-up a cair (loop) | opcional: enquanto está a cair |
| `powerup_pickup_01.wav` | Pickup | apanha power-up |
| `powerup_pickup_02.wav` | variação | aleatório |
| `powerup_pickup_03.wav` | variação | aleatório |
| `powerup_expand_on.wav` | Expand ON | activar expand |
| `powerup_expand_off.wav` | Expand OFF | expirar/desactivar |
| `powerup_slow_on.wav` | Slow ON | activar slow |
| `powerup_slow_off.wav` | Slow OFF | expirar/desactivar |
| `powerup_multiball_spawn.wav` | Multiball spawn | criar bolas adicionais |

**Boas práticas**
- `powerup_drop_loop.wav`: usa volume baixo, e pára assim que o power-up é apanhado ou sai do ecrã.
- Power-ups “ON/OFF” fazem o jogo parecer muito mais polido.

---

### 4.5 Endless mode
Pasta: `assets/audio/sfx/endless/`

| Ficheiro | Uso | Quando tocar |
|---|---|---|
| `endless_column_spawn_01.wav` | Nova coluna/linha | spawn de obstáculos/bricks |
| `endless_difficulty_up_01.wav` | Subida de dificuldade | milestone / tempo / score |
| `endless_warning_loop.wav` | Warning/panic (loop) | quando a situação fica crítica |
| `endless_warning_end.wav` | Sair do warning | quando a situação alivia |
| `endless_fail_01.wav` | Falha final (endless) | fim do endless (diferente do life_lost) |

**Boa regra para o warning**
- Entra em warning quando a “altura”/distância dos bricks passa um limiar.
- Sai do warning assim que volta a estar seguro.
- O loop deve ter **fade in/out** para não dar clicks.

---

### 4.6 Stingers (SFX curtos, não‑música)
Pasta: `assets/audio/sfx/stingers/`

| Ficheiro | Uso | Quando tocar |
|---|---|---|
| `stinger_level_clear.wav` | Passar nível | transição para próximo nível |
| `stinger_game_over.wav` | Game over | derrota |

> Se preferires, podes trocar estes por stingers musicais equivalentes (ver secção 4.8).

---

## 5) Música (soundtrack) — loops por estado

Pasta: `assets/audio/music/`

| Ficheiro | Estado | Notas |
|---|---|---|
| `music_menu_loop.wav` | MENU | chill, 16 barras, loop |
| `music_gameplay_loop.wav` | GAMEPLAY | energia arcade, loop |
| `music_endless_loop.wav` | ENDLESS | mais tenso/rápido, loop |
| `music_pause_loop.wav` | PAUSA | ambiente leve, loop (opcional) |

**Boas práticas**
- Sempre que mudares de estado, faz **crossfade** (ex.: 0.5s) em vez de cortar a música a seco.
- No endless, se o jogo tiver “panic”, podes manter a mesma música e usar o warning loop por cima, ou criar uma camada extra (futuro).

---

## 6) Stingers musicais (mini‑jingles)

Pasta: `assets/audio/stingers_music/`

| Ficheiro | Evento | Como usar |
|---|---|---|
| `stinger_level_start.wav` | Início do nível | por cima da música / antes do gameplay |
| `stinger_level_clear.wav` | Level clear | por cima da música + duck ligeiro |
| `stinger_game_over.wav` | Game over | por cima + reduzir música |
| `stinger_milestone.wav` | Milestone | endless / difficulty up |
| `stinger_extra_life.wav` | Vida extra | por cima + curto |

---

## 7) Regras de reprodução (quando toca o quê)

### 7.1 Máquina de estados (exemplo)
- `MENU`
- `PLAYING`
- `PAUSED`
- `ENDLESS`
- `LEVEL_CLEAR`
- `GAME_OVER`

### 7.2 Mapeamento recomendado
- `MENU`: `music_menu_loop.wav`
- `PLAYING`: `music_gameplay_loop.wav`
- `PAUSED`: `music_pause_loop.wav` (opcional)
- `ENDLESS`: `music_endless_loop.wav`
- `LEVEL_CLEAR`: tocar stinger + (opcional) manter música ou trocar para menu/next
- `GAME_OVER`: tocar stinger + (opcional) música menu ou silêncio

---

## 8) “Ducking” e prioridades (para ficar polido)

Quando toca um stinger importante:
- baixa a música por ~0.3–0.6s, e volta suavemente.

Exemplos:
- `stinger_level_clear.wav` ou `stinger_game_over.wav`
- `stinger_extra_life.wav`

**Implementação simples**
- `musicGain = lerp(musicGain, targetGain, dt * speed)`
- `targetGain` baixa temporariamente e volta.

---

## 9) Anti‑spam (limites e cooldowns)

### 9.1 Cooldowns por tipo
- Hits (`hit_*`): 20–60 ms
- UI: 50–120 ms (depende do input repeat)
- Warning loop: só liga/desliga quando muda de estado (não re‑iniciar a cada frame)

### 9.2 Polifonia
- Limitar sons simultâneos (especialmente multiball).
- Em vez de tocar 10 hits, toca 3–6 (os mais “recentes” ou os mais fortes).

---

## 10) Checklist de QA (antes de integrares)

Ouvir e confirmar:
- Hits (`hit_*`) — não estão demasiado altos, nem com silêncio no início.
- `brick_break_*` — não é “explosão” pesada demais.
- `endless_warning_loop.wav` — faz loop sem clicks.
- `powerup_drop_loop.wav` — não irrita ao fim de 20–30s.
- Música — transições com fade, sem cortes secos.

---

## 11) Sugestões rápidas de implementação (sem amarrar a uma biblioteca)

Mesmo sem escolher uma biblioteca específica, a arquitectura típica é:

- **AudioManager**
  - `playSfx(name, volume=1, pitch=1)`
  - `playMusic(name, loop=true)`
  - `stopMusic(fadeOut=0.5)`
  - `setBusVolume(bus, value)`  
- **Buses**
  - `MASTER`, `MUSIC`, `SFX`, `UI`

**Dica:** guarda settings (volumes) num ficheiro de config do jogo (ex.: JSON/TOML).

---

## 12) Resumo (o mínimo para o jogo soar completo)
- 3–4 loops de música (menu/gameplay/endless/pausa)
- hits com variações (`_01/_02/_03`)
- power-ups com pickup + on/off
- endless com warning loop + milestone
- stingers para eventos grandes

Se tiveres isto tudo bem integrado, o áudio do jogo fica **completo e consistente**.
