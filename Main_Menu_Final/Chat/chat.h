#ifndef CHAT_H
#define CHAT_H

//#include <WiFiClientSecure.h>
#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>
#include <WiFi.h> //Connect to WiFi Network
#include <SPI.h>
#include <TFT_eSPI.h>
#include <mpu9255_esp32.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "button.h"
#include "wifi_stuff.h"
#include "camera.h"
#include "support_functions.h"

static const char PREFIX[] = "{\"config\":{\"encoding\":\"MULAW\",\"sampleRateHertz\":8000,\"languageCode\": \"en-US\",\"speechContexts\":[{\"phrases\": [\"draw\",\"circle\",\"rectangle\",\"square\",\"doge\",\"hot\"]}]\}, \"audio\": {\"content\":\"";
static const char SUFFIX[] = "\"}}"; //suffix to POST request
static const int AUDIO_IN = A0; //pin where microphone is connected
static const char API_KEY[] = "AIzaSyD6ETx_Ammh1jgbfxG_wggm8eGa08yzQzQ"; //don't change this

static constexpr int DELAY = 1000;
static constexpr int SAMPLE_FREQ = 8000;                        // Hz, telephone sample rate
static constexpr int SAMPLE_DURATION = 3;                        // duration of fixed sampling (seconds)
static constexpr int NUM_SAMPLES = SAMPLE_FREQ * SAMPLE_DURATION;  // number of of samples
static constexpr int ENC_LEN = (NUM_SAMPLES + 2 - ((NUM_SAMPLES + 2) % 3)) / 3 * 4;  // Encoded length of clip

char speech_data[ENC_LEN + 200] = {0}; //global used for collecting speech data  //LENGTH WAS ORIGINALLY ENC_LEN + 200...this is bad??
char temp_holder[1010];
char transcript[1000] = {0};
//char subtrans[1000];
char* chat_request_buffer = request_buffer; //char chat_request_buffer[2000];
char* chat_body = body; //char chat_body[1000];

class Chat {
  public:

    int current;

    HardwareSerial mySoftwareSerial = HardwareSerial(2);
    DFRobotDFPlayerMini myDFPlayer;
    //void printDetail(uint8_t type, int value);  //don't know why this function is here?

    static constexpr uint16_t RESPONSE_TIMEOUT = 6000;
    static constexpr uint16_t OUT_BUFFER_SIZE = 1000; //size of buffer to hold HTTP response
    char *response = response_buffer; //char response[OUT_BUFFER_SIZE] //char array buffer to hold HTTP request

    /* CONSTANTS */
    //Prefix to POST request:
    //constexpr char PREFIX[] = "{\"config\":{\"encoding\":\"MULAW\",\"sampleRateHertz\":8000,\"languageCode\": \"en-US\"}, \"audio\": {\"content\":\"";


    static constexpr uint8_t PIN_1 = 26; //button 1
    static constexpr uint8_t PIN_2 = 27; //button 2
    static constexpr uint8_t PIN_EXIT = 16; //exit pin.
    static constexpr uint8_t PIN_JOYSTICK = 5;


    /* Global variables*/
    uint8_t button_state; //used for containing button state and detecting edges
    uint8_t button_state1; //used for containing button state and detecting edges
    int old_button_state; //used for detecting button edges
    uint32_t time_since_sample;      // used for microsecond timing


    static constexpr char* NETWORK     = "6s08";     // your network SSID (name of wifi network)
    static constexpr char* PASSWORD = "iesc6s08"; // your network password
    //static constexpr char* NETWORK     = "MIT";     // your network SSID (name of wifi network)
    //static constexpr char* PASSWORD = ""; // your network password
    static constexpr char*  SERVER = "speech.google.com";  // Server URL

    uint8_t old_val;
    uint32_t timer;
    //int timer;

    void display_chat_home_screen() {
      tft.fillScreen(TFT_WHITE);
      tft.setCursor(0, 0, 1);
      tft.printf("MESSENGER APP         \n\nHold %d to record\nPress %d to receive\nPress %d to exit\nPress joystick for camera\n", PIN_1, PIN_2, PIN_EXIT);
    }

