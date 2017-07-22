#include "fft_display.h"

#include "utils.h"

FftDisplay::FftDisplay(NormalizedFft* fft, BandDisplay* band_display, int x, int y, int width, int height, int bins, int color)
  : fft_(fft), band_display_(band_display), bins_(bins), focus_mode_(false), band_ids_(new int[bins_]) {
  for (int i = 0; i < bins_; ++i) {
    band_ids_[i] = band_display->AddBand(x + i * width / bins, y, width / bins, height, color, /*decaying=*/true);
  }
}

FftDisplay::~FftDisplay() {
  delete[] band_ids_;
}

void FftDisplay::OnFftAvailable() {
  if (focus_mode_) {
    float level = fft_->read(focus_mode_start_bin_, focus_mode_end_bin_);
    for (int i = 0; i < bins_; ++i) {
      band_display_->UpdateBand(band_ids_[i], level);
    }
  } else {
    for (int i = 0; i < bins_; ++i) {
      float level = fft_->read(i + 1);
      band_display_->UpdateBand(band_ids_[i], level);
    }
  }
}

void FftDisplay::DoCommands() {
  if (CheckSerial('f')) {
    if (Serial.peek() == -1) {
      focus_mode_ = false;
      Serial.println("Disabling focus mode.");
    } else {
      focus_mode_ = true;
      focus_mode_start_bin_ = Serial.parseInt();
      Serial.read();
      focus_mode_end_bin_ = Serial.parseInt();

      Serial.print("Will focus on bins ");
      Serial.print(focus_mode_start_bin_);
      Serial.print(" to ");
      Serial.print(focus_mode_end_bin_);
      Serial.println(".");
    }
  }
}

