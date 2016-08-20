#define FASTLED_ALLOW_INTERRUPTS 0

#include <Adafruit_ILI9341.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include "Arduino.h"
#include "AudioStream.h"
#include "DMAChannel.h"


#include "FastLED.h"

class AudioInputAnalogFixed : public AudioStream
{
public:
        AudioInputAnalogFixed() : AudioStream(0, NULL) { init(A2); }
        AudioInputAnalogFixed(uint8_t pin) : AudioStream(0, NULL) { init(pin); }
        virtual void update(void);
        friend void dma_ch9_isr(void);
private:
        static audio_block_t *block_left;
        static uint16_t block_offset;
        static int32_t dc_average_hist[16];
        static int32_t current_dc_average_index;
        static bool update_responsibility;
  static DMAChannel dma;
  static void isr(void);
  static void init(uint8_t pin);
};

#include "utility/pdb.h"
#include "utility/dspinst.h"

DMAMEM static uint16_t analog_rx_buffer[AUDIO_BLOCK_SAMPLES];
audio_block_t * AudioInputAnalogFixed::block_left = NULL;
uint16_t AudioInputAnalogFixed::block_offset = 0;
int32_t AudioInputAnalogFixed::dc_average_hist[16];
int32_t AudioInputAnalogFixed::current_dc_average_index = 0;
bool AudioInputAnalogFixed::update_responsibility = false;
DMAChannel AudioInputAnalogFixed::dma(false);


void AudioInputAnalogFixed::init(uint8_t pin)
{
  uint32_t i, sum=0;

  // Configure the ADC and run at least one software-triggered
  // conversion.  This completes the self calibration stuff and
  // leaves the ADC in a state that's mostly ready to use
  analogReadRes(16);
  analogReference(INTERNAL); // range 0 to 1.2 volts
  analogReadAveraging(8);
  // Actually, do many normal reads, to start with a nice DC level
  for (i=0; i < 1024; i++) {
    sum += analogRead(pin);
  }
        for (i = 0; i < 16; i++) {
                dc_average_hist[i] = sum >> 10;
        }

  // set the programmable delay block to trigger the ADC at 44.1 kHz
#if defined(KINETISK)
  if (!(SIM_SCGC6 & SIM_SCGC6_PDB)
    || (PDB0_SC & PDB_CONFIG) != PDB_CONFIG
    || PDB0_MOD != PDB_PERIOD
    || PDB0_IDLY != 1
    || PDB0_CH0C1 != 0x0101) {
    SIM_SCGC6 |= SIM_SCGC6_PDB;
    PDB0_IDLY = 1;
    PDB0_MOD = PDB_PERIOD;
    PDB0_SC = PDB_CONFIG | PDB_SC_LDOK;
    PDB0_SC = PDB_CONFIG | PDB_SC_SWTRIG;
    PDB0_CH0C1 = 0x0101;
  }
#endif
  // enable the ADC for hardware trigger and DMA
  ADC0_SC2 |= ADC_SC2_ADTRG | ADC_SC2_DMAEN;

  // set up a DMA channel to store the ADC data
  dma.begin(true);
#if defined(KINETISK)
  dma.TCD->SADDR = &ADC0_RA;
  dma.TCD->SOFF = 0;
  dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
  dma.TCD->NBYTES_MLNO = 2;
  dma.TCD->SLAST = 0;
  dma.TCD->DADDR = analog_rx_buffer;
  dma.TCD->DOFF = 2;
  dma.TCD->CITER_ELINKNO = sizeof(analog_rx_buffer) / 2;
  dma.TCD->DLASTSGA = -sizeof(analog_rx_buffer);
  dma.TCD->BITER_ELINKNO = sizeof(analog_rx_buffer) / 2;
  dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
#endif
  dma.triggerAtHardwareEvent(DMAMUX_SOURCE_ADC0);
  update_responsibility = update_setup();
  dma.enable();
  dma.attachInterrupt(isr);
}


