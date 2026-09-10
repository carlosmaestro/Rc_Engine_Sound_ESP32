//
// Placa física do carro Bluetooth do usuário (pinos + toggles), compartilhada por
// vários profiles (CarlosBtCar, GolQuadrado, ...). NÃO define veículo/comm/tuning —
// isso fica no profile que faz #include deste arquivo.
//
// Origem: referencia/Controller.ino (canal "Arduino Para Modelismo"). Refiação vs o
// .ino: ponte-H IN1 movida de GPIO27 -> GPIO33; alto-falante em 25/26; buzzer removido.
//

// -- Toggles de placa --
#define RZ7886_DRIVER_MODE            // ponte-H genérica de 2 pinos (L298/TB6612/DRV8833) via MCPWM unidade 1
#define PROFILE_NEOPIXEL 0            // sem fita WS2812
#define PROFILE_BATTERY_PROTECTION 0 // sem divisor de tensão
#define PROFILE_THIRD_BRAKELIGHT 0   // libera GPIO32 para a ponte-H (RZ7886_PIN2)

// -- Pinos (bloco PIN ASSIGNMENTS de main.cpp está sob #ifndef) --
#define STEERING_PIN 13              // Controller.ino: SERVO_DIRECAO
#define RZ7886_PIN1 32               // ponte-H IN1  (Controller.ino usava GPIO27; movido)
#define RZ7886_PIN2 33               // ponte-H IN2  (Controller.ino: PONTE_H_IN02)

#define HEADLIGHT_PIN 15             // Controller.ino: FAROL (strapping pin)
#define FOGLIGHT_PIN 2               // Controller.ino: FAROL_MILHA (strapping pin)
#define TAILLIGHT_PIN 4              // Controller.ino: LUZ_FREIO (lanterna + freio)
#define INDICATOR_RIGHT_PIN 16       // Controller.ino: SETA_DIREITA
#define INDICATOR_LEFT_PIN 17        // Controller.ino: SETA_ESQUERDA
#define REVERSING_LIGHT_PIN 22       // Controller.ino: LUZ_RE
#define ROOFLIGHT_PIN 19             // Controller.ino: LUZ_AUX01 (era aux nao usado)
#define SIDELIGHT_PIN 21             // Controller.ino: LUZ_AUX02 (era aux nao usado)

#define CABLIGHT_PIN -1              // ausentes nesta placa
#define BEACON_LIGHT1_PIN -1
#define BEACON_LIGHT2_PIN -1
#define SHAKER_MOTOR_PIN -1

#define BATTERY_DETECT_PIN 39
#define COMMAND_RX 36                // mantido, nao usado em modo Bluetooth
