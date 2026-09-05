# 02 — Arquitetura

Quase todo o firmware está em `src/main.cpp` (~5.700 linhas). Os arquivos `0_*.h` ..
`10_*.h`, `profiles/*.h`, `tuning/*.h`, `BluetoothMapping.h` e `vehicles/*.h` são
**incluídos** por `main.cpp` e contêm só `#define`s / variáveis de configuração. Os únicos `.cpp`
separados são `src/input/BluetoothInput.cpp` (receptor virtual Bluetooth) e os
auxiliares em `src/src/` (`SUMD.cpp`, `sbus.cpp`, `dashboard.cpp`). `main.h` traz as
_forward declarations_ que esses `.cpp` precisam.

## Camadas lógicas

```mermaid
flowchart TB
    subgraph ENTRADA["Entrada de comando"]
        RC["Receptor RC\n(PWM / SBUS / IBUS / SUMD / PPM)"]
        BT["Joystick Bluetooth\n(Bluepad32 / PS4 / PS5)"]
    end

    subgraph NUCLEO["Núcleo de simulação (main.cpp)"]
        CANAIS["Pipeline de canais\nprocessRawChannels()"]
        THR["mapThrottle()\ncurrentThrottle 0..500"]
        ENG["engineMassSimulation()\ncurrentRpm, engineSampleRate"]
        TRANS["Transmissão\ngearboxDetection() / automaticGearSelector()"]
        ESC["esc()\nmáquina de estados driveState\ncurrentSpeed"]
        LIGHTS["led() / triggerIndicators()\nupdateRGBLEDs()"]
        SND["Máquina de estados de som\nengineState"]
    end

    subgraph SAIDA["Saídas físicas"]
        DAC["2x DAC 8 bit (GPIO25/26)\n-> amplificador PAM8403"]
        MCPWM["MCPWM -> ESC (GPIO33)\ne servos CH1..CH4"]
        LEDC["LEDC (PWM) -> luzes + shaker"]
        NEO["RMT -> fita Neopixel WS2812"]
        NOW["ESP-NOW -> reboque"]
        LCD["SPI -> dashboard ST7735"]
    end

    RC --> CANAIS
    BT --> THR
    CANAIS --> THR
    THR --> ENG --> SND --> DAC
    THR --> ESC --> MCPWM
    ENG --> TRANS --> ESC
    CANAIS --> LIGHTS --> LEDC
    LIGHTS --> NEO
    ESC --> LIGHTS
    CANAIS --> NOW
    ENG --> LCD
```

## Modelo de execução (FreeRTOS / dual-core)

O ESP32 tem 2 núcleos. O firmware divide o trabalho para garantir que a geração de
áudio nunca sofra _jitter_:

```mermaid
flowchart LR
    subgraph CORE0["Núcleo 0 — Task1code() (loop infinito, prioridade 1)"]
        direction TB
        A0["dacOffsetFade()"]
        A1["engineMassSimulation()  (mutex xRpmSemaphore)"]
        A2["automaticGearSelector()  (se automático/dupla embreagem)"]
        A3["engineOnOff()"]
        A4["led()"]
        A5["shaker()  (se sem SPI_DASHBOARD)"]
        A6["gearboxDetection()"]
        A7["esc()  (+ proteção de bateria)"]
        A0-->A1-->A2-->A3-->A4-->A5-->A6-->A7-->A0
    end

    subgraph CORE1["Núcleo 1 — loop() do Arduino"]
        direction TB
        B1["readXxxCommands() / readBluetoothCommands()  (modo de comunicação ativo)"]
        B2["mcpwmOutput()  (servos, em modo BUS)"]
        B3["triggerHorn() / triggerIndicators()"]
        B4["mapThrottle() + rcTriggerRead()  (mutex xRpmSemaphore)"]
        B5["loaderControl() / excavatorControl() / ...  (por modo)"]
        B6["updateRGBLEDs()  (Neopixel)"]
        B7["trailerControl()  (ESP-NOW)"]
        B8["webInterface() / serialInterface()"]
        B9["rtc_wdt_feed()"]
        B1-->B2-->B3-->B4-->B5-->B6-->B7-->B8-->B9-->B1
    end

    subgraph ISR["Interrupções de hardware (executam no núcleo 0)"]
        direction TB
        T0["Timer 0 -> variablePlaybackTimer()\ntaxa de amostragem VARIÁVEL = som do motor"]
        T1["Timer 1 -> fixedPlaybackTimer()\ntaxa FIXA = buzina, sirene, seta, ré, engate..."]
        RMT["RMT ISR -> rmt_isr_handler()\nlargura de pulso PWM dos canais RC"]
        PPM["GPIO ISR -> readPpm()  (modo PPM)"]
        COUP["GPIO ISR -> trailerPresenceSwitchInterrupt()"]
    end

    T0 --> DACOUT["dacWrite(GPIO25/26)"]
    T1 --> DACOUT
```

