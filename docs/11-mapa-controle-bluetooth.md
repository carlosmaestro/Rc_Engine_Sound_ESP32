# 11 — Controle Bluetooth (receptor virtual)

Arquivos: `src/input/BluetoothInput.{cpp,h}`, `src/BluetoothMapping.h`.
Ativado por `#define BLUETOOTH_COMMUNICATION` em `src/2_Remote.h`.
Base: framework `pio-framework-bluepad32` (arduino-esp32 2.0.17 + Bluepad32 4.1.0 + BTstack).

## Conceito: "receptor virtual"

O firmware inteiro normaliza qualquer entrada para `pulseWidth[1..13]` em microssegundos
(1000–2000, centro 1500). Em vez de furar essa abstração, o modo Bluetooth **sintetiza
esse array a partir do gamepad** — igual ao que `readSbusCommands()` / `readIbusCommands()`
fazem a partir do rádio.

```mermaid
flowchart LR
    PAD["Gamepad\n(PS4 / PS5 / Xbox / Switch)"] -->|BT Classic HID| BP32["Bluepad32 + BTstack"]
    BP32 --> RBC["readBluetoothCommands()\n(loop, core 1, a cada 15 ms)"]
    RBC --> SYN["sintetiza pulseWidthRaw[1..13]\n+ estado interno (marcha, latches)"]
    SYN --> PRC["processRawChannels()\n(normaliza / expo / reverse)"]
    PRC --> FS["failsafeRcSignals()\n(centraliza se failSafe)"]
    FS --> PW["pulseWidth[1..13]"]
    PW --> DOWN["mapThrottle / rcTriggerRead / gearboxDetection\nesc / led / triggerHorn / mcpwmOutput\n(TUDO INALTERADO)"]
    RBC -.->|sem dados > 500 ms| FSFLAG["failSafe = true"]
```

Nada a jusante de `pulseWidth[]` mudou. `mapThrottle()` voltou ao mapeamento stock
(`pulseWidth[3]` → `currentThrottle`) — o PoC antigo (`BluetoothController.cpp`, que
injetava em `currentThrottle` e acionava a ponte-H por conta própria) foi descartado.

## Como ligar

1. `src/2_Remote.h`: descomente `#define BLUETOOTH_COMMUNICATION` e **comente** o modo
   RC ativo (`IBUS_COMMUNICATION` etc.). É mutuamente exclusivo.
2. `src/hardwareLayout.h`: selecione o layout da sua placa (ex. `LAYOUT_CARLOS_BT_CAR`).
3. `pio run -t upload`, `pio device monitor` (115200).
4. Coloque o controle em pareamento (DualShock/DualSense: PS + Share ~3 s até a lightbar
   piscar duplo). O boot fica bloqueado (setas piscando 2×) até um controle conectar —
   comportamento igual ao "aguardando sinal RC" dos outros modos.

## Mapa gamepad → canal

### Fase 4a (implementada) — dirigir + som

| Controle (PS4) | Canal | Efeito |
|---|---|---|
| Analógico esquerdo, eixo X | CH1 `STEERING` | Servo de direção (GPIO 13). `map(axisX, -512..511, 1020..1980)`, zona morta 24 |
| **R2** (gatilho) | CH3 `THROTTLE` | Acelerador para frente. `1500 + (R2/1020)·480` |
| **L2** (gatilho) | CH3 `THROTTLE` | Freio / ré. Subtrai de CH3: `net = R2 − L2` |
| **R1** / **L1** | CH2 `GEARBOX` | Marcha +/− (estado interno 1..3 → 1000/1500/2000 µs). Borda de subida |
| **Quadrado** (`x()`) | CH4 `HORN` | Buzina (momentâneo → 1980 µs; `> 1900` dispara `hornTrigger`) |
| **X / Cross** (`a()`) | CH10 `MOMENTARY1` | Liga/desliga motor. Segurar ~0,1 s → `momentary1Trigger.toggleLong()` alterna `engineOn` |
| — | CH5–9, CH11–13 | Neutro (1500) — reservados para a Fase 4b |

### Fase 4b (planejada)

- **Dpad ↑/↓** → CH5 (`FUNCTION_R`): estágios de luz, farol alto, jake brake — sintetizar
  nas zonas 1000 / 1150 / 1850 / 2000 µs que o `rcTriggerRead()` espera. Atenção:
  `channelReversed[5] = true` no perfil `FLYSKY_FS_I6S_LOADER` — compensar na síntese
  ou dar um bloco de perfil próprio para `BLUETOOTH_COMMUNICATION` com
  `channelReversed`/`channelAutoZero` todos `false`.
