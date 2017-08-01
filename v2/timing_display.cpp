#include "timing_display.h"

#include "utils.h"

TimingDisplay::TimingDisplay(NormalizedFft* fft, ILI9341_t3* display)
  : fft_(fft), display_(display), enabled_(false), showing_both_(false), pos_(0), start_bin_{1, 2}, end_bin_{1, 2}, frozen_(false) {
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
  for (int i = 0; i < (showing_both_ ? 2 : 1); ++i) {
    level_[i][pos_] = min(239, static_cast<int>(fft_->read(start_bin_[i], end_bin_[i]) * 240));
  }
  pos_ = (pos_ + 1) % kWidth;
  if (!frozen_) {
    UpdateDisplay();
  }
}

void TimingDisplay::UpdateDisplay() {
  for (int x = 0; x < kWidth; ++x) {
    for (int i = 0; i < 2; ++i) {
      unsigned int new_level = level_[i][(pos_ + x) % kWidth];
      if (new_level != displayed_level_[i][x]) {
        display_->drawPixel(x, 239 - displayed_level_[i][x], displayed_level_[i][x] % 12 != 0 || displayed_level_[i][x] == 0 ? ILI9341_BLACK : ILI9341_WHITE);
      }
    }
    for (int i = 0; i < (showing_both_ ? 2 : 1); ++i) {
      unsigned int new_level = level_[i][(pos_ + x) % kWidth];
      if (new_level != displayed_level_[i][x]) {
        display_->drawPixel(x, 239 - new_level, i == 0 ? ILI9341_RED : ILI9341_GREEN);
        displayed_level_[i][x] = new_level;
      }
    }
  }
}

void TimingDisplay::DoCommands() {
  if (!enabled_) {
    return;
  }
  if (CheckSerial('t')) {
    if (Serial.peek() == -1) {
      if (frozen_) {
        Serial.println("Unfreezing");
      } else {
        Serial.println("Freezing");
      }
      frozen_ = !frozen_;
    } else if (CheckSerial('s')) {
      if (Serial.peek() == -1) {
        showing_both_ = false;
        Serial.println("Will only show one");
        memset(level_[1], -1, sizeof(level_[1]));
      } else {
        showing_both_ = true;
        start_bin_[1] = Serial.parseInt();
        Serial.read();
        end_bin_[1] = Serial.parseInt();

        memset(level_[1], -1, sizeof(level_[1]));

        Serial.print("Will also display FFT bins ");
        Serial.print(start_bin_[1]);
        Serial.print(" to ");
        Serial.print(end_bin_[1]);
        Serial.println(".");
      }
    } else {
      start_bin_[0] = Serial.parseInt();
      Serial.read();
      end_bin_[0] = Serial.parseInt();

      memset(level_[0], -1, sizeof(level_[0]));

      Serial.print("Will display FFT bins ");
      Serial.print(start_bin_[0]);
      Serial.print(" to ");
      Serial.print(end_bin_[0]);
      Serial.println(".");
    }
  }
}

