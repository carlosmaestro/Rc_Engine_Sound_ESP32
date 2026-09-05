#include <Bluepad32.h>
#include <ESP32Servo.h>  // Biblioteca com as funções de controle do Servo Motor no ESP32, fundamental para o funcionamento do código.

ControllerPtr myControllers[BP32_MAX_GAMEPADS];

// Instânciando  (Criando) um objeto do tipo Servo para controle do Servo Motor
Servo servoDirecao;  // Servo de direção

///////////////////////////////////////

// DEFININDO NOMES PARA OS GPIOs USADOS NESTE CÓDIGO
#define FAROL 15
#define FAROL_MILHA 02
#define LUZ_FREIO 04
#define SETA_DIREITA 16
#define SETA_ESQUERDA 17
#define LUZ_AUX01 19
#define LUZ_AUX02 21
#define LUZ_RE 22

#define SERVO_DIRECAO 13
#define PONTE_H_IN01 27
#define PONTE_H_IN02 32

#define BUZINA_PIN 25

// VARIAVEIS GLOBAIS
boolean Re = 0;
boolean Frente = true;
boolean freio = 0;
int AuxjoyDireitaY;
boolean manualMode = false;

int PWM;
int Farol = 0;
int contFarolBaixo = 0;
boolean AtivaFreio = 1;  // ATIVA E DESATIVA FREIO AO USAR PONTE H ( 1 ATIVA, 0 DESATIVA)
boolean PiscaAlertaOFF = 0;
boolean farolMilha = false;

//SETAS
int BotaoSeta_D = 0;
int BotaoSeta_E = 0;

boolean AuxPiscaAlerta = 0;
boolean AuxBuzina = 0;

// CONTROLA DELAY DE ACIONAMENTO DE BOTOES
boolean delayButtonYold = false;

// BOTAO BOLA
uint32_t delayButtonBOLA;
boolean actionButtonBOLA = false;

// BOTAO TRIANGULO
uint32_t delayButtonTRIANGULO;
boolean actionButtonTRIANGULO = false;

// BOTAO X
uint32_t delayButtonX;
boolean actionButtonX = false;

// BOTAO QUADRADO
uint32_t delayButtonQUADRADO;
boolean actionButtonQUADRADO = false;

// BOTAO L1
uint32_t delayButtonL1;
boolean actionButtonL1 = false;

// BOTAO R1
uint32_t delayButtonR1;
boolean actionButtonR1 = false;

// BOTAO L3
uint32_t delayButtonL3;
boolean actionButtonL3 = false;

// BOTAO R3
uint32_t delayButtonR3;
boolean actionButtonR3 = false;

