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
    fft_distribution_display.OnFftAvailable();

    float bass1 = min(1, max(0, (normalized_fft.read(1))*8));
    debug_display.UpdateBand1(normalized_fft.read(1));
    float bass2 = min(1, max(0, (normalized_fft.read(3, 5) - 0.05)*6));
    debug_display.UpdateBand2(normalized_fft.read(1));
    debug_display.UpdateBand3(normalized_fft.read(1));
    debug_display.UpdateBand4(normalized_fft.read(1));

    for (int i = 0; i < NUM_LEDS; ++i) {
      if (idleness_detector.SecondsIdle() < 60) { 
        strips[0][i] = CHSV(194, 188, 255 * (0.2 + (1 - bass1) / 0.8));
      }
      strips[1][i] = CHSV(169, 188, 255 * (0.2 + (1 - bass2) / 0.8));
    }
  }

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
    debug_display.Loop();
  }

  {
    Profile p("DoCommands");
    debug_display.DoCommands();
    fft_distribution_display.DoCommands();
    idle_animation.DoCommands();
    if (CheckSerial('s')) {
      if (debug_display.enabled()) {
        debug_display.set_enabled(false);
        fft_distribution_display.set_enabled(true);
      } else {
        debug_display.set_enabled(true);
        fft_distribution_display.set_enabled(false);
      }
    }
    while (Serial.read() != -1);
  }
}

