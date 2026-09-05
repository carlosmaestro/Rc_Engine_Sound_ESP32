#include <Arduino.h>

/* HARDWARE LAYOUT SELECTOR *********************************************************************************************
 *
 * Ativa um conjunto completo de atribuicoes de pino ("layout de placa") para a build atual, usando a estrutura
 * do core sem tocar na logica de som / ESC / luzes.
 *
 * Padrao (mesmo estilo dos perfis de radio em "2_Remote.h" e dos perfis de servo em "7_Servos.h"):
 *   - uma lista de selecao abaixo, com EXATAMENTE UM layout descomentado;
 *   - um bloco #ifdef por layout, definindo os pinos que diferem do padrao;
 *   - os pinos NAO redefinidos aqui caem no valor padrao de "main.cpp" (bloco PIN ASSIGNMENTS, envolto em #ifndef).
 *
 * Este arquivo e incluido por "main.cpp" DEPOIS dos headers 0..10, entao pode #undef/#define os toggles de placa
 * (THIRD_BRAKELIGHT, NEOPIXEL_ON_CH4, RZ7886_DRIVER_MODE, SPI_DASHBOARD, NEOPIXEL_ENABLED, BATTERY_PROTECTION).
 *
 * Como adicionar um layout novo:
 *   1. adicione um "#define LAYOUT_MINHA_PLACA" na lista de selecao (comentado);
 *   2. crie um "#ifdef LAYOUT_MINHA_PLACA ... #endif" com os pinos/toggles da sua placa;
 *   3. selecione-o (remova o //) e compile. Os #error abaixo pegam layout duplo ou DAC fora de 25/26.
 */

// ---- Selecione UM layout (remova o //) ----------------------------------------------------------------------------
#define LAYOUT_STOCK_30PIN        // Placa padrao TheDIYGuy999 (30 pinos) - comportamento identico ao de sempre
// #define LAYOUT_WEMOS_D1_MINI   // Variante Wemos D1 Mini ESP32 (faróis no GPIO22, sem luz de cabine)
// #define LAYOUT_CARLOS_BT_CAR   // Carro Bluetooth do usuario (derivado de "referencia/Controller.ino")

// ---- Saida de audio: IMUTAVEL em todos os layouts ----------------------------------------------------------------
#define DAC1 25 // canal A -> resistor 10k -> entrada do PAM8403
#define DAC2 26 // canal B -> resistor 10k -> entrada do PAM8403

// =================================================================================================================
#ifdef LAYOUT_STOCK_30PIN
// Nada a redefinir: todos os pinos usam os defaults de "main.cpp".
// Toggles de placa continuam vindo dos headers numerados (6_Lights.h: THIRD_BRAKELIGHT, NEOPIXEL_ENABLED; etc.).
#endif

// =================================================================================================================
#ifdef LAYOUT_WEMOS_D1_MINI
#define WEMOS_D1_MINI_ESP32 // ativa os ramos #ifdef WEMOS_D1_MINI_ESP32 ja existentes em main.cpp (DEBUG_RX=3, HEADLIGHT_PIN=22, CABLIGHT_PIN=-1)
#endif

// =================================================================================================================
#ifdef LAYOUT_CARLOS_BT_CAR
// Carro Bluetooth (Bluepad32) montado no estilo "referencia/Controller.ino".
// Refiacao minima em relacao ao Controller.ino: ponte-H IN1 movida de GPIO27 -> GPIO33; alto-falante novo em 25/26;
// buzzer do Controller.ino (GPIO25) removido - a funcao e coberta pelo som sintetizado.

// -- Direcao (servo, MCPWM unidade 0) --
#define STEERING_PIN 13 // Controller.ino: SERVO_DIRECAO = 13

// -- Tracao: ponte-H generica de 2 pinos (L298N / TB6612 / DRV8833) via o caminho RZ7886 (MCPWM unidade 1, drive-brake) --
#define RZ7886_DRIVER_MODE
#define RZ7886_PIN1 33  // ponte-H IN1  (Controller.ino usava GPIO27; movido para o pino padrao do RZ7886)
#define RZ7886_PIN2 32  // ponte-H IN2  (Controller.ino: PONTE_H_IN02 = 32)

// -- Luzes (pinos do Controller.ino; todas via statusLED/LEDC) --
#define HEADLIGHT_PIN 15        // Controller.ino: FAROL (strapping pin - ok como saida pos-boot, ja usado assim)
#define FOGLIGHT_PIN 2          // Controller.ino: FAROL_MILHA (strapping pin)
#define TAILLIGHT_PIN 4         // Controller.ino: LUZ_FREIO (lanterna + freio combinados)
#define INDICATOR_RIGHT_PIN 16  // Controller.ino: SETA_DIREITA
#define INDICATOR_LEFT_PIN 17   // Controller.ino: SETA_ESQUERDA
#define REVERSING_LIGHT_PIN 22  // Controller.ino: LUZ_RE
#define ROOFLIGHT_PIN 19        // Controller.ino: LUZ_AUX01 (era aux nao usado)
#define SIDELIGHT_PIN 21        // Controller.ino: LUZ_AUX02 (era aux nao usado)

// -- Perifericos ausentes nesta placa (statusLED::begin(-1, ...) e no-op, mesmo precedente de CABLIGHT_PIN=-1 no modo Wemos) --
#define CABLIGHT_PIN -1
#define BEACON_LIGHT1_PIN -1
#define BEACON_LIGHT2_PIN -1
#define SHAKER_MOTOR_PIN -1 // GPIO27 fica livre p/ um shaker futuro (exigiria tambem realocar COUPLER_PIN)

// -- Entradas --
#define BATTERY_DETECT_PIN 39 // sem divisor de tensao nesta placa -> BATTERY_PROTECTION desligada abaixo
#define COMMAND_RX 36         // mantido, mas nao usado em modo Bluetooth

// -- Toggles de placa (este header e incluido depois de 3_ESC.h / 6_Lights.h / 9_Dashboard.h) --
#undef THIRD_BRAKELIGHT   // libera GPIO32 para a ponte-H (RZ7886_PIN2)
#undef NEOPIXEL_ENABLED   // sem fita WS2812 nesta placa
#undef BATTERY_PROTECTION // sem divisor de tensao nesta placa
#endif

// ---- Guardas de compilacao (primeiros #error do projeto) -------------------------------------------------------
#if (defined(LAYOUT_STOCK_30PIN) + defined(LAYOUT_WEMOS_D1_MINI) + defined(LAYOUT_CARLOS_BT_CAR)) != 1
#error "hardwareLayout.h: selecione EXATAMENTE UM LAYOUT_* na lista de selecao"
#endif

#if DAC1 != 25 || DAC2 != 26
#error "hardwareLayout.h: DAC1/DAC2 devem permanecer em 25/26 (saida de audio do motor de som)"
#endif
