//
// Created by Carlos on 11/01/2026.
//

#ifndef RC_ENGINE_SOUND_ESP32_BLUETOOTHCONTROLLER_H
#define RC_ENGINE_SOUND_ESP32_BLUETOOTHCONTROLLER_H
#include <Bluepad32.h>

void setupBluetoothController();
void loopBluetoothController();
void processControllers();
void processThrottle(ControllerPtr ctl);

int16_t getBLTCurrentThrottle();

#endif //RC_ENGINE_SOUND_ESP32_BLUETOOTHCONTROLLER_H