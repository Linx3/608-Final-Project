#ifndef CONNECT_4_GAME
#define CONNECT_4_GAME

#include <mpu9255_esp32.h>
#include <WiFi.h> //Connect to WiFi Network
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h> //Used in support of TFT Display
#include <time.h>
#include "button.h"
#include "scroller.h"
#include "board.h"
#include "wifi_stuff.h"
#include "chat.h"

//static constexpr uint16_t IN_BUFFER_SIZE = 300; //size of buffer to hold HTTP request
//static char request_buffer_c4[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
//static char body_c4[IN_BUFFER_SIZE];
static char* body_c4 = body;

class Connect4 {
public:

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

void get_angle(int* x) {
  *x = analogRead(A7);
// Serial.printf("%d %d\n", *x, *y);
}

char ME = 'X';

//END: STUFF THAT IS DIFFERENT FOR DIFFERENT PLAYERS

static constexpr uint32_t LIGHT_BLUE = 0x000bff;
static constexpr uint8_t PIN_1 = 26; //button 1
static constexpr uint8_t PIN_2 = 27; //button 2
static constexpr char* NETWORK     = "6s08";     // your network SSID (name of wifi network)
static constexpr char* PASSWORD = "iesc6s08"; // your network password
char *response_buffer; //char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response
//Board board(TTT_HEIGHT, TTT_WIDTH, COLORS_INT[0], COLORS_INT[1], 3);  //Actually, python used for board.
static constexpr int SCREEN_HEIGHT = 128, SCREEN_WIDTH = 160;
static constexpr char* C4_DATABASE = "connect4_query.py";
static constexpr char* COLORS_STR[2] = {"YELLOW", "RED"};
static constexpr uint32_t COLORS_INT[2] = {TFT_YELLOW, TFT_RED};


static const char* getcolor_str (char c) {
  if (c == 'X') {
    return COLORS_STR[0];
  } else if (c == 'O') {
    return COLORS_STR[1];
  } else {
    assert(!"bad; can't get color");
  }
}

uint32_t getcolor_int (char c) {
  if (c == 'X') {
    return COLORS_INT[0];
  } else if (c == 'O') {
    return COLORS_INT[1];
  } else {
    assert(!"bad; can't get color");
  }
}

static constexpr int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
static constexpr int GETTING_PERIOD = 5000; //periodicity of getting a number fact.

int nlines = 0;
char output[8][100], info[1000];

/* Global variables*/
bool button_pressed1, button_pressed2; //used for containing button state and detecting edges
bool old_button_pressed1, old_button_pressed2; //used for detecting button edges
uint32_t time_since_sample;      // used for microsecond timing
uint32_t timer;

int state = 0, old_state = -1;
int nstate = state;

//MPU9255 imu;

bool asleep_connect4 = false;
uint32_t last_action_connect4;

bool power_mode_take_care_connect4() {
  if (asleep_connect4) {
    //check for awake
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
      Serial.println("Wake up!!!");
      asleep_connect4 = false;
//          last_action_connect4 = millis(); //display takes care of it.
      tft.fillScreen(TFT_WHITE);
      display_connect4_home_screen();
    }
    return true;
  } else {
    if (millis() - last_action_connect4 >= 20000) {
      Serial.println("START SLEEP");
      tft.fillScreen(TFT_WHITE);
      tft.setTextSize(1);
      tft.setCursor(0, 5);
      tft.printf("Sleeping...\nPress 26 to continue\n");
      asleep_connect4 = true;
      esp_light_sleep_start();
      return true; 
    }
  }
  return false;
}


//void do_request (const char* a, const char* b, char* &response_buffer);
//void do_request (const char* a, const char* b, char* &response_buffer, bool c);
//void display_all_lines (bool reset, int col, int row);
//void display_all_lines();
//void display_selection (int pos);
//void remove_selection (int pos);
int next_avail[7];