    void setup() {
      //      Serial.begin(115200);               // Set up serial port
      Serial.println("CAREFUL: ");
      tft.init();  //init screen
      tft.setRotation(3); //adjust rotation
      loading();
      tft.setTextSize(1); //default font size
      tft.fillScreen(TFT_WHITE); //fill background
      tft.setTextColor(TFT_BLACK, TFT_WHITE); //set color of font to green foreground, black background
      tft.setCursor(0, 0, 1);
      tft.printf("Loading...");
      //  Serial.begin(115200); //begin serial comms
      delay(100); //wait a bit (100 ms)
//      pinMode(PIN_1, INPUT_PULLUP);
//      pinMode(PIN_2, INPUT_PULLUP);


      connect_mit_wifi();

      timer = millis();
      old_val = digitalRead(PIN_1);
      analogSetAttenuation(ADC_6db); //set to 6dB attenuation for 3.3V full scale reading.


      mySoftwareSerial.begin(9600, SERIAL_8N1, 32, 33);  // speed, type, RX, TX
      //  Serial.begin(115200);

      Serial.println();
      Serial.println(F("DFRobot DFPlayer Mini Demo"));
      Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
      delay(1000);
      while (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.

        Serial.println(myDFPlayer.readType(), HEX);
        Serial.println(F("Unable to begin:"));
        Serial.println(F("1.Please recheck the connection!"));
        Serial.println(F("2.Please insert the SD card!"));
      }
      Serial.println(F("DFPlayer Mini online."));

      myDFPlayer.setTimeOut(500); //Set serial communictaion time out 500ms

      //----Set volume----
      myDFPlayer.volume(25);  //Set volume value (0~30).
      myDFPlayer.volumeUp(); //Volume Up
      myDFPlayer.volumeDown(); //Volume Down

      //----Set different EQ----
      myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
      //  myDFPlayer.EQ(DFPLAYER_EQ_POP);
      //  myDFPlayer.EQ(DFPLAYER_EQ_ROCK);
      //  myDFPlayer.EQ(DFPLAYER_EQ_JAZZ);
      //  myDFPlayer.EQ(DFPLAYER_EQ_CLASSIC);
      //  myDFPlayer.EQ(DFPLAYER_EQ_BASS);

      //----Set device we use SD as default----
      //  myDFPlayer.outputDevice(DFPLAYER_DEVICE_U_DISK);
      myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
      //  myDFPlayer.outputDevice(DFPLAYER_DEVICE_AUX);
      //  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SLEEP);
      //  myDFPlayer.outputDevice(DFPLAYER_DEVICE_FLASH);

      //----Mp3 control----
      //  myDFPlayer.sleep();     //sleep
      //  myDFPlayer.reset();     //Reset the module
      //  myDFPlayer.enableDAC();  //Enable On-chip DAC
      //  myDFPlayer.disableDAC();  //Disable On-chip DAC
      //  myDFPlayer.outputSetting(true, 15); //output setting, enable the output and set the gain to 15

      int delayms = 100;
      //----Mp3 play----

      //----Read imformation----
      Serial.println(F("readState--------------------"));
      Serial.println(myDFPlayer.readState()); //read mp3 state
      Serial.println(F("readVolume--------------------"));
      Serial.println(myDFPlayer.read()); //read current volume
      //Serial.println(F("readEQ--------------------"));
      //Serial.println(myDFPlayer.readEQ()); //read EQ setting
      Serial.println(F("readFileCounts--------------------"));
      Serial.println(myDFPlayer.readFileCounts()); //read all file counts in SD card
      Serial.println(F("readCurrentFileNumber--------------------"));
      Serial.println(myDFPlayer.readCurrentFileNumber()); //read current play file number
      Serial.println(F("readFileCountsInFolder--------------------"));
      Serial.println(myDFPlayer.readFileCountsInFolder(3)); //read fill counts in folder SD:/03
      Serial.println(F("--------------------"));

      display_chat_home_screen();
    }

    Button button16 = Button(16);
    Button button_joystick = Button(PIN_JOYSTICK);

