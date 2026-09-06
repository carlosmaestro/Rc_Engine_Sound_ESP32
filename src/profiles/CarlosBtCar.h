//
// Build profile: carro Bluetooth do usuario (derivado de referencia/Controller.ino).
// Som/veiculo: Volvo L120H (carregadeira, LOADER_MODE).
//
// pio run -e carlos_bt_car
//

// -- Placa fisica (pinos + toggles) --
#include "_carlosBoard.h"

// -- Veiculo / conjunto de sons --
#define PROFILE_VEHICLE "vehicles/VolvoL120H.h"

// -- Perfil de radio (fornece channelReversed[]/channelAutoZero[]/pulseSpan) --
#define FLYSKY_FS_I6S_LOADER

// -- Modo de comunicacao: gamepad Bluetooth (receptor virtual) --
#define BLUETOOTH_COMMUNICATION

// -- Tuning: carrinho agil (resposta de acelerador direta) --
#include "../tuning/agileCar.h"
