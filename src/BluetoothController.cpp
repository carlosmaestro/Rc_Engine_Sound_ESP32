//
// Created by Carlos on 11/01/2026.
//

#include "BluetoothController.h"
#include "main.h"

/***************************/

ControllerPtr myControllers[BP32_MAX_GAMEPADS];
uint32_t loopBP32Millis = 0;

//THROTTLE
boolean Re = 0;
boolean Frente = true;
boolean freio = 0;
int AuxjoyDireitaY;
boolean manualMode = false;
int16_t BLT_CURRENT_THROTTLE;
uint32_t millisMeio = 0;
//

// This callback gets called any time a new gamepad is connected.
// Up to 4 gamepads can be connected at the same time.
void onConnectedController(ControllerPtr ctl) {
    bool foundEmptySlot = false;
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == nullptr) {
            Serial.printf("CALLBACK: Controller is connected, index=%d\n", i);
            // Additionally, you can get certain gamepad properties like:
            // Model, VID, PID, BTAddr, flags, etc.
            ControllerProperties properties = ctl->getProperties();
            Serial.printf("Controller model: %s, VID=0x%04x, PID=0x%04x\n", ctl->getModelName().c_str(),
                          properties.vendor_id,
                          properties.product_id);
            myControllers[i] = ctl;
            foundEmptySlot = true;
            break;
        }
    }
    if (!foundEmptySlot) {
        Serial.println("CALLBACK: Controller connected, but could not found empty slot");
    }
}

void onDisconnectedController(ControllerPtr ctl) {
    bool foundController = false;

    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == ctl) {
            Serial.printf("CALLBACK: Controller disconnected from index=%d\n", i);
            myControllers[i] = nullptr;
            foundController = true;
            break;
        }
    }

    if (!foundController) {
        Serial.println("CALLBACK: Controller disconnected, but not found in myControllers");
    }
}

void dumpGamepad(ControllerPtr ctl) {
    Serial.printf(
        "idx=%d, dpad: 0x%02x, buttons: 0x%04x, axis L: %4d, %4d, axis R: %4d, %4d, brake: %4d, throttle: %4d, "
        "misc: 0x%02x, gyro x:%6d y:%6d z:%6d, accel x:%6d y:%6d z:%6d\n",
        ctl->index(), // Controller Index
        ctl->dpad(), // D-pad
        ctl->buttons(), // bitmask of pressed buttons
        ctl->axisX(), // (-511 - 512) left X Axis
        ctl->axisY(), // (-511 - 512) left Y axis
        ctl->axisRX(), // (-511 - 512) right X axis
        ctl->axisRY(), // (-511 - 512) right Y axis
        ctl->brake(), // (0 - 1023): brake button
        ctl->throttle(), // (0 - 1023): throttle (AKA gas) button
        ctl->miscButtons(), // bitmask of pressed "misc" buttons
        ctl->gyroX(), // Gyro X
        ctl->gyroY(), // Gyro Y
        ctl->gyroZ(), // Gyro Z
        ctl->accelX(), // Accelerometer X
        ctl->accelY(), // Accelerometer Y
        ctl->accelZ() // Accelerometer Z
    );
}

void processGamepad(ControllerPtr ctl) {
    // There are different ways to query whether a button is pressed.
    // By query each button individually:
    //  a(), b(), x(), y(), l1(), etc...

    processThrottle(ctl);

    if (ctl->a()) {
        engineOnOff();
        static int colorIdx = 0;
        // Some gamepads like DS4 and DualSense support changing the color LED.
        // It is possible to change it by calling:
        switch (colorIdx % 3) {
            case 0:
                // Red
                ctl->setColorLED(255, 0, 0);
                break;
            case 1:
                // Green
                ctl->setColorLED(0, 255, 0);
                break;
            case 2:
                // Blue
                ctl->setColorLED(0, 0, 255);
                break;
        }
        colorIdx++;
        // triggerHorn();
    }

    if (ctl->b()) {
        // Turn on the 4 LED. Each bit represents one LED.
        static int led = 0;
        led++;
        // Some gamepads like the DS3, DualSense, Nintendo Wii, Nintendo Switch
        // support changing the "Player LEDs": those 4 LEDs that usually indicate
        // the "gamepad seat".
        // It is possible to change them by calling:
        ctl->setPlayerLEDs(led & 0x0f);
    }

    if (ctl->x()) {
        // Some gamepads like DS3, DS4, DualSense, Switch, Xbox One S, Stadia support rumble.
        // It is possible to set it by calling:
        // Some controllers have two motors: "strong motor", "weak motor".
        // It is possible to control them independently.
        ctl->playDualRumble(0 /* delayedStartMs */, 250 /* durationMs */, 0x80 /* weakMagnitude */,
                            0x40 /* strongMagnitude */);
        triggerHorn();
    }

    // Another way to query controller data is by getting the buttons() function.
    // See how the different "dump*" functions dump the Controller info.
    // dumpGamepad(ctl);
}

void processControllers() {
    for (auto myController: myControllers) {
        if (myController && myController->isConnected() && myController->hasData()) {
            if (myController->isGamepad()) {
                processGamepad(myController);
            } else {
                Serial.println("Unsupported controller");
            }
        }
    }
}

