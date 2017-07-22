#include "idle_animation.h"

#include "utils.h"

#define WIDTH 45

IdleAnimation::IdleAnimation(CRGB* strip, int num_leds)
  : strip_(strip), num_leds_(num_leds), idle_(false),
    speed_up_(1.0),
    fading_factor_(246),
    pixels_per_sec_(1.0),
    hue_rotation_secs_(120),
    reversal_freq_secs_(450),
    secs_to_reverse_(150),
    reverse_speed_adjusting_delay_ms_(30) {}
 
void IdleAnimation::OnIdle() {
  if (!idle_) {
    idle_ = true;
    for (int i = 0; i < num_leds_; ++i) {
      led(i) = CRGB(0, 0, 0);
    }
    speed_adjustment_factor_ = 1.0;
    fade_timer_.setPeriod(FadeDelayMillis());
    fade_timer_.reset();
    move_timer_.setPeriod(RotationDelayMillis());
    move_timer_.reset();
    reversal_timer_.setPeriod(ReversalDelayMillis());
    reversal_timer_.reset();
    reverse_speed_adjusting_timer_.setPeriod(reverse_speed_adjusting_delay_ms_);
    hue_ = 0;
    lead_offset_ = 0;
    direction_ = 1;
    slowing_ = false;
    speeding_up_ = false;
  }
  if (fade_timer_) {
    for (int i = 0; i < num_leds_; ++i) {
      led(i).nscale8(fading_factor_);
    }
  }
  if (move_timer_) {
    hue_ += 256.0 * move_timer_.getPeriod() / (hue_rotation_secs_ * 1000.0) * speed_up_;
    while (hue_ >= 256) {
      hue_ -= 256;
    }
    lead_offset_ = (lead_offset_ + direction_ + WIDTH) % WIDTH;
    for (int i = lead_offset_; i < num_leds_; i += WIDTH) {
      led(i) = CHSV(hue_, 255, 255);
    }
  }
  if (!(slowing_ || speeding_up_) && reversal_timer_) {
    slowing_ = true;
    reverse_speed_adjusting_timer_.reset();
  }
  if ((slowing_ || speeding_up_) && reverse_speed_adjusting_timer_) {
    float factor_adjust = 2.0 * reverse_speed_adjusting_delay_ms_ / (secs_to_reverse_ * 1000.0 / speed_up_);
    if (slowing_) {
      speed_adjustment_factor_ -= factor_adjust;
      if (speed_adjustment_factor_ <= 0) {
        slowing_ = false;
        speeding_up_ = true;
        lead_offset_ = (lead_offset_ + direction_ + WIDTH) % WIDTH;
        direction_ = -direction_;
        speed_adjustment_factor_ = -speed_adjustment_factor_;
        move_timer_.reset();
      }
    } else {
      speed_adjustment_factor_ += factor_adjust;
      if (speed_adjustment_factor_ >= 1) {
        speeding_up_ = false;
        speed_adjustment_factor_ = 1;
        reversal_timer_.reset();
      }
    }
    move_timer_.setPeriod(RotationDelayMillis());
  }
}

uint32_t IdleAnimation::FadeDelayMillis() {
  return 333.0 / speed_up_;
}

uint32_t IdleAnimation::RotationDelayMillis() {
  return 1000.0 / (speed_adjustment_factor_ * pixels_per_sec_) / speed_up_;
}

uint32_t IdleAnimation::ReversalDelayMillis() {
  return reversal_freq_secs_ * 1000.0 / speed_up_;
}

void IdleAnimation::OnNotIdle() {
  idle_ = false;
}

void IdleAnimation::DoCommands() {
  if (CheckSerial('i')) {
    if (CheckSerial('s')) {
      speed_up_ = AdjustFloat("speed_up");
      fade_timer_.setPeriod(FadeDelayMillis());
      move_timer_.setPeriod(RotationDelayMillis());
      reversal_timer_.setPeriod(ReversalDelayMillis());
      if (speed_up_ >= 10) {
        reverse_speed_adjusting_delay_ms_ = 5;
      } else {
        reverse_speed_adjusting_delay_ms_ = 30;
      }
      reverse_speed_adjusting_timer_.setPeriod(reverse_speed_adjusting_delay_ms_);
    } else if (CheckSerial('f')) {
      fading_factor_ = AdjustInt("fading_factor");
    } else if (CheckSerial('p')) {
      pixels_per_sec_ =  AdjustFloat("pixels_per_sec");
      move_timer_.setPeriod(RotationDelayMillis());
    } else if (CheckSerial('h')) {
      hue_rotation_secs_ = AdjustInt("hue_rotation_secs");
    } else if (CheckSerial('r')) {
      if (CheckSerial('f')) {
        reversal_freq_secs_ = AdjustInt("reversal_freq_secs");
        reversal_timer_.setPeriod(ReversalDelayMillis());
      } else if (CheckSerial('s')) {
        secs_to_reverse_ = AdjustInt("secs_to_reverse");
      } else if (CheckSerial('t')) {
        Serial.println("Triggering reversal");
        reversal_timer_.trigger();
      }
    }
  }
}

