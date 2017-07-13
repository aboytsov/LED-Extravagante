#include <Audio.h>
#include <ILI9341_t3.h>

#define PEAK_WIDTH 30

AudioInputAnalog adc(A3);
AudioRecordQueue queue;
AudioAnalyzePeak peak;
AudioConnection patch_cord1(adc, queue);
AudioConnection patch_cord2(adc, peak);

ILI9341_t3 display = ILI9341_t3(10, 9);
int16_t displayed_height[ILI9341_TFTHEIGHT - PEAK_WIDTH];
int16_t current_column = 0;

int16_t displayed_peak_height;

void setup() {
  AudioMemory(12);

  pinMode(A3, INPUT);

  display.begin();
  display.fillScreen(ILI9341_BLACK);
  display.drawLine(0, PEAK_WIDTH - 1, ILI9341_TFTWIDTH, PEAK_WIDTH - 1, ILI9341_GREEN);

  queue.begin();
}

void loop() {
  if (queue.available() > 0) {
    int16_t * buffer = queue.readBuffer();
    for (int i = 0; i < 128; ++i) {
      int16_t sample = buffer[i];
      int16_t height = (static_cast<int32_t>(sample) + 32768) * ILI9341_TFTWIDTH / 65536;
      if (height != displayed_height[current_column]) {
        display.drawPixel(displayed_height[current_column], current_column + PEAK_WIDTH, ILI9341_BLACK);
        displayed_height[current_column] = height;
        display.drawPixel(displayed_height[current_column], current_column + PEAK_WIDTH, ILI9341_RED);
      }
  
      current_column = (current_column + 1) % (ILI9341_TFTHEIGHT - PEAK_WIDTH);
    }
    queue.freeBuffer();
  }

  if (peak.available()) {
    int16_t height = static_cast<int16_t>(peak.read() * ILI9341_TFTWIDTH);
    if (height != displayed_peak_height) {
      display.drawLine(displayed_peak_height, 0, displayed_peak_height, PEAK_WIDTH - 2, ILI9341_BLACK);
      displayed_peak_height = height;
      display.drawLine(displayed_peak_height, 0, displayed_peak_height, PEAK_WIDTH - 2, ILI9341_GREEN);
    }
  }
}

