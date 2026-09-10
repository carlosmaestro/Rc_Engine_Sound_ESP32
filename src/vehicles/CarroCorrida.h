#include <Arduino.h>

// Vehicle preset: carro de corrida (V12), a partir do banco de sons "LaFerrari" da
// biblioteca (sounds/LaFerrari*.h). O preset original da biblioteca usa "doubleClutch"
// (troca semi-automatica simulada por "blip" de RPM) — nesse modo o firmware NUNCA toca
// a amostra de troca de marcha (ver main.cpp: "shiftingTrigger && !automatic && !doubleClutch").
// Aqui a caixa foi trocada para manual "virtual" (R1/L1 no controle, como no GolQuadrado),
// o que ativa de fato o som de troca de marcha (ClunkingGearShifting, volume ajustado) e
// deixa a resposta de acelerador mais direta/"punchy", como um carro de corrida.

// Sound files (22'050 Hz, 8 bit PCM) --------------------------------------------------------------------
// Partida --------
volatile int startVolumePercentage = 150;
#include "sounds/LaFerrariStart.h" // Ferrari LaFerrari, V12

// Ralenti --------
volatile int idleVolumePercentage = 80;
volatile int engineIdleVolumePercentage = 60;
volatile int fullThrottleVolumePercentage = 130;
#include "sounds/LaFerrariIdle.h"

// Sub som de aceleração (rev) — motor de giro alto e limpo ----------
#define REV_SOUND
volatile int revVolumePercentage = 120;
volatile int engineRevVolumePercentage = 60;
volatile const uint16_t revSwitchPoint = 50;   // acima disto toca o rev no lugar do idle
volatile const uint16_t idleEndPoint = 300;    // acima disto = 100% rev, 0% idle
volatile const uint16_t idleVolumeProportionPercentage = 100;
#ifdef REV_SOUND
#include "sounds/LaFerrariRev.h"
#endif

// Jake brake — carro não tem ----------
//#define JAKE_BRAKE_SOUND
volatile int jakeBrakeVolumePercentage = 0;
volatile int jakeBrakeIdleVolumePercentage = 0;
volatile int jakeBrakeMinRpm = 200;

// "Knock" = textura das explosões do V12 ----------
volatile int dieselKnockVolumePercentage = 600;
volatile int dieselKnockIdleVolumePercentage = 0;
volatile int dieselKnockStartPoint = 10;
volatile int dieselKnockInterval = 12;
#define V8 // mantido do preset original da biblioteca: pulsos 4 e 8 mais altos (padrão de escape)
volatile int dieselKnockAdaptiveVolumePercentage = 50;
#define RPM_DEPENDENT_KNOCK
#ifdef RPM_DEPENDENT_KNOCK
uint8_t minKnockVolumePercentage = 5;
uint16_t knockStartRpm = 400;
#endif
#include "sounds/LaFerrariKnock.h"

// Turbo — V12 aspirado (0) ----------
volatile int turboVolumePercentage = 0;
volatile int turboIdleVolumePercentage = 0;
#include "sounds/TurboWhistle.h"

// Compressor — não ----------
volatile int chargerVolumePercentage = 0;
volatile int chargerIdleVolumePercentage = 10;
volatile int chargerStartPoint = 10;
#include "sounds/supercharger.h"

// Wastegate — dá um "puf" ao soltar o acelerador em marcha alta (efeito, mesmo sem turbo) ----------
volatile int wastegateVolumePercentage = 90;
volatile int wastegateIdleVolumePercentage = 1;
#include "sounds/WastegateDummy.h"

// Ventoinha — não ----------
volatile int fanVolumePercentage = 0;
volatile int fanIdleVolumePercentage = 0;
volatile int fanStartPoint = 0;
#include "sounds/GenericFan.h"

// Buzina ----------
volatile int hornVolumePercentage = 100;
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

// Som de troca de marcha — "clunk" mecânico, audível porque a caixa é manual (ver acima) ----------
volatile int shiftingVolumePercentage = 140; // um pouco mais forte que o default (100), pra ficar nítido
#include "sounds/ClunkingGearShifting.h"

// Sound1 = porta ----------
volatile int sound1VolumePercentage = 100;
#include "sounds/door.h"

// Beep de ré — carro de corrida não tem ----------
volatile int reversingVolumePercentage = 0;
#include "sounds/TruckReversingBeep.h"

// Seta "tic-tac" ----------
volatile int indicatorVolumePercentage = 100;
const uint16_t indicatorOn = 300;
const boolean INDICATOR_DIR = true;
#include "sounds/Indicator.h"

// Cantada de pneu — combina bem com carro de corrida ----------
#define TIRE_SQUEAL
volatile int tireSquealVolumePercentage = 160;
#include "sounds/TireSqueal2.h"

// Luzes ----------
#define XENON_LIGHTS
const boolean doubleFlashBlueLight = true;

// Aceleração & frenagem — resposta direta/"punchy", como um carro de corrida ----------
// (TUNE_* pode ser sobrescrito por um profile / tuning set, ver docs/10-profiles-de-build.md)
#ifndef TUNE_ESC_RAMP_1ST
#define TUNE_ESC_RAMP_1ST 15
#endif
#ifndef TUNE_ESC_RAMP_2ND
#define TUNE_ESC_RAMP_2ND 35
#endif
#ifndef TUNE_ESC_RAMP_3RD
#define TUNE_ESC_RAMP_3RD 55
#endif
#ifndef TUNE_ESC_BRAKE_STEPS
#define TUNE_ESC_BRAKE_STEPS 40
#endif
#ifndef TUNE_ESC_ACCEL_STEPS
#define TUNE_ESC_ACCEL_STEPS 5
#endif
const uint8_t escRampTimeFirstGear = TUNE_ESC_RAMP_1ST;
const uint8_t escRampTimeSecondGear = TUNE_ESC_RAMP_2ND;
const uint8_t escRampTimeThirdGear = TUNE_ESC_RAMP_3RD;
const uint8_t escBrakeSteps = TUNE_ESC_BRAKE_STEPS;
const uint8_t escAccelerationSteps = TUNE_ESC_ACCEL_STEPS;

// Câmbio — manual "virtual" (VIRTUAL_3_SPEED de 4_Transmission.h; troca por R1/L1 no
// controle, igual ao GolQuadrado). Precisa ser "automatic=false" + "doubleClutch=false"
// para o som de troca de marcha acima realmente tocar. ----------
const boolean automatic = false;
#define NumberOfAutomaticGears 4
const boolean doubleClutch = false;
const boolean shiftingAutoThrottle = true; // ajuda a "blipar" o giro na troca, sincronizando

// Embreagem — pega cedo e firme, tipo carro de corrida ----------
uint16_t clutchEngagingPoint = 70;

// Motor — V12 de giro alto (limitado a maxIbusRpmPercentage=320 nos modos BUS/Bluetooth) ----------
uint32_t MAX_RPM_PERCENTAGE = 320;

// Simulação de massa do motor — leve e responsivo, gira e desacelera rápido ----------
#ifndef TUNE_ENGINE_ACC
#define TUNE_ENGINE_ACC 7
#endif
#ifndef TUNE_ENGINE_DEC
#define TUNE_ENGINE_DEC 4
#endif
const int8_t acc = TUNE_ENGINE_ACC;
const int8_t dec = TUNE_ENGINE_DEC;

// Tipo de veículo — carro normal ----------
// #define TRACKED_MODE