// Variáveis globais para auxiliar na contagem de tempo em millis
uint32_t Millis_PiscaAlerta, Millis_SetaDireita, Millis_SetaEsquerda, Millis_Meio;

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
      Serial.printf("Controller model: %s, VID=0x%04x, PID=0x%04x\n", ctl->getModelName().c_str(), properties.vendor_id,
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

void dumpMouse(ControllerPtr ctl) {
  Serial.printf("idx=%d, buttons: 0x%04x, scrollWheel=0x%04x, delta X: %4d, delta Y: %4d\n",
                ctl->index(),        // Controller Index
                ctl->buttons(),      // bitmask of pressed buttons
                ctl->scrollWheel(),  // Scroll Wheel
                ctl->deltaX(),       // (-511 - 512) left X Axis
                ctl->deltaY()        // (-511 - 512) left Y axis
  );
}

void dumpKeyboard(ControllerPtr ctl) {
  static const char *key_names[] = {
    // clang-format off
        // To avoid having too much noise in this file, only a few keys are mapped to strings.
        // Starts with "A", which is offset 4.
        "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V",
        "W", "X", "Y", "Z", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
        // Special keys
        "Enter", "Escape", "Backspace", "Tab", "Spacebar", "Underscore", "Equal", "OpenBracket", "CloseBracket",
        "Backslash", "Tilde", "SemiColon", "Quote", "GraveAccent", "Comma", "Dot", "Slash", "CapsLock",
        // Function keys
        "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
        // Cursors and others
        "PrintScreen", "ScrollLock", "Pause", "Insert", "Home", "PageUp", "Delete", "End", "PageDown",
        "RightArrow", "LeftArrow", "DownArrow", "UpArrow",
    // clang-format on
  };
  static const char *modifier_names[] = {
    // clang-format off
        // From 0xe0 to 0xe7
        "Left Control", "Left Shift", "Left Alt", "Left Meta",
        "Right Control", "Right Shift", "Right Alt", "Right Meta",
    // clang-format on
  };
  Serial.printf("idx=%d, Pressed keys: ", ctl->index());
  for (int key = Keyboard_A; key <= Keyboard_UpArrow; key++) {
    if (ctl->isKeyPressed(static_cast<KeyboardKey>(key))) {
      const char *keyName = key_names[key - 4];
      Serial.printf("%s,", keyName);
    }
  }
  for (int key = Keyboard_LeftControl; key <= Keyboard_RightMeta; key++) {
    if (ctl->isKeyPressed(static_cast<KeyboardKey>(key))) {
      const char *keyName = modifier_names[key - 0xe0];
      Serial.printf("%s,", keyName);
    }
  }
  Console.printf("\n");
}

void dumpBalanceBoard(ControllerPtr ctl) {
  Serial.printf("idx=%d,  TL=%u, TR=%u, BL=%u, BR=%u, temperature=%d\n",
                ctl->index(),        // Controller Index
                ctl->topLeft(),      // top-left scale
                ctl->topRight(),     // top-right scale
                ctl->bottomLeft(),   // bottom-left scale
                ctl->bottomRight(),  // bottom-right scale
                ctl->temperature()   // temperature: used to adjust the scale value's precision
  );
}

void dumpGamepad(ControllerPtr ctl) {
  Serial.printf(
    "idx=%d, dpad: 0x%02x, buttons: 0x%04x, axis L: %4d, %4d, axis R: %4d, %4d, brake: %4d, throttle: %4d, "
    "misc: 0x%02x, gyro x:%6d y:%6d z:%6d, accel x:%6d y:%6d z:%6d\n",
    ctl->index(),        // Controller Index
    ctl->dpad(),         // D-pad
    ctl->buttons(),      // bitmask of pressed buttons
    ctl->axisX(),        // (-511 - 512) left X Axis
    ctl->axisY(),        // (-511 - 512) left Y axis
    ctl->axisRX(),       // (-511 - 512) right X axis
    ctl->axisRY(),       // (-511 - 512) right Y axis
    ctl->brake(),        // (0 - 1023): brake button
    ctl->throttle(),     // (0 - 1023): throttle (AKA gas) button
    ctl->miscButtons(),  // bitmask of pressed "misc" buttons
    ctl->gyroX(),        // Gyro X
    ctl->gyroY(),        // Gyro Y
    ctl->gyroZ(),        // Gyro Z
    ctl->accelX(),       // Accelerometer X
    ctl->accelY(),       // Accelerometer Y
    ctl->accelZ()        // Accelerometer Z
  );
}

void processGamepad(ControllerPtr ctl) {
  // There are different ways to query whether a button is pressed.
  // By query each button individually:
  //  a(), b(), x(), y(), l1(), etc...
  //   if (ctl->a()) {
  //     static int colorIdx = 0;
  //     // Some gamepads like DS4 and DualSense support changing the color LED.
  //     // It is possible to change it by calling:
  //     switch (colorIdx % 3) {
  //       case 0:
  //         // Red
  //         ctl->setColorLED(255, 0, 0);
  //         break;
  //       case 1:
  //         // Green
  //         ctl->setColorLED(0, 255, 0);
  //         break;
  //       case 2:
  //         // Blue
  //         ctl->setColorLED(0, 0, 255);
  //         break;
  //     }
  //     colorIdx++;
  //   }

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
    // ctl->playDualRumble(0 /* delayedStartMs */, 250 /* durationMs */, 0x80 /* weakMagnitude */,
    //                   0x40 /* strongMagnitude */);
  }

  // MODO MANUAL
  if (!actionButtonR3 && ctl->thumbR() && millis() - delayButtonR3 > 400) {
    actionButtonR3 = true;
    delayButtonR3 = millis();
  }

  if (actionButtonR3) {
    manualMode = !manualMode;
    if (manualMode) {
      ctl->setColorLED(255, 0, 0);
    } else {
      ctl->setColorLED(0, 0, 255);
    }
    actionButtonR3 = false;
  }

  // CONTROLE SERVO DIRECAO
  // (-511 - 512) left X Axis
  if (ctl->axisX() > 10 || ctl->axisX() < 0) {
    // *** DIREÇÃO
    int GrauServo = 0;  // variável para receber a conversão dos valores dos joysticks compatíveis com os valores dos servos

    // Caso queira limitar os movimentos do braço do servo altere os valores 0, 180

    GrauServo = map(ctl->axisX(), -511, 511, 135, 45);  // Movimentação total do Servo 180 graus

    servoDirecao.write(GrauServo);
  }

  if (ctl->axisX() < 10 && ctl->axisX() > 0) {
    servoDirecao.write(90);  // CENTRAL
  }

  ////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////

  // ACELERAÇÃO FRENTE RÉ E FREIO

  AuxjoyDireitaY = ctl->axisRY();

  // ACELERACAO PELO R-TRIGGER (R2)
  //  acceleration = ctl->throttle();     // (0 - 1023): throttle (AKA gas) button
  if (manualMode) {
    PWM = map(ctl->throttle(), 0, 1020, 0, 255);
    // Serial.print("PWM: ");
    // Serial.println(PWM);
  }

  // *** FRENTE
  if (AuxjoyDireitaY <= -10) {

    // TRATANDO PONTE H

    //   PWM = map(AuxjoyDireitaY, 4, -508, 0, 255);
    if (!manualMode) {
      PWM = map(AuxjoyDireitaY, 4, -508, 0, 255);
    }
    Serial.print("PWM: ");
    Serial.println(PWM);

    if (freio == false) {  // Para verificar se deve ser acionado Motor para frente ou Freiar

      ledcWrite(10, 0);
      ledcWrite(11, PWM);
      // ledcWrite(11, map(AuxjoyDireitaY, 120, 25, 0, 255));
      // Serial.println(" FRENTE: ");

      digitalWrite(22, LOW);  // APAGA LUZ DE RÉ
      if (Farol == 0) {
        ledcWrite(13, 0);  // Apaga luz de freio
      } else {
        ledcWrite(13, 100);  // Diminui a intensidade da luz para Lanterna
      }
      Millis_Meio = millis();
      Re = false;  // jOYSTICK PARA CIMA
      Frente = true;
    }

    if ((freio == true) && (Re == true) && (AtivaFreio == true)) {  // FREIAR

      Serial.println("FREIO ACIONADO");
      ledcWrite(10, 255);  // FREIA MOTOR PONTEH
      ledcWrite(11, 255);  // FREIA MOTOR PONTEH
      ledcWrite(13, 255);  // Acende luz de freio
    } else {
      freio = false;
    }
  }

  // *** MEIO

  if ((AuxjoyDireitaY > -10) && (AuxjoyDireitaY < 10)) {

    // TRATANDO PONTE H
    ledcWrite(10, LOW);
    ledcWrite(11, LOW);

    digitalWrite(22, LOW);  // APAGA LUZ DE RÉ
    if (Farol == 0) {
      ledcWrite(13, 0);  // Apaga luz de freio
    } else {
      ledcWrite(13, 100);  // Diminui a intensidade da luz para Lanterna
    }

    if (millis() - Millis_Meio < 600) {
      Serial.println("Sem Aceleracao");
      freio = true;
      ledcWrite(14, 0);    // Para som no gpio 25
      ledcWrite(13, 255);  // Diminui a intensidade da luz para Lanterna
    } else {
      if (Farol == 0) {
        ledcWrite(13, 0);  // Apaga luz de freio
      } else {
        ledcWrite(13, 100);  // Diminui a intensidade da luz para Lanterna
      }
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
      PWM = map(AuxjoyDireitaY, 5, 512, 0, 255);
    }
    Serial.print("PWM: ");
    Serial.println(PWM);

    if (freio == false) {  // Para verificar se deve ser acionado Motor para trás ou Freiar
      // ledcWrite(10, map(AuxjoyDireitaY, 135, 255, 25, 255));
      ledcWrite(10, PWM);
      ledcWrite(11, LOW);
      Serial.println(" RE ");

      if (Farol == 0) {
        ledcWrite(13, 0);  // Apaga luz de freio
      } else {
        ledcWrite(13, 100);  // Diminui a intensidade da luz para Lanterna
      }
      digitalWrite(22, HIGH);  // ACENDE LUZ DE RÉ
      Millis_Meio = millis();
      Re = true;  // RÉ JOYSTICK PARA BAIXO
      Frente = false;
    }

    if ((freio == true) && (Frente == true) && (AtivaFreio == true)) {  // FREIAR
      Serial.println("FREIO ACIONADO");
      ledcWrite(10, 255);
      ledcWrite(11, 255);
      ledcWrite(13, 255);  // Acende luz de freio
    } else {
      freio = false;
    }
  }
  ////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////

  // Another way to query controller data is by getting the buttons() function.
  // See how the different "dump*" functions dump the Controller info.

  // PISCA-ALERTA
  // PS4 - BOTAO TRIANGULO
  if (!actionButtonTRIANGULO && ctl->y() && millis() - delayButtonTRIANGULO > 400) {
    actionButtonTRIANGULO = true;
    delayButtonTRIANGULO = millis();
  }

  if (actionButtonTRIANGULO) {
    if (AuxPiscaAlerta == HIGH) {
      AuxPiscaAlerta = LOW;
    } else {
      AuxPiscaAlerta = HIGH;
    }
    digitalWrite(SETA_DIREITA, LOW);
    digitalWrite(SETA_ESQUERDA, LOW);

    Serial.print("ACIONA PISCA-ALERTA ");
    actionButtonTRIANGULO = false;
  }

  if (AuxPiscaAlerta == HIGH) {
    if (millis() - Millis_PiscaAlerta > 375) {
      digitalWrite(SETA_DIREITA, !digitalRead(SETA_DIREITA));
      digitalWrite(SETA_ESQUERDA, !digitalRead(SETA_ESQUERDA));
      Millis_PiscaAlerta = millis();
      // simula som de tic tac dos reles das setas ao ligar pisca-alerta
      if ((AuxBuzina == LOW) && (Re == LOW)) {
        ledcSetup(14, 50, 8);
        ledcWrite(14, 1);  // gera um som no gpio 25
        delay(50);
        ledcWrite(14, 0);  // Para o som
        ledcSetup(14, 500, 8);
      }
    }
  } else {
    PiscaAlertaOFF = HIGH;
  }

  if (!actionButtonBOLA && ctl->b() && millis() - delayButtonBOLA > 400) {
    actionButtonBOLA = true;
    delayButtonBOLA = millis();
  }

  if (actionButtonBOLA) {
    if (Farol == 1) {
      Farol = 0;
      ledcWrite(12, 0);  // Farol Apagado
      ledcWrite(13, 0);  // Lanterna Apagada

      Serial.print("Desliga FAROL BAIXO ");
      Serial.println(Farol);
    } else {
      Farol = 1;
      ledcWrite(12, 100);  // Farol Luz Baixa
      ledcWrite(13, 100);  // Lanterna Luz Baixa

      Serial.print("Liga FAROL BAIXO ");
      Serial.println(Farol);
    }
    actionButtonBOLA = false;
  }

  if (!actionButtonQUADRADO && ctl->x() && millis() - delayButtonQUADRADO > 400) {
    actionButtonQUADRADO = true;
    delayButtonQUADRADO = millis();
  }

  if (actionButtonQUADRADO) {
    contFarolBaixo++;
    if (contFarolBaixo > 2) {
      contFarolBaixo = 0;
    }

    if (contFarolBaixo == 0) {

      ledcWrite(12, 0);
      ledcWrite(13, 0);  // Lanterna Apagada

      Serial.print("Desliga FAROL ");
      Serial.println(Farol);
    }

    if (contFarolBaixo == 1) {
      ledcWrite(12, 100);  // Farol Luz Baixa
      ledcWrite(13, 100);  // Lanterna Luz Baixa

      Serial.print("Liga FAROL ALTO ");
      Serial.println(contFarolBaixo);
    }
    if (contFarolBaixo == 2) {
      ledcWrite(12, 255);  // Farol Luz  Alta
      ledcWrite(13, 100);  // Lanterna Luz Baixa

      Serial.print("Liga FAROL BAIXO ");
      Serial.println(contFarolBaixo);
    }
    actionButtonQUADRADO = false;
  }

  // FAROL DE MILHA
  if (!actionButtonX && ctl->a() && millis() - delayButtonX > 400) {
    actionButtonX = true;
    delayButtonX = millis();
  }

  if (actionButtonX) {

    if (farolMilha) {
      digitalWrite(FAROL_MILHA, LOW);
      Serial.print("APAGA FAROL DE MILHA ");
    } else {
      digitalWrite(FAROL_MILHA, HIGH);
      Serial.print("ACENDE FAROL DE MILHA ");
    }
    farolMilha = !farolMilha;
    actionButtonX = false;
  }


  // SETA DIREITA
  if (!actionButtonR1 && ctl->r1() && millis() - delayButtonR1 > 400) {
    actionButtonR1 = true;
    delayButtonR1 = millis();
  }

  if (actionButtonR1) {

    BotaoSeta_D++;
    BotaoSeta_E = 0;

    if (BotaoSeta_D > 2) {
      BotaoSeta_D = 1;
    }
    if (BotaoSeta_D == 1) {
      // joyDireitaX = 131;
      digitalWrite(SETA_DIREITA, HIGH);
      digitalWrite(SETA_ESQUERDA, LOW);
      Serial.println("Liga Seta Direita no Botao");
    }
    if (BotaoSeta_D == 2) {
      digitalWrite(SETA_DIREITA, LOW);
      // joyDireitaX = 127;
      Serial.println("Desliga Seta Direita no Botao");
    }
    actionButtonR1 = false;
  }


  dumpGamepad(ctl);
}

