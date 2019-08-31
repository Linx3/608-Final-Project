#ifndef BOARD_H
#define BOARD_H

#include <mpu9255_esp32.h>
#include <WiFi.h> //Connect to WiFi Network
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h> //Used in support of TFT Display
#include <time.h>

class Board {
public:
  int h, w;
  char board[8][8];
  char pieces[3];
  uint32_t colors[3];
  int mp[128];
  int cur;
  int need;
  int npiece; 

  Board (int w, int h, char player0, char player1, uint32_t color0, uint32_t color1, int need) {
    this->h = h;
    this->w = w;
    this->need = need;
    cur = 0;
    for (int i = 0; i < w; i++) {
      for (int j = 0; j < h; j++) {
        board[i][j] = ' ';
      }
    }
    this->pieces[0] = player0;
    this->pieces[1] = player1;
    this->colors[0] = color0;
    this->colors[1] = color1;
    
    cur = 0;
    npiece = 0;

    for (int i = 0; i < 128; i++) {
      mp[i] = -1;
    }
    mp[player0] = 0;
    mp[player1] = 1;
  }

//  int vertical (int x, int y) {
//    if (!(0 <= x && x < h - need) || !(0 <= y && y < w)) return -1;
//
//    char c = board[x][y];
//    if (c == ' ') {
//      return -1;
//    }
//
//    int msk = mp[board[x][y]];
//    for (int i = x; i < x + need; i++) {
//      if (board[i][y] != c) {
//        return -1;
//      }
//      msk = (msk << 6) ^ (w * i + y);
//    }
//    return msk;
//  }
//
//  int horizontal (int x, int y) {
//    if (!(0 <= x && x < h) || !(0 <= y && y < w - need)) return -1;
//
//    char c = board[x][y];
//    if (c == ' ') {
//      return -1;
//    }
//
//    int msk = mp[board[x][y]];
//    for (int j = y; j < y + need; j++) {
//      if (board[x][j] != c) {
//        return -1;
//      }
//      msk = (msk << 6) ^ (w * x + j);
//    }
//    return msk;
//  }
//
//  int maindiagonal (int x, int y)  {
//    //maindiagonal: "\" diagonal.
//    if (!(0 <= x && x < h - need) || !(0 <= y && y < w - need)) return -1;
//
//    char c = board[x][y];
//    if (c == ' ') {
//      return -1;
//    }
//
//    int msk = mp[board[x][y]];
//    for (int i = x, j = y; j < y + need; i++, j++) {
//      if (board[i][j] != c) {
//        return -1;
//      }
//      msk = (msk << 6) ^ (w * i + j);
//    }
//    return msk;
//  }
//
//  int antidiagonal (int x, int y) {
//    //antidiagonal: "/" diagonal.
//    if (!(0 <= x && x < h - need) || !(need - 1 <= y && y < w)) return -1;
//    
//    char c = board[x][y];
//    if (c == ' ') {
//      return -1;
//    }
//
//    int msk = mp[board[x][y]];
//    for (int i = x, j = y; i < x + need; i++, j--) {
//      if (board[i][j] != c) {
//        return -1;
//      }
//      msk = (msk << 6) ^ (w * i + j);
//    }
//    return msk;
//  }
//
//  int status() {
//    //status: -1 if game is still continuing, -2 if game is drawn
//    //otherwise if player has won:
//    //the pieces:
//    // (0 if player 0 win, 1 if player 1 win), (the positions of the pieces)
//    
//    bool has_space = false;
//    for (int i = 0; i < h; i++) {
//      for (int j = 0; j < w; j++) {
//        if (board[i][j] == ' ') {
//          has_space = true;
//          break;
//        }
//      }
//      if (has_space) {
//        break;
//      }
//    }
//
//    if (!has_space) {
//      return -2;
//    }
//
//    for (int i = 0; i < h; i++) {
//      for (int j = 0; j < w; j++) {
//        int msk;
//
//        msk = vertical(i, j);
//        if (msk != -1) {
//          return msk;
//        }
//
//        msk = horizontal(i, j);
//        if (msk != -1) {
//          return msk;
//        }
//        
//        msk = maindiagonal(i, j);
//        if (msk != -1) {
//          return msk;
//        }
//        
//        msk = antidiagonal(i, j);
//        if (msk != -1) {
//          return msk;
//        }
//      }
//    }
//    return -1;
//  }

  void put (int x, int y) {
    board[x][y] = pieces[cur];
    cur ^= 1;
//    return status();
  }

  void print() {
    Serial.println("---Board---");
    for (int i = 0; i < h; i++) {
      Serial.println(board[i]);
    }
  }
};

#endif
