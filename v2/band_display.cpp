#include "band_display.h"

#include <FastLED.h>

int BandDisplay::AddBand(int x, int y, int width, int height, int color, bool decaying) {
  int id = num_bands_;

  Band* new_bands = new Band[num_bands_ + 1];
  memcpy(new_bands, bands_, num_bands_ * sizeof(Band));
  bands_ = new_bands;
  ++num_bands_;

  Band& band = bands_[id];
  band.x = x;
  band.y = y;
  band.width = width;
  band.height = height;
  band.color = color;
  band.decaying = decaying;
  band.current_level = 0;
  band.displayed_level = 1;  // Will force draw.

  return id;
}

void BandDisplay::UpdateBand(int id, float level) {
  Band& band = bands_[id];
  if (band.decaying) {
    band.current_level = max(band.current_level, level);
  } else {
    band.current_level = level;
  }
}

void BandDisplay::Loop() {
  EVERY_N_MILLISECONDS(10) {
    for (int i = 0; i < num_bands_; ++i) {
      Band& band = bands_[i];
      if (band.decaying) {
        band.current_level = 0.97 * band.current_level;
      }
    }
  }

  for (int i = 0; i < num_bands_; ++i) {
    Band& band = bands_[i];
    int current_y = static_cast<int>(band.current_level * band.height) + band.y; 
    int displayed_y = static_cast<int>(band.displayed_level * band.height) + band.y;
    if (current_y != displayed_y) {
      display_->drawLine(displayed_y, band.x, displayed_y, band.x + band.width - 1, ILI9341_BLACK);
      display_->drawLine(current_y, band.x, current_y, band.x + band.width - 1, band.color);
      band.displayed_level = band.current_level;
    }
  }
}

