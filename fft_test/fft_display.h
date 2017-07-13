#ifndef _FFT_DISPLAY_H_
#define _FFT_DISPLAY_H_

#include <Audio.h>
#include <ILI9341_t3.h>

#define MAX_FFT_DISPLAY_BANDS 1000

class FftDisplay {
  public:
    struct Band {
      int x, y, width, height;
      int color;
      int first_bin, last_bin;
      float decay;
    };
    void AddBand(Band band);

    void Update(ILI9341_t3* display, AudioAnalyzeFFT1024* fft1024);

  private:

    Band bands_[MAX_FFT_DISPLAY_BANDS];
    int num_bands_ = 0;

    float current_height_[MAX_FFT_DISPLAY_BANDS];
    float new_height_[MAX_FFT_DISPLAY_BANDS];
};

#endif