void setup() {
//  Serial.begin(115200);               // Set up serial port
  tft.init();  //init screen
  tft.setRotation(3); //adjust rotation
  tft.setTextSize(1); //default font size
  tft.fillScreen(TFT_WHITE); //fill background
  tft.setTextColor(TFT_BLACK, TFT_WHITE); //set color of font to green foreground, black background
//  Serial.begin(115200); //begin serial comms
  loading();
  
  delay(100); //wait a bit (100 ms)
//  pinMode(PIN_1, INPUT_PULLUP);
//  pinMode(PIN_2, INPUT_PULLUP);
//  pinMode(A7, INPUT_PULLUP);
//  pinMode(A3, INPUT_PULLUP);
//  collect_measurements(stab_meanx, stab_sdx, stab_meany, stab_sdy);
//  Serial.printf("Stable: %f %f %f %f\n", stab_meanx, stab_sdx, stab_meany, stab_sdy);

  connect_mit_wifi();
  
  //set up IMU
//  if (imu.setupIMU(1)) {
//    Serial.println("IMU Connected!");
//  } else {
//    Serial.println("IMU Not Connected :/");
//    Serial.println("Restarting");
//    ESP.restart(); // restart the ESP (proper way)
//  }
  
  timer = millis();
//  old_val = digitalRead(PIN_1);
  analogSetAttenuation(ADC_6db); //set to 6dB attenuation for 3.3V full scale reading.
  display_connect4_home_screen();
  Serial.println("SETUP DONE!!");
}

/*
 * STATES
 * 
 * Home screen - press any button to continue
 * 
 * One player has won, or DRAW
  */

Scroller scx = Scroller(7);
Board board = Board(7, 6, 'X', 'O', COLORS_INT[0], COLORS_INT[1], 4);
Button button_joystick = Button(5);
Button button16 = Button(16);
Button button_pin1 = Button(PIN_1);

int updbutton_joystick, updbutton_pin1, updbutton16;
char cur_player;

void display_continue() {
  tft.setCursor(5, 110, 1);
  char whose_turn = 'X' + 'O' - board.pieces[board.cur];
  tft.printf("%s's TURN          ", getcolor_str(whose_turn));
  tft.setCursor(5, 110 + 10, 1);
  tft.printf("PRESS BUTTON %d to CHAT", PIN_1);
  if (board.pieces[board.cur] != ME) display_selection(0);
}

void display_draw() {
  delay(500);
  tft.setCursor(5, 110, 1);
  tft.print("DRAW          ");
  delay(1000);
  tft.setCursor(5, 110 + 10, 1);
  tft.print("GAME OVER                 ");
  delay(400); 
}

void display_win() {
  int winner_id;
  char winner_char;

  /*
   * TODO:
   * 1. Draw the line (delay of 2 ms per)
   * 2. Delay 400ms
   * 3. Say wins!
   */

  int cols[4][2];

  sscanf(strchr(response_buffer, '(') - 2, "%d (%d, %d) (%d, %d) (%d, %d) (%d, %d)", &winner_id, &cols[0][0], &cols[0][1], &cols[1][0], &cols[1][1], &cols[2][0], &cols[2][1], &cols[3][0], &cols[3][1]);
  Serial.printf("response buffer: %s\n", response_buffer);
  
  sweep_line(cols[0][0], cols[0][1], cols[3][0], cols[3][1]);

  delay(1000);
  Serial.printf("winner_id = %d\n", winner_id);
  winner_char = board.pieces[winner_id];
  tft.setCursor(5, 110, 1);
  tft.printf("PLAYER %s WINS          ", getcolor_str(winner_char));
  delay(1000);
  tft.setCursor(5, 110 + 10, 1);
  tft.print("GAME OVER               ");
//            while (1);
  Serial.println("What the fuck game just ended");
}

bool prev_needs_another_player = true;
bool needs_another_player = true;

void display_entire_board() {
  display_grid(7, 6, LIGHT_BLUE);
  for (int i = 0; i < 7; i++) {
    for (int j = 0; j < 6; j++) {
      if (board.board[i][j] == 'X' || board.board[i][j] == 'O') {
        display_character(i, j, board.board[i][j], false);
      }
    }
  }
}

