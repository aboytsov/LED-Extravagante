#define USE_OCTOWS2811
#include <OctoWS2811.h>

#include <Audio.h>
#include <FastLED.h>
#include <ILI9341_t3.h>

#include "band_display.h"
#include "fft_display.h"
#include "idleness_detector.h"

// Audio input.
AudioInputAnalog adc(A3);
AudioAnalyzePeak peak;
AudioConnection patch_cord1(adc, peak);
AudioAnalyzeFFT1024 fft;
AudioConnection patch_cord2(adc, fft);

IdlenessDetector idleness_detector;

// Display.
ILI9341_t3 display = ILI9341_t3(10, 9);
BandDisplay band_display(&display);
FftDisplay fft_display(&fft, &band_display, /*x=*/160, /*y=*/0, /*width=*/160, /*height=*/240, /*bins=*/32, ILI9341_YELLOW);
int peak_band_id = band_display.AddBand(0, 0, 30, 240, ILI9341_RED, /*decaying=*/true);

int bass1_band_id = band_display.AddBand(30, 0, 30, 240, ILI9341_GREEN, /*decaying=*/false);
int bass2_band_id = band_display.AddBand(60, 0, 30, 240, ILI9341_BLUE, /*decaying=*/false);

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
  display.fillScreen(ILI9341_BLACK);

  FastLED.addLeds<OCTOWS2811>(leds, NUM_LEDS);
  FastLED.show();
}

void loop() {
  if (peak.available()) {
    float peak_value = peak.read();
    band_display.UpdateBand(peak_band_id, peak_value);
    idleness_detector.OnPeakAvailable(peak_value);
  }

  EVERY_N_MILLISECONDS(1000) {
    Serial.println(idleness_detector.SecondsIdle());
  }
  
  if (fft.available()) {
    fft_display.OnFftAvailable();

    float bass1 = min(1, max(0, (fft.read(1))*8));
    band_display.UpdateBand(bass1_band_id, bass1);
    float bass2 = min(1, max(0, (fft.read(3, 5) - 0.05)*6));
    band_display.UpdateBand(bass2_band_id, bass2);

//    for (int i = 0; i < NUM_LEDS; ++i) {
//      strips[0][i] = CHSV(194, 188, 255 * (0.2 + (1 - bass1) / 0.8));
//      strips[1][i] = CHSV(169, 188, 255 * (0.2 + (1 - bass2) / 0.8));
//    }
    for (int i = 0; i < NUM_LEDS; ++i) {
      if (1.0 * i / NUM_LEDS < bass1) {
        strips[0][i] = CHSV(194 * 255 / 360, 188, 255);
      } else {
        strips[0][i] = CHSV(194 * 255 / 360, 188, 0);
      }
      if (1.0 * i / NUM_LEDS < bass2) {
        strips[1][i] = CHSV(169 * 255 / 360, 188, 255);
      } else {
        strips[1][i] = CHSV(169 * 255 / 360, 188, 0);
      }
    }
  }



  FastLED.show();

  // On Teensy 3.2, this loop takes ~700ms max with 33 bands.
  band_display.Loop();
}

