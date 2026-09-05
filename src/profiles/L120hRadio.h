//
// Build profile: Volvo L120H com radio real (IBUS) na placa padrao de 30 pinos.
// = comportamento historico do firmware. Build byte-equivalente ao antigo STOCK+IBUS.
//
// pio run -e l120h_radio
//

// -- Veiculo / conjunto de sons --
#define PROFILE_VEHICLE "vehicles/VolvoL120H.h"

// -- Perfil de radio (2_Remote.h) --
#define FLYSKY_FS_I6S_LOADER

// -- Modo de comunicacao --
#define IBUS_COMMUNICATION

// -- Pinos: nenhum override -> defaults de main.cpp (placa 30 pinos) --
// -- Toggles: nenhum override -> NEOPIXEL_ENABLED, THIRD_BRAKELIGHT, BATTERY_PROTECTION ligados (default) --
// -- Tuning: nenhum override -> valores originais do vehicle file / 3_ESC.h --
