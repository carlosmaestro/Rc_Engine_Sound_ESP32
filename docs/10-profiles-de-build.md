# 10 — Build profiles (um arquivo por carro)

Diretório `src/profiles/` + `src/tuning/`. Selecionado por `[env:*]` no `platformio.ini`.
Evolução do antigo `hardwareLayout.h` (commit `1e61421` → Fase 6).

## Ideia

Uma build depende de várias escolhas: qual **veículo/sons**, qual **modo de comunicação**
(Bluetooth / IBUS / SBUS / ...), qual **perfil de rádio**, **toggles de placa** (Neopixel?
proteção de bateria? 3ª luz de freio? driver de motor?), **pinagem**, e o **tuning de
sensação** (rampas do ESC, `acc`/`dec`, arranque). Antes isso estava espalhado em
`1_Vehicle.h` + `2_Remote.h` + `hardwareLayout.h` + `vehicles/*.h` + `3_ESC.h`, editado à
mão em cada arquivo.

Um **build profile** é **um arquivo** (`src/profiles/<Nome>.h`) que empacota tudo isso
para um carro físico. Trocar de carro = trocar de `env`:

```bash
pio run -e l120h_radio        # Volvo L120H + rádio IBUS + placa 30 pinos  (default)
pio run -e carlos_bt_car      # carro Bluetooth (PS4/PS5) do usuário
pio run -e wemos_d1_mini      # variante Wemos D1 Mini
pio run -e carlos_bt_car -t upload
```

## Como funciona

```mermaid
flowchart TB
    ENV["platformio.ini: [env:carlos_bt_car]\nbuild_flags = -D PROFILE_CARLOS_BT_CAR"]
    ENV --> ACT["src/profiles/active.h  (dispatcher)\n#if PROFILE_X -> #include o profile / #else #error"]
    ACT --> PROF["src/profiles/CarlosBtCar.h\n#define PROFILE_VEHICLE / comm / radio / PROFILE_* toggles / pinos\n#include ../tuning/agileCar.h"]
    PROF -.incluido 1o em main.cpp.-> HDRS["headers 0..10"]
    HDRS --> V["1_Vehicle.h: #ifdef PROFILE_VEHICLE -> #include PROFILE_VEHICLE"]
    HDRS --> R["2_Remote.h: fallback de comm/radio so #if nada definido"]
    HDRS --> T["3_ESC.h / 6_Lights.h: #if PROFILE_NEOPIXEL / PROFILE_BATTERY_PROTECTION / PROFILE_THIRD_BRAKELIGHT"]
    HDRS --> TU["vehicles/VolvoL120H.h + 3_ESC.h: const/var = TUNE_* (#ifndef default)"]
    HDRS --> PINS["main.cpp bloco PIN ASSIGNMENTS: cada #define sob #ifndef -> profile vence"]
```

- **`profiles/active.h` é o 1º `#include` de `main.cpp`** (antes de `0_GeneralSettings.h`).
  Precisa vir primeiro porque influencia `1_Vehicle.h` e os toggles guardados.
- Sem nenhum `PROFILE_*` → `active.h` dá `#error` ("compile com `pio run -e <profile>`").
- Cada default de config virou **guardado**:
  - Pinos: `#ifndef X_PIN / #define X_PIN <default> / #endif` (bloco PIN ASSIGNMENTS de `main.cpp`).
  - Toggles: `#ifndef PROFILE_NEOPIXEL / #define PROFILE_NEOPIXEL 1 / #endif` + `#if PROFILE_NEOPIXEL / #define NEOPIXEL_ENABLED / #endif` (idem `PROFILE_BATTERY_PROTECTION`, `PROFILE_THIRD_BRAKELIGHT`).
  - Comm mode / perfil de rádio (`2_Remote.h`): só define o default (`IBUS_COMMUNICATION`,
    `FLYSKY_FS_I6S_LOADER`) `#if` nenhum foi escolhido pelo profile.
  - `EXPONENTIAL_THROTTLE`: `PROFILE_EXPO_THROTTLE` (default 1).
  - Tuning: `#ifndef TUNE_X / #define TUNE_X <default> / #endif` e a linha vira
    `const uint8_t escRampTimeSecondGear = TUNE_ESC_RAMP_2ND;` (efeito imediato) ou
    `uint16_t escTakeoffPunch = TUNE_ESC_TAKEOFF_PUNCH;` (EEPROM-backed — ver abaixo).