- **Dpad ←/→** → CH6 (`FUNCTION_L`): setas / hazard.
- **Círculo** → CH11 (hazard, latch). **Options / Share** → MODE1 / MODE2.
  **L3** → neutro do câmbio. **Touchpad** → 5ª roda / winch.
- **Feedback**: rumble em troca de marcha e na partida; cor da lightbar por estado do
  motor (desligado = vermelho, ralenti = verde, acelerando = azul) — atualizar só na
  mudança de estado, nunca por frame.
- **Re-pareamento**: `BP32.forgetBluetoothKeys()` só num gesto (ex. PS + Share por 2 s) —
  **nunca no boot** (o PoC antigo fazia, forçando re-pareamento sempre).
- **Debug**: `#define BLUETOOTH_DEBUG` no estilo do `CHANNEL_DEBUG`.

## Constantes de ajuste — `src/BluetoothMapping.h`

| Constante | Padrão | Efeito |
|---|---|---|
| `BT_UPDATE_INTERVAL_MS` | 15 | Período de `BP32.update()` + reamostragem (~66 Hz) |
| `BT_FAILSAFE_TIMEOUT_MS` | 500 | Sem dados do gamepad por mais que isso → `failSafe` (canais ao centro) |
| `BT_PULSE_CENTER` / `BT_PULSE_SPAN` | 1500 / 480 | Faixa dos canais sintetizados (bate com `pulseSpan` do perfil) |
| `BT_TRIGGER_MAX` / `BT_TRIGGER_DEADZONE` | 1020 / 20 | Faixa/zona morta de `throttle()`/`brake()` (0..1023) |
| `BT_AXIS_MIN/MAX` / `BT_AXIS_DEADZONE` | −512..511 / 24 | Faixa/zona morta de `axisX()` |
| `BT_GEAR_US[]` | {—, 1000, 1500, 2000} | µs de cada posição de marcha |

## Failsafe

`readBluetoothCommands()` marca `failSafe = (millis() - últimoDado > BT_FAILSAFE_TIMEOUT_MS)`.
Quando `failSafe`, `failsafeRcSignals()` força `pulseWidth[]` ao centro (exceto CH1/2/8/9),
o `esc()` entra em rampa de frenagem de emergência e o motor vai a zero. Cobre
desconexão, controle desligado e saída de alcance do Bluetooth.

## Latência dos comandos — ajuste (config, sem código)

O preset **Volvo L120H** simula a **inércia de uma carregadeira pesada**. Num carrinho
pequeno isso vira "delay" perceptível na aceleração. Cadeia:

- `esc()` só avança a máquina de estados **a cada `escRampTime`**; para transmissão
  automática, `escRampTime = escRampTimeSecondGear` (**50 ms** em `VolvoL120H.h`).
- Por passo, `escPulseWidth += driveRampRate · driveRampGain` — no máximo `5 · 4 = 20`.
- Do neutro (1500) até potência total (~1980): 24 passos × 50 ms ≈ **1,2 s**.
- Compõe com a ponte-H **L298 Mini** + atrito do motor: abaixo de um PWM mínimo o motor
  nem gira, e `escTakeoffPunch = 0` não dá "arranque".

Knobs para um carro ágil:

| Arquivo | Parâmetro | Padrão | Sugerido p/ carrinho |
|---|---|---|---|
| `vehicles/VolvoL120H.h` | `escRampTimeSecondGear` | 50 | 15–20 |
| `vehicles/VolvoL120H.h` | `escAccelerationSteps` | 5 | 10–15 |
| `vehicles/VolvoL120H.h` | `escBrakeSteps` | 100 | 30–40 |
| `vehicles/VolvoL120H.h` | `acc` / `dec` (som do motor) | 6 / 3 | 12 / 6 |
| `3_ESC.h` | `escTakeoffPunch` (arranque p/ vencer o atrito) | 0 | 50–80 |
| `3_ESC.h` | `globalAccelerationPercentage` (divide `escRampTime`) | 100 | 150 |
| `3_ESC.h` | `crawlerEscRampTime` (modo crawler = controle quase direto) | 10 | — |

Atalho: `masterVolume ≤ 44` (passo de volume mais baixo) liga o **modo crawler**
(`escRampTime = crawlerEscRampTime`, quase sem inércia virtual).

A **buzina** em si não tem atraso na lógica (`triggerHorn()` dispara no mesmo ciclo);
o que se ouve é a natural entrada do arquivo de som `CarHorn.h`.