void processMouse(ControllerPtr ctl) {
  // This is just an example.
  if (ctl->scrollWheel() > 0) {
    // Do Something
  } else if (ctl->scrollWheel() < 0) {
    // Do something else
  }

  // See "dumpMouse" for possible things to query.
  dumpMouse(ctl);
}

void processKeyboard(ControllerPtr ctl) {
  if (!ctl->isAnyKeyPressed())
    return;

  // This is just an example.
  if (ctl->isKeyPressed(Keyboard_A)) {
    // Do Something
    Serial.println("Key 'A' pressed");
  }

  // Don't do "else" here.
  // Multiple keys can be pressed at the same time.
  if (ctl->isKeyPressed(Keyboard_LeftShift)) {
    // Do something else
    Serial.println("Key 'LEFT SHIFT' pressed");
  }

  // Don't do "else" here.
  // Multiple keys can be pressed at the same time.
  if (ctl->isKeyPressed(Keyboard_LeftArrow)) {
    // Do something else
    Serial.println("Key 'Left Arrow' pressed");
  }

  // See "dumpKeyboard" for possible things to query.
  dumpKeyboard(ctl);
}

void processBalanceBoard(ControllerPtr ctl) {
  // This is just an example.
  if (ctl->topLeft() > 10000) {
    // Do Something
  }

  // See "dumpBalanceBoard" for possible things to query.
  dumpBalanceBoard(ctl);
}