- **`DAC1`/`DAC2` = 25/26** — imutáveis. `#error` em `main.cpp` trava qualquer profile que mude.

## A "API do profile" — o que um profile pode definir

| Categoria | Símbolos |
|---|---|
| Veículo / sons | `PROFILE_VEHICLE "vehicles/<X>.h"` (o único obrigatório) |
| Modo de comunicação | `BLUETOOTH_COMMUNICATION` \| `IBUS_COMMUNICATION` \| `SBUS_COMMUNICATION` \| `SUMD_COMMUNICATION` \| `PPM_COMMUNICATION` \| (nenhum = PWM nos headers CH1–CH6) |
| Perfil de rádio | `FLYSKY_FS_I6S_LOADER` \| `FLYSKY_FS_I6X` \| `FLYSKY_GT5` \| `RGT_EX86100` \| `GRAUPNER_MZ_12` \| `MICRO_RC` \| ... (fornece `channelReversed[]`/`channelAutoZero[]`/`pulseSpan`) |
| Toggles de placa | `PROFILE_NEOPIXEL` (0/1) · `PROFILE_BATTERY_PROTECTION` (0/1) · `PROFILE_THIRD_BRAKELIGHT` (0/1) · `PROFILE_EXPO_THROTTLE` (0/1) · `PROFILE_STEERING_REVERSED` (0/1 — inverte CH1/direção, para chassi com servo/tirante espelhado) · `RZ7886_DRIVER_MODE` (opt-in) · `SPI_DASHBOARD` (opt-in) · `WEMOS_D1_MINI_ESP32` (opt-in) |
| Pinos | qualquer `*_PIN` do bloco PIN ASSIGNMENTS: `STEERING_PIN`, `RZ7886_PIN1/2`, `HEADLIGHT_PIN`, `TAILLIGHT_PIN`, `INDICATOR_LEFT/RIGHT_PIN`, `FOGLIGHT_PIN`, `REVERSING_LIGHT_PIN`, `ROOFLIGHT_PIN`, `SIDELIGHT_PIN`, `BEACON_LIGHT1/2_PIN`, `CABLIGHT_PIN`, `SHAKER_MOTOR_PIN`, `COMMAND_RX`, `BATTERY_DETECT_PIN`, `PWM_PINS_INIT`/`PWM_CHANNELS_INIT` (`-1` = ausente / no-op) |
| Tuning (via `#define TUNE_*` ou `#include ../tuning/<set>.h`) | `TUNE_ESC_RAMP_1ST/2ND/3RD`, `TUNE_ESC_BRAKE_STEPS`, `TUNE_ESC_ACCEL_STEPS`, `TUNE_ENGINE_ACC`, `TUNE_ENGINE_DEC` (const — efeito imediato) · `TUNE_ESC_TAKEOFF_PUNCH`, `TUNE_ESC_PULSE_SPAN`, `TUNE_ESC_CRAWLER_RAMP`, `TUNE_GLOBAL_ACCEL_PCT` (EEPROM-backed) |

## Profiles existentes

| `[env]` / `PROFILE_*` | Arquivo | Resumo |
|---|---|---|
| `l120h_radio` | `src/profiles/L120hRadio.h` | L120H + IBUS + `FLYSKY_FS_I6S_LOADER` + placa 30 pinos + tuning original. **Build idêntico ao histórico.** Default (`platformio.ini` → `default_envs`). |
| `carlos_bt_car` | `src/profiles/CarlosBtCar.h` | placa `_carlosBoard.h` (Bluetooth, ponte‑H `RZ7886` 33/32, pinos do `Controller.ino`, sem Neopixel/bateria/3ª luz) + veículo **L120H** (carregadeira) + `#include ../tuning/agileCar.h`. |
| `gol_quadrado` | `src/profiles/GolQuadrado.h` | mesma placa `_carlosBoard.h` + veículo **`vehicles/GolQuadrado.h`** (carro leve 4 cil. gasolina, câmbio manual R1/L1) + dinâmica "de carro" (tuning ágil opcional, comentado). |
| `carro_corrida` | `src/profiles/CarroCorrida.h` | mesma placa `_carlosBoard.h` + veículo **`vehicles/CarroCorrida.h`** (V12, som da biblioteca "LaFerrari", câmbio manual R1/L1 com som de troca de marcha ajustado) + dinâmica "de corrida" (tuning ágil opcional, comentado). |
| `wemos_d1_mini` | `src/profiles/WemosD1Mini.h` | L120H + IBUS + `WEMOS_D1_MINI_ESP32`. |

