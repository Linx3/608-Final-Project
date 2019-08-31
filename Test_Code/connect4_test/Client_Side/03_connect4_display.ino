const int RECT_HEIGHT = 30, RECT_WIDTH = 30;

uint8_t char_append(char* buff, char c, uint16_t buff_size) {
        int len = strlen(buff);
        if (len>buff_size) return false;
        buff[len] = c;
        buff[len+1] = '\0';
        return true;
}

#include <mpu9255_esp32.h>
#include <WiFi.h> //Connect to WiFi Network
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h> //Used in support of TFT Display
#include <time.h>


void display_all_lines (bool reset, int col, int row) {
  Serial.println("display all lines");
  if (reset) {
    Serial.println("tft fill white screen");
    tft.fillScreen(TFT_WHITE);
  }
  tft.setCursor(col, row, 1);
  info[0] = 0;
  int cum_len = 0;
  for (int i = 0; i < nlines; i++) {
    cum_len += sprintf(info + cum_len, "%s\n", output[i]);
  }
//  Serial.println(info);
  tft.print(info);
}

void display_all_lines() {
  display_all_lines(true, 0, 0);
}

const int base_row = 10, base_col = 5, rect_height = 16, rect_width = 16;
const int piece_radius = 6;

void display_grid (int w, int h, uint32_t color_interior) {
  tft.fillScreen(TFT_WHITE);

  int xmn = base_col, xmx = w * rect_width + base_col;
  int ymn = base_row, ymx = h * rect_height + base_row;

  tft.fillRect(xmn, ymn, w * rect_width, h * rect_height, color_interior);

//  for (int i = 0; i <= h; i++) {
//    int y = i * rect_height + base_row;
//    int ycent = i + rect_height / 2;
//    tft.drawLine(xmn, y, xmx, y, color);
//  }
//  for (int i = 0; i <= w; i++) {
//    int x = i * rect_width + base_col;
//    int xcent = i + rect_height / 2;
//    tft.drawLine(x, ymn, x, ymx, color);
//  }
  
  for (int i = 0; i < w; i++) {
    int x = i * rect_width + base_col;
    int xcent = x + rect_height / 2;
    for (int j = 0; j < h; j++) {
      int y = j * rect_height + base_row;
      int ycent = y + rect_height / 2;
      tft.fillCircle(xcent, ycent, piece_radius, TFT_WHITE);
    }
  }
  
//  tft.drawLine(0 * rect_width + base_col, 0 * rect_height + base_row + 1, w * rect_width + base_col, 0 * rect_height + base_row + 1, color);
//  tft.drawLine(0 * rect_width + base_col - 1, 0 * rect_height + base_row, 0 * rect_width + base_col - 1, h * rect_height + base_row, color);
//  tft.drawLine(w * rect_width + base_col, h * rect_height + base_row, w * rect_width + base_col, 0 * rect_height + base_row, color);
//  tft.drawLine(w * rect_width + base_col, h * rect_height + base_row, 0 * rect_width + base_col, h * rect_height + base_row, color);
}

void draw_selection (int pos, uint32_t color_interior, uint32_t color_boundary) {
  int ytop = 4, ybot = 8;
  int xcent_base = 13;
  int rad = 4;

  xcent_base += rect_width * pos;

  int x1 = xcent_base, y1 = ybot, x2 = xcent_base - rad, y2 = ytop, x3 = xcent_base + rad, y3 = ytop;

  tft.fillTriangle(x1, y1, x2, y2, x3, y3, color_interior);
  tft.drawTriangle(x1, y1, x2, y2, x3, y3, color_boundary);
}

void display_selection (int pos) {
  draw_selection(pos, TFT_GREEN, TFT_BLACK);
}

void remove_selection (int pos) {
  draw_selection(pos, TFT_WHITE, TFT_WHITE);
}

void display_character (int x, int y, char c) {
  assert(c == 'X' || c == 'O');
  uint32_t color = getcolor_int(c);
  Serial.printf("display_piece(%d, %d)\n", x, y);
  
  int xcent = x * rect_width + base_col + rect_width / 2;

  for (int i = 0; i <= y; i++) {
    //add this selection
    int ycent = i * rect_height + base_row + rect_height / 2;
    tft.fillCircle(xcent, ycent, piece_radius, color);

    delay(100);
    if (i != y) {
      //remove this selection
      tft.fillCircle(xcent, ycent, piece_radius, TFT_WHITE);
    }
  }
}

int sgn (int x) {
  if (x == 0) {
    return 0;
  }
  return x < 0 ? -1 : 1;
}

void sweep_line (int x1, int y1, int x2, int y2) {
  if (y1 < y2) {
    swap(x1, x2);
    swap(y1, y2);
  }
  
  Serial.printf("sweep line (%d, %d) to (%d, %d) - Board Coordinates.\n", x1, y1, x2, y2);
  x1 = x1 * rect_width + base_col + rect_height / 2;
  y1 = y1 * rect_height + base_row + rect_width / 2;
  x2 = x2 * rect_width + base_col + rect_height / 2;
  y2 = y2 * rect_height + base_row + rect_width / 2;

  Serial.printf("sweep line (%d, %d) to (%d, %d) - Screen Coordinates.\n", x1, y1, x2, y2);
  
  assert(abs(x1 - x2) == abs(y1 - y2) || x1 == x2 || y1 == y2);
  int incx = sgn(x2 - x1), incy = sgn(y2 - y1);

  int line_width = 1;

  for (int i = x1, j = y1; !(i == x2 && j == y2); i += incx, j += incy) {
    tft.drawRect(i - line_width, j - line_width, 2 * line_width, 2 * line_width, TFT_BLACK);
    delay(2);
  }
}

void display_connect4_home_screen() {
  nlines = 0;
  strcpy(output[nlines++], "HOME SCREEN\n");
  strcpy(output[nlines++], "CONNECT 4");
  display_all_lines();
}

void loading() {
  tft.fillScreen(TFT_WHITE);
  tft.setCursor(0,0,1);
  tft.print("Loading...");
}
