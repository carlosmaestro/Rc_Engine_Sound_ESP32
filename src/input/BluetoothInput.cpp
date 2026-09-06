//
// Bluetooth gamepad input -> virtual RC receiver.
// See BluetoothInput.h and ../BluetoothMapping.h.
//

#include <Bluepad32.h>
#include "BluetoothInput.h"
#include "../main.h"            // processRawChannels(), failsafeRcSignals()
#include "../BluetoothMapping.h"
#include "../8_Sound.h"         // numberOfVolumeSteps, masterVolumePercentage[] (const, internal linkage - own copy here)

// --- Globals owned by main.cpp that we feed / read ---
extern uint16_t pulseWidthRaw[]; // [1..13] = channel pulse width in microseconds
extern volatile bool failSafe;
// Experience controls (Fase 4b): driven directly, not via synthesized channels
extern uint8_t volumeIndex;                // index into masterVolumePercentage[]
extern volatile int16_t masterVolume;      // applied value (%), also re-applied by rcTriggerRead() when stopped
extern int8_t lightsState;                 // light-stage state machine (0..5), read by led()
extern volatile bool headLightsHighBeamOn; // high/low beam, read by led()

// --- Bluepad32 D-pad bit values (from uni_gamepad.h: UP=0, DOWN=1, RIGHT=2, LEFT=3) ---
static const uint8_t BT_DPAD_UP = 0x01;
static const uint8_t BT_DPAD_DOWN = 0x02;
static const uint8_t BT_DPAD_RIGHT = 0x04;
static const uint8_t BT_DPAD_LEFT = 0x08;

// --- State ---
static ControllerPtr s_pads[BP32_MAX_GAMEPADS];
static bool s_ready = false;             // first gamepad data received
static uint32_t s_lastDataMs = 0;        // for failsafe timeout
static uint32_t s_lastUpdateMs = 0;      // for BP32.update() rate limiting

// Internal control state (momentary buttons -> latched / switch-like channels)
static uint8_t s_gear = BT_GEAR_MIN;
static bool s_prevR1 = false;
static bool s_prevL1 = false;
static uint8_t s_prevDpad = 0;         // D-pad bitmask last frame (rising-edge detection)
static uint32_t s_psHeldSinceMs = 0;   // millis() when PS was first seen held, 0 = not held

// --- Bluepad32 callbacks ---
static void onConnected(ControllerPtr ctl) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (s_pads[i] == nullptr) {
            s_pads[i] = ctl;
            Serial.printf("Bluetooth: controller connected, slot %d (%s)\n", i, ctl->getModelName().c_str());
            return;
        }
    }
    Serial.println("Bluetooth: controller connected, but no free slot");
}

static void onDisconnected(ControllerPtr ctl) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (s_pads[i] == ctl) {
            s_pads[i] = nullptr;
            Serial.printf("Bluetooth: controller disconnected, slot %d\n", i);
            return;
        }
    }
}

// --- Helpers ---
static int16_t applyDeadzone(int32_t v, int16_t dz) {
    return (v > -dz && v < dz) ? 0 : (int16_t) v;
}

// Map a bipolar value [-range..+range] to [center-span .. center+span], clamped.
static uint16_t toPulse(int32_t value, int32_t range, uint16_t center, uint16_t span) {
    long us = (long) center + (long) value * (long) span / (range == 0 ? 1 : range);
    if (us < center - span) us = center - span;
    if (us > center + span) us = center + span;
    return (uint16_t) us;
}

// Return the first connected gamepad that has fresh data, or nullptr.
static ControllerPtr activePad() {
    for (auto p : s_pads) {
        if (p && p->isConnected() && p->isGamepad() && p->hasData())
            return p;
    }
    return nullptr;
}

// --- Public API ---
void setupBluetoothInput() {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) s_pads[i] = nullptr;

    Serial.printf("Bluetooth: Bluepad32 firmware %s\n", BP32.firmwareVersion());
    const uint8_t *addr = BP32.localBdAddress();
    Serial.printf("Bluetooth: BD address %02X:%02X:%02X:%02X:%02X:%02X\n",
                  addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

    BP32.setup(&onConnected, &onDisconnected);
    BP32.enableVirtualDevice(false);
    // NOTE: do NOT call BP32.forgetBluetoothKeys() on every boot - only on an explicit re-pair gesture (phase 4b).
}

bool bluetoothInputReady() {
    return s_ready;
}

