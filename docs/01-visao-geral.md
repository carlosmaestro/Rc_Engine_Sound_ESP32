# 01 — Visão geral

## O que é

Firmware para um **controlador de som e luzes de motor** para modelos RC (caminhões,
carregadeiras, escavadeiras, carros, tanques, locomotivas...). Roda em um ESP32 e:

- Gera **som de motor sintetizado** em tempo real por dois DACs de 8 bits (GPIO25/26),
  alimentando um amplificador PAM8403 e alto-falantes.
- Simula **inércia de motor**, **embreagem**, **caixa de câmbio** (manual real, virtual
  de 3 marchas, sequencial de 16, automática com conversor de torque, dupla embreagem).
- Controla um **ESC** de crawler (ou um driver RZ7886) com rampa de aceleração/frenagem
  virtual (comportamento "peso" do veículo).
- Aciona **luzes** (faróis, lanternas, freio, ré, seta, farol de milha, giroflex,
  luzes de teto, luzes laterais) e uma fita **Neopixel** (Knight Rider / giroflex / etc.).
- Aciona um **motor shaker** que vibra o modelo no ralenti e na partida.
- Fornece saídas de **servo** (direção, câmbio, guincho, engate de 5ª roda) em modo BUS.
- Comunica com um **reboque sem fio** por ESP-NOW.
- Publica uma **página de configuração** em `192.168.4.1` (ver [08 — Configuração](08-configuracao.md)).
- Suporta um **dashboard LCD** SPI ST7735 (80×160) opcional.

## Origem e créditos

Baseado em `Rc_Engine_Sound_ESP32` de **TheDIYGuy999** (`char codeVersion[] = "9.13.0"`).
Contribuições upstream: bitluni (conversor de áudio), Wombii (transmissão automática),
Gamadril (dashboard, Neopixel, SUMD), Christian Fiebig.

## O que esta branch (`ps4-5-bluetooth`) acrescenta

Duas features, mais um snapshot de trabalho pré-existente do fork:

1. **Build profiles** (`src/profiles/*.h` + `src/tuning/`) — um arquivo por carro físico
   que empacota veículo/sons + modo de comunicação + perfil de rádio + toggles de placa +
   pinagem + tuning de "sensação". Selecionado por `[env:*]` no `platformio.ini`
   (`pio run -e carlos_bt_car`). Ver [10 — Build profiles](10-profiles-de-build.md).
2. **Controle Bluetooth como receptor virtual** (`src/input/BluetoothInput.cpp`,
   `src/BluetoothMapping.h`) — pareia um **DualShock 4 / DualSense (PS4/PS5)** via
   **Bluepad32** e *sintetiza* `pulseWidth[1..13]`, entregando ao pipeline normal
   (`processRawChannels` → `mapThrottle` / `esc` / `led` / servos, tudo inalterado).
   Ativado por `#define BLUETOOTH_COMMUNICATION` em `2_Remote.h`. Ver
   [11 — Controle Bluetooth](11-mapa-controle-bluetooth.md).

O PoC anterior — `src/BluetoothController.cpp` — foi **removido** (commit `fefe8fd`): ele
furava a abstração (injetava em `currentThrottle`) e acionava uma ponte-H própria via
`ledcWrite`, colidindo com os canais LEDC das luzes.

Integração com o upstream:

| Local | Alteração |
|-------|-----------|
| `platformio.ini` | `platform_packages` → fork `pio-framework-bluepad32` (arduino-esp32 2.0.17 + Bluepad32 4.1.0 + BTstack) |
| `src/main.cpp` | `#include "profiles/active.h"` como 1º header; bloco de pinos sob `#ifndef`; ramo `BLUETOOTH_COMMUNICATION` nas cadeias de `setup()`/`loop()`/wait-loop |
| `src/2_Remote.h`, `3_ESC.h`, `6_Lights.h` | defaults de comm/rádio/toggles/tuning guardados (`#if !defined ...` / `#ifndef PROFILE_* / TUNE_*`) para o profile poder sobrescrever |
| `src/input/BluetoothInput.cpp` | `readBluetoothCommands()` — poll BP32, sintetiza canais, failsafe por timeout de 500 ms |

> **Upgrade do core (arduino-esp32 3.x / IDF 5.x): adiado.** Não há caminho
> `framework=arduino` + Bluepad32 no core 3.x. Fica como projeto separado.

## Configuração (profile `l120h_radio`, o default)

Cada linha abaixo é o **default**, definido pelo profile `l120h_radio` (`pio run -e l120h_radio`).
O profile `carlos_bt_car` muda: comm → `BLUETOOTH_COMMUNICATION`, Neopixel/proteção de
bateria/3ª luz de freio → **off**, `RZ7886_DRIVER_MODE` → **on**, pinos → `referencia/Controller.ino`,
tuning → `agileCar.h`.

| Item | Valor | Onde |
|------|-------|------|
| Preset de veículo | `vehicles/VolvoL120H.h` | `profiles/*.h` → `PROFILE_VEHICLE` → `1_Vehicle.h` |
| Modo de veículo | `LOADER_MODE` (carregadeira) | `VolvoL120H.h` |
| Modo de comunicação | `IBUS_COMMUNICATION` (profile `l120h_radio`) | `profiles/*.h` |
| Perfil de rádio | `FLYSKY_FS_I6S_LOADER` | `2_Remote.h` |
| Transmissão | automática, 1 marcha (`NumberOfAutomaticGears 1`), + `VIRTUAL_3_SPEED` disponível | `VolvoL120H.h`, `4_Transmission.h` |
| Câmbio virtual | `VIRTUAL_3_SPEED`, `TRANSMISSION_NEUTRAL` | `4_Transmission.h` |
| Neopixel | habilitado, 8 LEDs, modo 2 (Knight Rider) | `6_Lights.h` |
| Terceira luz de freio | `THIRD_BRAKELIGHT` (GPIO32) | `6_Lights.h` |
| Proteção de bateria | `BATTERY_PROTECTION` ativo, corte 3,3 V/célula | `3_ESC.h` |
| id de EEPROM | `eeprom_id = 6` | `0_generalSettings.h` |
| Dashboard SPI | **desligado** (`SPI_DASHBOARD` comentado) | `9_Dashboard.h` |
| Rede sem fio (ESP-NOW/WiFi) | `ENABLE_WIRELESS` **comentado** em `0_generalSettings.h` (mas `setupEspNow()` ainda é chamado) | `0_generalSettings.h` |

## Recursos por "modo de veículo"

Os modos são mutuamente exclusivos e definidos no arquivo do veículo:

- **Normal** (padrão): caminhão/carro com câmbio.
- `TRACKED_MODE`: entrada de acelerador dupla (CH2+CH3) para lagartas/tanques.
- `EXCAVATOR_MODE`: escavadeira hidráulica (bomba, fluxo, chocalho de esteira/caçamba).
- `LOADER_MODE`: **carregadeira** — RPM da bomba hidráulica proporcional à lança/caçamba
  (`loaderControl()`), sem inércia de tração pesada. **← modo ativo neste checkout.**
- `STEAM_LOCOMOTIVE_MODE`: locomotiva a vapor (batidas por rotação).
- `AIRPLANE_MODE`: avião (sem ESC, embreagem sempre solta).