    //main chat_body of code
    bool loop() {
      if (button_joystick.update() >= 1) {
        enter_camera();
        setup();
        return false;
      }
      button_state1 = digitalRead(PIN_2);
      if (!button_state1) {
        receiving_response();
      }
      button_state = digitalRead(PIN_1);
      if (!button_state && button_state != old_button_state) {
        //    timer=millis();
        Serial.println("What the hell?");
        //    tft.printf("WHAT THE HELL");
        //    delay(30000);

        Serial.println("listening...");
        tft.setCursor(0, 0, 1);
        tft.printf("Recording audio...           \n");
        record_audio();
        Serial.println("sending...");
        Serial.print("\nStarting connection to server...");
        delay(300);

        while (true) {
          bool conn = false;
          Serial.println("Before loop");
          for (int i = 0; i < 10; i++) {
            int val = (int)client.connect(SERVER, 443);
            Serial.print(i); Serial.print(": "); Serial.println(val);
            if (val != 0) {
              conn = true;
              break;
            }
            Serial.print(".");
            delay(300);
          }
          if (!conn) {
            Serial.println("Connection failed!");
            continue;
          } else {
            Serial.println("Connected to server!");
            Serial.println(client.connected());
            int len = strlen(speech_data);
            // Make a HTTP request:
            client.print("POST /v1/speech:recognize?key="); client.print(API_KEY); client.print(" HTTP/1.1\r\n");
            client.print("Host: speech.googleapis.com\r\n");
            client.print("Content-Type: application/json\r\n");
            client.print("cache-control: no-cache\r\n");
            client.print("Content-Length: "); client.print(len);
            client.print("\r\n\r\n");
            int ind = 0;
            int jump_size = 1000;
            //            char temp_holder[jump_size + 10];
            memset(temp_holder, 0, sizeof(temp_holder));
            Serial.println("sending data");
            while (ind < len) {
              delay(80);//experiment with this number!
              //if (ind + jump_size < len) client.print(speech_data.substring(ind, ind + jump_size));
              strncat(temp_holder, speech_data + ind, jump_size);
              client.print(temp_holder);
              ind += jump_size;
              memset(temp_holder, 0, sizeof(temp_holder));
            }
            client.print("\r\n");
            //Serial.print("\r\n\r\n");
            Serial.println("Through send...");
            unsigned long count = millis();
            while (client.connected()) {
              Serial.println("IN!");
              String line = client.readStringUntil('\n');
              Serial.print(line);
              if (line == "\r") { //got header of response
                Serial.println("headers received");
                break;
              }
              if (millis() - count > RESPONSE_TIMEOUT) break;
            }
            Serial.println("");
            Serial.println("Response...");
            count = millis();
            while (!client.available()) {
              delay(100);
              Serial.print(".");
              if (millis() - count > RESPONSE_TIMEOUT) break;
            }
            Serial.println();
            Serial.println("-----------");
            //            memset(response, 0, sizeof(response));
//            global_response(response);
            for (int i = 0; client.available() && i + 1 < OUT_BUFFER_SIZE; i++) {
              response[i] = client.read();
              response[i + 1] = 0;
              //              char_append(response, client.read(), OUT_BUFFER_SIZE);
            }
            Serial.println(response);
            char* trans_id = strstr(response, "transcript");
            bool could_send = false;
            if (trans_id != NULL) {
              char* foll_coll = strstr(trans_id, ":");
              char* starto = foll_coll + 2; //starting index
              char* endo = strstr(starto + 1, "\""); //ending index
              int transcript_len = endo - starto + 1;
              memset(transcript, 0, sizeof(transcript));
              strncat(transcript, starto, transcript_len);
              //        Serial.println(trans_id);
              //         tft.fillScreen(TFT_BLACK); //fill background
              //        char* phrases_id = strstr(response,"phrases");
              //        Serial.println(phrases_id);
              Serial.println(transcript);
              //        char buff[]="sad happy hot dog";
              // char *buff = "this is a test string";
              //              char subtrans[strlen(transcript) - 1];
              //      char subtrans2[]="hot";
              sprintf(chat_body, "user=%c&type=0&message=", ME);
              char* subtrans = chat_body + strlen(chat_body);
              memcpy( subtrans, &transcript[1], strlen(transcript) - 2);
              subtrans[strlen(transcript) - 2] = '\0';
              Serial.println("subbbbb");
              Serial.println(subtrans);
//              strcat(chat_body, subtrans);
              int chat_body_len = strlen(chat_body);
              sprintf(chat_request_buffer, "POST http://608dev.net/sandbox/sc/whu2704/chat.py HTTP/1.1\r\n");
              strcat(chat_request_buffer, "Host: 608dev.net\r\n");
              strcat(chat_request_buffer, "Content-Type: application/x-www-form-urlencoded\r\n");
              sprintf(chat_request_buffer + strlen(chat_request_buffer), "Content-Length: %d\r\n", chat_body_len); //append string formatted to end of request buffer
              strcat(chat_request_buffer, "\r\n"); //new line from header to chat_body
              strcat(chat_request_buffer, chat_body); //chat_body
              strcat(chat_request_buffer, "\r\n"); //header
              Serial.println(chat_request_buffer);

              //CHANGE WHERE THIS IS TARGETED! IT SHOULD TARGET YOUR SERVER SCRIPT
              //CHANGE WHERE THIS IS TARGETED! IT SHOULD TARGET YOUR SERVER SCRIPT
              //CHANGE WHERE THIS IS TARGETED! IT SHOULD TARGET YOUR SERVER SCRIPT
              //  sprintf(chat_request_buffer, "GET /sandbox/sc/yuyu/wiki_getter.py?topic=%s HTTP/1.1\r\n", query);
              //  strcat(chat_request_buffer, "Host: 608dev.net\r\n");
              //  strcat(chat_request_buffer, "\r\n"); //new line from header to chat_body

              strcpy(response, do_http_request("608dev.net", chat_request_buffer, 200, RESPONSE_TIMEOUT, true));
              //        receiving_response();
              could_send = true;
            }
            Serial.println("-----------");
            client.stop();
            Serial.println("done.");

            tft.fillScreen(TFT_WHITE);
            tft.setCursor(0, 0, 1);
            if (could_send) {
              tft.printf("Done sending message.     \n");
            } else {
              tft.printf("Could not send message.   \n");
            }
            delay(1000);
            display_chat_home_screen();
            Serial.println("FUCK");
            break;
          }
        }
        //  Serial.println(button_state);
      }
      old_button_state = button_state;

      return (button16.update() >= 1);
    }

