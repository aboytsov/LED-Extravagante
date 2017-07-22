#include "band_display.h"

#include <FastLED.h>

BandDisplay::BandDisplay(ILI9341_t3* display) : display_(display), num_bands_(0), bands_(new Band[0]) {}

int BandDisplay::AddBand(int x, int y, int width, int height, int color, bool decaying) {
  int id = num_bands_;

  Band* new_bands = new Band[num_bands_ + 1];
  memcpy(new_bands, bands_, num_bands_ * sizeof(Band));
  delete[] bands_;
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
        band.current_level = max(band.current_level - 0.03, 0.0);
      }
    }
  }

  for (int i = 0; i < num_bands_; ++i) {
    Band& band = bands_[i];
    int current_y = band.y + band.height - 1 - static_cast<int>(band.current_level * (band.height - 1)); 
    int displayed_y = band.y + band.height - 1 - static_cast<int>(band.displayed_level * (band.height - 1));
    if (current_y != displayed_y) {
      display_->drawLine(band.x, displayed_y, band.x + band.width - 1, displayed_y, ILI9341_BLACK);
      display_->drawLine(band.x, current_y, band.x + band.width - 1, current_y, band.color);
      band.displayed_level = band.current_level;
    }
  }
}

