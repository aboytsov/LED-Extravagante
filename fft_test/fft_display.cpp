#include "fft_display.h"

#include <FastLED.h>

void FftDisplay::AddBand(FftDisplay::Band band) {
  if (num_bands_ == MAX_FFT_DISPLAY_BANDS) {
    return;
  }
  bands_[num_bands_] = band;
  current_height_[num_bands_] = 0;
  ++num_bands_;
}

uint64_t total_micros;
uint32_t counts;
uint16_t last_micros[4000];
uint32_t current_pos;

void FftDisplay::Update(ILI9341_t3* display, AudioAnalyzeFFT1024* fft1024) {
  for (int i = 0; i < num_bands_; ++i) {
    new_height_[i] = current_height_[i];
  }

  if (fft1024->available()) {
    for (int i = 0; i < num_bands_; ++i) {
      const Band& band = bands_[i];
      float height = fft1024->read(band.first_bin, band.last_bin) * band.height;
      if (height > new_height_[i]) {
        new_height_[i] = height;
      }
    }
  }

  EVERY_N_MILLISECONDS(10) {
    for (int i = 0; i < num_bands_; ++i) {
      new_height_[i] = bands_[i].decay * new_height_[i];

    }
  }

  for (int i = 0; i < num_bands_; ++i) {
    const Band& band = bands_[i];
    if (static_cast<int>(new_height_[i]) != static_cast<int>(current_height_[i])) {
      display->drawLine(band.y + static_cast<int>(current_height_[i]), band.x, band.y + static_cast<int>(current_height_[i]), band.x + band.width - 1, ILI9341_BLACK);
    }
  }

  for (int i = 0; i < num_bands_; ++i) {
    const Band& band = bands_[i];
    if (static_cast<int>(new_height_[i]) != static_cast<int>(current_height_[i])) {
      display->drawLine(band.y + static_cast<int>(new_height_[i]), band.x, band.y + static_cast<int>(new_height_[i]), band.x + band.width - 1, band.color);
    }
    current_height_[i] = new_height_[i];
  }
}

