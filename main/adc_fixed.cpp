#include "adc_fixed.h"
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