    //function used to record audio at sample rate for a fixed nmber of samples
    void record_audio() {
      tft.fillScreen(TFT_WHITE);
      tft.setCursor(0, 0, 1);
      tft.printf("Recording audio...           \n");
      Serial.println("\"Recording audio...\" should have been printed to screen.");
      int sample_num = 0;    // counter for samples
      int enc_index = strlen(PREFIX) - 1;  // index counter for encoded samples
      float time_between_samples = 1000000 / SAMPLE_FREQ;
      int value = 0;
      char raw_samples[3];   // 8-bit raw sample data array
//      memset(speech_data, 0, sizeof(speech_data));
      sprintf(speech_data, "%s", PREFIX);
      char holder[5] = {0};
      Serial.println("starting");
      uint32_t text_index = enc_index;
      uint32_t start = millis();
      time_since_sample = micros();
      while (sample_num < NUM_SAMPLES && digitalRead(PIN_1) == 0) { //read in NUM_SAMPLES worth of audio data
        value = analogRead(AUDIO_IN);  //make measurement
        raw_samples[sample_num % 3] = mulaw_encode(value - 1241); //remove 1.0V offset (from 12 bit reading)
        sample_num++;
        if (sample_num % 3 == 0) {
          base64_encode(holder, raw_samples, 3);
          strncat(speech_data + text_index, holder, 4);
          text_index += 4;
        }
        // wait till next time to read
        while (micros() - time_since_sample <= time_between_samples); //wait...
        time_since_sample = micros();
      }
      tft.fillScreen(TFT_WHITE);
      tft.setCursor(0, 0, 1);
      tft.printf("Sending...              \n");
      Serial.println(millis() - start);
      strcat(speech_data, SUFFIX); //      sprintf(speech_data + strlen(speech_data), "%s", SUFFIX);
      Serial.println("out");
      delay(2000);
    }


