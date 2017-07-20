#ifndef _SIMPLE_FFT_DISPLAY_H_
#define _SIMPLE_FFT_DISPLAY_H_

#include "band_display.h"
#include "normalized_fft.h"

class FftDisplay {
 public:
  FftDisplay(NormalizedFft* fft, BandDisplay* band_display, int x, int y, int width, int height, int bins, int color);
  ~FftDisplay();

  void OnFftAvailable();

 private:
  NormalizedFft* fft_;
  BandDisplay* band_display_;
  int bins_;
  int* band_ids_;
};

#endif

