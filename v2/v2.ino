#define USE_OCTOWS2811
#include <OctoWS2811.h>

#include <Audio.h>
#include <FastLED.h>
#include <ILI9341_t3.h>

#include "debug_display.h"
#include "fft_distribution_display.h"
#include "idle_animation.h"
#include "idleness_detector.h"
#include "normalized_fft.h"
#include "profiler.h"
#include "simple_display.h"
#include "timing_display.h"
#include "utils.h"

// Audio input.
AudioInputAnalog adc(A3);
AudioAnalyzeRMS rms;
AudioConnection patch_cord1(adc, rms);
AudioAnalyzePeak peak;
AudioConnection patch_cord2(adc, peak);
AudioAnalyzeFFT1024 fft;
AudioConnection patch_cord3(adc, fft);
IdlenessDetector idleness_detector;
NormalizedFft normalized_fft(&fft);

// Display.
ILI9341_t3 display = ILI9341_t3(10, 9);
SimpleDisplay simple_display(&normalized_fft, &display);
DebugDisplay debug_display(&normalized_fft, &display);
TimingDisplay timing_display(&normalized_fft, &display);
FftDistributionDisplay fft_distribution_display(&normalized_fft, &display);

// LEDs.
#define NUM_LEDS 220
#define NUM_STRIPS 8
CRGB leds[NUM_LEDS * NUM_STRIPS];
CRGB* strips[] = {leds, leds + 1 * NUM_LEDS, leds + 2 * NUM_LEDS, leds + 4 * NUM_LEDS, leds + 5 * NUM_LEDS, leds + 6 * NUM_LEDS};
IdleAnimation idle_animation(strips[5], NUM_LEDS);
CRGB* bass1_strips[] = {strips[0], strips[1], strips[2], strips[3], strips[4]};
CRGB* bass2_strips[] = {strips[5]};

void setup() {
  // EnableProfiling();
  Serial.setTimeout(200);

  pinMode(A3, INPUT);
  AudioMemory(12);

  display.begin();
  display.setRotation(1);
  
  simple_display.set_enabled(true);

  FastLED.addLeds<OCTOWS2811>(leds, NUM_LEDS);
  FastLED.show();
}

void loop() {
  Profile p("Loop");
  if (rms.available()) {
    Profile p("OnRmsAvailable");
    float rms_value = rms.read();
    idleness_detector.OnRmsAvailable(rms_value);
    normalized_fft.OnRmsAvailable(rms_value);
    simple_display.OnRmsAvailable(rms_value);
    debug_display.OnRmsAvailable(rms_value);
  }

  if (peak.available()) {
    Profile p("OnPeakAvailable");
    float peak_value = peak.read();
    debug_display.OnPeakAvailable(peak_value);
  }

  static float bass1 = 0;
  static float bass2 = 0;
  if (normalized_fft.available()) {
    // FFT is available every 12ms or so. It takes ~700ms to compute the FFT.
    // Note that the FFT computation is done in an interrupt, so it could occur
    // at any time during the execution of loop().
    Profile p("OnFftAvailable");
    simple_display.OnFftAvailable();
    debug_display.OnFftAvailable();
    timing_display.OnFftAvailable();
    fft_distribution_display.OnFftAvailable();

    bass1 = (normalized_fft.read(1, 2) - 0.05) / 0.35;
    bass1 = max(0.0, bass1);
    bass1 = min(1.0, bass1);
    if (bass1 <= 0.5) {
      bass1 = bass1 / 0.5 * 0.7;
    } else {
      bass1 = (bass1 - 0.5) / (1 - 0.5) * (1 - 0.7) + 0.7;
    }
    debug_display.UpdateBand1(bass1);

    bass2 = (normalized_fft.read(4, 7) - 0.1) / 0.25;
    bass2 = max(0.0, bass2);
    bass2 = min(1.0, bass2);
    if (bass2 <= 0.5) {
      bass2 = bass2 / 0.5 * 0.7;
    } else {
      bass2 = (bass2 - 0.5) / (1 - 0.5) * (1 - 0.7) + 0.7;
    }
    debug_display.UpdateBand2(bass2);
  }

  {
    Profile p("ComputeLEDs");
    static const float kWithSoundMinBrightness = 0.2;
    static const float kIdleBrightness = 0.6;
    float min_brightness = min(idleness_detector.SecondsIdle() / 60, 1) * (kIdleBrightness - kWithSoundMinBrightness) + kWithSoundMinBrightness;

    static float bass1_smoothed = 0;
    static float bass2_smoothed = 0;
    bass1_smoothed = max(bass1_smoothed, bass1);
    bass2_smoothed = max(bass2_smoothed, bass2);

    static const float kBass1Decay = 0.83;
    static const float kBass2Decay = 0.75;
    EVERY_N_MILLISECONDS(25) {
      bass1_smoothed *= kBass1Decay;
      bass2_smoothed *= kBass2Decay;
    }

    for (CRGB* strip : bass1_strips) {
      for (int i = 0; i < NUM_LEDS; ++i) {
        strip[i] = CHSV(160, 255, 255 * (min_brightness + bass1_smoothed * (1 - min_brightness)));
      }
    }

    for (CRGB* strip : bass2_strips) {
      for (int i = 0; i < NUM_LEDS; ++i) {
        strip[i] = CHSV(250, 255, 255 * (min_brightness + bass2_smoothed * (1 - min_brightness)));
      }
    }
  }

  // Idle animation overrides one of the strips if enabled.
  if (idleness_detector.SecondsIdle() >= 60) {
    Profile p("IdleAnimationOnIdle");
    idle_animation.OnIdle();
  } else {
    idle_animation.OnNotIdle();
  }

  {
    Profile p("FastLedShow");
    FastLED.show();
  }

  {
    Profile p("DebugDisplayLoop");
    simple_display.Loop();
    debug_display.Loop();
  }

  {
    Profile p("DoCommands");
    idleness_detector.DoCommands();  // Prefix l.
    normalized_fft.DoCommands();  // Prefix n.
    debug_display.DoCommands();  // Prefix d.
    timing_display.DoCommands();  // Prefix t.
    fft_distribution_display.DoCommands();  // Prefix f.
    idle_animation.DoCommands();  // Prefix i.
    if (CheckSerial('s')) {
      if (simple_display.enabled()) {
        simple_display.set_enabled(false);
        debug_display.set_enabled(true);
      } else if (debug_display.enabled()) {
        debug_display.set_enabled(false);
        timing_display.set_enabled(true);
      } else if (timing_display.enabled()) {
        timing_display.set_enabled(false);
        fft_distribution_display.set_enabled(true);
      } else {
        fft_distribution_display.set_enabled(false);
        debug_display.set_enabled(true);
      }
    }
    while (Serial.read() != -1);
  }
}

