#ifndef SCROLLER_H
#define SCROLLER_H

#include <mpu9255_esp32.h>
#include <WiFi.h> //Connect to WiFi Network
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h> //Used in support of TFT Display
#include <time.h>
#include <vector>

class Scroller {
public:
  int pos1, pos2;
  int length;
  int index;
  int state;
  unsigned long scrolling_timer;
  int scrolling_threshold;
  int low_threshold, high_threshold;
  bool small_extreme, large_extreme;

  Scroller (int length) {
    this->length = length;
    state = 0;
    index = 0;
    scrolling_timer = millis();
    scrolling_threshold = 200;
    low_threshold = 400;
    high_threshold = 4095;
  }

  bool update (int reading) {
    bool ret = false;
//  void update (float mean, float sd, float stab_mean, float stab_sd) {
    if (reading <= low_threshold) {
      if (small_extreme) {
        if (millis() - scrolling_timer >= scrolling_threshold) {
          //then you update!
          index = (index + length - 1) % length;
          scrolling_timer = millis();
          ret = true;
        }
      } else {
        scrolling_timer = millis();
      }
      
      small_extreme = true;
      large_extreme = false;
    } else if (reading >= high_threshold) {
      if (large_extreme) {
        if (millis() - scrolling_timer >= scrolling_threshold) {
          //then you update!
          index = (index + 1) % length;
          ret = true;
          scrolling_timer = millis();
        }
      } else {
        scrolling_timer = millis();
      }

      small_extreme = false;
      large_extreme = true;
    } else {
      small_extreme = large_extreme = false;
    }

//    if (mean <= stab_mean - 3 * stab_sd) {
//      index = (index + length - 1) % length;
//    } else if (mean >= stab_mean + 3 * stab_sd) {
//      index = (index + 1) % length;
//    }
    return ret;
  }
};

#endif
