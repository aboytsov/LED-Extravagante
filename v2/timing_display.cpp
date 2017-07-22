#include "timing_display.h"

#include "utils.h"

TimingDisplay::TimingDisplay(NormalizedFft* fft, ILI9341_t3* display)
  : fft_(fft), display_(display), enabled_(false), pos_(0), start_bin_(1), end_bin_(1), frozen_(false) {
  memset(displayed_level_, 0, sizeof(displayed_level_));
  memset(level_, 0, sizeof(level_));
}

bool TimingDisplay::enabled() {
  return enabled_;
}

void TimingDisplay::set_enabled(bool value) {
  if (enabled_ != value) {
    enabled_ = value;
    if (enabled_) {
      Begin();
    }
  }
}

void TimingDisplay::Begin() {
  if (!enabled_) {
    return;
  }
  display_->fillScreen(ILI9341_BLACK);

  display_->setTextColor(ILI9341_WHITE);
  display_->setTextSize(1);
  for (int y = 5; y <= 95; y += 5) {
    display_->drawLine(0, 239 - 12 * y / 5, kWidth - 1, 239 - 12 * y / 5, ILI9341_WHITE);
    display_->setCursor(305, 239 - 12 * y / 5 - 3);
    display_->println(y);
  }
}

void TimingDisplay::OnFftAvailable() {
  if (!enabled_) {
    return;
  }
  level_[pos_] = min(239, static_cast<int>(fft_->read(start_bin_, end_bin_) * 240));
  pos_ = (pos_ + 1) % kWidth;
  if (!frozen_) {
    UpdateDisplay();
  }
}

void TimingDisplay::UpdateDisplay() {
  for (int x = 0; x < kWidth; ++x) {
    unsigned int new_level = level_[(pos_ + x) % kWidth];
    if (new_level != displayed_level_[x]) {
      display_->drawPixel(x, 239 - displayed_level_[x], displayed_level_[x] % 12 != 0 || displayed_level_[x] == 0 ? ILI9341_BLACK : ILI9341_WHITE);
      display_->drawPixel(x, 239 - new_level, ILI9341_RED);
      displayed_level_[x] = new_level;
    }
  }
}

void TimingDisplay::DoCommands() {
  if (!enabled_) {
    return;
  }
  if (CheckSerial('t')) {
    if (CheckSerial('b')) {
      start_bin_ = Serial.parseInt();
      Serial.read();
      end_bin_ = Serial.parseInt();

      memset(level_, 0, sizeof(level_));

      Serial.print("Will display FFT bins ");
      Serial.print(start_bin_);
      Serial.print(" to ");
      Serial.print(end_bin_);
      Serial.println(".");
    } else {
      if (frozen_) {
        Serial.println("Unfreezing");
      } else {
        Serial.println("Freezing");
      }
      frozen_ = !frozen_;
    }
  }
}

