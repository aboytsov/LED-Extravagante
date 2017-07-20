#include "normalized_fft.h"

#include <FastLED.h>

#define TARGET_RMS 0.5

NormalizedFft::NormalizedFft(AudioAnalyzeFFT1024* fft)
  : fft_(fft), num_adjustments_(0), max_rms_since_last_update_(0), rms_(TARGET_RMS) {}

void NormalizedFft::OnRmsAvailable(float rms) {
  max_rms_since_last_update_ = max(max_rms_since_last_update_, rms);

  EVERY_N_MILLISECONDS(500) {
    float adjustment_rate;

    if (max_rms_since_last_update_ < 0.07) {
      // Don't touch anything if there's nothing playing.
      adjustment_rate = 0;
    } else if (num_adjustments_ < 10) {
      // For the first 5 seconds after the device is turned on, adjust quickly.
      adjustment_rate = 0.05;
    } else if (max_rms_since_last_update_ - rms_ > 0.1) {
      // Big difference, adjust quickly.
      adjustment_rate = 0.025;
    } else {
      // Otherwise, adjust slowly.
      adjustment_rate = 0.0015;
    }

    // Adjust.
    if (max_rms_since_last_update_ > rms_) {
      rms_ = min(rms_ + adjustment_rate, max_rms_since_last_update_);
    } else {
      rms_ = max(max(rms_ - adjustment_rate, max_rms_since_last_update_), 0.07);
    }

    ++num_adjustments_;
    max_rms_since_last_update_ = 0;
  }
}

bool NormalizedFft::available() {
  return fft_->available();
}

float NormalizedFft::Normalize(float value) {
  return min(1.0, value * TARGET_RMS / rms_);
}

float NormalizedFft::Rms() {
  return rms_;
}

float NormalizedFft::read(int bin) {
  return Normalize(fft_->read(bin));
}

float NormalizedFft::read(int start_bin, int end_bin) {
  return Normalize(fft_->read(start_bin, end_bin));
}


