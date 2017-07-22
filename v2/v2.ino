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
DebugDisplay debug_display(&normalized_fft, &display);
TimingDisplay timing_display(&normalized_fft, &display);
FftDistributionDisplay fft_distribution_display(&normalized_fft, &display);

// LEDs.
#define NUM_LEDS 150
#define NUM_STRIPS 8
CRGB leds[NUM_LEDS * NUM_STRIPS];
CRGB* strips[6] = {leds, leds + 1 * NUM_LEDS, leds + 2 * NUM_LEDS, leds + 3 * NUM_LEDS, leds + 4 * NUM_LEDS, leds + 5 * NUM_LEDS};
IdleAnimation idle_animation(strips[0], NUM_LEDS);

float Sigmoid(float x){
    return 1 / (1 + exp(-1 * x));
}

void setup() {
  // EnableProfiling();
  Serial.setTimeout(200);

  pinMode(A3, INPUT);
  AudioMemory(12);

  display.begin();
  display.setRotation(1);
  
  debug_display.set_enabled(true);

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
    debug_display.OnRmsAvailable(rms_value);
  }

  if (peak.available()) {
    Profile p("OnPeakAvailable");
    float peak_value = peak.read();
    debug_display.OnPeakAvailable(peak_value);
  }

  if (normalized_fft.available()) {
    // FFT is available every 12ms or so. It takes ~700ms to compute the FFT.
    // Note that the FFT computation is done in an interrupt, so it could occur
    // at any time during the execution of loop().
    Profile p("OnFftAvailable");
    debug_display.OnFftAvailable();
    timing_display.OnFftAvailable();
    fft_distribution_display.OnFftAvailable();

    float bass1 = min(1, max(0, (normalized_fft.read(1) - 0.07)*13));
    debug_display.UpdateBand1(bass1);
    float bass2 = min(1, max(0, (normalized_fft.read(4, 7) - 0.15)*4)) - 0.2 * bass1;
    float mid = min(1, max(0, (normalized_fft.read(12, 31) - 0.15)*4)) - 0.1 * bass1 - 0.05 * bass2;
    debug_display.UpdateBand2(bass1);
    debug_display.UpdateBand3(bass2);
    debug_display.UpdateBand4(bass2);

    // TODO: For higher need more sensititivy at the bottom of the range.

    for (int i = 0; i < NUM_LEDS; ++i) {
      if (idleness_detector.SecondsIdle() < 60) { 
        strips[0][i] = CHSV(194, 188, static_cast<int>(255 * (0.3 + 0.7 * bass1)));
      }
      strips[1][i] = CHSV(169, 188, 255 * (0.3 +  0.7 * bass2));
      strips[2][i] = CHSV(110, 188, 255 * (0.3 +  0.7 * mid));
    }
//    for (int i = 0; i < NUM_LEDS; ++i) {
//      if (i % 2 == 0) {
//        strips[0][i] = strips[1][i] = strips[2][i] = CHSV(194, 188, static_cast<int>(255 * (0.3 + 0.7 * bass1)));
//      } else {
//        strips[0][i] = strips[1][i] = strips[2][i] = CHSV(194, 188, static_cast<int>(255 * (0.3 + 0.7 * bass2)));
//      }
//    }
  }

//  if (idleness_detector.SecondsIdle() >= 60) {
//    Profile p("IdleAnimationOnIdle");
//    idle_animation.OnIdle();
//  } else {
//    idle_animation.OnNotIdle();
//  }

  {
    Profile p("FastLedShow");
    FastLED.show();
  }

  {
    Profile p("DebugDisplayLoop");
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
      if (debug_display.enabled()) {
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

