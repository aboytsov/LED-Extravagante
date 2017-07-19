#include "idleness_detector.h"

void IdlenessDetector::OnPeakAvailable(float peak_value) {
  if (peak_value > 0.18) {
    last_idle_time_millis_ = millis();
  }
}

float IdlenessDetector::SecondsIdle() {
  return (millis() - last_idle_time_millis_) / 1000.0;
}

