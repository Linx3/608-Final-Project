#ifndef _TIC_TAC_TOE_SCROLLER_H
#define _TIC_TAC_TOE_SCROLLER_H

#include <mpu9255_esp32.h>
#include <WiFi.h> //Connect to WiFi Network
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h> //Used in support of TFT Display
#include <time.h>
#include <vector>
#include <math.h>

class Button{
public:
  uint32_t t_of_state_2;
  uint32_t t_of_button_change;    
  uint32_t debounce_time;
  uint32_t long_press_time;
  uint8_t pin;
  uint8_t flag;
  bool button_pressed;
  uint8_t state; // This is public for the sake of convenience
  Button(int p) {
  flag = 0;  
    state = 0;
    pin = p;
    t_of_state_2 = millis(); //init
    t_of_button_change = millis(); //init
    debounce_time = 10;
    long_press_time = 500;
    button_pressed = 0;
  }
  void read() {
    uint8_t button_state = digitalRead(pin);  
    button_pressed = !button_state;
  }
  
  int update() {
    flag = 0;
    read();
    if (state==0) {
      if (button_pressed) {
        state = 1;
        // flag = 0; //right? it's changed.
        t_of_button_change = millis();  //originally set to 0 here???
        Serial.printf("t_of_button_change = %d\n", t_of_button_change);
      }
    } else if (state==1) {
      if (button_pressed) {
        if (millis() - t_of_button_change >= debounce_time) {
          Serial.printf("millis() - t_of_button_change >= debounce_time\n");
          state = 2;
         // flag = 0;
          t_of_state_2 = millis();
        }
      } else {
        state = 0;
       // flag = 0; //it's only been debounced
        t_of_button_change = millis();
      }
    } else if (state==2) {
      if (button_pressed) {
       // flag = 0;
        if (millis() - t_of_state_2 >= long_press_time) {
          Serial.printf("millis() - t_of_state_2 >= long_press_time\n");
          state = 3;
        }
      } else {
        state = 4;
       // flag = 1;
        t_of_button_change = millis();
      }
    } else if (state==3) {
      if (button_pressed) {
        flag = 0;
        //do nothing - long enough to be a long press
      } else {
        //go to state 4
        state = 4;
       // flag = 2;
        t_of_button_change = millis();
      }
    } else if (state==4) { 
      flag = 0;
      if (button_pressed) {
        int diff = millis() - t_of_state_2;
        t_of_button_change = millis();
        if (diff < long_press_time) {
          //go to 2
          state = 2;
        } else {
          //go to 3
          state = 3;
        }
      } else {
        int diff = millis() - t_of_button_change;
        if (diff >= debounce_time) {
          //go to state 0
          int press_length = millis() - t_of_state_2;
          if (press_length >= long_press_time) {
            flag = 2;
          } else {
            flag = 1;
          }
          state = 0;
        }
      }
    }
    return flag;
  }
};

//int vx[200];
//int vy[200];
//
//void collect_measurements (float &meanx, float &sdx, float &meany, float &sdy) {
//  int tm = millis();
//  int sz = 200;
//  for (int i = 0; i < sz; i++) {
//    vx[i] = analogRead(A7);
//    vy[i] = analogRead(A3);
//  }
//
//  meanx = sdx = meany = sdy = 0;
//  for (int i = 0; i < sz; i++) {
//    meanx += vx[i];
//    meany += vy[i];
//  }
//
//  meanx /= sz;
//  meany /= sz;
//
//  for (int i = 0; i < sz; i++) {
//    sdx += (vx[i] - meanx) * (vx[i] - meanx);
//    sdy += (vy[i] - meany) * (vy[i] - meany);
//  }
//
//  sdx = sqrt(sdx / sz);
//  sdy = sqrt(sdy / sz);
//}

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

  void update (int angle) {
//  void update (float mean, float sd, float stab_mean, float stab_sd) {
    if (angle <= low_threshold) {
      if (small_extreme) {
        if (millis() - scrolling_timer >= scrolling_threshold) {
          //then you update!
          index = (index + length - 1) % length;
          scrolling_timer = millis();
        }
      } else {
        scrolling_timer = millis();
      }
      
      small_extreme = true;
      large_extreme = false;
    } else if (angle >= high_threshold) {
      if (large_extreme) {
        if (millis() - scrolling_timer >= scrolling_threshold) {
          //then you update!
          index = (index + 1) % length;
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
  }
};

void get_angle(int* x) {
  *x = analogRead(A7);
// Serial.printf("%d %d\n", *x, *y);
}

#endif
