#include <WiFi.h> //Connect to WiFi Network
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h> //Used in support of TFT Display
#include <time.h>

uint8_t char_append(char* buff, char c, uint16_t buff_size) {
        int len = strlen(buff);
        if (len>buff_size) return false;
        buff[len] = c;
        buff[len+1] = '\0';
        return true;
}

void display_all_lines() {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0,0,1);
  info[0] = 0;
  int cum_len = 0;
  for (int i = 0; i < nlines; i++) {
    cum_len += sprintf(info + cum_len, "%s\n\n", output[i]);
  }
//  Serial.println(info);
  tft.print(info);
}

void display_home_screen() {
  nlines = 0;
  strcpy(output[nlines++], "HOME SCREEN");
  display_all_lines();
}

void loading() {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0,0,1);
  tft.print("Loading...");
}