void do_my_turn() {
  if (needs_another_player) {
    delay(2000);
    do_request(C4_DATABASE, "needs_another_player=True", response_buffer, true);
    if (strstr(response_buffer, "True")) {
      Serial.println("Still waiting for the other turn");
      return;
    } else {
      needs_another_player = false;
      tft.setCursor(5, 110 + 10, 1);
      tft.printf("PRESS BUTTON %d to CHAT", PIN_1);
    }
  }

  if (updbutton_pin1 >= 1) {
    //then to go chat
    enter_chat();
    //TODO: add logic for returing back to screen - for "my turn" and for "not my turn".
    //I believe this is now over? The logic for "not my turn" is slightly more complicated. You might need to make it your turn.
    tft.fillScreen(TFT_WHITE);
    display_entire_board();
    display_selection(scx.index);
    tft.setCursor(5, 110, 1);
    tft.printf("%s's TURN          ", getcolor_str(ME));
    tft.setCursor(5, 110 + 10, 1);
    tft.printf("PRESS BUTTON %d to CHAT", PIN_1);
    return;
  }  
  
  Serial.println("MY TURN!!!");
    if (updbutton_joystick >= 1) {
      //then just do the thing
      int x = scx.index, &y = next_avail[x];
      Serial.printf("----SELECT %d %d----\n", x, y);
      cur_player = board.pieces[board.cur];
      if (y >= 0) {
        Serial.printf("I'm playing %d %d\n", x, y);
        display_character(x, y, cur_player);
        sprintf(body_c4, "update=True&player=%c&putx=%d&puty=%d", board.pieces[board.cur], x, y); 
        do_request(C4_DATABASE, body_c4, response_buffer, true);
        assert(!strstr(response_buffer, "cannot put"));
        remove_selection(x);
//        char body_c4[120];

        if (strstr(response_buffer, "continue")) {
          //then we continue.
          display_continue();
//          sprintf(body_c4, "update=True&putx=%d&puty=%d&end=False", x, y);
//          do_request(C4_DATABASE, body_c4, response_buffer, true);
        } else {
          //then we end.
          if (strstr(response_buffer, "draw")) {
            //DRAW
            display_draw();
//            sprintf(body_c4, "update=True&putx=%d&puty=%d&end=True", x, y);
//            do_request(C4_DATABASE, body_c4, response_buffer, true);
          } else {
            display_win();
//            sprintf(body_c4, "update=True&putx=%d&puty=%d&end=%s", x, y, strstr(response_buffer, "("));  //is this format OK?
//            do_request(C4_DATABASE, body_c4, response_buffer, true);
          }
          nstate = 2;
        }
        Serial.printf("board trying to put %d %d\n", x, y);
        board.put(x, y);
        Serial.printf("board has been put %d %d\n", x, y);
        //reset scrollers!
        scx = Scroller(7);
        y--;
        Serial.printf("Scrollers reset\n");
      } else {
        Serial.printf("(%d, %d) cannot put; %c already there\n", x, y, board.board[x][y]);
      }
      Serial.printf("Reached end of NOT CONTINUE in state 1\n");
    } else {
//      delay(100);
//      int bv = button.update(); //get button value
      int angx = analogRead(A7);
      get_angle(&angx); //get angle values
//      assert(x != 0);
//      assert(x != 4095);
//      assert(y != 0);
//      assert(y != 4095);
//      if (millis() % 1000 == 0) Serial.printf("Angles: %d, %d\n", x, y);
      int oldxind = scx.index;
//      float meanx, meany, sdx, sdy;
//      collect_measurements(meanx, sdx, meany, sdy);
//      scx.update(meanx, sdx, stab_meanx, stab_sdx);
      scx.update(angx);
      if (!(scx.index == oldxind)) {
        remove_selection(oldxind);
        display_selection(scx.index);
        Serial.printf("Selection is (%d, %d)\n", scx.index, next_avail[scx.index]);
      }
    }
}

