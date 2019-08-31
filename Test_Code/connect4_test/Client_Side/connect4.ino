//This is for TWO PLAYERS.

//Could add this: One player cannot make a move until the other player is there.

#include <mpu9255_esp32.h>
#include <WiFi.h> //Connect to WiFi Network
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h> //Used in support of TFT Display
#include <time.h>
#include "connect4_board.h"
#include "connect4_scroller.h"

TFT_eSPI tft = TFT_eSPI();

//START: STUFF THAT IS DIFFERENT FOR DIFFERENT PLAYERS

char ME = 'X';

//END: STUFF THAT IS DIFFERENT FOR DIFFERENT PLAYERS

const int TTT_HEIGHT = 3;
const int TTT_WIDTH = 3;
const uint32_t LIGHT_BLUE = 0x000bff;
const uint8_t PIN_1 = 5; //button 1
const uint8_t PIN_2 = 19; //button 2
const char* NETWORK     = "6s08";     // your network SSID (name of wifi network)
const char* PASSWORD = "iesc6s08"; // your network password
//Board board(TTT_HEIGHT, TTT_WIDTH, COLORS_INT[0], COLORS_INT[1], 3);  //Actually, python used for board.
const int SCREEN_HEIGHT = 128, SCREEN_WIDTH = 160;
const char* C4_DATABASE = "connect4_query.py";
const char* COLORS_STR[2] = {"YELLOW", "RED"};
const uint32_t COLORS_INT[2] = {TFT_YELLOW, TFT_RED};


const char* getcolor_str (char c) {
  if (c == 'X') {
    return COLORS_STR[0];
  } else if (c == 'O') {
    return COLORS_STR[1];
  } else {
    assert(!"bad; can't get color");
  }
}

const uint32_t getcolor_int (char c) {
  if (c == 'X') {
    return COLORS_INT[0];
  } else if (c == 'O') {
    return COLORS_INT[1];
  } else {
    assert(!"bad; can't get color");
  }
}

const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const int GETTING_PERIOD = 5000; //periodicity of getting a number fact.
const uint16_t IN_BUFFER_SIZE = 2000; //size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 2000; //size of buffer to hold HTTP response
char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response
char body[IN_BUFFER_SIZE];

int nlines = 0;
char output[8][1000], info[10000];

/* Global variables*/
bool button_pressed1, button_pressed2; //used for containing button state and detecting edges
bool old_button_pressed1, old_button_pressed2; //used for detecting button edges
uint32_t time_since_sample;      // used for microsecond timing
uint32_t timer;

int state = 0, old_state = -1;
int nstate = state;

MPU9255 imu;

//functions that belong in other documents...

void do_request (const char* a, const char* b);
void do_request (const char* a, const char* b, bool c);
void display_all_lines (bool reset, int col, int row);
void display_all_lines();
void display_selection (int pos);
void remove_selection (int pos);
int next_avail[7];

void swap (int &x, int &y) {
  int t = x;
  x = y;
  y = t;
}


