//
// BUILD PROFILE DISPATCHER
//
// Um "build profile" e um arquivo em src/profiles/<Nome>.h que descreve TUDO de uma
// build para um carro fisico: veiculo/sons, modo de comunicacao, perfil de radio,
// toggles de placa, pinagem e tuning.
//
// O profile ativo e escolhido por um -D PROFILE_<NOME> no platformio.ini (um [env:*]
// por carro). Rode:  pio run -e l120h_radio   /   pio run -e carlos_bt_car
//
// Este arquivo e incluido por main.cpp como PRIMEIRO header de config (antes de
// 0_GeneralSettings.h), para poder influenciar 1_Vehicle.h e os toggles guardados em
// 2_Remote.h / 3_ESC.h / 6_Lights.h.
//
// Como adicionar um carro:
//   1. crie src/profiles/MeuCarro.h (copie de CarlosBtCar.h ou L120hRadio.h);
//   2. adicione um "#elif defined(PROFILE_MEU_CARRO)" abaixo;
//   3. adicione um "[env:meu_carro]" no platformio.ini com "-D PROFILE_MEU_CARRO".
//

#if defined(PROFILE_CARLOS_BT_CAR)
#include "CarlosBtCar.h"

#elif defined(PROFILE_L120H_RADIO)
#include "L120hRadio.h"

#elif defined(PROFILE_WEMOS_D1_MINI)
#include "WemosD1Mini.h"

#else
#error "Nenhum PROFILE_* definido. Compile com 'pio run -e <profile>' (ver [env:*] no platformio.ini)."
#endif