void processControllers() {
  for (auto myController : myControllers) {
    if (myController && myController->isConnected() && myController->hasData()) {
      if (myController->isGamepad()) {
        processGamepad(myController);
      } else if (myController->isMouse()) {
        processMouse(myController);
      } else if (myController->isKeyboard()) {
        processKeyboard(myController);
      } else if (myController->isBalanceBoard()) {
        processBalanceBoard(myController);
      } else {
        Serial.println("Unsupported controller");
      }
    }
  }
}

// Arduino setup function. Runs in CPU 1
void setup() {
  Serial.begin(115200);
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

  // CONFIGURANDO GPIOs DA PONTE H
  // PWM PONTE H
  ledcAttachPin(PONTE_H_IN01, 10);  //  ledcAttachPin(Pino do ESP, Nº Canal PWM 0 a 15)
  ledcSetup(10, 500, 8);            //  ledcSetup(Nº Canal PWM, Frequencia em Hz, Resolução em Bits)
  // PWM PONTE H
  ledcAttachPin(PONTE_H_IN02, 11);
  ledcSetup(11, 500, 8);

  // CONFIGURANDO GPIOs COMO SAIDA DE DADOS PARA LEDs e MOTOR DE VIBRAÇÃO
  pinMode(FAROL, OUTPUT);          // FAROL
  pinMode(FAROL_MILHA, OUTPUT);    // FAROL DE MILHA
  pinMode(LUZ_FREIO, OUTPUT);      // LUZ DE FREIO / LANTERNA TRAZEIRA
  pinMode(SETA_DIREITA, OUTPUT);   // SETA DIREITA
  pinMode(SETA_ESQUERDA, OUTPUT);  // SETA ESQUERDA
  pinMode(LUZ_AUX01, OUTPUT);      // LUZ AUXILIAR 01
  pinMode(LUZ_AUX02, OUTPUT);      // LUZ AUXILIAR 02
  pinMode(LUZ_RE, OUTPUT);         // LUZ DE RÉ
  pinMode(25, OUTPUT);             // MOTOR DE VIBRAÇÃO

  // CONFIGURANDO SERVO MOTOR
  servoDirecao.attach(SERVO_DIRECAO);

  // DIFININDO UM VALOR INICIAL PARA OS SERVOS MOTORES
  servoDirecao.write(90);

  // CONFIGURANDO PWM LED FAROL
  ledcAttachPin(FAROL, 12);
  ledcSetup(12, 500, 8);

  // CONFIGURANDO PWM LED LUZ DE FREIO
  ledcAttachPin(LUZ_FREIO, 13);
  ledcSetup(13, 500, 8);

  // CONFIGURANDO PWM Para BIP DA BUZINA NO GPIO 25
  ledcAttachPin(BUZINA_PIN, 14);
  ledcSetup(14, 500, 8);
}