void readBluetoothCommands() {
    const uint32_t now = millis();
    if (now - s_lastUpdateMs < BT_UPDATE_INTERVAL_MS) return;
    s_lastUpdateMs = now;

    BP32.update();
    ControllerPtr ctl = activePad();

    if (ctl) {
        s_lastDataMs = now;
        s_ready = true;

        // CH1 - steering (left stick X) ------------------------------------------------
        int16_t sx = applyDeadzone(ctl->axisX(), BT_AXIS_DEADZONE);
        pulseWidthRaw[1] = toPulse(sx, BT_AXIS_MAX, BT_PULSE_CENTER, BT_PULSE_SPAN);

        // CH2 - gearbox (R1 = up, L1 = down; 3 positions) -----------------------------
        bool r1 = ctl->r1();
        bool l1 = ctl->l1();
        if (r1 && !s_prevR1 && s_gear < BT_GEAR_MAX) s_gear++;
        if (l1 && !s_prevL1 && s_gear > BT_GEAR_MIN) s_gear--;
        s_prevR1 = r1;
        s_prevL1 = l1;
        pulseWidthRaw[2] = BT_GEAR_US[s_gear];

        // CH3 - throttle: R2 forward, L2 brake/reverse -------------------------------
        int32_t fwd = ctl->throttle(); // 0..1023
        int32_t rev = ctl->brake();    // 0..1023
        if (fwd < BT_TRIGGER_DEADZONE) fwd = 0;
        if (rev < BT_TRIGGER_DEADZONE) rev = 0;
        int32_t net = fwd - rev; // -1023..1023
        pulseWidthRaw[3] = toPulse(net, BT_TRIGGER_MAX, BT_PULSE_CENTER, BT_PULSE_SPAN);

        // CH4 - horn (Square) -------------------------------------------------------
        pulseWidthRaw[4] = ctl->x() ? (BT_PULSE_CENTER + BT_PULSE_SPAN) : BT_PULSE_CENTER;

        // CH10 - engine on/off (Cross) -> handled by momentary1Trigger.toggleLong() ---
        pulseWidthRaw[10] = ctl->a() ? (BT_PULSE_CENTER + BT_PULSE_SPAN) : BT_PULSE_CENTER;

        // --- Experience controls (Fase 4b): D-pad + PS, applied directly to firmware globals ---
        uint8_t dpad = ctl->dpad();
        const bool upEdge    = (dpad & BT_DPAD_UP)    && !(s_prevDpad & BT_DPAD_UP);
        const bool downEdge  = (dpad & BT_DPAD_DOWN)  && !(s_prevDpad & BT_DPAD_DOWN);
        const bool rightEdge = (dpad & BT_DPAD_RIGHT) && !(s_prevDpad & BT_DPAD_RIGHT);
        const bool leftEdge  = (dpad & BT_DPAD_LEFT)  && !(s_prevDpad & BT_DPAD_LEFT);
        s_prevDpad = dpad;

        // Volume + / - (no wrap). masterVolume applied now; rcTriggerRead() re-applies it when stopped.
        if (upEdge && volumeIndex + 1 < numberOfVolumeSteps) {
            volumeIndex++;
            masterVolume = masterVolumePercentage[volumeIndex];
            Serial.printf("Bluetooth: volume step %u = %d%%\n", volumeIndex, masterVolume);
        }
        if (downEdge && volumeIndex > 0) {
            volumeIndex--;
            masterVolume = masterVolumePercentage[volumeIndex];
            Serial.printf("Bluetooth: volume step %u = %d%%\n", volumeIndex, masterVolume);
        }
        // Light stage cycle (0..5) / high-low beam toggle
        if (rightEdge) {
            lightsState = (lightsState >= 5) ? 0 : (int8_t) (lightsState + 1);
            Serial.printf("Bluetooth: light stage %d\n", lightsState);
        }
        if (leftEdge) {
            headLightsHighBeamOn = !headLightsHighBeamOn;
            Serial.printf("Bluetooth: high beam %s\n", headLightsHighBeamOn ? "on" : "off");
        }

        // Hold PS ~2s -> forget BT pairings + reboot (re-pair gesture)
        if (ctl->miscSystem()) {
            if (s_psHeldSinceMs == 0) {
                s_psHeldSinceMs = now;
            } else if (now - s_psHeldSinceMs > BT_REPAIR_HOLD_MS) {
                Serial.println("Bluetooth: PS held -> forgetting keys, rebooting to re-pair...");
                BP32.forgetBluetoothKeys();
                delay(200);
                ESP.restart();
            }
        } else {
            s_psHeldSinceMs = 0;
        }

        // Channels not mapped yet -> neutral (phase 4c) ---------------------------------
        pulseWidthRaw[5] = BT_PULSE_CENTER;
        pulseWidthRaw[6] = BT_PULSE_CENTER;
        pulseWidthRaw[7] = BT_PULSE_CENTER;
        pulseWidthRaw[8] = BT_PULSE_CENTER;
        pulseWidthRaw[9] = BT_PULSE_CENTER;
        pulseWidthRaw[11] = BT_PULSE_CENTER;
        pulseWidthRaw[12] = BT_PULSE_CENTER;
        pulseWidthRaw[13] = BT_PULSE_CENTER;
    }

    // Failsafe: no fresh gamepad data within the timeout -> signal loss
    failSafe = (now - s_lastDataMs > BT_FAILSAFE_TIMEOUT_MS);

    if (s_ready) {
        processRawChannels();  // normalize / auto-zero / reverse -> pulseWidth[]
        failsafeRcSignals();   // forces pulseWidth[] to neutral while failSafe
    }
}
