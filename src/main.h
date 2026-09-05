//
// Created by Carlos on 11/01/2026.
//
#include <cstdint>

#ifndef RC_ENGINE_SOUND_ESP32_SRC_H
#define RC_ENGINE_SOUND_ESP32_SRC_H

// Forward declare functions
void Task1code(void *parameters);
void readSbusCommands();
void readIbusCommands();
void readSumdCommands();
void readPpmCommands();
void readPwmSignals();
void processRawChannels();
void failsafeRcSignals();
void channelZero();
float batteryVolts();
void eepromDebugRead();
void eepromRead();
void eepromInit();
void serialInterface();
void webInterface();

//***
void triggerHorn();
void engineOnOff();


#endif //RC_ENGINE_SOUND_ESP32_SRC_H
