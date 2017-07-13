#include <Audio.h>
#include <ILI9341_t3.h>

#include "fft_display.h"

#define PEAK_WIDTH 32

AudioInputAnalog adc(A3);
AudioAnalyzeFFT1024 fft1024;
AudioAnalyzePeak peak;
AudioConnection patch_cord1(adc, fft1024);
AudioConnection patch_cord2(adc, peak);

ILI9341_t3 display = ILI9341_t3(10, 9);

FftDisplay fft_display;

int displayed_peak_height;

void AddFftDisplayBand(int first_bin, int last_bin, int total_bins, int color, float decay) {
  FftDisplay::Band band;
  band.x = PEAK_WIDTH + (first_bin - 1) * (ILI9341_TFTHEIGHT - PEAK_WIDTH) / total_bins;
  band.y = 0;
  band.width = (last_bin - first_bin + 1) * (ILI9341_TFTHEIGHT - PEAK_WIDTH) / total_bins;
  band.height = ILI9341_TFTWIDTH;
  band.color = color;
  band.first_bin = first_bin;
  band.last_bin = last_bin;
  band.decay = decay;
  fft_display.AddBand(band);
}

void setup() {
  AudioMemory(12);

  pinMode(A3, INPUT);

  display.begin();
  display.fillScreen(ILI9341_BLACK);
  display.drawLine(0, PEAK_WIDTH - 1, ILI9341_TFTWIDTH, PEAK_WIDTH - 1, ILI9341_GREEN);


  for (int bin = 1; bin <= 36; ++bin) {
    AddFftDisplayBand(bin, bin, 36, ILI9341_YELLOW, 0.95);
  }
  AddFftDisplayBand(1, 1, 36, ILI9341_RED, 0.98);
  AddFftDisplayBand(2, 2, 36, ILI9341_NAVY, 0.98);
  AddFftDisplayBand(6, 7, 36, ILI9341_GREEN, 0.98);
  AddFftDisplayBand(13, 22, 36, ILI9341_BLUE, 0.98);
}

void loop() {
  fft_display.Update(&display, &fft1024);

  if (peak.available()) {
    int16_t height = static_cast<int16_t>(peak.read() * ILI9341_TFTWIDTH);
    if (height != displayed_peak_height) {
      display.drawLine(displayed_peak_height, 0, displayed_peak_height, PEAK_WIDTH - 2, ILI9341_BLACK);
      displayed_peak_height = height;
      display.drawLine(displayed_peak_height, 0, displayed_peak_height, PEAK_WIDTH - 2, ILI9341_GREEN);
    }
  }
}
