#ifndef ADC_FIXED_H_
#define ADC_FIXED_H_

#include "Arduino.h"
#include "AudioStream.h"
#include "DMAChannel.h"

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

#endif
