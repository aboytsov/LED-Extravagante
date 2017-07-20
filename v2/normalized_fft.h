#ifndef _NORMALIZED_FFT_H_
#define _NORMALIZED_FFT_H_

#include <Audio.h>

class NormalizedFft {
 public:
  NormalizedFft(AudioAnalyzeFFT1024* fft);

  void OnPeakAvailable(float peak);
  float Peak();

  bool available();
  float read(int bin);
  float read(int start_bin, int end_bin);

 private:
  float Normalize(float value);
 
  AudioAnalyzeFFT1024* fft_;
  int num_adjustments_;
  float recorded_peak_;
  float max_peak_since_last_update_;
  float peak_;
};

#endif
