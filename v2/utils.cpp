#include "utils.h"

bool CheckSerial(char ch) {
  if (Serial.peek() == ch) {
    Serial.read();
    return true;
  }
  return false;
}

int AdjustInt(const char* name) {
  int value = Serial.parseInt();
  PrintValue(name, value);
  return value;
}

float AdjustFloat(const char* name) {
  float value = Serial.parseFloat();
  PrintValue(name, value);
  return value;
}

