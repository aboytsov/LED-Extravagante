#include "fft_display.h"

FftDisplay::FftDisplay(NormalizedFft* fft, BandDisplay* band_display, int x, int y, int width, int height, int bins, int color)
  : fft_(fft), band_display_(band_display), bins_(bins), band_ids_(new int[bins_]) {
  for (int i = 0; i < bins_; ++i) {
    band_ids_[i] = band_display->AddBand(x + i * width / bins, y, width / bins, height, color, /*decaying=*/true);
  }
}

FftDisplay::~FftDisplay() {
  delete[] band_ids_;
}

void FftDisplay::OnFftAvailable() {
  for (int i = 0; i < bins_; ++i) {
    float level = fft_->read(i + 1);
    band_display_->UpdateBand(band_ids_[i], level);
  }
}