`src/profiles/_carlosBoard.h` = pinos + toggles da placa física do usuário, compartilhado
por `carlos_bt_car` e `gol_quadrado` (o profile só faz `#include "_carlosBoard.h"`).

### `vehicles/GolQuadrado.h`

Preset de som para um **VW Gol "quadrado"** (G1/G2). Usa o banco de sons do VW ar / Fusca
(`VWBeetleStart/Idle/Rev2/Knock.h`) — que é literalmente o motor do Gol BX inicial. Dinâmica
de carro leve: `automatic=false` + `VIRTUAL_3_SPEED` (troca por R1/L1), `MAX_RPM_PERCENTAGE=320`,
`acc/dec=5/3`, `escAccelerationSteps=4`, sem turbo/jake/beep de ré. Os params de rampa/`acc`
são `#ifndef TUNE_*` — um profile pode aplicar `agileCar.h` por cima. Para som real do seu Gol
(motor AP), converta o áudio com `tools/Audio2Header.html` e troque os `#include "sounds/..."`.

### `vehicles/CarroCorrida.h`

Preset de som de **carro de corrida (V12)**, usando o banco de sons `LaFerrari*.h` já
presente na biblioteca (`sounds/LaFerrariStart/Idle/Rev/Knock.h`). O preset original da
biblioteca (`vehicles/LaFerrari.h`) usa `doubleClutch = true` — nesse modo o firmware
**nunca** toca a amostra de troca de marcha (`main.cpp`: `shiftingTrigger && !automatic &&
!doubleClutch`), porque a troca é simulada por um "blip" de RPM em vez de um som de
engate. Este preset troca a caixa para manual "virtual" (`automatic=false` +
`doubleClutch=false`, `VIRTUAL_3_SPEED`, troca por R1/L1 — igual ao `GolQuadrado`), o que
ativa de fato `sounds/ClunkingGearShifting.h` (`shiftingVolumePercentage=140`, um pouco
acima do default 100 para ficar nítido). Também ajusta a dinâmica pra ficar mais "punchy"
que o preset original (`escRampTime*` menor, `escBrakeSteps`/`escAccelerationSteps`
maiores, `acc/dec=7/4`, `clutchEngagingPoint=70`) e mantém `TIRE_SQUEAL` ligado.
`MAX_RPM_PERCENTAGE=320` (mesmo teto de `maxIbusRpmPercentage` já aplicado nos modos
BUS/Bluetooth). O chassi deste carro tem a direção espelhada em relação aos outros carros
do usuário, então o profile também define `PROFILE_STEERING_REVERSED 1` (ver abaixo) —
sem isso o carro vira para o lado errado.

### `carlos_bt_car` — pinagem (de `referencia/Controller.ino`)

| Símbolo | GPIO | Origem |
|---|---|---|
| `DAC1` / `DAC2` (áudio → PAM8403) | 25 / 26 | *(novo — buzzer do `.ino` sai)* |
| `STEERING_PIN` (servo, MCPWM 0) | 13 | `SERVO_DIRECAO` |
| `RZ7886_PIN1` (ponte-H IN1, MCPWM 1) | 33 | `PONTE_H_IN01` era 27 → **movido** |
| `RZ7886_PIN2` (ponte-H IN2, MCPWM 1) | 32 | `PONTE_H_IN02` |
| `HEADLIGHT_PIN` | 15 | `FAROL` *(strapping)* |
| `FOGLIGHT_PIN` | 2 | `FAROL_MILHA` *(strapping)* |
| `TAILLIGHT_PIN` (lanterna+freio) | 4 | `LUZ_FREIO` |
| `INDICATOR_RIGHT_PIN` / `INDICATOR_LEFT_PIN` | 16 / 17 | `SETA_DIREITA` / `SETA_ESQUERDA` |
| `REVERSING_LIGHT_PIN` | 22 | `LUZ_RE` |
| `ROOFLIGHT_PIN` / `SIDELIGHT_PIN` | 19 / 21 | `LUZ_AUX01` / `LUZ_AUX02` (não usados no `.ino`) |
| `CABLIGHT_PIN`, `BEACON_LIGHT1/2_PIN`, `SHAKER_MOTOR_PIN` | -1 | ausentes |
| `BATTERY_DETECT_PIN` | 39 | *(sem divisor — `PROFILE_BATTERY_PROTECTION 0`)* |

