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

Um módulo novo — **`src/BluetoothController.cpp` / `.h`** — que usa **Bluepad32** para
parear um controle **DualShock 4 / DualSense (PS4/PS5)** e usá-lo como fonte de comando
no lugar (ou além) do receptor RC.

Pontos de integração com o código upstream:

| Local | Alteração |
|-------|-----------|
| `platformio.ini` | `platform_packages` aponta para o fork `pio-framework-bluepad32` do Arduino-ESP32 (necessário para o Bluepad32 substituir a pilha BT-HID) |
| `src/main.cpp` (topo) | `#include "BluetoothController.h"` |
| `setup()` | chama `setupBluetoothController()` (registra callbacks BP32, esquece chaves BT) |
| `loop()` | primeira chamada é `loopBluetoothController()` (poll a cada 50 ms) |
| `mapThrottle()` (modo normal) | `currentThrottle = getBLTCurrentThrottle();` — o acelerador do firmware passa a vir do joystick |
| `processGamepad()` | botão **A** → `engineOnOff()` + cor do LED do controle; **X** → `triggerHorn()` + rumble; **B** → LEDs de "player" |

O `BluetoothController` também aciona **diretamente** uma ponte-H de tração
(`ledcWrite(10/11, ...)`) e uma luz de ré (`digitalWrite(22, ...)`) a partir do eixo Y
do analógico direito (`axisRY`) ou do gatilho R2 (`throttle()`, quando `manualMode`).
Ver detalhes e ressalvas em [04 — Entrada de controle](04-entrada-de-controle.md#camada-bluetooth-bluetoothcontroller).

## Configuração atual do checkout

| Item | Valor | Onde |
|------|-------|------|
| Preset de veículo | `vehicles/VolvoL120H.h` | `1_Vehicle.h` |
| Modo de veículo | `LOADER_MODE` (carregadeira) | `VolvoL120H.h` |
| Protocolo de recepção RC | `IBUS_COMMUNICATION` | `2_Remote.h` |
| Perfil de rádio | `FLYSKY_FS_I6S_LOADER` | `2_Remote.h` |
| Transmissão | automática, 1 marcha (`NumberOfAutomaticGears 1`), + `VIRTUAL_3_SPEED` disponível | `VolvoL120H.h`, `4_Transmission.h` |
| Câmbio virtual | `VIRTUAL_3_SPEED`, `TRANSMISSION_NEUTRAL` | `4_Transmission.h` |
| Neopixel | habilitado, 8 LEDs, modo 2 (Knight Rider) | `6_Lights.h` |
| Terceira luz de freio | `THIRD_BRAKELIGHT` (GPIO32) | `6_Lights.h` |
| Proteção de bateria | `BATTERY_PROTECTION` ativo, corte 3,3 V/célula | `3_ESC.h` |
| id de EEPROM | `eeprom_id = 5` | `0_generalSettings.h` |
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