Detalhes importantes:

- `Task1code` é criada em `setup()` com `xTaskCreatePinnedToCore(..., core 0)`, stack 8192.
- O `loop()` do Arduino roda no **núcleo 1** por padrão.
- `disableCore0WDT()` é chamado — a Task1 roda sem `delay()`, então o watchdog do
  núcleo 0 é desligado. Um **RTC WDT** de 10 s é armado e precisa ser "alimentado"
  (`rtc_wdt_feed()`) nos dois laços; se algum travar, o chip reinicia.
- **Mutexes**: `xRpmSemaphore` protege o grupo `currentRpm` / `currentThrottle` /
  gear state compartilhado entre os dois núcleos; `xPwmSemaphore` existe para a
  variável de PWM (uso legado).
- Os dois timers de playback reprogramam o próprio `timerAlarmWrite()` a cada disparo
  para variar a taxa de amostragem conforme o RPM.

## Sequência de `setup()`

```mermaid
sequenceDiagram
    participant S as setup()
    S->>S: disableCore0WDT() + arma RTC WDT (10 s)
    S->>S: Serial.begin(115200)
    S->>S: battery.attach(GPIO39)
    S->>S: imprime infos de sistema / reset reason
    S->>S: setupEeprom()  (lê ou grava defaults conforme eeprom_id)
    S->>S: cria mutexes xPwmSemaphore / xRpmSemaphore
    S->>S: pinMode + attachInterrupt (chave de engate)
    S->>S: begin() de cada statusLED (faróis, setas, shaker...) em canais LEDC 2..15
    S->>S: setupEspNow()
    S->>S: setupNeopixel()  (FastLED, se NEOPIXEL_ENABLED)
    S->>S: seleção do modo de comunicação (BLUETOOTH/SBUS/IBUS/PPM/SUMD/PWM) + setupMcpwm()
    Note over S: se BLUETOOTH_COMMUNICATION: setupBluetoothInput() (callbacks BP32)
    S->>S: xTaskCreatePinnedToCore(Task1code, core 0)
    S->>S: dacWrite(DAC1/DAC2, 0)
    S->>S: timerBegin(0) -> variablePlaybackTimer ; timerBegin(1) -> fixedPlaybackTimer
    S->>S: espera o receptor RC / o gamepad conectar (loop de flash nas setas)
    S->>S: calcula faixas de pulso por canal (pulseZero=1500 +/- pulseSpan)
    S->>S: setupMcpwmESC()
```

## Convenção de nomes dos "canais"

O firmware trabalha internamente com um array `pulseWidth[1..13]` em **microssegundos**
(1000–2000, centro 1500), independente do protocolo de entrada. O mapeamento
"canal do rádio → canal do controlador" é feito pelos `#define STEERING/THROTTLE/...`
no perfil de rádio em `2_Remote.h`. Ver [04 — Entrada de controle](04-entrada-de-controle.md).

| Canal interno | Função padrão |
|---------------|---------------|
| CH1 | Direção (ou caçamba, em loader/excavator) |
| CH2 | Câmbio 3 posições (ou lança) |
| CH3 | Acelerador + freio |
| CH4 | Buzina / giroflex / sirene |
| CH5 | `FUNCTION_R` — jake brake, farol alto, lampejo, motor on/off |
| CH6 | `FUNCTION_L` — setas, pisca-alerta |
| CH7 | `POT2` |
| CH8 / CH9 | `MODE1` / `MODE2` |
| CH10..CH13 | `MOMENTARY1`, `HAZARDS`, `INDICATOR_LEFT/RIGHT` |
