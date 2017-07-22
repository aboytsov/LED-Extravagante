#ifndef _SIMPLE_FFT_DISPLAY_H_
#define _SIMPLE_FFT_DISPLAY_H_

#include "band_display.h"
#include "normalized_fft.h"

// Supports the following commands in the Serial monitor:
// f[start_bin]:[end_bin] - Focus on bins start_bin to end_bin.
// f - Disable focus mode.
class FftDisplay {
 public:
  FftDisplay(NormalizedFft* fft, BandDisplay* band_display, int x, int y, int width, int height, int bins, int color);
  ~FftDisplay();

  void OnFftAvailable();
  void DoCommands();

 private:
  NormalizedFft* fft_;
  BandDisplay* band_display_;
  int bins_;
  bool focus_mode_;
  int focus_mode_start_bin_;
  int focus_mode_end_bin_;
  int* band_ids_;
};

#endif

