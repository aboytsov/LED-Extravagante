#define USE_OCTOWS2811
#include <OctoWS2811.h>

#include <Audio.h>
#include <FastLED.h>
#include <ILI9341_t3.h>
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>
#include <Wire.h>

#include "adc_fixed.h"

// Audio input.
AudioInputAnalogFixed    adc(A3);
AudioAnalyzeFFT1024      fft1024;
AudioAnalyzeRMS          rms;
AudioConnection          patchCord1(adc, fft1024);
AudioConnection          patchCord2(adc, rms);

float sound_level[18];                   // last 2 levels are base and treble
float &base_level = sound_level[16];
float &treble_level = sound_level[17];

// Display.
ILI9341_t3 tft = ILI9341_t3(10, 9);
bool display_is_updating = false;
int old_freq_width[18];
int old_rms_height;

#define NUM_LEDS   225
#define NUM_STRIPS 8
struct CRGB leds[NUM_LEDS * NUM_STRIPS];                                   // Initialize our LED array.

void setup() {
  Serial.begin(9600);
  pinMode(A3, INPUT);
  
  // Audio requires memory to work.
  AudioMemory(12);

  FastLED.addLeds<OCTOWS2811>(leds, NUM_LEDS);
  FastLED.show();

  // Configure the display.
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
}

int base_data[SPLASH_WIDTH];
int base_data2[SPLASH_WIDTH];

#define BASE_SIGMOID_CENTER   0.35
#define BASE_SIGMOID_TILT     13
#define TREBLE_SIGMOID_CENTER 0.45
#define TREBLE_SIGMOID_TILT   13

#define RAINBOW_SPEED 8                 // rainbow step in ms
#define RAINBOW_DELTA_HUE 4             // hue change between pixels
uint8_t rainbow_hue = 0;                // current rainbow hue

float Sigmoid(float x){
    return 1 / (1 + exp(-1 * x));
}

void fill_rainbow(struct CRGB * pFirstLED, int numToFill,
                  uint8_t initialhue,
                  uint8_t deltahue,
                  uint8_t value,
                  uint8_t saturation) {
    CHSV hsv;
    hsv.hue = initialhue;
    hsv.val = value;
    hsv.sat = saturation;
    for (int i = 0; i < numToFill; i++) {
        pFirstLED[i] = hsv;
        hsv.hue += deltahue;
    }
}

