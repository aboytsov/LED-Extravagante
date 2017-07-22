#include "fft_distribution_display.h"

#include <FastLED.h>

#include "utils.h"

#define TEXT_HEIGHT 20

FftDistributionDisplay::FftDistributionDisplay(NormalizedFft* fft, ILI9341_t3* display)
  : fft_(fft), display_(display), enabled_(false), period_(0), start_bin_(0), end_bin_(0) {
  memset(counts_, 0, sizeof(counts_));
  memset(previous_position_, 0, sizeof(previous_position_));
}

bool FftDistributionDisplay::enabled() {
  return enabled_;
}

void FftDistributionDisplay::set_enabled(bool value) {
  if (enabled_ != value) {
    enabled_ = value;
    if (enabled_) {
      Begin();
    }
  }
}

void FftDistributionDisplay::Begin() {
  if (!enabled_) {
    return;
  }

  display_->fillScreen(ILI9341_BLACK);

  display_->setTextColor(ILI9341_WHITE);
  display_->setTextSize(1);
  for (int i = 0; i < 20; ++i) {
    int x = i * 320 / 20;
    display_->drawLine(x, 0, x, 239 - TEXT_HEIGHT, ILI9341_WHITE);
    display_->drawLine(0, 240 - TEXT_HEIGHT, 319, 240 - TEXT_HEIGHT, ILI9341_WHITE);
    if (i != 0) {
      display_->setCursor(x - 5, 240 - TEXT_HEIGHT + 6);
      display_->println(i * 5);
    }
  }
}

void FftDistributionDisplay::OnFftAvailable() {
  if (!enabled_) {
    return;
  }

  float value = fft_->read(start_bin_, end_bin_);
  int i = min(319, static_cast<int>(320 * value));
  ++counts_[period_][i];

  EVERY_N_MILLISECONDS(1000) {
    unsigned long aggregate[320];
    memset(aggregate, 0, sizeof(aggregate));
    for (int i = 0; i < kSeconds; ++i) {
      for (int j = 0; j < 320; ++j) {
         aggregate[j] += counts_[i][j];
      }
    }
    for (int i = 318; i >= 0; i--) {
      aggregate[i] = aggregate[i] + aggregate[i + 1];
    }
    for (int i = 319; i >= 0; i--) {
      aggregate[i] = 240 - TEXT_HEIGHT - 1 - (240 - TEXT_HEIGHT - 1) * aggregate[i] / aggregate[0];
    }
    for (int i = 0; i < 320; ++i) {
      if (i % 16 != 0 && aggregate[i] != previous_position_[i]) {
        display_->drawPixel(i, previous_position_[i], ILI9341_BLACK);
        display_->drawPixel(i, aggregate[i], ILI9341_RED);
        previous_position_[i] = aggregate[i];
      }
    }

    period_ = (period_ + 1) % kSeconds;
    memset(counts_[period_], 0, sizeof(counts_[period_]));
  }
}

void FftDistributionDisplay::DoCommands() {
  if (!enabled_) {
    return;
  }

  if (CheckSerial('f')) {
    start_bin_ = Serial.parseInt();
    Serial.read();
    end_bin_ = Serial.parseInt();

    memset(counts_, 0, sizeof(counts_));
    period_ = 0;

    Serial.print("Will display FFT bins ");
    Serial.print(start_bin_);
    Serial.print(" to ");
    Serial.print(end_bin_);
    Serial.println(".");
  }
}

