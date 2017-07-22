#include "idleness_detector.h"

#include <Audio.h>

#include "utils.h"

void IdlenessDetector::OnRmsAvailable(float rms) {
  if (rms > 0.10) {
    last_idle_time_millis_ = millis();
  }
}

float IdlenessDetector::SecondsIdle() {
  return (millis() - last_idle_time_millis_) / 1000.0;
}

void IdlenessDetector::DoCommands() {
  if (CheckSerial('l')) {
    last_idle_time_millis_ = - 300 * 1000;
  }
}

