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
AudioInputAnalogFixed         adc(A3);
AudioAnalyzeFFT1024      fft1024;
AudioAnalyzeRMS          rms;
AudioConnection          patchCord1(adc, fft1024);
AudioConnection          patchCord2(adc, rms);

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

#define SPLASH_WIDTH 10
int base_data[SPLASH_WIDTH];
int base_data2[SPLASH_WIDTH];

// Initialize global variables for sequences
uint8_t thisdelay = 8;                                        // A delay value for the sequence(s)
uint8_t thishue = 0;                                          // Starting hue value.
uint8_t deltahue = 4;                                        // Hue change between pixels.

float Sigmoid(float x){
    return 1 / (1 + exp(-1 * x));
}

void fill_rainbow1( struct CRGB * pFirstLED, int numToFill,
                  uint8_t initialhue,
                  uint8_t deltahue,
                  uint8_t value,
                  uint8_t saturation)
{
    CHSV hsv;
    hsv.hue = initialhue;
    hsv.val = value;
    hsv.sat = saturation;
    for( int i = 0; i < numToFill; i++) {
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
  
  // Display the levels.
  if (fft1024.available()) {
    float level[18];                      // last 2 levels are base and treble
    // read the 512 FFT frequencies into 16 levels
    // music is heard in octaves, but the FFT data
    // is linear, so for the higher octaves, read
    // many FFT bins together.
    level[0] =  fft1024.read(0);
    level[1] =  fft1024.read(1);
    level[2] =  fft1024.read(2, 3);
    level[3] =  fft1024.read(4, 6);
    level[4] =  fft1024.read(7, 10);
    level[5] =  fft1024.read(11, 15);
    level[6] =  fft1024.read(16, 22);
    level[7] =  fft1024.read(23, 32);
    level[8] =  fft1024.read(33, 46);
    level[9] =  fft1024.read(47, 66);
    level[10] = fft1024.read(67, 93);
    level[11] = fft1024.read(94, 131);
    level[12] = fft1024.read(132, 184);
    level[13] = fft1024.read(185, 257);
    level[14] = fft1024.read(258, 359);
    level[15] = fft1024.read(360, 511);
    level[16] = fft1024.read(0, 10);        // base
    level[17] = fft1024.read(67, 359);     // voice
//    Serial.println("before tr");
//    Serial.println(level[16]);
//    Serial.println(level[17]);
    level[16] = Sigmoid((level[16]-0.5)*9);
    level[17] = Sigmoid((level[17]*0.8-0.5)*9);
//    Serial.println("--");
//    Serial.println(level[16]);
//    Serial.println(level[17]);

    // Populate the display.
    if (display_is_updating) {
      for (int i = 0; i < 18; ++i) {
        int new_freq_width = (int) ((ILI9341_TFTWIDTH - 10) * level[i]);
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
  EVERY_N_MILLISECONDS(thisdelay) {                           // FastLED based non-blocking routine to update/display the sequence.
    thishue++;                                                 
    //fill_rainbow1(leds, NUM_LEDS, thishue, deltahue, (int)(base_ratio * 255 + (1.0 - base_ratio) * base_data[0]), 240); 
  }

//  Serial.println(base_data[0]);
  
  if (level[16] > 0) {
    for (int i = 0; i < SPLASH_WIDTH; i++) {
      base_data[i] = max(level[16] * 256, base_data[i]);
    }
  }

  if (level[17] > 0) {
    for (int i = 0; i < SPLASH_WIDTH; i++) {
      base_data2[i] = max(level[17] * 256, base_data2[i]);
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
  
//  Serial.println("--");
//  Serial.println(base_data[0]);


//  for (int i = 0; i < SPLASH_WIDTH; i++) {
//    leds[10 + i] = CRGB(base_data[i], 0, 0);
//    leds[10 + SPLASH_WIDTH+ 10 + i] = CRGB(0, 0, base_data2[i]);
//    //leds[i] = CRGB(255, 0, 0);
//  }

  EVERY_N_MILLISECONDS(25) { 
    for (int i = 0; i < SPLASH_WIDTH; i++) {
      base_data[i] = (int)(base_data[i] * 0.85);
      base_data2[i] = (int)(base_data2[i] * 0.95);
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

    

    
//    // Light the LEDs.
//    // First strip.
//    for (int j = 0; j < ledsPerPin; ++j) {
//      if (j < ledsPerPin * level[2]) {
//        leds.setPixel(j, 0xFF0000);
//      } else {
//        leds.setPixel(j, 0x000000);
//      }
//    }
//    // Second strip.
//    for (int j = ledsPerPin; j < 2 * ledsPerPin; ++j) {
//      if (j - ledsPerPin < ledsPerPin * level[14]) {
//        leds.setPixel(j, 0x00FF00);
//      } else {
//        leds.setPixel(j, 0x000000);
//      }
//    }

    


  }
  
}


// https://github.com/atuline/FastLED-Demos/blob/master/aatemplate/aatemplate.ino
//   dots fading out slowly
// https://github.com/atuline/FastLED-Demos/blob/master/easing/easing.ino
//   good ease out example
// https://github.com/atuline/FastLED-Demos/blob/master/fill_grad/fill_grad.ino
//   nice gradient example
// https://github.com/atuline/FastLED-Demos/blob/master/juggle/juggle.ino
//   nice moving lines (too quick - make it better and it should work)
// ! https://github.com/atuline/FastLED-Demos/blob/master/lightnings/lightnings.ino
//   even better moving lines (doesn't work)
// https://github.com/atuline/FastLED-Demos/blob/master/two_sin_pal_demo/two_sin_pal_demo.ino
//   sinus waves - not bad but requires some work
