#ifndef _SIMPLE_FFT_DISPLAY_H_
#define _SIMPLE_FFT_DISPLAY_H_

#include <Audio.h>

#include "band_display.h"

class FftDisplay {
 public:
  FftDisplay(AudioAnalyzeFFT1024* fft, BandDisplay* band_display, int x, int y, int width, int height, int bins, int color);
  ~FftDisplay();

  void OnFftAvailable();

 private:
  AudioAnalyzeFFT1024* fft_;
  BandDisplay* band_display_;
  int bins_;
  int* band_ids_;
};

#endif

