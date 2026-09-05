//
// Build profile: carro Bluetooth do usuario (derivado de referencia/Controller.ino,
// canal "Arduino Para Modelismo"). Bluepad32 (PS4/PS5) + ponte-H L298 Mini + servo +
// 8 GPIOs de luz + alto-falante/PAM8403. Sem divisor de bateria, sem Neopixel.
//
// pio run -e carlos_bt_car
//
// Refiacao vs Controller.ino: ponte-H IN1 movida de GPIO27 -> GPIO33; alto-falante
// novo em 25/26; buzzer do Controller.ino (GPIO25) removido (som sintetizado cobre).
//

// -- Veiculo / conjunto de sons --
#define PROFILE_VEHICLE "vehicles/VolvoL120H.h"

// -- Perfil de radio (fornece channelReversed[]/channelAutoZero[]/pulseSpan; ok p/ o mapa BT da fase 4a) --
#define FLYSKY_FS_I6S_LOADER

// -- Modo de comunicacao: gamepad Bluetooth (receptor virtual) --
#define BLUETOOTH_COMMUNICATION

// -- Toggles de placa --
#define RZ7886_DRIVER_MODE            // ponte-H generica de 2 pinos (L298/TB6612/DRV8833) via MCPWM unidade 1
#define PROFILE_NEOPIXEL 0            // sem fita WS2812
#define PROFILE_BATTERY_PROTECTION 0 // sem divisor de tensao
#define PROFILE_THIRD_BRAKELIGHT 0   // libera GPIO32 para a ponte-H (RZ7886_PIN2)

// -- Pinos (referencia/Controller.ino; bloco PIN ASSIGNMENTS de main.cpp esta sob #ifndef) --
#define STEERING_PIN 13              // Controller.ino: SERVO_DIRECAO
#define RZ7886_PIN1 33               // ponte-H IN1  (Controller.ino usava GPIO27; movido para o pino padrao do RZ7886)
#define RZ7886_PIN2 32               // ponte-H IN2  (Controller.ino: PONTE_H_IN02)

#define HEADLIGHT_PIN 15             // Controller.ino: FAROL (strapping pin - ok como saida pos-boot)
#define FOGLIGHT_PIN 2               // Controller.ino: FAROL_MILHA (strapping pin)
#define TAILLIGHT_PIN 4              // Controller.ino: LUZ_FREIO (lanterna + freio combinados)
#define INDICATOR_RIGHT_PIN 16       // Controller.ino: SETA_DIREITA
#define INDICATOR_LEFT_PIN 17        // Controller.ino: SETA_ESQUERDA
#define REVERSING_LIGHT_PIN 22       // Controller.ino: LUZ_RE
#define ROOFLIGHT_PIN 19             // Controller.ino: LUZ_AUX01 (era aux nao usado)
#define SIDELIGHT_PIN 21             // Controller.ino: LUZ_AUX02 (era aux nao usado)

#define CABLIGHT_PIN -1              // ausentes nesta placa (statusLED::begin(-1,...) e no-op)
#define BEACON_LIGHT1_PIN -1
#define BEACON_LIGHT2_PIN -1
#define SHAKER_MOTOR_PIN -1          // GPIO27 livre p/ um shaker futuro (exigiria realocar COUPLER_PIN)

#define BATTERY_DETECT_PIN 39        // sem divisor -> BATTERY_PROTECTION off acima
#define COMMAND_RX 36                // mantido, nao usado em modo Bluetooth

// -- Tuning: carrinho agil (resposta de acelerador direta) --
#include "../tuning/agileCar.h"
// (para sobrescrever um knob individual, #define TUNE_X ANTES desta linha)
