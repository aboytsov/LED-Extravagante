#ifndef _BAND_DISPLAY_H_
#define _BAND_DISPLAY_H_

#include <ILI9341_t3.h>

class BandDisplay {
 public:
  BandDisplay(ILI9341_t3* display);

  int AddBand(int x, int y, int width, int height, int color, bool decaying, float decay);

  // level is from 0 to 1.
  void UpdateBand(int id, float level);

  void Loop();

 private:
   struct Band {
    int x;
    int y;
    int width;
    int height;
    int color;
    bool decaying;
    float decay;
    float current_level;
    float displayed_level;
  };

  ILI9341_t3* display_;
  int num_bands_;
  Band* bands_;
};

#endif

