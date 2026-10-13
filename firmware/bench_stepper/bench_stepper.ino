/*
 * Deck Sorter — Arduino Nano / A4988 safe bench-test firmware
 * 2026-10-13
 *
 * PURPOSE: Move an UNLOADED NEMA17 by small, explicit commands.
 * Does NOT advance cards automatically, detect jams, or verify scans.
 *
 * IMPORTANT: Before energizing VMOT, wire a >=47uF electrolytic capacitor
 * across VMOT/GND, identify both motor coils, and set A4988 current limit.
 * Power NEMA17 from a SEPARATE 12V supply (NOT Arduino 5V/USB).
 *
 * WIRING: Nano D2 -> STEP; D3 -> DIR; D4 -> EN (active LOW).
 *         Nano 5V -> VDD; Nano GND -> logic GND; common motor GND.
 *         SLEEP and RESET -> VDD; MS1/MS2/MS3 -> GND (full step).
 * Check labels on YOUR driver; do not assume its orientation.
 *
 * Arduino IDE: Board "Arduino Nano", Processor ATmega328P
 * Serial Monitor: 115200 baud, line ending "Newline".
 *
 * COMMANDS:
 *    f 10   move 10 steps forward
 *    b 10   move 10 steps backward
 *    v 100  set speed: 50..400 full-steps per second
 *    x      stop now and release motor
 *    ?      print help
 *
 * Maximum single command is 200 steps (one full motor turn).
 * Firmware never moves the motor automatically on boot.
 */
#include <Arduino.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>

const uint8_t STEP_PIN = 2;
const uint8_t DIR_PIN  = 3;
const uint8_t EN_PIN   = 4;

const uint16_t MAX_STEPS = 200;
const uint16_t MIN_SPEED = 50;
const uint16_t MAX_SPEED = 400;

uint16_t speedStepsPerSec = 100;
uint16_t remainingSteps = 0;
uint32_t nextStepMicros = 0;

char lineBuf[32];
uint8_t lineLength = 0;

void stopMotor(bool printStatus) {
  remainingSteps = 0;
  digitalWrite(STEP_PIN, LOW);
  digitalWrite(EN_PIN, HIGH); // A4988 EN is active LOW; release motor.
  if (printStatus) Serial.println(F("STOPPED (driver disabled)."));
}

void printHelp() {
  Serial.println(F("Deck Sorter: SAFE BENCH TEST (no automatic movement)"));
  Serial.println(F("f 10 = 10 forward steps; b 10 = 10 backward steps"));
  Serial.println(F("v 100 = 100 steps/s (range 50..400)"));
  Serial.println(F("x = immediate software stop; ? = help"));
  Serial.println(F("Use a physical motor-power disconnect for emergency stop."));
}

bool parseNumber(char *ptr, long *value) {
  while (*ptr == ' ' || *ptr == '\t') ++ptr;
  if (!*ptr) return false;
  char *endPtr;
  *value = strtol(ptr, &endPtr, 10);
  if (endPtr == ptr) return false;
  while (*endPtr == ' ' || *endPtr == '\t') ++endPtr;
  return *endPtr == '\0';
}

void handleCommand(char *line) {
  while (*line == ' ' || *line == '\t') ++line;
  if (!*line) return;
  const char command = (char)tolower(*line);
  if (command == 'x') {
    stopMotor(true);
    return;
  }
  if (command == '?') {
    printHelp();
    return;
  }
  if (command != 'f' && command != 'b' && command != 'v') {
    Serial.println(F("Unknown command. Send '?' for help."));
    return;
  }
  long number = 0;
  if (!parseNumber(line + 1, &number)) {
    Serial.println(F("Expected integer, e.g. 'f 10' or 'v 100'."));
    return;
  }
  if (command == 'v') {
    if (number < MIN_SPEED || number > MAX_SPEED) {
      Serial.println(F("Speed must be 50..400 steps/s."));
      return;
    }
    speedStepsPerSec = (uint16_t)number;
    Serial.print(F("Speed = "));
    Serial.println(speedStepsPerSec);
    return;
  }
  if (remainingSteps > 0) {
    Serial.println(F("Busy; wait or send x to stop."));
    return;
  }
  if (number < 1 || number > MAX_STEPS) {
    Serial.println(F("Steps must be 1..200 (start with 10)."));
    return;
  }
  digitalWrite(DIR_PIN, command == 'f' ? HIGH : LOW);
  digitalWrite(STEP_PIN, LOW);
  digitalWrite(EN_PIN, LOW);
  remainingSteps = (uint16_t)number;
  nextStepMicros = micros() + 2000UL; // Allow driver to enable and DIR to settle.
  Serial.print(F("Moving "));
  Serial.print(number);
  Serial.println(command == 'f' ? F(" forward steps.") : F(" backward steps."));
}

void readCommands() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\r' || c == '\n') {
      if (lineLength > 0) {
        lineBuf[lineLength] = '\0';
        handleCommand(lineBuf);
        lineLength = 0;
      }
    } else if (lineLength < sizeof(lineBuf) - 1) {
      lineBuf[lineLength++] = c;
    } else {
      lineLength = 0;
      Serial.println(F("Command too long; discarded."));
    }
  }
}

void setup() {
  // Configure ENABLE first; HIGH disables the driver.
  digitalWrite(EN_PIN, HIGH);
  pinMode(EN_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(STEP_PIN, LOW);
  digitalWrite(DIR_PIN, LOW);
  Serial.begin(115200);
  printHelp();
}

void loop() {
  readCommands(); // Allows 'x' between individual motor steps.
  if (remainingSteps == 0) return;

  const uint32_t now = micros();
  if ((int32_t)(now - nextStepMicros) < 0) return;

  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(3); // A4988 minimum STEP high time is ~1 us.
  digitalWrite(STEP_PIN, LOW);

  --remainingSteps;
  if (remainingSteps == 0) {
    stopMotor(false);
    Serial.println(F("DONE. Driver disabled."));
  } else {
    nextStepMicros += 1000000UL / speedStepsPerSec;
  }
}
