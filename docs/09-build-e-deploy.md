# 09 — Build e deploy

## Ambiente

- **PlatformIO** (recomendado pelo upstream; VS Code + extensão PlatformIO).
- Requer **git** instalado (as libs são baixadas de repositórios git).
- Arduino IDE também é possível, mas exige gerenciar libs/boards à mão e a pilha
  Bluepad32 — **não recomendado neste fork**.

## `platformio.ini` (env `esp32dev`)

```ini
platform          = espressif32@6.10.0
framework         = arduino
platform_packages = framework-arduinoespressif32@
                    https://github.com/maxgerhardt/pio-framework-bluepad32/archive/refs/heads/main.zip
board             = esp32dev
board_build.mcu   = esp32
board_build.f_cpu = 240000000L          ; obrigatório 240 MHz (geração de áudio)
board_build.f_flash        = 40000000L
board_build.partitions     = huge_app.csv   ; app grande, SEM OTA
monitor_speed     = 115200
monitor_filters   = esp32_exception_decoder ; decodifica backtrace
upload_protocol   = esptool
upload_speed      = 921600              ; baixe p/ 115200 se der erro de upload
```

> **Ponto crítico do fork:** `platform_packages` substitui o framework Arduino-ESP32
> padrão pelo fork **`pio-framework-bluepad32`** do `maxgerhardt`. É isso que fornece
> `<Bluepad32.h>` e a pilha BT-HID que o `BluetoothController` usa. Sem essa linha, o
> build quebra em `BluetoothController.cpp`.

### `build_flags` — dashboard TFT_eSPI

Mesmo com o dashboard desligado no código, os flags do `TFT_eSPI` são passados
(`-DUSER_SETUP_LOADED=1`, `-DST7735_DRIVER=1`, `-DTFT_WIDTH=80`, `-DTFT_HEIGHT=160`,
`-DST7735_REDTAB160x80=1`, `-DTFT_RGB_ORDER=TFT_BGR`, pinos `TFT_MOSI=23`, `TFT_SCLK=18`,
`TFT_CS=-1`, `TFT_DC=19`, `TFT_RST=21`, fontes `LOAD_GLCD/FONT2/FONT4`,
`SPI_FREQUENCY=27000000`, `USE_HSPI_PORT=1`). Ajuste `TFT_RGB_ORDER` se as cores do LCD
saírem trocadas.

### `lib_deps`

Baixadas automaticamente:

| Lib | Uso |
|-----|-----|
| `SPI` | barramento do LCD |
| `TheDIYGuy999/statusLED` | controle de cada luz + shaker + (base do) ESC |
| `TheDIYGuy999/SBUS` | SBUS (só se **não** `EMBEDDED_SBUS`) |
| `TheDIYGuy999/rcTrigger` | detecção de gatilhos curtos/longos nos canais de função |
| `bmellink/IBusBM` | protocolo IBUS (**ativo**) |
| `Bodmer/TFT_eSPI` **2.3.70** | dashboard LCD (versão fixada — outras quebram) |
| `FastLED` **3.3.3** | fita Neopixel WS2812 |
| `madhephaestus/ESP32AnalogRead` | leitura do divisor de bateria |
| `lbernstone/Tone32` | bipes de detecção de células da bateria |

Bluepad32 vem junto com o pacote do framework, não em `lib_deps`.

## Comandos

```bash
# compilar
pio run

# compilar + gravar (ajuste a porta se preciso: --upload-port COMx)
pio run -t upload

# monitor serial (115200, com decodificador de exceção)
pio device monitor

# limpar
pio run -t clean
```

No VS Code: barra do PlatformIO → **Build** / **Upload** / **Monitor**.

## Fluxo de gravação

```mermaid
flowchart LR
    A["Editar cabeçalhos\n(veículo, rádio, ESC, luzes...)"] --> B["pio run"]
    B --> C{"Compilou?"}
    C -- não --> A
    C -- sim --> D["pio run -t upload\n(ESP32 no bootloader se necessário)"]
    D --> E["pio device monitor"]
    E --> F["Conferir: versão, MAC, motivo do reset,\ncalibração bateria/ESC, offsets de canal,\nBD Addr do Bluepad32"]
    F --> G["Parear controle PS4/PS5\n(controle em modo pairing)"]
```

## Checklist de primeira gravação

1. `board_build.f_cpu = 240000000L` (não mexer).
2. Conferir o veículo em `1_Vehicle.h` e o par **protocolo + perfil** em `2_Remote.h`.
3. Calibrar o divisor de bateria em `3_ESC.h` (`RESISTOR_*`, `DIODE_DROP`) comparando
   com o multímetro.
4. Se trocou configuração e quer resetar a EEPROM, incrementar `eeprom_id` em
   `0_generalSettings.h`.
5. Gravar, abrir o monitor, ligar o rádio/receptor — as setas piscam durante a
   inicialização do BUS; erro de sinal = 3 flashes rápidos; erro de bateria = 2 flashes.
6. Colocar o controle Bluetooth em modo de pareamento no primeiro uso
   (`forgetBluetoothKeys()` roda a cada boot).

## Notas

- **Sem OTA** — a partição `huge_app.csv` usa todo o espaço para o app + arrays de som.
- `src/src copy.txt` e `src/main.cpp` guardam versões quase idênticas; o build usa
  `src/main.cpp`. `src/vehicles/FreightlinerCummins350 2.h` e afins são duplicatas
  com espaço no nome — inofensivas, mas evite referenciá-las.
- O diretório `build/` na raiz e `src/build/` são resíduos; o output real do PlatformIO
  vai para `.pio/build/esp32dev/`.
- O `.git` deste checkout está com packfile corrompido (`bad object HEAD`); recrie o
  clone se precisar de histórico/branches.
