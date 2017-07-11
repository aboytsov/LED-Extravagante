#define USE_OCTOWS2811
#include <OctoWS2811.h>

#include <Audio.h>
#include <FastLED.h>
#include <ILI9341_t3.h>
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>
#include <Wire.h>

// Audio input.
AudioInputAnalog    adc(A3);
AudioAnalyzeFFT1024      fft1024;
AudioAnalyzeRMS          rms;
AudioConnection          patchCord1(adc, fft1024);
AudioConnection          patchCord2(adc, rms);

float sound_level[18];                   // last 2 levels are base and treble
float &base_level = sound_level[16];
float &treble_level = sound_level[17];
float base_level_smoothed;
float treble_level_smoothed;

// Display.
ILI9341_t3 tft = ILI9341_t3(10, 9);
bool display_is_updating = false;
int old_freq_width[18];
int old_rms_height;

#define NUM_LEDS   225
#define NUM_STRIPS 8
struct CRGB leds[NUM_LEDS * NUM_STRIPS];                                   // Initialize our LED array.

#define NUM_BASS_STRIPS 4
CRGB* bass_strips[NUM_BASS_STRIPS] = {leds + 1 * NUM_LEDS, leds + 2 * NUM_LEDS, leds + 5 * NUM_LEDS, leds + 6 * NUM_LEDS};

#define NUM_TREBLE_STRIPS 2
CRGB* treble_strips[NUM_TREBLE_STRIPS] = {leds + 0 * NUM_LEDS, leds + 4 * NUM_LEDS};

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

#define BASE_SIGMOID_CENTER   0.35
#define BASE_SIGMOID_TILT     13
#define TREBLE_SIGMOID_CENTER 0.7
#define TREBLE_SIGMOID_TILT   3
#define BASE_LEVEL_DECAY      0.88
#define TREBLE_LEVEL_DECAY    0.95

#define RAINBOW_SPEED 20                                     // rainbow step in ms
#define RAINBOW_DELTA_HUE (256.0 * 3 / NUM_LEDS)             // hue change between pixels
#define RAINBOW_WITH_SOUND_BASE_LEVEL 0.2                    // rainbow base level brightness when there is bass.
#define RAINBOW_NO_SOUND_BASE_LEVEL 0.5                      // rainbow base level brightness when there is no bass for a while.
#define RAINBOW_BASE_LEVEL_INCREASE_SECS 20
int no_bass_millis = 0;
uint8_t rainbow_hue = 0;                                     // current rainbow hue

#define NUM_DOTS 8
#define GAP_WIDTH (NUM_LEDS / NUM_DOTS)
#define DOTS_SATURATION 160
#define DOTS_MIN_BRIGHTNESS 0.15
#define DOTS_FADEOUT 100
#define DOTS_MIN_HUE_REMOVE 25          // Aqua
#define DOTS_MAX_HUE_REMOVE 65          // Purple
uint8_t dots_hue = 0;                   // current hue for treble dots
#define DOTS_HUE_CHANGE_SPEED 200

int dots_offset = 0;                    // current offset for treble dots
#define DOTS_SPEED 300                  // dots speed (step delay in ms)

float Sigmoid(float x){
    return 1 / (1 + exp(-1 * x));
}

uint8_t toColor(float x) {
  return (int)(x * 255);
}

void fill_rainbow(struct CRGB * strips[], int num_strips,
                  int numToFill,
                  uint8_t initialhue,
                  float deltahue,
                  uint8_t value,
                  uint8_t saturation) {
    float hue = initialhue;
    CHSV hsv;
    hsv.val = value;
    hsv.sat = saturation;
    for (int i = 0; i < numToFill; i++) {
        hsv.hue = (int) (hue + 0.5);
        for (int j = 0; j < num_strips; j++) {
          strips[j][i] = hsv;
        }
        hue += deltahue;
    }
}

void loop() {
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
    treble_level = Sigmoid((fft1024.read(7, 22) * 0.7 + fft1024.read(23, 66) * 0.3  -TREBLE_SIGMOID_CENTER)*TREBLE_SIGMOID_TILT);

    base_level_smoothed = max(base_level, base_level_smoothed);
    treble_level_smoothed = max(treble_level, treble_level_smoothed);

    // Populate the display.
    // Enable the display with a serial input (i.e. any character sent as a serial input).
//    EVERY_N_MILLISECONDS(100) {
//      if (Serial.read() != -1) {
//        display_is_updating = !display_is_updating;
//      }
//    }
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
  }

  EVERY_N_MILLISECONDS(25) { 
    base_level_smoothed *= BASE_LEVEL_DECAY;
    treble_level_smoothed *= TREBLE_LEVEL_DECAY;
  }

  // Rainbow (bass)
  
  EVERY_N_MILLISECONDS(RAINBOW_SPEED) {    
    ++rainbow_hue;
  }
  // When the bass stops, increase to a higher brightness. However, when there is bass we want more contrast.
  EVERY_N_MILLISECONDS(5) {
    if (base_level < 0.2) {
      no_bass_millis = min(RAINBOW_BASE_LEVEL_INCREASE_SECS * 1000, no_bass_millis + 5);
    } else if (base_level < 0.6) {
      no_bass_millis = max(no_bass_millis - 30, 0);
    } else {
      no_bass_millis = 0;
    }
  }
  float adjusted_rainbow_base_level = RAINBOW_WITH_SOUND_BASE_LEVEL + (RAINBOW_NO_SOUND_BASE_LEVEL - RAINBOW_WITH_SOUND_BASE_LEVEL) * (no_bass_millis / (RAINBOW_BASE_LEVEL_INCREASE_SECS * 1000.0));
  fill_rainbow(bass_strips,
               NUM_BASS_STRIPS, 
               NUM_LEDS, 
               rainbow_hue, 
               RAINBOW_DELTA_HUE, 
               toColor(adjusted_rainbow_base_level + (1.0 - adjusted_rainbow_base_level) * base_level_smoothed), 
               240);

  // Dots (treble)

  for (int i = 0; i < NUM_LEDS; i++) {
    int x = (i + dots_offset) % GAP_WIDTH;
    int level = (int)(max(treble_level_smoothed, DOTS_MIN_BRIGHTNESS) * GAP_WIDTH);
    for (int j = 0; j < NUM_TREBLE_STRIPS; ++j) {
      if (x > level) {
        treble_strips[j][i] = 0;
      } else {
        treble_strips[j][i] = CHSV(dots_hue, DOTS_SATURATION, 255 - 10 * (GAP_WIDTH - x + level));
      }
    }
  }

  EVERY_N_MILLISECONDS(DOTS_HUE_CHANGE_SPEED) {
    if (++dots_hue == DOTS_MIN_HUE_REMOVE) dots_hue = DOTS_MAX_HUE_REMOVE;
  }

  EVERY_N_MILLISECONDS(DOTS_SPEED) {
    dots_offset = (dots_offset + 1) % NUM_LEDS;
  }

  FastLED.show();
}
