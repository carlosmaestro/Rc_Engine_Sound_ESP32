# 03 — Hardware e pinos

Mapa de GPIO conforme os `#define` em `src/main.cpp` (bloco "PIN ASSIGNMENTS",
linhas ~140–215). Vale para a placa padrão (30 pinos). O modo
`#define WEMOS_D1_MINI_ESP32` (em `0_generalSettings.h`, **desativado** aqui) muda
alguns pinos de luz.

## Entradas

| GPIO | Símbolo | Uso |
|------|---------|-----|
| 36 (VP) | `COMMAND_RX` | Entrada serial do receptor (SBUS/IBUS/SUMD/PPM) — `INPUT_PULLDOWN` |
| 39 (VN) | `BATTERY_DETECT_PIN` | Divisor de tensão para medir a bateria (`ESP32AnalogRead`) |
| 32 | `COUPLER_SWITCH_PIN` | Chave de presença de reboque (`INPUT_PULLUP`, interrupção `CHANGE`) — **somente** se `THIRD_BRAKELIGHT` e `RZ7886_DRIVER_MODE` não estiverem definidos |
| 34/35/13/12/14/27 | `PWM_PINS[]` | 6 canais de entrada PWM clássica (via periférico RMT) — usados só no modo PWM |

> No modo PWM, os canais entram por 6 GPIOs lidos pelo periférico **RMT** (mede a
> largptura de pulso por ISR, `rmt_isr_handler`). Nos modos BUS (SBUS/IBUS/SUMD) a
> mesma pinagem CH1..CH6 vira **saída** de servo — nunca ligue um receptor PWM
> nesses pinos em modo BUS.

## Saída de áudio

| GPIO | Símbolo | Uso |
|------|---------|-----|
| 25 | `DAC1` | DAC 8 bit, canal A → resistor 10k → entrada do PAM8403 |
| 26 | `DAC2` | DAC 8 bit, canal B → resistor 10k → entrada do PAM8403 |

`DAC1` (GPIO25) carrega o **som do motor** (taxa de amostragem variável) e `DAC2`
(GPIO26) carrega os **sons de taxa fixa** (buzina, sirene, seta, ré, engate...). Um
potenciômetro de 20k soma/atenua os dois canais — controle de volume analógico para
dois alto-falantes. A escrita é feita direto no registrador do DAC dentro das ISRs de
playback (`SET_PERI_REG_BITS`, mais rápido que `dacWrite`). Ver
[05 — Motor de som](05-motor-de-som.md).

## Saídas de servo (modo BUS) — via MCPWM

| GPIO | Símbolo | Função |
|------|---------|--------|
| 13 | `STEERING_PIN` | CH1 — servo de direção |
| 12 | `SHIFTING_PIN` | CH2 — servo de câmbio |
| 14 | `WINCH_PIN` | CH3 — guincho / giroflex (`CH3_BEACON`) |
| 27 | `COUPLER_PIN` | CH4 — engate de 5ª roda / (ou saída Neopixel se `NEOPIXEL_ON_CH4`) |
| 33 | `ESC_OUT_PIN` | Sinal do ESC do crawler (MCPWM dedicado, `setupMcpwmESC()`) |

`RZ7886_PIN1 = 33`, `RZ7886_PIN2 = 32` quando `RZ7886_DRIVER_MODE` (driver de motor
no lugar de ESC — não usado neste checkout).

## Luzes (LEDC / PWM 20 kHz) — placa padrão

