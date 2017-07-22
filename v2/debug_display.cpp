#include "debug_display.h"

#define HEIGHT 240
#define WIDTH 320
#define TEXT_HEIGHT 20
#define BAND_WIDTH 30

DebugDisplay::DebugDisplay(NormalizedFft* fft, ILI9341_t3* display)
  : fft_(fft), display_(display), enabled_(false),
    band_display_(display),
    fft_display_(fft, &band_display_, /*x=*/160, /*y=*/TEXT_HEIGHT, /*width=*/160, /*height=*/HEIGHT - TEXT_HEIGHT, /*bins=*/32, ILI9341_GREEN),
    peak_band_id_(band_display_.AddBand(0, TEXT_HEIGHT, BAND_WIDTH - 1, 220, ILI9341_RED, /*decaying=*/true)),
    rms_band_id_(band_display_.AddBand(BAND_WIDTH, TEXT_HEIGHT, BAND_WIDTH / 2 - 1, 220, ILI9341_RED, /*decaying=*/true)),
    normalized_rms_band_id_(band_display_.AddBand(BAND_WIDTH + BAND_WIDTH / 2, TEXT_HEIGHT, BAND_WIDTH / 2 - 1, 220, ILI9341_RED, /*decaying=*/false)),
    band_1_id_(band_display_.AddBand(2 * BAND_WIDTH + 1, TEXT_HEIGHT, 24, HEIGHT - TEXT_HEIGHT, ILI9341_BLUE, /*decaying=*/false)),
    band_2_id_(band_display_.AddBand(2 * BAND_WIDTH + 1 + 24, TEXT_HEIGHT, 24, HEIGHT - TEXT_HEIGHT, ILI9341_GREEN, /*decaying=*/false)),
    band_3_id_(band_display_.AddBand(2 * BAND_WIDTH + 1 + 48, TEXT_HEIGHT, 24, HEIGHT - TEXT_HEIGHT, ILI9341_BLUE, /*decaying=*/false)),
    band_4_id_(band_display_.AddBand(2 * BAND_WIDTH + 1 + 72, TEXT_HEIGHT, 24, HEIGHT - TEXT_HEIGHT, ILI9341_GREEN, /*decaying=*/false)) {
}

bool DebugDisplay::enabled() {
  return enabled_;
}

void DebugDisplay::set_enabled(bool value) {
  if (enabled_ != value) {
    enabled_ = value;
    if (enabled_) {
      Begin();
    }
  }
}

void DebugDisplay::Begin() {
  if (!enabled_) {
    return;
  }
  display_->fillScreen(ILI9341_BLACK);
  
  // Set up header text.
  display_->setTextColor(ILI9341_WHITE);
  display_->setTextSize(1);
  display_->setCursor(2, 6);
  display_->println("Peak");
  display_->setCursor(2 + BAND_WIDTH, 6);
  display_->println("RMS");
  display_->setCursor(160, 6);
  display_->println("Normalized FFT");
  display_->drawLine(0, TEXT_HEIGHT - 1, WIDTH - 1, TEXT_HEIGHT - 1, ILI9341_WHITE);

  // Draw band line edges.
  display_->drawLine(BAND_WIDTH - 1, TEXT_HEIGHT, BAND_WIDTH - 1, HEIGHT - 1, ILI9341_RED);
  display_->drawLine(BAND_WIDTH + BAND_WIDTH / 2 - 1, TEXT_HEIGHT, BAND_WIDTH + BAND_WIDTH / 2 - 1, HEIGHT - 1, ILI9341_RED);
  display_->drawLine(2 * BAND_WIDTH - 1, TEXT_HEIGHT, 2 * BAND_WIDTH - 1, HEIGHT - 1, ILI9341_RED);
  display_->drawLine(158, TEXT_HEIGHT, 158, HEIGHT - 1, ILI9341_GREEN);
}

void DebugDisplay::OnRmsAvailable(float rms) {
  if (!enabled_) {
    return;
  }
  band_display_.UpdateBand(rms_band_id_, rms);
  band_display_.UpdateBand(normalized_rms_band_id_, fft_->Rms());
}

void DebugDisplay::OnPeakAvailable(float peak) {
  if (!enabled_) {
    return;
  }
  band_display_.UpdateBand(peak_band_id_, peak);
}

void DebugDisplay::OnFftAvailable() {
  if (!enabled_) {
    return;
  }
  fft_display_.OnFftAvailable();
}

void DebugDisplay::UpdateBand1(float value) {
  if (!enabled_) {
    return;
  }
  band_display_.UpdateBand(band_1_id_, value);
}

void DebugDisplay::UpdateBand2(float value) {
  if (!enabled_) {
    return;
  }
  band_display_.UpdateBand(band_2_id_, value);
}

void DebugDisplay::UpdateBand3(float value) {
  if (!enabled_) {
    return;
  }
  band_display_.UpdateBand(band_3_id_, value);
}

void DebugDisplay::UpdateBand4(float value) {
  if (!enabled_) {
    return;
  }
  band_display_.UpdateBand(band_4_id_, value);
}

void DebugDisplay::Loop() {
  if (!enabled_) {
    return;
  }
  band_display_.Loop();
}

void DebugDisplay::DoCommands() {
  if (!enabled_) {
    return;
  }
  fft_display_.DoCommands();
}