    int8_t mulaw_encode(int16_t sample) {
      //paste the fast one here.
      const uint16_t MULAW_MAX = 0x1FFF;
      const uint16_t MULAW_BIAS = 33;
      uint16_t mask = 0x1000;
      uint8_t sign = 0;
      uint8_t position = 12;
      uint8_t lsb = 0;
      if (sample < 0)
      {
        sample = -sample;
        sign = 0x80;
      }
      sample += MULAW_BIAS;
      if (sample > MULAW_MAX)
      {
        sample = MULAW_MAX;
      }
      for (; ((sample & mask) != mask && position >= 5); mask >>= 1, position--)
        ;
      lsb = (sample >> (position - 4)) & 0x0f;
      return (~(sign | ((position - 5) << 4) | lsb));
    }


    void receiving_response() {
      tft.fillScreen(TFT_WHITE);
      tft.setCursor(0, 0, 1);
      tft.printf("Requesting...              \n");
      //      char result[1000];
      char requesting[1000];

      bool has_no_msg = false;

      sprintf(requesting, "GET http://608dev.net/sandbox/sc/whu2704/chat.py?last_type=True");
      //  strcat(requesting,sentence);
      strcat(requesting, " HTTP/1.1\r\n");
      strcat(requesting, "Host: 608dev.net\r\n");
      strcat(requesting, "\r\n"); //add blank line!
      char *result = do_http_request("608dev.net", requesting, 1000, RESPONSE_TIMEOUT, true);
      int typ = atoi(result);
      if (typ == -1) {
        has_no_msg = true;
        tft.fillScreen(TFT_WHITE);
        tft.setCursor(0, 0, 1);
        tft.printf("No messages in inbox.      \n");
      } else {
        if (typ == 0) {
          sprintf(requesting, "GET http://608dev.net/sandbox/sc/whu2704/chat.py?user=%c&index=0", ME);
          //  strcat(requesting,sentence);
          strcat(requesting, " HTTP/1.1\r\n");
          strcat(requesting, "Host: 608dev.net\r\n");
          strcat(requesting, "\r\n"); //add blank line!
          char *result = do_http_request("608dev.net", requesting, 1000, RESPONSE_TIMEOUT, true);
          Serial.println(result);
          
          if (strstr(result, "Something is not working in your code")) {
            tft.fillScreen(TFT_WHITE);
            tft.setCursor(0, 0, 1);
            tft.printf("Opponent's message couldn't be interpreted\n");
          } else {
            myDFPlayer.volume(0);
            myDFPlayer.play(1);
            delay(3000);
            myDFPlayer.volume(25);
            //        int numbers[10];
            int pos = 0;
    //        char delim[] = ",";
            char *token = strtok(result, ",");
            int type = atoi(token);
            assert(type == 0 || type == 1);
    //        myDFPlayer.play(atoi(token));
    //        delay(2000);
            play_message(token);
    
            tft.fillScreen(TFT_WHITE);
            tft.setCursor(0, 0, 1);
            tft.printf("Done receiving message.    \n");
          }
        } else {
          
        }
      }
      delay(1000);
      display_chat_home_screen();
    }

    void display_picture (char *token) {
      //NOT IMPLEMENTED!!!!!
      token = strtok(NULL, ",");
    }

    void play_message (char* token) {
      while (token)
      {
        token = strtok(NULL, ",");
        if (token) {
          myDFPlayer.play(atoi(token));
          delay(2000);  //was originally delay(3000);
        }
      }
    }
};

void enter_chat() {
  Chat chat = Chat();
  Serial.println("------START CHAT INTEGRATION----------");
  chat.setup();
  while (!chat.loop());
  Serial.println("-------END CHAT INTEGRATION--------");
  //note that you need a reset to original screen too.
}

#endif
