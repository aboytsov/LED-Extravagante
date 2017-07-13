#ifndef _BAND_DISPLAY_H_
#define _BAND_DISPLAY_H_

#include <ILI9341_t3.h>

class BandDisplay {
 public:
  BandDisplay(ILI9341_t3* display) : display_(display), num_bands_(0), bands_(new Band[0]) {}

  int AddBand(int x, int y, int width, int height, int color, bool decaying);

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
    float current_level;
    float displayed_level;
  };

  ILI9341_t3* display_;
  int num_bands_;
  Band* bands_;
};

#endif

