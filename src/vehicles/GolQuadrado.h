#include <Arduino.h>

// Vehicle preset: VW Gol "quadrado" (G1/G2, 1980-1994) — carro leve, motor 4 cilindros a gasolina.
// Usa o banco de sons do VW Fusca/Beetle (boxer refrigerado a ar), que É o motor do Gol BX inicial
// (VW ar 1300/1600). Dinâmica ajustada para um carro pequeno e revvy (não caminhão / não carregadeira).
// Para o som do AP (refrigerado a água, 1.6/1.8) o firmware não tem samples dedicados — este preset
// dá a textura de 4 cilindros leve; troque os #include "sounds/..." se você converter áudio real
// do seu Gol com tools/Audio2Header.html.

// Sound files (22'050 Hz, 8 bit PCM) --------------------------------------------------------------------
// Partida --------
volatile int startVolumePercentage = 90;
#include "sounds/VWBeetleStart.h" // VW ar / Fusca / Gol BX

// Ralenti --------
volatile int idleVolumePercentage = 80;
volatile int engineIdleVolumePercentage = 75;
volatile int fullThrottleVolumePercentage = 170;
#include "sounds/VWBeetleIdle.h"

// Sub som de aceleração (rev) — ativado: carro que gira, sobe de giro de forma limpa ----------
#define REV_SOUND
volatile int revVolumePercentage = 90;
volatile int engineRevVolumePercentage = 80;
volatile const uint16_t revSwitchPoint = 320;  // acima disto toca o rev no lugar do idle
volatile const uint16_t idleEndPoint = 400;    // acima disto = 100% rev, 0% idle
volatile const uint16_t idleVolumeProportionPercentage = 90;
#ifdef REV_SOUND
#include "sounds/VWBeetleRev2.h"
#endif

// Jake brake — carro não tem ----------
//#define JAKE_BRAKE_SOUND
volatile int jakeBrakeVolumePercentage = 0;
volatile int jakeBrakeIdleVolumePercentage = 0;
volatile int jakeBrakeMinRpm = 200;

// "Knock" = textura das explosões dos 4 cilindros a gasolina ----------
volatile int dieselKnockVolumePercentage = 85;   // um pouco menos "putt-putt" que o Fusca (100)
volatile int dieselKnockIdleVolumePercentage = 0;
volatile int dieselKnockInterval = 4;            // 4 cilindros
volatile int dieselKnockStartPoint = 40;
volatile int dieselKnockAdaptiveVolumePercentage = 50;
#include "sounds/VWBeetleKnock.h"

// Turbo — Gol comum é aspirado (0) ----------
volatile int turboVolumePercentage = 0;
volatile int turboIdleVolumePercentage = 0;
#include "sounds/TurboWhistle.h"

// Compressor — não ----------
volatile int chargerVolumePercentage = 0;
volatile int chargerIdleVolumePercentage = 10;
volatile int chargerStartPoint = 10;
#include "sounds/supercharger.h"

// Wastegate — não ----------
volatile int wastegateVolumePercentage = 0;
volatile int wastegateIdleVolumePercentage = 1;
#include "sounds/WastegateDummy.h"

// Ventoinha — leve (o boxer refrigerado a ar tem; AP a água quase nada) ----------
volatile int fanVolumePercentage = 8;
volatile int fanIdleVolumePercentage = 0;
volatile int fanStartPoint = 20;
#include "sounds/GenericFan.h"

// Buzina — buzina fininha de carro pequeno ----------
volatile int hornVolumePercentage = 110;
#include "sounds/CarHorn.h"

// Sirene — não ----------
volatile int sirenVolumePercentage = 100;
#include "sounds/sirenDummy.h"

// Freio de ar — não ----------
volatile int brakeVolumePercentage = 0;
#include "sounds/AirBrakeDummy.h"

// Freio de estacionamento — não ----------
volatile int parkingBrakeVolumePercentage = 0;
#include "sounds/ParkingBrakeDummy.h"

// Som de troca de marcha — não (pneumático) ----------
volatile int shiftingVolumePercentage = 100;
#include "sounds/AirShiftingDummy.h"

// Sound1 = porta ----------
volatile int sound1VolumePercentage = 100;
#include "sounds/door.h"

// Beep de ré — carro não tem ----------
volatile int reversingVolumePercentage = 0;
#include "sounds/TruckReversingBeep.h"

// Seta "tic-tac" ----------
volatile int indicatorVolumePercentage = 100;
const uint16_t indicatorOn = 300;
const boolean INDICATOR_DIR = true;
#include "sounds/Indicator.h"

// Luzes ----------
//#define XENON_LIGHTS
const boolean doubleFlashBlueLight = true;

// Aceleração & frenagem (carro leve; TUNE_* pode ser sobrescrito por um profile / tuning set) ---------
#ifndef TUNE_ESC_RAMP_1ST
#define TUNE_ESC_RAMP_1ST 12
#endif
#ifndef TUNE_ESC_RAMP_2ND
#define TUNE_ESC_RAMP_2ND 30
#endif
#ifndef TUNE_ESC_RAMP_3RD
#define TUNE_ESC_RAMP_3RD 50
#endif
#ifndef TUNE_ESC_BRAKE_STEPS
#define TUNE_ESC_BRAKE_STEPS 25
#endif
#ifndef TUNE_ESC_ACCEL_STEPS
#define TUNE_ESC_ACCEL_STEPS 4
#endif
const uint8_t escRampTimeFirstGear = TUNE_ESC_RAMP_1ST;
const uint8_t escRampTimeSecondGear = TUNE_ESC_RAMP_2ND;
const uint8_t escRampTimeThirdGear = TUNE_ESC_RAMP_3RD;
const uint8_t escBrakeSteps = TUNE_ESC_BRAKE_STEPS;
const uint8_t escAccelerationSteps = TUNE_ESC_ACCEL_STEPS;

// Câmbio — manual (com VIRTUAL_3_SPEED de 4_Transmission.h; troca por R1/L1 no controle) ----------
const boolean automatic = false;
#define NumberOfAutomaticGears 3
const boolean doubleClutch = false;
const boolean shiftingAutoThrottle = true;

// Embreagem — pega cedo, tipo carro ----------
uint16_t clutchEngagingPoint = 90;

// Motor — carro leve gira alto ----------
uint32_t MAX_RPM_PERCENTAGE = 320; // (é limitado a maxIbusRpmPercentage=320 nos modos BUS/Bluetooth)

// Simulação de massa do motor — leve e responsivo ----------
#ifndef TUNE_ENGINE_ACC
#define TUNE_ENGINE_ACC 5
#endif
#ifndef TUNE_ENGINE_DEC
#define TUNE_ENGINE_DEC 3
#endif
const int8_t acc = TUNE_ENGINE_ACC;
const int8_t dec = TUNE_ENGINE_DEC;

// Tipo de veículo — carro normal ----------
// #define TRACKED_MODE
