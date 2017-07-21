#ifndef _FFT_DISTRIBUTION_DISPLAY_H_
#define _FFT_DISTRIBUTION_DISPLAY_H_

#include <ILI9341_t3.h>

#include "normalized_fft.h"

class FftDistributionDisplay {
 public:
  FftDistributionDisplay(NormalizedFft* fft, ILI9341_t3* display, bool enabled);

  void Begin();
  void OnFftAvailable();

 private:
  static constexpr int kSeconds = 3;
 
  NormalizedFft* fft_;
  ILI9341_t3* display_;
  bool enabled_;
  int start_bin_;
  int end_bin_;
  int period_;
  unsigned int counts_[kSeconds][320];
  unsigned int previous_position_[320];
};

#endif