void do_not_my_turn() {
  if (needs_another_player) {
    delay(2000);
    do_request(C4_DATABASE, "needs_another_player=True", response_buffer, true);
    if (strstr(response_buffer, "True")) {
//      return;
    } else {
      needs_another_player = false;
      tft.setCursor(5, 110 + 10, 1);
      tft.printf("PRESS BUTTON %d to CHAT", PIN_1);
    }
  }

  timer = millis();
  while (millis() - timer < 3000) {
    if (digitalRead(PIN_1) == 0) {
      enter_chat();
      sprintf(body_c4, "update=False&player=%c", ME);
      do_request(C4_DATABASE, body_c4, response_buffer, false);  //emphasizing here that you do not suppress "Loading..." message.
      //Actually, after this you also need to restore to original board.
      display_entire_board();
      tft.setCursor(5, 110, 1);
      tft.printf("%c's TURN          ", ME);
      tft.setCursor(5, 110 + 10, 1);
      tft.printf("PRESS BUTTON %d to CHAT", PIN_1);
      break;
    } else if (millis() - timer >= 2000) {
      if (needs_another_player) {
        Serial.println("Needs another player, didn't click chat. So gonna return.");
        return;
      }
      Serial.println("NOT MY TURN!!");
//      char body_c4[100];
      sprintf(body_c4, "update=False&player=%c", ME);
      do_request(C4_DATABASE, body_c4, response_buffer, true);
      delay(2000);
      break;
    }
  }
  
  if (*response_buffer && strstr(response_buffer, "-1")) {
    //-1 - don't do anything.

    //this should happen most  of time
    Serial.println("Other player not done");
    return;
  }

  Serial.println("");
  Serial.println("DAVID");
  Serial.println("OTHER PLAYER IS DONE!!!");

//  while (true) {
//  delay(10000);
  Serial.printf("this->response_buffer = %s\n", this->response_buffer);
  Serial.printf("Was able to print this->response_buffer!!!!\n");

  int x, y;
  sscanf(response_buffer,"%d %d", &x, &y);
  char *bufend = strstr(response_buffer, " AND ") + strlen(" AND ");
  Serial.printf("The other person played. At %d %d\n", x, y);
  display_character(x, y, 'X' + 'O' - ME);
  
  if (strncmp(bufend, "False", 5) == 0) {
    //CONTINUE
    //put the new cursor
    assert(scx.index == 0);
    Serial.println("CONTINUE");
    display_continue();
  } else if (strncmp(bufend, "True", 4) == 0) {
    //DRAW 
    Serial.println("DRAW");
    display_draw();
    nstate = 2;
  } else {
    //LOSE
    display_win();
    nstate = 2;
    Serial.println("LOSE");
  }
  board.put(x, y);
  assert(y == next_avail[x]);
  next_avail[x]--;
}

