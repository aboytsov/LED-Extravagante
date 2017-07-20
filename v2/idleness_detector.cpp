#include "idleness_detector.h"

#include <Audio.h>

void IdlenessDetector::OnRmsAvailable(float rms) {
  if (rms > 0.07) {
    last_idle_time_millis_ = millis();
  }
}

float IdlenessDetector::SecondsIdle() {
  return (millis() - last_idle_time_millis_) / 1000.0;
}