> **Não conectar nada em GPIO 12, 14, 27** neste profile: o `setupMcpwm()` gera sinal de
> servo ocioso neles (CH2/3/4 do modo BUS, não usados). GPIO 12 é strapping pin.
> Uma versão futura restringe o `setupMcpwm()` só à direção em modo Bluetooth.

## Tuning: `src/tuning/agileCar.h`

O preset Volvo L120H simula inércia de **carregadeira pesada** → resposta lenta num
carrinho pequeno (`esc()` avança a máquina a cada `escRampTimeSecondGear` = 50 ms;
~1 s do neutro à potência total). O set `agileCar.h` sobrescreve os knobs de sensação:

| `TUNE_*` | default | ágil | onde aplica |
|---|---|---|---|
| `TUNE_ESC_RAMP_2ND` | 50 | **18** | `escRampTimeSecondGear` (const, imediato) |
| `TUNE_ESC_ACCEL_STEPS` | 5 | **12** | `escAccelerationSteps` (const) |
| `TUNE_ESC_BRAKE_STEPS` | 100 | **35** | `escBrakeSteps` (const) |
| `TUNE_ENGINE_ACC` / `TUNE_ENGINE_DEC` | 6 / 3 | **12 / 6** | `acc` / `dec` — spool-up do som (const) |
| `TUNE_ESC_TAKEOFF_PUNCH` | 0 | **60** | `escTakeoffPunch` — chute inicial p/ vencer atrito **(EEPROM)** |
| `TUNE_GLOBAL_ACCEL_PCT` | 100 | **130** | `globalAccelerationPercentage` **(EEPROM)** |

- Um profile pode sobrescrever um knob individual: `#define TUNE_ESC_RAMP_2ND 25` **antes**
  do `#include "../tuning/agileCar.h"`.
- **EEPROM-backed:** `escTakeoffPunch` e `globalAccelerationPercentage` são lidos da EEPROM
  no boot; o `#define` só é o default de **EEPROM fresca**. Por isso `eeprom_id` foi
  bumpado 5 → 6 em `0_generalSettings.h` — a EEPROM é regravada com os novos defaults na
  primeira gravação. (Se você personalizou algo pela web em `192.168.4.1`, será perdido —
  mas `ENABLE_WIRELESS` está off.)
- Confirme os valores ativos no monitor serial no boot: linha
  `Vehicle ramp/accel (feel): escRampTime 2nd=18, accelSteps=12, ...`.

## Como adicionar um carro

1. `src/profiles/MeuCarro.h` — copie de `CarlosBtCar.h` ou `L120hRadio.h`, ajuste
   `PROFILE_VEHICLE`, comm, toggles, pinos, tuning.
2. `src/profiles/active.h` — adicione `#elif defined(PROFILE_MEU_CARRO)` / `#include "MeuCarro.h"`.
3. `platformio.ini` — adicione `[env:meu_carro]` com `build_flags = ${env.build_flags} -D PROFILE_MEU_CARRO`.
4. `pio run -e meu_carro`.

## Verificação (Fase 6)

| env | Build |
|---|---|
| `l120h_radio` | ✅ RAM 31.0% / Flash 42.2% — **byte-idêntico** ao build STOCK+IBUS anterior |
| `carlos_bt_car` | ✅ RAM 30.8% / Flash 39.8% (Neopixel + proteção de bateria compilados fora) |
| `gol_quadrado` | ✅ RAM 30.8% / Flash 34.6% (sons hidráulicos/esteira do L120H não entram) |
| `wemos_d1_mini` | ✅ |
| sem `-D PROFILE_*` | ✅ falha esperada: `#error` do `active.h` |
| `DAC1`/`DAC2` != 25/26 | ✅ falha esperada: `#error` em `main.cpp` |
