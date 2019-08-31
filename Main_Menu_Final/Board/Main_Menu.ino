#include <math.h>
#include <string.h>
//#include <SPI.h>
#include <TFT_eSPI.h>
#include "tic_tac_toe_game.h"
#include "connect_4_game.h"
#include "chat.h"
#include "button.h"

const int TEXT_COLOR = TFT_WHITE;
const int GRID_COLOR = TFT_WHITE;
const int CURSOR_COLOR = TFT_CYAN;
const int PIECE_COLOR = TFT_YELLOW;
const int GPS_COLOR = TFT_PINK;
const int BACKGROUND = TFT_BLACK;

const int SCREEN_HEIGHT = 160;
const int SCREEN_WIDTH = 128;
const int BUTTON_PIN_R = 39;
const int BUTTON_PIN_M = 16;
const int BUTTON_PIN_L = 5;
const int BUTTON_WAKEUP = 26;

//Loop Timing Variables
const int LOOP_PERIOD = 40;
const int JOYSTICK_PERIOD = 200;
uint32_t primary_timer; //used for timing loop
//uint32_t joystick_timer;


const int CURSOR_BLINK_PERIOD = 500;
uint32_t cursor_timer;

//Board array
//bool arr[2];
int ix;
int r;
int state;  //actually, this variable is always 0. The original design was to make it unblocking.

//renders battery symbol including level amount

void drawMenu(TFT_eSPI* screen, uint16_t fc, uint16_t bc) {
  screen->setTextSize(2);
  screen->setCursor(25, 20);
  screen->println("MAIN MENU");
  screen->println("");
  screen->setTextSize(2);
  screen->println("   CHAT");
  screen->println("   TICTACTOE");
  screen->println("   CONNECT 4");
  screen->setTextSize(1);
  screen->setCursor(10, 110);
  screen->println("Press joystick to select");
  drawCursor(screen, fc);
}

void setup();

void runTicTacToe() {
//  screen->fillScreen(BACKGROUND);
//  screen->setTextSize(1);
//  screen->setCursor(10, 40);
//  screen->println("TICTACTOE GAME IN PROGRESS");  //REPLACE THIS...
  enter_tictactoe();
  setup();
}

void runConnect4() {
//  screen->fillScreen(BACKGROUND);
//  screen->setTextSize(1);
//  screen->setCursor(10, 40);
//  screen->println("CONNECT4 GAME IN PROGRESS");  //REPLACE THIS...
  enter_connect4();
  setup();
}

void runChat() {
//  screen->fillScreen(BACKGROUND);
//  screen->setTextSize(1);
//  screen->setCursor(10, 40);
//  screen->println("CHAT IN PROGRESS");
  enter_chat();
  setup();
}

void drawCursor(TFT_eSPI* screen, uint16_t fc) {
  screen->fillTriangle(15 + r, 54 + ix*16, 15 + r, 64 + ix*16, 25 + r, 59 + ix*16, fc);

  
}

Button buttonL(BUTTON_PIN_L);
Button buttonM(BUTTON_PIN_M);
Button buttonWake(BUTTON_WAKEUP);
//Button buttonR(BUTTON_PIN_R);
Scroller sc(3);

//char request_buffer_main_menu[1000];
char body_main_menu[300];
bool asleep_main_menu;
uint32_t last_action_main_menu;

