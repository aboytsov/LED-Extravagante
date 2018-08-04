#include "simple_display.h"

#include "utils.h"

#define HEIGHT 240
#define WIDTH 320
#define TEXT_HEIGHT 20

SimpleDisplay::SimpleDisplay(NormalizedFft* fft, ILI9341_t3* display)
  : fft_(fft), display_(display), enabled_(false),
    band_display_(display),
    fft_display_(fft, &band_display_, /*x=*/96, /*y=*/TEXT_HEIGHT, /*width=*/224, /*height=*/HEIGHT - TEXT_HEIGHT, /*bins=*/32, ILI9341_BLUE),
    rms_band_id_(band_display_.AddBand(0, TEXT_HEIGHT, 93, 220, ILI9341_RED, /*decaying=*/true, /*decay=*/0.01)) {
}

bool SimpleDisplay::enabled() {
  return enabled_;
}

void SimpleDisplay::set_enabled(bool value) {
  if (enabled_ != value) {
    enabled_ = value;
    if (enabled_) {
      Begin();
    }
  }
}

void SimpleDisplay::Begin() {
  if (!enabled_) {
    return;
  }
  display_->fillScreen(ILI9341_BLACK);
  
  // Set up header text.
  display_->setTextColor(ILI9341_WHITE);
  display_->setTextSize(1);
  display_->setCursor(35, 6);
  display_->println("Level");
  display_->setCursor(180, 6);
  display_->println("Frequencies");
  display_->drawLine(0, TEXT_HEIGHT - 1, WIDTH - 1, TEXT_HEIGHT - 1, ILI9341_WHITE);

  // Draw band line edges.
  display_->drawLine(94, TEXT_HEIGHT, 94, HEIGHT - 1, ILI9341_GREEN);
}

void SimpleDisplay::OnRmsAvailable(float rms) {
  if (!enabled_) {
    return;
  }
  band_display_.UpdateBand(rms_band_id_, rms);
}

void SimpleDisplay::OnFftAvailable() {
  if (!enabled_) {
    return;
  }
  fft_display_.OnFftAvailable();
}

void SimpleDisplay::Loop() {
  if (!enabled_) {
    return;
  }
  band_display_.Loop();
}


