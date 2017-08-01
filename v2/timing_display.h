#ifndef _TIMING_DISPLAY_H_
#define _TIMING_DISPLAY_H_

#include <ILI9341_t3.h>

#include "normalized_fft.h"

class TimingDisplay {
 public:
  TimingDisplay(NormalizedFft* fft, ILI9341_t3* display);

  bool enabled();
  void set_enabled(bool value);

  void OnFftAvailable();
  void DoCommands();

 private:
  static constexpr int kWidth = 301;
  void Begin();
  void UpdateDisplay();

  NormalizedFft* fft_;
  ILI9341_t3* display_;
  bool enabled_;

  bool showing_both_;
  unsigned int displayed_level_[2][kWidth];
  unsigned int level_[2][kWidth];
  int pos_;
  int start_bin_[2];
  int end_bin_[2];
  bool frozen_;
};

#endif
