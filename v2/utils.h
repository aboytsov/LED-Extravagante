#ifndef _UTILS_H_
#define _UTILS_H_

#include <Arduino.h>

bool CheckSerial(char ch);

int AdjustInt(const char* name);

float AdjustFloat(const char* name);

template<typename ValueType>
void PrintValue(const char* name, ValueType value) {
  Serial.print(name);
  Serial.print("=");
  Serial.println(value);
}

#endif
