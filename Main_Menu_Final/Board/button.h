#ifndef BUTTON_H
#define BUTTON_H

#include <mpu9255_esp32.h>
#include <WiFi.h> //Connect to WiFi Network
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h> //Used in support of TFT Display
#include <time.h>
#include <vector>

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

#endif
