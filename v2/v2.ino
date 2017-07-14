#define USE_OCTOWS2811
#include <OctoWS2811.h>

#include <Audio.h>
#include <FastLED.h>
#include <ILI9341_t3.h>

#include "band_display.h"
#include "fft_display.h"

// Audio input.
AudioInputAnalog adc(A3);
AudioAnalyzeFFT1024 fft;
AudioConnection patch_cord1(adc, fft);
AudioAnalyzePeak peak;
AudioConnection patch_cord2(adc, peak);

// Display.
ILI9341_t3 display = ILI9341_t3(10, 9);
BandDisplay band_display(&display);
FftDisplay fft_display(&fft, &band_display, /*x=*/160, /*y=*/0, /*width=*/160, /*height=*/240, /*bins=*/32, ILI9341_YELLOW);
int peak_band_id = band_display.AddBand(0, 0, 30, 240, ILI9341_RED, /*decaying=*/true);

// LEDs.
#define NUM_LEDS 225
#define NUM_STRIPS 8
CRGB leds[NUM_LEDS * NUM_STRIPS];

void setup() {
  pinMode(A3, INPUT);
  AudioMemory(12);

  display.begin();
  display.fillScreen(ILI9341_BLACK);

  FastLED.addLeds<OCTOWS2811>(leds, NUM_LEDS);
  FastLED.show();
}

void loop() {
  if (fft.available()) {
    fft_display.OnFftAvailable();
  }

  if (peak.available()) {
    band_display.UpdateBand(peak_band_id, peak.read());
  }

  // On Teensy 3.2, this loop takes ~700ms max with 33 bands.
  band_display.Loop();
}

