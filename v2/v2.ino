#define USE_OCTOWS2811
#include <OctoWS2811.h>

#include <Audio.h>
#include <FastLED.h>
#include <ILI9341_t3.h>

#include "debug_display.h"
#include "fft_distribution_display.h"
#include "idleness_detector.h"
#include "normalized_fft.h"

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
DebugDisplay debug_display(&normalized_fft, &display, /*enabled=*/false);
FftDistributionDisplay fft_distribution_display(&normalized_fft, &display, /*enabled=*/true);

// LEDs.
#define NUM_LEDS 225
#define NUM_STRIPS 8
CRGB leds[NUM_LEDS * NUM_STRIPS];
CRGB* strips[6] = {leds, leds + 1 * NUM_LEDS, leds + 2 * NUM_LEDS, leds + 3 * NUM_LEDS, leds + 4 * NUM_LEDS, leds + 5 * NUM_LEDS};

float Sigmoid(float x){
    return 1 / (1 + exp(-1 * x));
}

void setup() {
  pinMode(A3, INPUT);
  AudioMemory(12);

  display.begin();
  display.setRotation(1);
  display.fillScreen(ILI9341_BLACK);
  debug_display.Begin();
  fft_distribution_display.Begin();

  FastLED.addLeds<OCTOWS2811>(leds, NUM_LEDS);
  FastLED.show();
}

void loop() {
  if (rms.available()) {
    float rms_value = rms.read();
    idleness_detector.OnRmsAvailable(rms_value);
    normalized_fft.OnRmsAvailable(rms_value);
    debug_display.OnRmsAvailable(rms_value);
  }

  if (peak.available()) {
    float peak_value = peak.read();
    debug_display.OnPeakAvailable(peak_value);
  }

  if (normalized_fft.available()) {
    debug_display.OnFftAvailable();
    fft_distribution_display.OnFftAvailable();

    float bass1 = min(1, max(0, (normalized_fft.read(1))*8));
    debug_display.UpdateBand1(normalized_fft.read(1));
    float bass2 = min(1, max(0, (normalized_fft.read(3, 5) - 0.05)*6));
    debug_display.UpdateBand2(normalized_fft.read(2));
    debug_display.UpdateBand3(normalized_fft.read(3));
    debug_display.UpdateBand4(normalized_fft.read(4));

//    for (int i = 0; i < NUM_LEDS; ++i) {
//      strips[0][i] = CHSV(194, 188, 255 * (0.2 + (1 - bass1) / 0.8));
//      strips[1][i] = CHSV(169, 188, 255 * (0.2 + (1 - bass2) / 0.8));
//    }
//    for (int i = 0; i < NUM_LEDS; ++i) {
//      if (1.0 * i / NUM_LEDS < bass1) {
//        strips[0][i] = CHSV(194 * 255 / 360, 188, 255);
//      } else {
//        strips[0][i] = CHSV(194 * 255 / 360, 188, 0);
//      }
//      if (1.0 * i / NUM_LEDS < bass2) {
//        strips[1][i] = CHSV(169 * 255 / 360, 188, 255);
//      } else {
//        strips[1][i] = CHSV(169 * 255 / 360, 188, 0);
//      }
//    }
  }

  FastLED.show();

  // On Teensy 3.2, this loop takes ~700ms max with 33 bands.
  debug_display.Loop();
}

