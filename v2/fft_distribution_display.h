#ifndef _FFT_DISTRIBUTION_DISPLAY_H_
#define _FFT_DISTRIBUTION_DISPLAY_H_

#include <ILI9341_t3.h>

#include "normalized_fft.h"

// Displays the distribution of a given bins of the FFT. The bins to display
// can be changed by typing the following into the Serial monitor:
// f[start_bin]:[end_bin] (e.g. b0:1 for bins 0 and 1).
class FftDistributionDisplay {
 public:
  FftDistributionDisplay(NormalizedFft* fft, ILI9341_t3* display);

  bool enabled();
  void set_enabled(bool value);

  void OnFftAvailable();
  void DoCommands();

 private:
  void Begin();

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
