#ifndef _IDLE_ANIMATION_H_
#define _IDLE_ANIMATION_H_

#include <FastLED.h>

class IdleAnimation {
 public:
  IdleAnimation(CRGB* strip, int num_leds);
  ~IdleAnimation();

  void OnIdle();
  void OnNotIdle();
  void DoCommands();

 private:
  uint32_t FadeDelayMillis();
  uint32_t RotationDelayMillis();
  uint32_t ReversalDelayMillis();
  CRGB& led(int i) { return strip_[i]; }
 
  CRGB* strip_;
  int num_leds_;
  bool idle_;

  float speed_up_;
  int fading_factor_;
  float pixels_per_sec_;
  uint32_t hue_rotation_secs_;
  uint32_t reversal_freq_secs_;
  uint32_t secs_to_reverse_;
  int reverse_speed_adjusting_delay_ms_;

  CEveryNMillis fade_timer_;
  CEveryNMillis move_timer_;
  CEveryNMillis reversal_timer_;
  CEveryNMillis reverse_speed_adjusting_timer_;
  float* pixel_hue_;
  float* pixel_value_;
  float hue_;
  int lead_offset_;
  int direction_;
  bool slowing_;
  bool speeding_up_;
  float speed_adjustment_factor_;
};

#endif

