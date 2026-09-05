//
// Tuning set "carrinho agil" — resposta de acelerador direta para carros pequenos.
//
// O preset Volvo L120H (e a maioria dos presets) simula a inercia de um veiculo
// pesado: o esc() so avanca a maquina de estados a cada escRampTimeSecondGear ms
// (50) e o motor ganha poucos passos por vez -> ~1 s do neutro a potencia total.
// Este arquivo sobrescreve os knobs de "sensacao" para uma resposta quase direta.
//
// Uso: um profile em src/profiles/*.h faz  #include "tuning/agileCar.h"  ANTES de
// os headers 1_Vehicle.h / 3_ESC.h serem lidos. Cada TUNE_* e guardado com #ifndef,
// entao o profile pode sobrescrever um knob individual antes deste #include.
//
// NOTA: TUNE_ESC_TAKEOFF_PUNCH e TUNE_GLOBAL_ACCEL_PCT alimentam variaveis que sao
// persistidas na EEPROM (3_ESC.h). O valor abaixo so vale numa EEPROM "fresca" ->
// bumpe "eeprom_id" em 0_generalSettings.h ao mudar este arquivo.
//

// -- vehicles/VolvoL120H.h (const, efeito imediato) --
#ifndef TUNE_ESC_RAMP_2ND
#define TUNE_ESC_RAMP_2ND 18       // era 50 — ms entre passos da maquina de estados do ESC (automatico usa sempre a 2a)
#endif
#ifndef TUNE_ESC_ACCEL_STEPS
#define TUNE_ESC_ACCEL_STEPS 12    // era 5  — quanto o ESC acelera por passo
#endif
#ifndef TUNE_ESC_BRAKE_STEPS
#define TUNE_ESC_BRAKE_STEPS 35    // era 100 — quanto o ESC freia por passo
#endif
#ifndef TUNE_ENGINE_ACC
#define TUNE_ENGINE_ACC 12         // era 6  — spool-up do som do motor
#endif
#ifndef TUNE_ENGINE_DEC
#define TUNE_ENGINE_DEC 6          // era 3  — desaceleracao do som do motor
#endif

// -- 3_ESC.h (EEPROM-backed: exige bump de eeprom_id) --
#ifndef TUNE_ESC_TAKEOFF_PUNCH
#define TUNE_ESC_TAKEOFF_PUNCH 60  // era 0  — chute inicial p/ vencer o atrito estatico do motor / queda da ponte-H
#endif
#ifndef TUNE_GLOBAL_ACCEL_PCT
#define TUNE_GLOBAL_ACCEL_PCT 130  // era 100 — divide escRampTime (maior = mais rapido)
#endif
