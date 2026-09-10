//
// Build profile: mesma placa física do carlos_bt_car, mas com som de carro de corrida
// (V12, banco de sons LaFerrari da biblioteca). Câmbio manual (R1/L1 no controle), com
// som de troca de marcha ajustado/audível — ver vehicles/CarroCorrida.h.
//
// pio run -e carro_corrida
//

// -- Placa fisica (pinos + toggles) --
#include "_carlosBoard.h"

// -- Veiculo / conjunto de sons --
#define PROFILE_VEHICLE "vehicles/CarroCorrida.h"

// -- Perfil de radio (fornece channelReversed[]/channelAutoZero[]/pulseSpan) --
#define FLYSKY_FS_I6S_LOADER

// -- Direcao (CH1) invertida: chassi deste carro tem a servo/tirante da direcao
//    espelhado em relacao aos outros carros do usuario (CarlosBtCar/GolQuadrado) --
#define PROFILE_STEERING_REVERSED 1

// -- Modo de comunicacao: gamepad Bluetooth (receptor virtual) --
#define BLUETOOTH_COMMUNICATION

// -- Tuning: deixe comentado para usar a dinamica "de corrida" ja ajustada em
//    vehicles/CarroCorrida.h. Descomente para uma resposta ainda mais direta.
// #include "../tuning/agileCar.h"
