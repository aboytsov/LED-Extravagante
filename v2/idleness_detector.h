#ifndef _IDLENESS_DETECTOR_H_
#define _IDLENESS_DETECTOR_H_

#include <Audio.h>

class IdlenessDetector {
 public:
  void OnPeakAvailable(float peak_value);

  float SecondsIdle();

 private:
  AudioAnalyzePeak* peak_;
  unsigned long last_idle_time_millis_ = 0;
};
#endif