bool loop() {
//  Serial.println("loop");
  bool retloop = false;
  nstate = state;

  updbutton_joystick = button_joystick.update();
  updbutton16 = button16.update();
  updbutton_pin1 = button_pin1.update();
//  Serial.printf("updbutton_joystick = %d\n", updbutton_joystick);
  if (updbutton_pin1 >= 1) Serial.printf("updbutton_pin1 (pin %d) = %d\n", PIN_1, updbutton_pin1);

  if (state == 0) {
    //This is the homepaage
    if (old_state != state) {
      //reset.
      Serial.println("display_connect4_home_screen()");
      display_connect4_home_screen();
    } else {
      if (power_mode_take_care_connect4()) return false;
//      Serial.println("republican");
      if (updbutton_joystick >= 1) {
        Serial.println("hereeee");
        old_state = 0;
        nstate = 1;
        loading();
      } else if (updbutton16 >= 1) {
        retloop = true;
        loading();
      }
    }
  } else if (state == 1) {
    //GAME PLAY!!!
    if (old_state != state) {
      //who am i?
      for (int i = 0; i < 7; i++) next_avail[i] = 5;
      Serial.println("Who Am I?");

      do {
        delay(500);
        do_request(C4_DATABASE, "who_am_i=True", response_buffer);
        Serial.printf("who_am_i %s\n", response_buffer);
        ME = response_buffer[0];
      } while (ME != 'X' && ME != 'O');

//      sprintf(body_c4, "player=%c&reset=True", ME);
//      do_request(C4_DATABASE, body_c4, response_buffer);
      if (ME == 'X') {
        Serial.println("X reset");
        sprintf(body_c4, "player=%c&reset=True", ME);
        do {
          delay(100);
          do_request(C4_DATABASE, body_c4, response_buffer);
        } while (!strstr(response_buffer, "reset_successful"));
      }
      
      nlines = 0;
      sprintf(output[nlines++], "YOU ARE PLAYER %s", getcolor_str(ME));
      display_all_lines();
      delay(2000);
      
      //just getting in here. display the thing.
      Serial.println("Display Grid");
      display_grid(7, 6, LIGHT_BLUE); //connect4_display
      //this is a test below.
//      tft.drawLine(30, 5, 80, 5, TFT_BLACK);
//      tft.drawLine(30, 5, 80, 5, TFT_BLACK);
      scx = Scroller(7);
      board = Board(7, 6, 'X', 'O', COLORS_INT[0], COLORS_INT[1], 4);

      needs_another_player = true;
      tft.setCursor(5, 110 + 10, 1);
      tft.setTextColor(TFT_RED, TFT_WHITE);
      Serial.printf("AAAAWWWWAAAAIITTTINNNNGGGG\n");
      tft.printf("AWAITING PLAYER %s      ", getcolor_str('X' + 'O' - ME));
      tft.setTextColor(TFT_BLACK, TFT_WHITE);
      
      if (board.pieces[board.cur] == ME) display_selection(scx.index);
      Serial.println("got it");

      tft.setCursor(5, 110, 1);
      tft.printf("%s's TURN          ", getcolor_str('X'));
    }

    if (board.pieces[board.cur] == ME) {
      do_my_turn();
    } else {
      do_not_my_turn();
    }
  } else if (state == 2) {
    //GAME END!!!
//    Serial.println("Gaame End");
    if (updbutton_joystick >= 1) {
      nstate = 0;
    } else if (updbutton16 >= 1) {
      retloop = true;
      loading();
    }
  }

  old_state = state;
  state = nstate;
  return retloop;
}

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

static constexpr int base_row = 10, base_col = 5, rect_height = 16, rect_width = 16;
static constexpr int piece_radius = 6;

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

void display_character (int x, int y, char c, bool do_delay = true) {
  assert(c == 'X' || c == 'O');
  uint32_t color = getcolor_int(c);
  Serial.printf("display_piece(%d, %d)\n", x, y);
  
  int xcent = x * rect_width + base_col + rect_width / 2;

  if (do_delay) {
    for (int i = 0; i <= y; i++) {
      //add this selection
      int ycent = i * rect_height + base_row + rect_height / 2;
      tft.fillCircle(xcent, ycent, piece_radius, color);
  
      delay((int) (100 * (sqrt(i + 1) - sqrt(i))));
      if (i != y) {
        //remove this selection
        tft.fillCircle(xcent, ycent, piece_radius, TFT_WHITE);
      }
    }
  } else {
    //just display one circle
    int ycent = y * rect_height + base_row + rect_height / 2;
    tft.fillCircle(xcent, ycent, piece_radius, color);
  }
}

int sgn (int x) {
  if (x == 0) {
    return 0;
  }
  return x < 0 ? -1 : 1;
}

void sweep_line (int x1, int y1, int x2, int y2, bool do_delay = true) {
  if (y1 < y2) {
    int foo = x1;
    x1 = x2;
    x2 = foo;
    
    foo = y1;
    y1 = y2;
    y2 = foo;
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
    if (do_delay) delay(2);
  }
}

void display_connect4_home_screen() {
  nlines = 0;
  strcpy(output[nlines++], "HOME SCREEN\n");
  strcpy(output[nlines++], "CONNECT 4");
  strcpy(output[nlines++], "");
  strcpy(output[nlines++], "Press:");
  strcpy(output[nlines++], "-16 to return to main menu");
  strcpy(output[nlines++], "-joystick to start game");
  display_all_lines();
  asleep_connect4 = false;
  last_action_connect4 = millis();
}


};

void enter_connect4() {
  Connect4 c4 = Connect4();
  Serial.println("------START CONNECT4 INTEGRATION---------");
  c4.setup();
  while (!c4.loop());
  Serial.println("-------END CONNECT4 INTEGRATION-----------");
}

#endif
