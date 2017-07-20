#ifndef _NORMALIZED_FFT_H_
#define _NORMALIZED_FFT_H_

#include <Audio.h>

// Normalizes the FFT, returning values as if the RMS (i.e. volume) was fixed to 0.5.
// Thus, if someone turns on music from a source with a different overall volume level than before,
// the LED system adjusts.
class NormalizedFft {
 public:
  NormalizedFft(AudioAnalyzeFFT1024* fft);

  void OnRmsAvailable(float rms);
  float Rms();

  bool available();
  float read(int bin);
  float read(int start_bin, int end_bin);

 private:
  float Normalize(float value);
 
  AudioAnalyzeFFT1024* fft_;
  int num_adjustments_;
  float max_rms_since_last_update_;
  float rms_;
};

#endif