void AudioInputAnalogFixed::isr(void)
{
  uint32_t daddr, offset;
  const uint16_t *src, *end;
  uint16_t *dest_left;
  audio_block_t *left;

#if defined(KINETISK)
  daddr = (uint32_t)(dma.TCD->DADDR);
#endif
  dma.clearInterrupt();

  if (daddr < (uint32_t)analog_rx_buffer + sizeof(analog_rx_buffer) / 2) {
    // DMA is receiving to the first half of the buffer
    // need to remove data from the second half
    src = (uint16_t *)&analog_rx_buffer[AUDIO_BLOCK_SAMPLES/2];
    end = (uint16_t *)&analog_rx_buffer[AUDIO_BLOCK_SAMPLES];
    if (update_responsibility) AudioStream::update_all();
  } else {
    // DMA is receiving to the second half of the buffer
    // need to remove data from the first half
    src = (uint16_t *)&analog_rx_buffer[0];
    end = (uint16_t *)&analog_rx_buffer[AUDIO_BLOCK_SAMPLES/2];
  }
  left = block_left;
  if (left != NULL) {
    offset = block_offset;
    if (offset > AUDIO_BLOCK_SAMPLES/2) offset = AUDIO_BLOCK_SAMPLES/2;
    dest_left = (uint16_t *)&(left->data[offset]);
    block_offset = offset + AUDIO_BLOCK_SAMPLES/2;
    do {
      *dest_left++ = *src++;
    } while (src < end);
  }
}



void AudioInputAnalogFixed::update(void)
{
  audio_block_t *new_left=NULL, *out_left=NULL;
        uint32_t i, dc, offset;
  int32_t tmp;
  int16_t s, *p, *end;

  //Serial.println("update");

  // allocate new block (ok if NULL)
  new_left = allocate();

  __disable_irq();
  offset = block_offset;
  if (offset < AUDIO_BLOCK_SAMPLES) {
    // the DMA didn't fill a block
    if (new_left != NULL) {
      // but we allocated a block
      if (block_left == NULL) {
        // the DMA doesn't have any blocks to fill, so
        // give it the one we just allocated
        block_left = new_left;
        block_offset = 0;
        __enable_irq();
         //Serial.println("fail1");
      } else {
        // the DMA already has blocks, doesn't need this
        __enable_irq();
        release(new_left);
         //Serial.print("fail2, offset=");
         //Serial.println(offset);
      }
    } else {
      // The DMA didn't fill a block, and we could not allocate
      // memory... the system is likely starving for memory!
      // Sadly, there's nothing we can do.
      __enable_irq();
       //Serial.println("fail3");
    }
    return;
  }
  // the DMA filled a block, so grab it and get the
  // new block to the DMA, as quickly as possible
  out_left = block_left;
  block_left = new_left;
  block_offset = 0;
  __enable_irq();

  // Find and subtract DC offset... We use an average of the
        // last 16 * AUDIO_BLOCK_SAMPLES samples.
        dc = 0;
        for (i = 0; i < 16; i++) {
          dc += dc_average_hist[i];
        }
        dc /= 16 * AUDIO_BLOCK_SAMPLES;
        dc_average_hist[current_dc_average_index] = 0;
  p = out_left->data;
  end = p + AUDIO_BLOCK_SAMPLES;
  do {
                dc_average_hist[current_dc_average_index] += (uint16_t)(*p);
    tmp = (uint16_t)(*p) - (int32_t)dc;
    s = signed_saturate_rshift(tmp, 16, 0);
    *p++ = s;
  } while (p < end);
        current_dc_average_index = (current_dc_average_index + 1) % 16;

  // then transmit the AC data
  transmit(out_left);
  release(out_left);
}

// Audio input.
AudioInputAnalogFixed         adc(A3);
AudioAnalyzeFFT1024      fft1024;
AudioAnalyzeRMS          rms;
AudioConnection          patchCord1(adc, fft1024);
AudioConnection          patchCord2(adc, rms);

// Display.
#define TFT_DC 9
#define TFT_CS 10
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

#define NUM_LEDS   225
#define PIN 2
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB

struct CRGB leds[NUM_LEDS];                                   // Initialize our LED array.

void setup() {
  Serial.begin(9600);
  pinMode(A3, INPUT);
  
  // Audio requires memory to work.
  AudioMemory(12);

  FastLED.addLeds<LED_TYPE, PIN, COLOR_ORDER>(leds, NUM_LEDS); 
  show_at_max_brightness_for_power();

  // Configure the display.
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
}

float level[18];                      // last 2 levels are base and treble
int old_freq_width[18];
int old_rms_height;

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
  // Display the levels.
  if (fft1024.available()) {
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
    FastLED.show();
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
