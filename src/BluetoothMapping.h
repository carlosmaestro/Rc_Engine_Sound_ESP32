#include <Arduino.h>

/* BLUETOOTH GAMEPAD MAPPING *******************************************************************************************
 *
 * Ativo somente com "#define BLUETOOTH_COMMUNICATION" em "2_Remote.h".
 *
 * O gamepad (Bluepad32: PS4 / PS5 / Xbox / Switch...) atua como um "receptor virtual": "BluetoothInput.cpp"
 * sintetiza pulseWidthRaw[1..13] em microssegundos (1000-2000, centro 1500) e entrega ao pipeline normal
 * (processRawChannels -> mapThrottle / rcTriggerRead / gearboxDetection / esc / led / mcpwmOutput).
 *
 * FASE 4a (atual) - so o essencial para dirigir + som:
 *   Analogico esquerdo X ....... CH1  direcao
 *   R2 (gatilho) ............... CH3  acelerador para frente
 *   L2 (gatilho) .............. CH3  freio / re
 *   R1 / L1 ................... CH2  marcha + / marcha - (estado interno 1..3)
 *   Quadrado (Square/X) ....... CH4  buzina (momentaneo)
 *   Cross (X/A) .............. CH10  liga/desliga motor (via momentary1Trigger.toggleLong do rcTriggerRead)
 *
 * FASE 4b (a fazer): Dpad -> luzes / setas (CH5/CH6), Circle -> hazard, Options/Share -> mode1/mode2,
 *   touchpad -> 5a roda / winch, feedback de rumble e cor da lightbar por estado do motor.
 */

// -- Temporizacao / failsafe --
const uint16_t BT_UPDATE_INTERVAL_MS = 15;  // com que frequencia chamar BP32.update() + reamostrar o gamepad
const uint16_t BT_FAILSAFE_TIMEOUT_MS = 500; // sem dados do gamepad por mais que isto -> failSafe (canais ao centro)

// -- Faixa dos canais sintetizados (bate com pulseNeutral/pulseSpan do perfil de radio ativo) --
const uint16_t BT_PULSE_CENTER = 1500;
const uint16_t BT_PULSE_SPAN = 480; // 1500 +/- 480 = 1020..1980

// -- Acelerador --
const int16_t BT_TRIGGER_MAX = 1020; // valor "cheio" de ctl->throttle() / ctl->brake() no Bluepad32 (0..1023)
const int16_t BT_TRIGGER_DEADZONE = 20;

// -- Direcao --
const int16_t BT_AXIS_MIN = -512; // faixa de ctl->axisX() no Bluepad32
const int16_t BT_AXIS_MAX = 511;
const int16_t BT_AXIS_DEADZONE = 24;

// -- Marcha (CH2, 3 posicoes) --
const uint8_t BT_GEAR_MIN = 1;
const uint8_t BT_GEAR_MAX = 3;
const uint16_t BT_GEAR_US[4] = {1500, 1000, 1500, 2000}; // [0] nao usado, [1..3] = posicoes da chave de cambio
