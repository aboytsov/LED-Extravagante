#ifndef _DEBUG_DISPLAY_H_
#define _DEBUG_DISPLAY_H_

#include "band_display.h"
#include "fft_display.h"
#include "normalized_fft.h"

class DebugDisplay {
 public:
  DebugDisplay(NormalizedFft* fft, ILI9341_t3* display, bool enabled);

  void Begin();
  void OnRmsAvailable(float rms);
  void OnPeakAvailable(float peak);
  void OnFftAvailable();
  void Loop();

  void UpdateBand1(float value);
  void UpdateBand2(float value);
  void UpdateBand3(float value);
  void UpdateBand4(float value);

 private:
  NormalizedFft* fft_;
  ILI9341_t3* display_;
  bool enabled_;

  BandDisplay band_display_;
  FftDisplay fft_display_;

  int peak_band_id_;
  int rms_band_id_;
  int normalized_rms_band_id_;
  int band_1_id_;
  int band_2_id_;
  int band_3_id_;
  int band_4_id_;
};

#endif