| GPIO | Símbolo | Luz | Canal/timer LEDC |
|------|---------|-----|------------------|
| 3 | `HEADLIGHT_PIN` | Faróis | 15 |
| 22 | `CABLIGHT_PIN` | Luzes de cabine | 12 |
| 15 | `TAILLIGHT_PIN` | Lanterna + freio (combinadas) | 2 |
| 2 | `INDICATOR_LEFT_PIN` | Seta esquerda | 3 |
| 4 | `INDICATOR_RIGHT_PIN` | Seta direita | 4 |
| 16 (RX2) | `FOGLIGHT_PIN` | Farol de neblina | 5 |
| 17 (TX2) | `REVERSING_LIGHT_PIN` | Luz de ré | 6 |
| 5 | `ROOFLIGHT_PIN` | Luzes de teto / farol alto (se `SEPARATE_FULL_BEAM`) | 7 |
| 18 | `SIDELIGHT_PIN` | Luzes laterais | 8 |
| 21 | `BEACON_LIGHT1_PIN` | Giroflex azul 1 | 9 |
| 19 | `BEACON_LIGHT2_PIN` | Giroflex azul 2 | 10 |
| 32 | `BRAKELIGHT_PIN` | 3ª luz de freio (se `THIRD_BRAKELIGHT`) | 11 |
| 23 | `SHAKER_MOTOR_PIN` | Motor shaker (vibração) | 13 |
| 0 (BOOT) | `RGB_LEDS_PIN` | Dados da fita Neopixel WS2812 (ou CH4 se `NEOPIXEL_ON_CH4`) | RMT (FastLED) |

Os canais LEDC 0 e 1 são reservados para os timers de interrupção de áudio; por isso
as luzes começam no canal 2. Cada luz é um objeto `statusLED` (biblioteca do
TheDIYGuy999) que encapsula fade, flash e brilho.

### Conflitos de pino com `SPI_DASHBOARD`

Se o dashboard SPI estiver ligado, os pinos **18 (SCL), 19 (DC), 21 (RES), 23 (SDA)**
vão para o LCD. Nesse caso **luzes laterais, os dois giroflex e o shaker não funcionam**
(o `begin()` deles é pulado por `#if not defined SPI_DASHBOARD`). Parâmetros do LCD
(driver ST7735, 80×160, pinos) são passados por `build_flags` no `platformio.ini`.

### Conflitos introduzidos pela camada Bluetooth

`BluetoothController::processThrottle()` faz `ledcWrite(10, ...)`, `ledcWrite(11, ...)`
e `digitalWrite(22, LOW/HIGH)` para acionar uma ponte-H de tração e uma luz de ré
próprias. Na placa padrão:

- canal LEDC **10** = giroflex 2 (`beaconLight2`, GPIO19)
- canal LEDC **11** = 3ª luz de freio (`brakeLight`, GPIO32, ativa aqui)
- GPIO **22** = `CABLIGHT_PIN` (luzes de cabine)

Ou seja, essa parte do `BluetoothController` assume uma fiação/pinagem alternativa e
**colide** com as luzes na configuração padrão. Trate-a como **experimental** e
específica do hardware de quem escreveu o fork. Ver
[04 — Entrada de controle](04-entrada-de-controle.md#camada-bluetooth-bluetoothcontroller).

## Alimentação e proteção de bateria

- `BATTERY_PROTECTION` ativo (`3_ESC.h`). Divisor: `RESISTOR_TO_BATTTERY_PLUS = 9400 Ω`,
  `RESISTOR_TO_GND = 1000 Ω`, `DIODE_DROP = 0,31 V`. Ajuste esses três valores até a
  leitura bater com a tensão real.
- `CUTOFF_VOLTAGE = 3,3 V/célula`, `FULLY_CHARGED_VOLTAGE = 4,2 V`,
  `RECOVERY_HYSTERESIS = 0,2 V`.
- Detecção automática do número de células na partida (bipes = nº de células;
  10 bipes rápidos = erro de bateria).
- Ao atingir o corte: `batteryProtection = true` → `esc()` limita a saída e dispara a
  mensagem sonora "out of fuel" (`vehicles/sounds/OutOfFuelEnglish.h`).

## PCBs

`hardware/` contém os arquivos das placas (SMD 30 pinos, through-hole 30 pinos, base STL)
e o PDF "How To Order Your PCB".
