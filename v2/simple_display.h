#ifndef _SIMPLE_DISPLAY_H_
#define _SIMPLE_DISPLAY_H_

#include "band_display.h"
#include "fft_display.h"
#include "normalized_fft.h"

class SimpleDisplay {
 public:
  SimpleDisplay(NormalizedFft* fft, ILI9341_t3* display);

  bool enabled();
  void set_enabled(bool value);

  void OnRmsAvailable(float rms);
  void OnFftAvailable();
  void Loop();

 private:
  void Begin();

  NormalizedFft* fft_;
  ILI9341_t3* display_;
  bool enabled_;

  BandDisplay band_display_;
  FftDisplay fft_display_;

  int rms_band_id_;
};

#endif
