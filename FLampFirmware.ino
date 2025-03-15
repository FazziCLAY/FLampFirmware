// SPDX-License-Identifier: Apache-2.0
/*
   Firmware for my Arduino socket connected 24/7 to my laptop USB
   Copyright (c) 2025, Stanislav Mironov <fazziclay@gmail.com>
   https://fazziclay.com/?p=flampfirmware
*/

#include <EEPROM.h>

// settings
#define PIN_RELAY 2
#define INVERT_RELAY true

#define FEATURE_EEPROM 1
#define FEATURE_BUILTIN_BUTTON 1
#define FEATURE_SERIAL 1
#define FEATURE_LED 1

// led
#define PIN_LED 13
#define INVERT_LED false

// messages (FEATURE_SERIAL)
#define MAX_COMMAND_LENGTH 64
#define MSG_CMD_TOO_LONG "MSG_CMD_TOO_LONG"
#define MSG_CMD_UNKNOWN "MSG_CMD_UNKNOWN"
#define CMD_ON "CMD_ON"
#define CMD_OFF "CMD_OFF"
#define CMD_QUERY "CMD_QUERY"
#define CMD_INVERT "CMD_INVERT"
#define STATUS_ON "STATUS_ON"
#define STATUS_OFF "STATUS_OFF"
#define STATUS_STARTED "STATUS_STARTED"

// NVM for FEATURE_EEPROM: https://en.wikipedia.org/wiki/Non-volatile_memory; https://alexgyver.ru/lessons/eeprom/
#define EEPROM_ON 94
#define EEPROM_OFF 255
#define EEPROM_ADDR 10

static bool status = false;

void setup() {
#if FEATURE_SERIAL
  Serial.begin(9600);
  Serial.println(STATUS_STARTED);
#endif

#if FEATURE_LED
  pinMode(PIN_LED, OUTPUT);
#endif

  // relay
  pinMode(PIN_RELAY, OUTPUT);

#if FEATURE_EEPROM
  statusChange(EEPROM[EEPROM_ADDR] == EEPROM_ON);
#else
  statusChange(false);
#endif

#if FEATURE_BUILTIN_BUTTON
  pinMode(9, OUTPUT);
  digitalWrite(9, LOW); // set GND to D9 pin
  pinMode(11, INPUT_PULLUP);
#endif
}

void statusChange(bool _status) {
  status = _status;
  digitalWrite(PIN_RELAY, INVERT_RELAY ? !status : status); // for my 'JQC-3FF-S-Z' relay module: LOW - activate

#if FEATURE_LED
  digitalWrite(PIN_LED, INVERT_LED ? !status : status); // off red 13 LED
#endif

#if FEATURE_SERIAL
  Serial.println(status ? STATUS_ON : STATUS_OFF);
#endif

#if FEATURE_EEPROM
  EEPROM.update(EEPROM_ADDR, status ? EEPROM_ON : EEPROM_OFF);
#endif
}

#if FEATURE_SERIAL
void processCommand(const char* cmd) {
  if (strcmp(cmd, CMD_ON) == 0) {
    statusChange(true);
  } else if (strcmp(cmd, CMD_OFF) == 0) {
    statusChange(false);
  } else if (strcmp(cmd, CMD_QUERY) == 0) {
    Serial.println(status ? STATUS_ON : STATUS_OFF);
  } else if (strcmp(cmd, CMD_INVERT) == 0) { // so lazy method to control relay
    statusChange(!status);
  } else {
    Serial.println(MSG_CMD_UNKNOWN);
  }
}
#endif

#if FEATURE_SERIAL
inline void loopCommand() {
  static char command[MAX_COMMAND_LENGTH];
  static int commandIndex = 0;

  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n') {
      command[commandIndex] = '\0';
      processCommand(command);
      commandIndex = 0;
      memset(command, 0, sizeof(command));
    } else if (commandIndex < MAX_COMMAND_LENGTH - 1) {
      command[commandIndex++] = c;
    } else {
      Serial.println(MSG_CMD_TOO_LONG);
      commandIndex = 0;
      memset(command, 0, sizeof(command));
    }
  }
}
#endif

#if FEATURE_BUILTIN_BUTTON
inline void loopButton() {
  // читаем инвертированное значение для удобства
  static bool flag = false;
  static uint32_t btnTimer = 0;
  bool btnState = !digitalRead(11);
  if (btnState && !flag && millis() - btnTimer > 300) {
    flag = true;
    btnTimer = millis();
  }
  if (!btnState && flag && millis() - btnTimer > 300) {
    flag = false;
    int dur = millis() - btnTimer;
    if (dur < 2000) {
      statusChange(!status);
    }

    if (dur >= 2000) {
      statusChange(true);
    }

    btnTimer = millis();
  }
}
#endif

void loop() {
#if FEATURE_SERIAL
  loopCommand();
#endif

#if FEATURE_BUILTIN_BUTTON
  loopButton();
#endif
}
