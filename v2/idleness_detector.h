#ifndef _IDLENESS_DETECTOR_H_
#define _IDLENESS_DETECTOR_H_

// Detects if there is no music playing.
class IdlenessDetector {
 public:
  void OnRmsAvailable(float rms);

  float SecondsIdle();

 private:
  unsigned long last_idle_time_millis_ = 0;
};
#endif
