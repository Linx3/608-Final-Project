#ifndef TIC_TAC_TOE_GAME_H
#define TIC_TAC_TOE_GAME_H

//This is for TWO PLAYERS.

//Could add this: One player cannot make a move until the other player is there.

#include <mpu9255_esp32.h>
#include <WiFi.h> //Connect to WiFi Network
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h> //Used in support of TFT Display
#include <time.h>
#include <vector>
#include "button.h"
#include "scroller.h"
#include "board.h"
#include "wifi_stuff.h"
#include "chat.h"

//static constexpr uint16_t IN_BUFFER_SIZE = 3000; //size of buffer to hold HTTP request
//static char request_buffer_ttt[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
//static char body_ttt[IN_BUFFER_SIZE];
static char* body_ttt = body;

class TicTacToe {
  public:

    //START: STUFF THAT IS DIFFERENT FOR DIFFERENT PLAYERS

    char ME = 'X';

    //END: STUFF THAT IS DIFFERENT FOR DIFFERENT PLAYERS

    static constexpr uint8_t PIN_JOYSTICK = 5;
    static constexpr uint8_t PIN_1 = 26; //button 1
    static constexpr uint8_t PIN_2 = 27; //button 2
    static constexpr int TTT_HEIGHT = 3;
    static constexpr int TTT_WIDTH = 3;
    static constexpr char* NETWORK     = "6s08";     // your network SSID (name of wifi network)
    static constexpr char* PASSWORD = "iesc6s08"; // your network password
    char *response_buffer; //char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response
    //Board board(TTT_HEIGHT, TTT_WIDTH, 'X', 'O', 3);  //Actually, python used for board.
    static constexpr int SCREEN_HEIGHT = 128, SCREEN_WIDTH = 160;
    static constexpr char* TTT_DATABASE = "tic_tac_toe_query.py";

    static constexpr int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
    static constexpr int GETTING_PERIOD = 5000; //periodicity of getting a number fact.
    static constexpr uint16_t OUT_BUFFER_SIZE = 3000; //size of buffer to hold HTTP response

    int nlines = 0;
    char output[8][100], info[1000];

    /* Global variables*/
    bool button_pressed1, button_pressed2; //used for containing button state and detecting edges
    bool old_button_pressed1, old_button_pressed2; //used for detecting button edges
    uint32_t time_since_sample;      // used for microsecond timing
    uint32_t timer;

    int state = 0, old_state = -1;
    int nstate = state;

    MPU9255 imu;

    //void do_request (static constexpr char* a, static constexpr char* b, char* &response_buffer);
    //void do_request (static constexpr char* a, static constexpr char* b, char* &response_buffer, bool c);
    //void display_all_lines (bool reset, int col, int row);
    //void display_all_lines();

    bool asleep_tic_tac_toe = false;
    uint32_t last_action_tic_tac_toe;
    