void setup() {
  Serial.begin(115200);               // Set up serial port
  tft.init();  //init screen
  tft.setRotation(3); //adjust rotation
  tft.setTextSize(1); //default font size
  tft.fillScreen(TFT_WHITE); //fill background
  tft.setTextColor(TFT_BLACK, TFT_WHITE); //set color of font to green foreground, black background
//  Serial.begin(115200); //begin serial comms
  loading();
  
  delay(100); //wait a bit (100 ms)
  pinMode(PIN_1, INPUT_PULLUP);
  pinMode(PIN_2, INPUT_PULLUP);
//  pinMode(A7, INPUT_PULLUP);
//  pinMode(A3, INPUT_PULLUP);
//  collect_measurements(stab_meanx, stab_sdx, stab_meany, stab_sdy);
//  Serial.printf("Stable: %f %f %f %f\n", stab_meanx, stab_sdx, stab_meany, stab_sdy);

  while (true) {
    if (connect_to_wifi("6s08", "iesc6s08")) { 
      break;
    }
    if (connect_to_wifi("MIT", NULL)) {
      break;
    }
  }
  
  //set up IMU
  if (imu.setupIMU(1)) {
    Serial.println("IMU Connected!");
  } else {
    Serial.println("IMU Not Connected :/");
    Serial.println("Restarting");
    ESP.restart(); // restart the ESP (proper way)
  }
  
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

Scroller scx(7);
Board board = Board(7, 6, 'X', 'O', COLORS_INT[0], COLORS_INT[1], 4);
Button button1(5);

int updbutton1;
char cur_player;

void display_continue() {
  tft.setCursor(5, 110, 1);
  char whose_turn = 'X' + 'O' - board.pieces[board.cur];
  tft.printf("%s's TURN          ", getcolor_str(whose_turn));
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

void do_my_turn() {
  if (needs_another_player) {
    delay(1000);
    do_request(C4_DATABASE, "needs_another_player=True", true);
    if (strstr(response_buffer, "True")) {
      Serial.println("Still waiting for the other turn");
      return;
    } else {
      needs_another_player = false;
      tft.setCursor(5, 110 + 10, 1);
      tft.print("                         ");
    }
  }
  
  Serial.println("MY TURN!!!");
    if (updbutton1 >= 1) {
      //then just do the thing
      int x = scx.index, &y = next_avail[x];
      Serial.printf("----SELECT %d %d----\n", x, y);
      cur_player = board.pieces[board.cur];
      if (y >= 0) {
        Serial.printf("I'm playing %d %d\n", x, y);
        display_character(x, y, cur_player);
        sprintf(body, "update=True&player=%c&putx=%d&puty=%d", board.pieces[board.cur], x, y); 
        do_request(C4_DATABASE, body, true);
        assert(!strstr(response_buffer, "cannot put"));
        remove_selection(x);
        char body[120];

        if (strstr(response_buffer, "continue")) {
          //then we continue.
          display_continue();
//          sprintf(body, "update=True&putx=%d&puty=%d&end=False", x, y);
//          do_request(C4_DATABASE, body, true);
        } else {
          //then we end.
          if (strstr(response_buffer, "draw")) {
            //DRAW
            display_draw();
//            sprintf(body, "update=True&putx=%d&puty=%d&end=True", x, y);
//            do_request(C4_DATABASE, body, true);
          } else {
            display_win();
//            sprintf(body, "update=True&putx=%d&puty=%d&end=%s", x, y, strstr(response_buffer, "("));  //is this format OK?
//            do_request(C4_DATABASE, body, true);
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
    delay(1000);
    do_request(C4_DATABASE, "needs_another_player=True", true);
    if (strstr(response_buffer, "True")) {
      return;
    } else {
      needs_another_player = false;
      tft.setCursor(5, 110 + 10, 1);
      tft.print("                      ");
    }
  }
  
  Serial.println("NOT MY TURN!!");
  char body[100];
  sprintf(body, "update=False&player=%c", ME);
  delay(1000);
  do_request(C4_DATABASE, body, true);
  if (strstr(response_buffer, "-1")) {
    //-1 - don't do anything.

    //this should happen most  of time
    Serial.println("Other player not done");
    return;
  }

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

void loop() {
//  Serial.println("loop");
  nstate = state;

  updbutton1 = button1.update();
//  Serial.printf("updbutton1 = %d\n", updbutton1);

  if (state == 0) {
    //This is the homepaage
    if (old_state != state) {
      //reset.
      Serial.println("display_connect4_home_screen()");
      display_connect4_home_screen();
    } else {
//      Serial.println("republican");
      if (updbutton1 >= 1) {
        Serial.println("hereeee");
        old_state = 0;
        nstate = 1;
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
        do_request(C4_DATABASE, "who_am_i=True");
        Serial.printf("who_am_i %s\n", response_buffer);
        ME = response_buffer[0];
      } while (ME != 'X' && ME != 'O');

//      sprintf(body, "player=%c&reset=True", ME);
//      do_request(C4_DATABASE, body);
      if (ME == 'X') {
        Serial.println("X reset");
        sprintf(body, "player=%c&reset=True", ME);
        do {
          delay(100);
          do_request(C4_DATABASE, body);
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
    if (updbutton1 >= 1) {
      nstate = 0;
    }
  }

  old_state = state;
  state = nstate;
}
