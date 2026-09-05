//
// Bluetooth gamepad input as a "virtual receiver" for the RC Engine Sound firmware.
// Active only when "#define BLUETOOTH_COMMUNICATION" is set in "2_Remote.h".
//

#ifndef RC_ENGINE_SOUND_ESP32_BLUETOOTHINPUT_H
#define RC_ENGINE_SOUND_ESP32_BLUETOOTHINPUT_H

// Register Bluepad32 callbacks. Call once from setup(), before the init wait loop.
void setupBluetoothInput();

// Poll the gamepad, synthesize pulseWidthRaw[1..13] (microseconds), set failSafe,
// then run processRawChannels() + failsafeRcSignals(). Call every loop() iteration.
void readBluetoothCommands();

// true once at least one gamepad has delivered data (used by the setup() init wait loop,
// mirrors sbusInit / ibusInit).
bool bluetoothInputReady();

#endif // RC_ENGINE_SOUND_ESP32_BLUETOOTHINPUT_H
