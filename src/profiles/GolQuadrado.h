//
// Build profile: mesma placa física do carlos_bt_car, mas com som de carro leve
// (VW Gol "quadrado" — 4 cilindros a gasolina). Câmbio manual (R1/L1 no controle).
//
// pio run -e gol_quadrado
//

// -- Placa fisica (pinos + toggles) --
#include "_carlosBoard.h"

// -- Veiculo / conjunto de sons --
#define PROFILE_VEHICLE "vehicles/GolQuadrado.h"

// -- Perfil de radio (fornece channelReversed[]/channelAutoZero[]/pulseSpan) --
#define FLYSKY_FS_I6S_LOADER

// -- Modo de comunicacao: gamepad Bluetooth (receptor virtual) --
#define BLUETOOTH_COMMUNICATION

// -- Tuning: carrinho agil. Deixe comentado para usar a dinamica "de carro" do GolQuadrado.h.
//    Descomente para uma resposta ainda mais direta (bom para bancada / manobra lenta).
// #include "../tuning/agileCar.h"
