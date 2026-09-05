# Documentação — RC Engine Sound & Light Controller (ESP32)

Este diretório documenta o firmware contido em `src/`. O projeto é um **fork** do
[Rc_Engine_Sound_ESP32 do TheDIYGuy999](https://github.com/TheDIYGuy999/Rc_Engine_Sound_ESP32)
(versão de código `9.13.0`), com uma camada adicional de **controle via joystick
Bluetooth (PS4/PS5)** usando a biblioteca [Bluepad32](https://bluepad32.readthedocs.io/).

- Branch de trabalho: `ps4-5-bluetooth`
- Branch principal: `master`
- Plataforma alvo: ESP32 (`esp32dev`, 240 MHz), toolchain PlatformIO
- Build por **profile** (`pio run -e <profile>`): `l120h_radio` (default), `carlos_bt_car`, `wemos_d1_mini` — ver [doc 10](10-profiles-de-build.md)
- Veículo em todos os profiles atuais: **Volvo L120H** (carregadeira / `LOADER_MODE`)

---

## Índice

| # | Documento | Conteúdo |
|---|-----------|----------|
| 01 | [Visão geral](01-visao-geral.md) | O que o firmware faz, origem, recursos, o que muda nesta branch |
| 02 | [Arquitetura](02-arquitetura.md) | Modelo dual-core, laços principais, interrupções, fluxo de dados, diagramas |
| 03 | [Hardware e pinos](03-hardware-e-pinos.md) | Mapa de GPIO, periféricos, saída de áudio DAC, alimentação |
| 04 | [Entrada de controle](04-entrada-de-controle.md) | PWM/SBUS/IBUS/SUMD/PPM, pipeline de canais (o receptor virtual Bluetooth está no doc 11) |
| 05 | [Motor de som](05-motor-de-som.md) | Timers de interrupção, máquina de estados de som, mixer, arquivos de som |
| 06 | [Simulação de motor, transmissão e ESC](06-simulacao-motor-transmissao-esc.md) | `engineMassSimulation`, marchas, máquina de estados do ESC, proteção de bateria |
| 07 | [Luzes, servos e periféricos](07-luzes-servos-perifericos.md) | Luzes/Neopixel, saídas de servo MCPWM, shaker, dashboard, reboque ESP-NOW |
| 08 | [Configuração](08-configuracao.md) | Arquivos `0_`..`10_`, presets de veículo, mapa de EEPROM, interface web/serial |
| 09 | [Build e deploy](09-build-e-deploy.md) | `platformio.ini`, dependências, partições, gravação, depuração |
| 10 | [Build profiles](10-profiles-de-build.md) | `src/profiles/*.h` — 1 arquivo por carro (veículo + pinos + comm + toggles + tuning); `pio run -e carlos_bt_car`. Set de tuning `src/tuning/agileCar.h`. |
| 11 | [Controle Bluetooth](11-mapa-controle-bluetooth.md) | Receptor virtual, mapa gamepad→canal, failsafe, ajuste de latência |

---

## Mapa rápido do repositório

```text
Rc_Engine_Sound_ESP32/
├── platformio.ini            # Configuração de build (env esp32dev, framework pio-framework-bluepad32)
├── src/
│   ├── main.cpp              # ~5.700 linhas — todo o firmware (setup/loop/Task1 + funções)
│   ├── main.h                # Forward declarations das funções de main.cpp
│   ├── profiles/             # 1 arquivo por carro (active.h + L120hRadio.h / CarlosBtCar.h / WemosD1Mini.h) — ver doc 10
│   ├── tuning/agileCar.h     # Set de tuning "carrinho ágil" (TUNE_* macros) — ver doc 10
│   ├── BluetoothMapping.h    # Mapa gamepad → canal + constantes de ajuste (modo Bluetooth) — ver doc 11
│   ├── input/BluetoothInput.h/.cpp  # Receptor virtual Bluepad32 (PS4/PS5) — ver doc 11
│   ├── 0_generalSettings.h  # WiFi, debug, id de EEPROM
│   ├── 1_Vehicle.h          # Seleção do preset de veículo (inclui vehicles/VolvoL120H.h)
│   ├── 2_Remote.h           # Perfil do rádio + protocolo de recepção (IBUS ou BLUETOOTH_COMMUNICATION)
│   ├── 3_ESC.h              # Parâmetros do ESC e proteção de bateria
│   ├── 4_Transmission.h     # Opções de câmbio
│   ├── 5_Shaker.h           # Motor de vibração
│   ├── 6_Lights.h           # Luzes e Neopixel
│   ├── 7_Servos.h           # Limites e perfis de servo (modo BUS)
│   ├── 8_Sound.h            # Volume mestre / passos de volume
│   ├── 9_Dashboard.h        # Display LCD ST7735
│   ├── 10_Trailer.h         # Endereços MAC dos reboques ESP-NOW
│   ├── src/                 # Cabeçalhos auxiliares (curves.h, helper.h, dashboard, sbus, SUMD, webInterface.h)
│   └── vehicles/            # ~90 presets de veículo + vehicles/sounds/*.h (áudio em PROGMEM)
├── referencia/Controller.ino # Sketch de origem do profile carlos_bt_car (canal Arduino Para Modelismo)
├── data/                    # calibration.txt
├── hardware/                # PCBs (Gerber/STL) do controlador
├── tools/                   # Audio2Header.html / Header2Audio.html (conversores de som)
├── documentation/           # Documentação original do upstream (Changelog, planilhas, manual PDF)
└── docs/                    # ESTA documentação
```
