//
// Build profile: variante Wemos D1 Mini ESP32 (ex.: controlador de reboque).
// Farois no GPIO22, sem luz de cabine, DEBUG_RX no GPIO3.
//
// pio run -e wemos_d1_mini
//

// -- Veiculo / conjunto de sons --
#define PROFILE_VEHICLE "vehicles/VolvoL120H.h"

// -- Perfil de radio --
#define FLYSKY_FS_I6S_LOADER

// -- Modo de comunicacao --
#define IBUS_COMMUNICATION

// -- Placa: ativa os ramos #ifdef WEMOS_D1_MINI_ESP32 ja existentes em main.cpp --
#define WEMOS_D1_MINI_ESP32
