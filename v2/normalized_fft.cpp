#include "normalized_fft.h"

#include <FastLED.h>

NormalizedFft::NormalizedFft(AudioAnalyzeFFT1024* fft)
  : fft_(fft), num_adjustments_(0), max_peak_since_last_update_(0), peak_(0.7) {}

void NormalizedFft::OnPeakAvailable(float peak) {
  recorded_peak_ = peak;
  max_peak_since_last_update_ = max(max_peak_since_last_update_, peak);

  EVERY_N_MILLISECONDS(1000) {
    float adjustment_rate;

    if (num_adjustments_ == 0) {
      // Don't touch anything right when the device boots.
      adjustment_rate = 0;
    } else if (max_peak_since_last_update_ < 0.18) {
      // Don't touch anything if there's nothing playing.
      adjustment_rate = 0;
    } else if (num_adjustments_ < 6) {
      // For the first few seconds after the device is turned on, adjust quickly.
      adjustment_rate = 0.1;
    } else if (max_peak_since_last_update_ - peak_ > 0.1) {
      // Big difference, adjust quickly.
      adjustment_rate = 0.05;
    } else {
      // Otherwise, adjust slowly.
      adjustment_rate = 0.003;
    }

    // Adjust.
    if (max_peak_since_last_update_ > peak_) {
      peak_ = min(peak_ + adjustment_rate, max_peak_since_last_update_);
    } else {
      peak_ = max(max(peak_ - adjustment_rate, max_peak_since_last_update_), 0.18);
    }

    ++num_adjustments_;
    max_peak_since_last_update_ = 0;
  }
}

bool NormalizedFft::available() {
  return fft_->available();
}

float NormalizedFft::Normalize(float value) {
  return min(1.0, value * 0.7 / peak_);
}

float NormalizedFft::Peak() {
  return Normalize(recorded_peak_);
}

float NormalizedFft::read(int bin) {
  return Normalize(fft_->read(bin));
}

float NormalizedFft::read(int start_bin, int end_bin) {
  return Normalize(fft_->read(start_bin, end_bin));
}