// Arduino loop function. Runs in CPU 1.
void loop() {
  // This call fetches all the controllers' data.
  // Call this function in your main loop.
  bool dataUpdated = BP32.update();
  if (dataUpdated)
    processControllers();

  // The main loop must have some kind of "yield to lower priority task" event.
  // Otherwise, the watchdog will get triggered.
  // If your main loop doesn't have one, just add a simple `vTaskDelay(1)`.
  // Detailed info here:
  // https://stackoverflow.com/questions/66278271/task-watchdog-got-triggered-the-tasks-did-not-reset-the-watchdog-in-time

  //     vTaskDelay(1);
  delay(10);
}

// REFERENCIAS
//  To test one button at a time.
// PS4 - X bool a() const { return buttons() & BUTTON_A; }
// PS4 - BOLA bool b() const { return buttons() & BUTTON_B; }
// PS4 - QUADRADO bool x() const { return buttons() & BUTTON_X; }
// PS4 - TRIANGULO bool y() const { return buttons() & BUTTON_Y; }
// PS4 - IGUAL bool l1() const { return buttons() & BUTTON_SHOULDER_L; }
// PS4 - IGUAL bool l2() const { return buttons() & BUTTON_TRIGGER_L; }
// PS4 - IGUAL bool r1() const { return buttons() & BUTTON_SHOULDER_R; }
// PS4 - IGUAL bool r2() const { return buttons() & BUTTON_TRIGGER_R; }
// PS4 - IGUAL bool thumbL() const { return buttons() & BUTTON_THUMB_L; }
// PS4 - IGUAL bool thumbR() const { return buttons() & BUTTON_THUMB_R; }