void loop() {
  // Enable the display with a serial input (i.e. any character sent as a serial input).
  EVERY_N_MILLISECONDS(100) {
    if (Serial.read() != -1) {
      display_is_updating = !display_is_updating;
    }
  }
  
  // Display the sound levels
  if (fft1024.available()) {
    // read the 512 FFT frequencies into 16 levels
    // music is heard in octaves, but the FFT data
    // is linear, so for the higher octaves, read
    // many FFT bins together.
    sound_level[0]  = fft1024.read(0);
    sound_level[1]  = fft1024.read(1);
    sound_level[2]  = fft1024.read(2, 3);
    sound_level[3]  = fft1024.read(4, 6);
    sound_level[4]  = fft1024.read(7, 10);
    sound_level[5]  = fft1024.read(11, 15);
    sound_level[6]  = fft1024.read(16, 22);
    sound_level[7]  = fft1024.read(23, 32);
    sound_level[8]  = fft1024.read(33, 46);
    sound_level[9]  = fft1024.read(47, 66);
    sound_level[10] = fft1024.read(67, 93);
    sound_level[11] = fft1024.read(94, 131);
    sound_level[12] = fft1024.read(132, 184);
    sound_level[13] = fft1024.read(185, 257);
    sound_level[14] = fft1024.read(258, 359);
    sound_level[15] = fft1024.read(360, 511);
    // base
    base_level = Sigmoid((fft1024.read(1, 2)-BASE_SIGMOID_CENTER)*BASE_SIGMOID_TILT);    
    // voice, snare, instrumentals
    treble_level = Sigmoid((fft1024.read(7, 22)-TREBLE_SIGMOID_CENTER)*TREBLE_SIGMOID_TILT);

    // Populate the display.
    if (display_is_updating) {
      for (int i = 0; i < 18; ++i) {
        int new_freq_width = (int) ((ILI9341_TFTWIDTH - 10) * sound_level[i]);
        if (new_freq_width < old_freq_width[i]) {
          tft.fillRect(new_freq_width + 1, i * ILI9341_TFTHEIGHT / 18, old_freq_width[i] - new_freq_width, ILI9341_TFTHEIGHT / 18, ILI9341_BLACK);
        } else {
          tft.fillRect(old_freq_width[i], i * ILI9341_TFTHEIGHT / 18, new_freq_width - old_freq_width[i], ILI9341_TFTHEIGHT / 18, ILI9341_RED);
        }
        old_freq_width[i] = new_freq_width;
      }

      // Disploy the total amplitude on the display.
      if (rms.available()) {
        int new_rms_height = (int) (rms.read() * ILI9341_TFTHEIGHT);
        if (new_rms_height < old_rms_height) {
          tft.fillRect(ILI9341_TFTWIDTH - 10, new_rms_height + 1, 10, old_rms_height - new_rms_height, ILI9341_BLACK);
        } else {
          tft.fillRect(ILI9341_TFTWIDTH - 10, old_rms_height, 10, new_rms_height - old_rms_height, ILI9341_RED);
        }
        old_rms_height = new_rms_height;
      }
    }

  float base_ratio = 0.4;
  EVERY_N_MILLISECONDS(RAINBOW_SPEED) {    
    fill_rainbow(leds + NUM_LEDS, NUM_LEDS, ++rainbow_hue, RAINBOW_DELTA_HUE, 
                 (int)(base_ratio * 255 + (1.0 - base_ratio) * base_data[0]), 240); 
  }

  if (sound_level[16] > 0) {
    for (int i = 0; i < SPLASH_WIDTH; i++) {
      base_data[i] = max(sound_level[16] * 256, base_data[i]);
    }
  }

  if (sound_level[17] > 0) {
    for (int i = 0; i < SPLASH_WIDTH; i++) {
      base_data2[i] = max(sound_level[17] * 256, base_data2[i]);
    }
  }

  static int num_dots = 8;
  static int gap = NUM_LEDS / num_dots;   
  static uint8_t hue = 0;
  static uint8_t offset = 0;

  for( int i = 0; i < NUM_LEDS; i++) {
    CHSV hsv;
    hsv.hue = hue;
    hsv.val = 255;
    hsv.sat = 130;
    int x = (i + offset) % gap;
    if (x == 0) {
      leds[i] = hsv;
    } else {
      int level = (int)(((float)(max(base_data2[0], 30)) / 256.0) * gap);
      if (level < x) {
        leds[i] = 0;
      } else {
        static uint8_t fadeout_step = 190 / gap;
        hsv.val = 255 - fadeout_step * (gap - level + x);
        leds[i] = hsv;
      }
    }
  }

  FastLED.show();

  EVERY_N_MILLISECONDS(300) {
    hue++;
  }

  EVERY_N_MILLISECONDS(200) {
    offset++;         // correct for LEDs number
  }
  
  EVERY_N_MILLISECONDS(25) { 
    for (int i = 0; i < SPLASH_WIDTH; i++) {
      base_data[i] = (int)(base_data[i] * 0.85);
      base_data2[i] = (int)(base_data2[i] * 0.7);
    }
  }

  EVERY_N_MILLISECONDS(500) {
    for (int i = 1; i < SPLASH_WIDTH / 2; i++) {
        base_data[i-1] = max(base_data[i-1], base_data[i]);
      }
    for (int i = SPLASH_WIDTH - 2; i >= SPLASH_WIDTH / 2; i--) {
        base_data[i+1] = max(base_data[i+1], base_data[i]);
      }
    }
  }
}