void setup() {
  asleep_main_menu = false;
  last_action_main_menu = millis();
  static bool initial_setup = true;
  // put your setup code here, to run once:
  if (initial_setup) {
    Serial.begin(115200);  // Set up serial port
    delay(300); //wait a bit (100 ms)
    pinMode(26, INPUT_PULLUP);
    pinMode(5, INPUT_PULLUP);
    pinMode(19, INPUT_PULLUP);
    pinMode(16, INPUT_PULLUP);
    pinMode(27, INPUT_PULLUP);
    tft.init();  //init screen
    tft.setRotation(3); //adjust rotation
    tft.setTextSize(1); //default font size
  }
  tft.fillScreen(BACKGROUND); //fill background
  tft.setTextColor(TFT_WHITE, BACKGROUND); //set color of font to green foreground, black background
  primary_timer = millis();
  cursor_timer = millis();
  //bool test[10] = {0, 1, 1, 0, 1, 0, 0, 0, 1, 0};
  
  ix = 0;
  r = 0;
  state = 0;
  
  drawMenu(&tft, TEXT_COLOR, BACKGROUND);
  initial_setup = false;

//  connect_mit_wifi();

//  sprintf(body_main_menu, "op=get");
//  sprintf(request_buffer_main_menu,"POST http://608dev.net/sandbox/sc/whu2704/power_analysis.py HTTP/1.1\r\n");
//  strcat(request_buffer_main_menu,"Host: 608dev.net\r\n");
//  strcat(request_buffer_main_menu,"Content-Type: application/x-www-form-urlencoded\r\n");
//  sprintf(request_buffer_main_menu+strlen(request_buffer_main_menu),"Content-Length: %d\r\n", strlen(body_main_menu)); //append string formatted to end of request buffer
//  strcat(request_buffer_main_menu,"\r\n"); //new line from header to body_main_menu
//  strcat(request_buffer_main_menu,body_main_menu); //body_main_menu
//  strcat(request_buffer_main_menu,"\r\n"); //header
//  do_http_request("608dev.net", request_buffer_main_menu, 300, 6000, true);

//  sprintf(body_main_menu, "op=first");
//  sprintf(request_buffer_main_menu,"POST http://608dev.net/sandbox/sc/whu2704/power_analysis.py  HTTP/1.1\r\n");
//  strcat(request_buffer_main_menu,"Host: 608dev.net\r\n");
//  strcat(request_buffer_main_menu,"Content-Type: application/x-www-form-urlencoded\r\n");
//  sprintf(request_buffer_main_menu+strlen(request_buffer_main_menu),"Content-Length: %d\r\n", strlen(body_main_menu)); //append string formatted to end of request buffer
//  strcat(request_buffer_main_menu,"\r\n"); //new line from header to body_main_menu
//  strcat(request_buffer_main_menu,body_main_menu); //body_main_menu
//  strcat(request_buffer_main_menu,"\r\n"); //header
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_26, 0);
}

bool power_mode_take_care_main_menu() {
  if (asleep_main_menu) {
    //check for awake
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
      Serial.println("Wake up!!!");
      asleep_main_menu = false;
      last_action_main_menu = millis();
      tft.fillScreen(TFT_BLACK);
      drawMenu(&tft, TEXT_COLOR, BACKGROUND);
    }
    return true;
  } else {
    if (millis() - last_action_main_menu >= 20000) {
      Serial.println("START SLEEP");
      tft.fillScreen(TFT_BLACK);
      tft.setTextSize(1);
      tft.setCursor(0, 5);
      tft.printf("Sleeping...\nPress 26 to continue\n");
      asleep_main_menu = true;
      esp_light_sleep_start();
      return true; 
    }
  }
  return false;
}

void loop() {
//  Serial.printf("digitalRead(26) = %d\n", digitalRead(26));
  // put your main code here, to run repeatedly:
//  do_http_request("608dev.net", request_buffer_main_menu, 300, 6000, true);
//  static Button buttonL(BUTTON_PIN_L);

//  pingit();

  if (power_mode_take_care_main_menu()) return;
  
  bool refresh = false;
  int bvL = buttonL.update();
  if (bvL >= 1) {
    last_action_main_menu = millis();
  }
//  int bvM = buttonM.update();
  if (state == 0 && bvL >= 1) {
    if (ix%3 == 0) {
      runChat();
//      state = 1;
      return;
    }
    else if (ix%3 == 1) {
      runTicTacToe();
//      state = 2;
      return;
    }
    else {
      runConnect4();
//      state = 3;
      return;
    }
  }
//  else if ((state == 1 || state == 2 || state == 3) && bvM == 1) {
//    state = 0;
//    tft.fillScreen(BACKGROUND); //fill background
//  }
//  Serial.println(analogRead(BUTTON_PIN_R));

  refresh = sc.update(analogRead(BUTTON_PIN_R));
  if (refresh) last_action_main_menu = millis();
  ix = sc.index;
//  if (analogRead(BUTTON_PIN_R) > 4090 && millis() - joystick_timer > JOYSTICK_PERIOD) {
//    Serial.printf("Scroll down, PIN %d\n", BUTTON_PIN_R);
//    ix = (ix + 1)%3;
//    refresh = true;
//    joystick_timer = millis();
//  }
//  if (analogRead(BUTTON_PIN_R) < 300 && millis() - joystick_timer > JOYSTICK_PERIOD) {
//    ix = (ix + 2)%3;
//    refresh = true;
//    joystick_timer = millis();
//  }
  
  if (millis() - cursor_timer > CURSOR_BLINK_PERIOD) {
    r = 4 - r;
    cursor_timer = millis();
    refresh = true;
  }
  if (state == 0) {
    if (refresh) tft.drawRect(15, 53, 15, 40, BACKGROUND); //fill background
    drawMenu(&tft, TEXT_COLOR, BACKGROUND);
  }  
  while (millis() - primary_timer < LOOP_PERIOD); //wait for primary timer to increment
  primary_timer = millis();
}