    bool power_mode_take_care_tic_tac_toe() {
      if (asleep_tic_tac_toe) {
        //check for awake
        if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
          Serial.println("Wake up!!!");
          asleep_tic_tac_toe = false;
//          last_action_tic_tac_toe = millis(); //display takes care of it.
          tft.fillScreen(TFT_WHITE);
          display_tic_tac_toe_home_screen();
        }
        return true;
      } else {
        if (millis() - last_action_tic_tac_toe >= 20000) {
          Serial.println("START SLEEP");
          tft.fillScreen(TFT_WHITE);
          tft.setTextSize(1);
          tft.setCursor(0, 5);
          tft.printf("Sleeping...\nPress 26 to continue\n");
          asleep_tic_tac_toe = true;
          esp_light_sleep_start();
          return true; 
        }
      }
      return false;
    }

    void setup() {
      //  Serial.begin(115200);               // Set up serial port
      //  tft.init();  //init screen
      //  tft.setRotation(3); //adjust rotation
      Serial.println("HERE");
      tft.setTextSize(1); //default font size
      Serial.println("ERE");
      tft.fillScreen(TFT_WHITE); //fill background
      Serial.println("HER");
      tft.setTextColor(TFT_BLACK, TFT_WHITE); //set color of font to green foreground, black background
      //  Serial.begin(115200); //begin serial comms
      Serial.println("HERE");
      loading();
      Serial.println("HEREEE");

      delay(100); //wait a bit (100 ms)
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
      analogSetAttenuation(ADC_6db); //set to 6dB attenuation for 3.3V full scale reading.
      display_tic_tac_toe_home_screen();
      Serial.println("SETUP DONE!!");
    }

    /*
       STATES

       Home screen - press any button to continue

       One player has won, or DRAW
    */

    Scroller scx = Scroller(3), scy = Scroller(3);
    Board board = Board(3, 3, 'X', 'O', TFT_RED, TFT_BLUE, 3);
    Button button_joystick = Button(PIN_JOYSTICK);
    Button button16 = Button(16);
    Button button_pin1 = Button(PIN_1);

    int updbutton_joystick, updbutton_pin1, updbutton16;
    char cur_player;

    void display_continue() {
      tft.setCursor(5, 105, 1);
      char whose_turn = 'X' + 'O' - board.pieces[board.cur];
      tft.printf("%c's TURN          ", whose_turn);
      tft.setCursor(5, 105 + 10, 1);
      tft.printf("PRESS BUTTON %d to CHAT", PIN_1);
      if (board.pieces[board.cur] != ME) display_selection(0, 0);
    }

    void display_draw() {
      delay(500);
      tft.setCursor(5, 105, 1);
      tft.print("DRAW          ");
      delay(1000);
      tft.setCursor(5, 105 + 10, 1);
      tft.print("GAME OVER                   ");
      delay(400);
    }

    char msg_lines[2][100];

    void display_win() {
      int winner_id;
      char winner_char;

      /*
         TODO:
         1. Draw the line (delay of 2 ms per)
         2. Delay 400ms
         3. Say wins!
      */

      int cols[3][2];

      sscanf(strchr(this->response_buffer, '(') - 2, "%d (%d, %d) (%d, %d) (%d, %d)", &winner_id, &cols[0][0], &cols[0][1], &cols[1][0], &cols[1][1], &cols[2][0], &cols[2][1]);
      Serial.printf("response buffer: %s\n", this->response_buffer);

      sweep_line(cols[0][0], cols[0][1], cols[2][0], cols[2][1]);

      delay(1000);
      Serial.printf("winner_id = %d\n", winner_id);
      winner_char = board.pieces[winner_id];
      tft.setCursor(5, 105, 1);
      sprintf(msg_lines[0], "PLAYER %c WINS", winner_char);
      tft.printf("%s          ", msg_lines[0]);
      delay(1000);
      tft.setCursor(5, 105 + 10, 1);
      sprintf(msg_lines[1], "GAME OVER");
      tft.printf("%s                   ", msg_lines[1]);
      //            while (1);
      Serial.println("What the fuck game just ended");
    }

    bool prev_needs_another_player = true;
    bool needs_another_player = true;

    void display_entire_board() {
      display_grid(3, 3, TFT_BLACK); //tic_tac_toe_display
      for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
          if (board.board[i][j] == 'X' || board.board[i][j] == 'O') {
            display_character(i, j, board.board[i][j]);
          }
        }
      }
    }

    void do_my_turn() {
      if (needs_another_player) {
        delay(2000);
        do_request(TTT_DATABASE, "needs_another_player=True", response_buffer, true);
        if (strstr(this->response_buffer, "True")) {
          return;
        } else {
          needs_another_player = false;
          tft.setCursor(5, 105 + 10, 1);
          tft.printf("PRESS BUTTON %d to CHAT", PIN_1);
        }
      }

      //only then can you message the other player
      if (updbutton_pin1 >= 1) {
        //then to go chat
        enter_chat();
        //TODO: add logic for returing back to screen - for "my turn" and for "not my turn".
        //I believe this is now over? The logic for "not my turn" is slightly more complicated. You might need to make it your turn.
        tft.fillScreen(TFT_WHITE);
        display_entire_board();
        display_selection(scx.index, scy.index);
        tft.setCursor(5, 105, 1);
        tft.printf("%c's TURN          ", ME);
        tft.setCursor(5, 105 + 10, 1);
        tft.printf("PRESS BUTTON %d to CHAT", PIN_1);
        return;
      }

      Serial.println("MY TURN!!!");
      if (updbutton_joystick >= 1) {
        //then just do the thing
        int x = scx.index, y = scy.index;
        Serial.printf("----SELECT %d %d----\n", x, y);
        cur_player = board.pieces[board.cur];
        if (board.board[x][y] == ' ') {
          Serial.printf("I'm playing %d %d\n", x, y);
          display_character(x, y, cur_player);
          sprintf(body_ttt, "update=True&player=%c&putx=%d&puty=%d", board.pieces[board.cur], scx.index, scy.index);
          do_request(TTT_DATABASE, body_ttt, response_buffer, true);
          assert(!strstr(this->response_buffer, "cannot put"));
          remove_selection(scx.index, scy.index);
          //        char body_ttt[120];

          if (strstr(this->response_buffer, "continue")) {
            //then we continue.
            display_continue();
            //          sprintf(body_ttt, "update=True&putx=%d&puty=%d&end=False", x, y);
            //          do_request(TTT_DATABASE, body_ttt, m response_buffer, true);
          } else {
            //then we end.
            if (strstr(this->response_buffer, "draw")) {
              //DRAW
              display_draw();
              //            sprintf(body_ttt, "update=True&putx=%d&puty=%d&end=True", x, y);
              //            do_request(TTT_DATABASE, body_ttt, response_buffer, true);
            } else {
              display_win();
              //            sprintf(body_ttt, "update=True&putx=%d&puty=%d&end=%s", x, y, strstr(this->response_buffer, "("));  //is this format OK?
              //            do_request(TTT_DATABASE, body_ttt, response_buffer, true);
            }
            nstate = 2;
          }
          Serial.printf("board trying to put %d %d\n", x, y);
          board.put(x, y);
          Serial.printf("board has been put %d %d\n", x, y);
          //reset scrollers!
          scx = Scroller(3);
          scy = Scroller(3);
          Serial.printf("Scrollers reset\n");
        } else {
          Serial.printf("(%d, %d) cannot put; %c already there\n", x, y, board.board[x][y]);
        }
        Serial.printf("Reached end of NOT CONTINUE in state 1\n");
      } else {
        //      delay(100);
        //      int bv = button.update(); //get button value
        int angx, angy;
        get_angle(&angx, &angy); //get angle values
        //      assert(x != 0);
        //      assert(x != 4095);
        //      assert(y != 0);
        //      assert(y != 4095);
        //      if (millis() % 1000 == 0) Serial.printf("Angles: %d, %d\n", x, y);
        int oldxind = scx.index, oldyind = scy.index;
        //      float meanx, meany, sdx, sdy;
        //      collect_measurements(meanx, sdx, meany, sdy);
        //      scx.update(meanx, sdx, stab_meanx, stab_sdx);
        //      scy.update(meany, sdy, stab_meany, stab_sdy);
        scx.update(angx);
        scy.update(angy);
        if (!(scx.index == oldxind && scy.index == oldyind)) {
          remove_selection(oldxind, oldyind);
          display_selection(scx.index, scy.index);
          Serial.printf("Selection is (%d, %d)\n", scx.index, scy.index);
        }
      }
    }

    void do_not_my_turn() {
      if (needs_another_player) {
        delay(2000);
        do_request(TTT_DATABASE, "needs_another_player=True", response_buffer, true);
        if (strstr(this->response_buffer, "True")) {

        } else {
          needs_another_player = false;
          tft.setCursor(5, 105 + 10, 1);
          tft.printf("PRESS BUTTON %d to CHAT", PIN_1);
        }
      }

      //otherwise try to look for the chat.
      //  updbutton_pin1 |= (button_pin1.update());
      timer = millis();
      while (millis() - timer < 3000) {
        if (digitalRead(PIN_1) == 0) {
          enter_chat();
          //restore the original board.
          sprintf(body_ttt, "update=False&player=%c", ME);
          do_request(TTT_DATABASE, body_ttt, response_buffer, false);  //emphasizing here that you do not suppress "Loading..." message.
          //Actually, after this you also need to restore to original board.
          display_entire_board();
          tft.setCursor(5, 105, 1);
          tft.printf("%c's TURN          ", ME);
          tft.setCursor(5, 105 + 10, 1);
          tft.printf("PRESS BUTTON %d to CHAT", PIN_1);
          break;
        } else if (millis() - timer >= 2000) {
          if (needs_another_player) {
            //then definitely bad.
            Serial.println("Needs another player, didn't click chat. So gonna return.");
            return;
          }
          Serial.println("NOT MY TURN!!");
          //      char body_ttt[25];
          sprintf(body_ttt, "update=False&player=%c", ME);
          //      delay(1000);
          do_request(TTT_DATABASE, body_ttt, response_buffer, true);
          delay(2000);
          break;
        }
      }

      if (*this->response_buffer && strstr(this->response_buffer, "-1")) {
        //-1 - don't do anything.

        //this should happen most  of timeu
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
      //  }
      int x, y;
      sscanf(this->response_buffer, "%d %d", &x, &y);
      char *bufend = strstr(this->response_buffer, " AND ") + strlen(" AND ");
      Serial.printf("The other person played. At %d %d\n", x, y);
      display_character(x, y, 'X' + 'O' - ME);

      if (strncmp(bufend, "False", 5) == 0) {
        //CONTINUE
        //put the new cursor
        assert(scx.index == 0);
        assert(scy.index == 0);
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
    }

    bool loop() {
      //  Serial.println("loop");
      bool retloop = false;

      nstate = state;

      updbutton_joystick = button_joystick.update();
      updbutton16 = button16.update();
      updbutton_pin1 = button_pin1.update();
      //  Serial.printf("updbutton_joystick = %d\n", updbutton_joystick);
      //  Serial.printf("button1  is %d\n", digitalRead(PIN_1));
      if (updbutton_pin1 >= 1) Serial.printf("updbutton_pin1 (pin %d) = %d\n", PIN_1, updbutton_pin1);

      if (state == 0) {
        //This is the homepaage
        if (old_state != state) {
          //reset.
          Serial.println("display_tic_tac_toe_home_screen()");
          display_tic_tac_toe_home_screen();
          memset(msg_lines, 0, sizeof(msg_lines));
        } else {
          if (power_mode_take_care_tic_tac_toe()) return false;
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
          Serial.println("Who Am I?");
          do {
            delay(500);
            do_request(TTT_DATABASE, "who_am_i=True", response_buffer);
            Serial.printf("who_am_i %s\n", this->response_buffer);
            ME = this->response_buffer[0];
          } while (ME != 'X' && ME != 'O');

          //      sprintf(body_ttt, "player=%c&reset=True", ME);
          //      do_request(TTT_DATABASE, body_ttt, response_buffer);
          if (ME == 'X') {
            Serial.println("X reset");
            sprintf(body_ttt, "player=%c&reset=True", ME);
            delay(100);
            do_request(TTT_DATABASE, body_ttt, response_buffer);
          }

          nlines = 0;
          sprintf(output[nlines++], "YOU ARE PLAYER %c", ME);
          display_all_lines();
          delay(2000);

          //just getting in here. display the thing.
          Serial.println("Display Grid");
          display_grid(3, 3, TFT_BLACK); //tic_tac_toe_display
          //this is a test below.
          //      tft.drawLine(30, 5, 80, 5, TFT_BLACK);
          //      tft.drawLine(30, 5, 80, 5, TFT_BLACK);
          scx = Scroller(3);
          scy = Scroller(3);
          board = Board(3, 3, 'X', 'O', TFT_RED, TFT_BLUE, 3);

          needs_another_player = true;
          tft.setCursor(5, 105 + 10, 1);
          tft.setTextColor(TFT_RED, TFT_WHITE);
          tft.printf("AWAITING PLAYER %c         ", 'X' + 'O' - ME);
          tft.setTextColor(TFT_BLACK, TFT_WHITE);

          if (board.pieces[board.cur] == ME) display_selection(scx.index, scy.index);
          Serial.println("got it");

          tft.setCursor(5, 105, 1);
          tft.printf("%c's TURN          ", 'X');
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

    static constexpr int RECT_HEIGHT = 30, RECT_WIDTH = 30;

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

    void display_grid (int w, int h, uint32_t color) {
      tft.fillScreen(TFT_WHITE);
      static constexpr int base_row = 10, base_col = 10, rect_height = 30, rect_width = 30;

      int xmn = base_col, xmx = w * rect_width + base_col;
      int ymn = base_row, ymx = h * rect_height + base_row;

      for (int i = 0; i <= h; i++) {
        int y = i * rect_height + base_row;
        tft.drawLine(xmn, y, xmx, y, color);
      }
      for (int i = 0; i <= w; i++) {
        int x = i * rect_width + base_col;
        tft.drawLine(x, ymn, x, ymx, color);
      }

      //  for (int i = 0; i < w; i++) {
      //    for (int j = 0; j < h; j++) {
      //      tft.drawRect(i * rect_width + base_col, j * rect_height + base_row, rect_width, rect_height, color);
      //    }
      //  }

      //  tft.drawLine(0 * rect_width + base_col, 0 * rect_height + base_row + 1, w * rect_width + base_col, 0 * rect_height + base_row + 1, color);
      //  tft.drawLine(0 * rect_width + base_col - 1, 0 * rect_height + base_row, 0 * rect_width + base_col - 1, h * rect_height + base_row, color);
      //  tft.drawLine(w * rect_width + base_col, h * rect_height + base_row, w * rect_width + base_col, 0 * rect_height + base_row, color);
      //  tft.drawLine(w * rect_width + base_col, h * rect_height + base_row, 0 * rect_width + base_col, h * rect_height + base_row, color);
    }

    void draw_selection (int x, int y, uint32_t color) {
      static constexpr int base_row = 10, base_col = 10, rect_height = 30, rect_width = 30;
      static constexpr int thick = 1;
      static constexpr int line_length = rect_height / 4 - thick;

      int xmn = base_col + x * rect_width, xmx = base_col + (x + 1) * rect_width;
      int ymn = base_row + y * rect_height, ymx = base_row + (y + 1) * rect_height;

      //top left
      tft.drawLine(xmn + thick, ymn + thick, xmn + thick + line_length, ymn + thick, color);
      tft.drawLine(xmn + thick, ymn + thick, xmn + thick, ymn + thick + line_length, color);

      //top right
      tft.drawLine(xmx - thick, ymn + thick, xmx - thick - line_length, ymn + thick, color);
      tft.drawLine(xmx - thick, ymn + thick, xmx - thick, ymn + thick + line_length, color);

      //bottom left
      tft.drawLine(xmn + thick, ymx - thick, xmn + thick + line_length, ymx - thick, color);
      tft.drawLine(xmn + thick, ymx - thick, xmn + thick, ymx - thick - line_length, color);

      //bottom right
      tft.drawLine(xmx - thick, ymx - thick, xmx - thick - line_length, ymx - thick, color);
      tft.drawLine(xmx - thick, ymx - thick, xmx - thick, ymx - thick - line_length, color);
    }

    void display_selection (int x, int y) {
      draw_selection(x, y, TFT_RED);
    }

    void remove_selection (int x, int y) {
      draw_selection(x, y, TFT_WHITE);
    }

    void display_character (int x, int y, char c) {
      Serial.printf("display_character(%d, %d, %c)\n", x, y, c);
      static constexpr int base_row = 10, base_col = 10, rect_height = 30, rect_width = 30;
      int center_x = x * rect_height + base_row + rect_height / 2;
      int center_y = y * rect_width + base_col + rect_width / 2;

      if (c == 'X') {
        int radius = rect_height / 4;
        //tft.drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
        //from http://wiki.microduinoinc.com/tft.drawLine()
        tft.drawLine(center_x - radius, center_y - radius, center_x + radius, center_y + radius, TFT_RED);
        tft.drawLine(center_x - radius, center_y + radius, center_x + radius, center_y - radius, TFT_RED);
      } else if (c == 'O') {
        int radius = rect_height / 4;
        //tft.drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
        //from http://wiki.microduinoinc.com/tft.drawCircle()
        tft.drawCircle(center_x, center_y, radius, TFT_BLUE);
      } else {
        assert(!"Invalid character");
      }
    }

    int sgn (int x) {
      if (x == 0) {
        return 0;
      }
      return x < 0 ? -1 : 1;
    }

    void sweep_line (int x1, int y1, int x2, int y2, bool do_delay = true) {
      Serial.printf("sweep line (%d, %d) to (%d, %d) - Board Coordinates.\n", x1, y1, x2, y2);
      static constexpr int base_row = 10, base_col = 10, rect_height = 30, rect_width = 30;
      x1 = x1 * rect_width + base_col + rect_height / 2;
      y1 = y1 * rect_height + base_row + rect_width / 2;
      x2 = x2 * rect_width + base_col + rect_height / 2;
      y2 = y2 * rect_height + base_row + rect_width / 2;

      Serial.printf("sweep line (%d, %d) to (%d, %d) - Screen Coordinates.\n", x1, y1, x2, y2);

      assert(abs(x1 - x2) == abs(y1 - y2) || x1 == x2 || y1 == y2);
      int incx = sgn(x2 - x1), incy = sgn(y2 - y1);

      const int line_width = 3;

      for (int i = x1, j = y1; !(i == x2 && j == y2); i += incx, j += incy) {
        tft.drawRect(i - line_width, j - line_width, 2 * line_width, 2 * line_width, TFT_GREEN);
        if (do_delay) delay(2);
      }
    }

    void display_tic_tac_toe_home_screen() {
      nlines = 0;
      strcpy(output[nlines++], "HOME SCREEN\n");
      strcpy(output[nlines++], "TIC TAC TOE");
      strcpy(output[nlines++], "");
      strcpy(output[nlines++], "Press:");
      strcpy(output[nlines++], "-16 to return to main menu");
      strcpy(output[nlines++], "-joystick to start game");
      display_all_lines();
      asleep_tic_tac_toe = false;
      last_action_tic_tac_toe = millis();
    }

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

    void get_angle(int* x, int* y) {
      *x = analogRead(A7);
      *y = analogRead(A3);
      // Serial.printf("%d %d\n", *x, *y);
    }

};

void enter_tictactoe() {
  TicTacToe ttt = TicTacToe();
  Serial.println("------START TICTACTOE INTEGRATION---------");
  ttt.setup();
  while (!ttt.loop());
  Serial.println("------END TICTACTOE INTEGRATION---------");
}

//END FILE.
#endif