void setupBluetoothController() {
    /*** *********************** ***/
    Serial.printf("Firmware: %s\n", BP32.firmwareVersion());
    const uint8_t *addr = BP32.localBdAddress();
    Serial.printf("BD Addr: %2X:%2X:%2X:%2X:%2X:%2X\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

    // Setup the Bluepad32 callbacks
    BP32.setup(&onConnectedController, &onDisconnectedController);

    // "forgetBluetoothKeys()" should be called when the user performs
    // a "device factory reset", or similar.
    // Calling "forgetBluetoothKeys" in setup() just as an example.
    // Forgetting Bluetooth keys prevents "paired" gamepads to reconnect.
    // But it might also fix some connection / re-connection issues.
    BP32.forgetBluetoothKeys();

    // Enables mouse / touchpad support for gamepads that support them.
    // When enabled, controllers like DualSense and DualShock4 generate two connected devices:
    // - First one: the gamepad
    // - Second one, which is a "virtual device", is a mouse.
    // By default, it is disabled.
    BP32.enableVirtualDevice(false);
    /*** *********************** ***/
}

void loopBluetoothController() {
    if (millis() - loopBP32Millis > 50) {
        loopBP32Millis = millis();
        bool dataUpdated = BP32.update();
        if (dataUpdated)
            processControllers();
    }
}

void processThrottle(ControllerPtr ctl) {
    // ACELERAÇÃO FRENTE RÉ E FREIO

    AuxjoyDireitaY = ctl->axisRY();

    // ACELERACAO PELO R-TRIGGER (R2)
    //  acceleration = ctl->throttle();     // (0 - 1023): throttle (AKA gas) button
    if (manualMode) {
        BLT_CURRENT_THROTTLE = map(ctl->throttle(), 0, 1020, 0, 500);
        // Serial.print("PWM: ");
        // Serial.println(PWM);
    }

    // *** FRENTE
    if (AuxjoyDireitaY <= -10) {
        // TRATANDO PONTE H

        //   PWM = map(AuxjoyDireitaY, 4, -508, 0, 255);
        if (!manualMode) {
            BLT_CURRENT_THROTTLE = map(AuxjoyDireitaY, 4, -508, 0, 500);
        }
        Serial.print("PWM: ");
        Serial.println(BLT_CURRENT_THROTTLE);

        if (freio == false) {
            // Para verificar se deve ser acionado Motor para frente ou Freiar

            ledcWrite(10, 0);
            ledcWrite(11, BLT_CURRENT_THROTTLE);
            // ledcWrite(11, map(AuxjoyDireitaY, 120, 25, 0, 255));
            // Serial.println(" FRENTE: ");

            digitalWrite(22, LOW); // APAGA LUZ DE RÉ
            // if (Farol == 0) {
            //   ledcWrite(13, 0);  // Apaga luz de freio
            // } else {
            //   ledcWrite(13, 100);  // Diminui a intensidade da luz para Lanterna
            // }
            millisMeio = millis();
            Re = false; // jOYSTICK PARA CIMA
            Frente = true;
        }

        // if ((freio == true) && (Re == true) && (AtivaFreio == true)) {  // FREIAR
        //
        //   Serial.println("FREIO ACIONADO");
        //   ledcWrite(10, 255);  // FREIA MOTOR PONTEH
        //   ledcWrite(11, 255);  // FREIA MOTOR PONTEH
        //   ledcWrite(13, 255);  // Acende luz de freio
        // } else {
        //   freio = false;
        // }
    }

    // *** MEIO

    if ((AuxjoyDireitaY > -10) && (AuxjoyDireitaY < 10)) {
        // TRATANDO PONTE H
        // ledcWrite(10, LOW);
        // ledcWrite(11, LOW);

        // digitalWrite(22, LOW);  // APAGA LUZ DE RÉ
        // if (Farol == 0) {
        //   ledcWrite(13, 0);  // Apaga luz de freio
        // } else {
        //   ledcWrite(13, 100);  // Diminui a intensidade da luz para Lanterna
        // }

        if (millis() - millisMeio < 600) {
            Serial.println("Sem Aceleracao");
            freio = true;
            // ledcWrite(14, 0);    // Para som no gpio 25
            // ledcWrite(13, 255);  // Diminui a intensidade da luz para Lanterna
        } else {
            // if (Farol == 0) {
            //   ledcWrite(13, 0);  // Apaga luz de freio
            // } else {
            //   ledcWrite(13, 100);  // Diminui a intensidade da luz para Lanterna
            // }
            freio = false;
            Re = false;
            Frente = false;
        }
    }

    // ***RÉ

    if (AuxjoyDireitaY >= 10) {
        // TRATANDO PONTE H

        //   PWM = map(AuxjoyDireitaY, 5, 512, 0, 255);
        if (!manualMode) {
            BLT_CURRENT_THROTTLE = map(AuxjoyDireitaY, 5, 512, 0, 255);
        }

        if (freio == false) {
            // Para verificar se deve ser acionado Motor para trás ou Freiar
            // ledcWrite(10, map(AuxjoyDireitaY, 135, 255, 25, 255));
            ledcWrite(10, BLT_CURRENT_THROTTLE);
            ledcWrite(11, LOW);
            Serial.println(" RE ");

            // if (Farol == 0) {
            //   ledcWrite(13, 0);  // Apaga luz de freio
            // } else {
            //   ledcWrite(13, 100);  // Diminui a intensidade da luz para Lanterna
            // }
            // digitalWrite(22, HIGH);  // ACENDE LUZ DE RÉ
            millisMeio = millis();
            Re = true; // RÉ JOYSTICK PARA BAIXO
            Frente = false;
        }

        // if ((freio == true) && (Frente == true) && (AtivaFreio == true)) {  // FREIAR
        //   Serial.println("FREIO ACIONADO");
        //   ledcWrite(10, 255);
        //   ledcWrite(11, 255);
        //   ledcWrite(13, 255);  // Acende luz de freio
        // } else {
        //   freio = false;
        // }
    }
    Serial.println(BLT_CURRENT_THROTTLE);
    Serial.print("PWM: ");
}

 int16_t getBLTCurrentThrottle() {
    return BLT_CURRENT_THROTTLE;
}
