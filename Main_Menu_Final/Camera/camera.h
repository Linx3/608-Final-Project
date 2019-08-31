// ArduCAM Mini demo (C)2017 Lee
// Web: http://www.ArduCAM.com
// This program is a demo of how to use most of the functions
// of the library with ArduCAM ESP32 2MP/5MP camera.
// This demo was made for ArduCAM ESP32 2MP/5MP Camera.
// It can take photo and send to the Web.
// It can take photo continuously as video streaming and send to the Web.
// The demo sketch will do the following tasks:
// 1. Set the camera to JPEG output mode.
// 2. if server receives "GET /capture",it can take photo and send to the Web.
// 3.if server receives "GET /stream",it can take photo continuously as video
//streaming and send to the Web.

// This program requires the ArduCAM V4.0.0 (or later) library and ArduCAM ESP32 2MP/5MP camera
// and use Arduino IDE 1.8.1 compiler or above
//#include <WiFiClientSecure.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <WiFi.h>
#include <Wire.h>
//#include <ESP32WebServer.h>
#include <ArduCAM.h>
#include <SPI.h>
#include "memorysaver.h"
#include "JPEGDecoder.h"  // JPEG decoder library
//#include "jpeg1.h"
#include "button.h"
#include "wifi_stuff.h"
#include "support_functions.h"
//#include "chat.h"

template<class T>
T minimum (T a, T b) {
  return a < b ? a : b;
}

int counter = 0;
//TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

//constexpr uint16_t RESPONSE_TIMEOUT = 12000;

#if !(defined ESP32 )
#error Please select the ArduCAM ESP32 UNO board in the Tools/Board
#endif
//This demo can only work on OV2640_MINI_2MP or ARDUCAM_SHIELD_V2 platform.
#if !(defined (OV2640_MINI_2MP)||defined (OV5640_MINI_5MP_PLUS) || defined (OV5642_MINI_5MP_PLUS) \
    || defined (OV5642_MINI_5MP) || defined (OV5642_MINI_5MP_BIT_ROTATION_FIXED) \
    ||(defined (ARDUCAM_SHIELD_V2) && (defined (OV2640_CAM) || defined (OV5640_CAM) || defined (OV5642_CAM))))
#error Please select the hardware platform and camera module in the ../libraries/ArduCAM/memorysaver.h file
#endif

// set GPIO17 as the slave select :
constexpr int CS = 17;
constexpr int CAM_POWER_ON = 10;

//you can change the value of wifiType to select Station or AP mode.
//Default is AP mode.
//int wifiType = 0; // 0:Station  1:AP

////AP mode configuration
////Default is arducam_esp8266.If you want,you can change the AP_aaid  to your favorite name
//constexpr char *AP_ssid = "arducam_esp32";
////Default is no password.If you want to set password,put your password here
//constexpr char *AP_password = NULL;

static constexpr size_t bufferSize = 10000;
static uint8_t buffer[bufferSize] = {0xFF};

void buffer2string2(char* result, int ix) {
  Serial.println("asdfasdf");
  for (int i = 0; i < 2000; i++) {
    result[2 * i] = (char) (buffer[i + ix * 2000] % 16 + 97);
    result[2 * i + 1] = (char) (buffer[i + ix * 2000] / 16 + 97);
  }
  result[2 * 2000] = 0;
}

void string2buffer(char* string, int ix) {
  for (int i = 0; i < 2000; i++) {
    buffer[i + ix * 2000] = string[2 * i] - 97 + 16 * (string[2 * i + 1] - 97);
  }
}


uint8_t temp = 0, temp_last = 0;
int i = 0;
bool is_header = false;

static constexpr uint16_t RESPONSE_TIMEOUT = 30000;
static constexpr uint16_t OUT_BUFFER_SIZE_CAMERA = 20000; //size of buffer to hold HTTP response
//char response[OUT_BUFFER_SIZE_CAMERA]; //char array buffer to hold HTTP request
char* result_cam = response_buffer;
char *request_buffer_cam = request_buffer;
//char request_buffer_cam[bufferSize + 200];

//ESP32WebServer server(80);
static constexpr int BUTTON_PIN1 = 26;
static constexpr int BUTTON_PIN2 = 5;
static constexpr int BUTTON_EXIT = 16;

class Camera {
  public:
    Button cameraButton = Button(BUTTON_PIN1);
    Button getButton = Button(BUTTON_PIN2);
    Button exitButton = Button(BUTTON_EXIT);

#if defined (OV2640_MINI_2MP) || defined (OV2640_CAM)
    ArduCAM myCAM = ArduCAM(OV2640, CS);
#elif defined (OV5640_MINI_5MP_PLUS) || defined (OV5640_CAM)
    ArduCAM myCAM = ArduCAM(OV5640, CS);
#elif defined (OV5642_MINI_5MP_PLUS) || defined (OV5642_MINI_5MP) || defined (OV5642_MINI_5MP_BIT_ROTATION_FIXED) ||(defined (OV5642_CAM))
    ArduCAM myCAM = ArduCAM(OV5642, CS);
#endif
    //Station mode you should put your ssid and password
    //constexpr char *NETWORK = "6s08"; // Put your SSID here
    //constexpr char *PASSWORD = "iesc6s08"; // Put your PASSWORD here
    //constexpr char *NETWORK = "iPhone"; // Put your SSID here
    //constexpr char *PASSWORD = "sammyisthebest"; // Put your PASSWORD here
    //constexpr char *NETWORK = "MIT"; // Put your SSID here
    //constexpr char *PASSWORD = ""; // Put your PASSWORD here

    void start_capture() {
      myCAM.clear_fifo_flag();
      myCAM.start_capture();
    }

    void camCapture(ArduCAM myCAM) {
      Serial.println(1221);
      //WiFiClient client = server.client();
      uint32_t len  = myCAM.read_fifo_length();
      Serial.print("LENGTH: ");
      Serial.println(len);
      if (len >= MAX_FIFO_SIZE) //8M
      {
        Serial.println(F("Over size."));
      }
      if (len == 0) //0 kb
      {
        Serial.println(F("Size is 0."));
      }
      myCAM.CS_LOW();
      myCAM.set_fifo_burst();
      //if (!client.connected()) return;
      String response = "HTTP/1.1 200 OK\r\n";
      response += "Content-Type: image/jpeg\r\n";
      response += "Content-len: " + String(len) + "\r\n\r\n";
      //server.sendContent(response);
      i = 0;
      while ( len-- )
      {
        //    Serial.println(myCAM.read_fifo());
        //    counter=counter+1;
        temp_last = temp;
        temp = SPI.transfer(0x00);
        //Read JPEG data from FIFO
        if ( (temp == 0xD9) && (temp_last == 0xFF) ) //If find the end ,break while,
        {
          buffer[i++] = temp;  //save the last  0XD9
          //Write the remain bytes in the buffer
          //if (!client.connected()) break;
          //client.write(&buffer[0], i);
          is_header = false;
          i = 0;
          myCAM.CS_HIGH();
          break;
        }
        if (is_header == true)
        {
          counter++;
          //Write image data to buffer if not full
          if (i < bufferSize)
            buffer[i++] = temp;
          else
          {
            //Write bufferSize bytes image data to file
            //if (!client.connected()) break;
            //client.write(&buffer[0], bufferSize);
            i = 0;
            buffer[i++] = temp;
          }
        }
        else if ((temp == 0xD8) & (temp_last == 0xFF))
        {
          is_header = true;
          buffer[i++] = temp_last;
          buffer[i++] = temp;
        }
      }
      tft.fillScreen(TFT_WHITE);
      tft.setCursor(0, 0, 1);
      tft.printf("Sending picture...\n");
      //  Serial.println()
      //this can't ever happen, as far as I'm concerned????
//      for (int j = 0; j < 10000; j++) {
//        //Serial.println("cap");
//        //Serial.println(buffer[i]);
//        if (buffer[j] < 0 || buffer[j] > 255) {
//          Serial.println(buffer[j]);
//          Serial.println("PANIC! GURU MEDITATION!");
//        }
//      }

//      tft.fillScreen(TFT_WHITE);
//      tft.setCursor(0, 0, 1);
//      tft.printf("Sending picture...\n");
      Serial.println("qwerweqr");
      //  string2buffer("adsf");
//      memset(request_buffer_cam, '\0', sizeof(request_buffer_cam));
      for (int i = 0; i < 5; i++) {
        //    memset(result_cam, 0, sizeof(result_cam));  //no need for this...
        buffer2string2(result_cam, i);
        body[0] = 0;  //actually this is enough.
        sprintf(body, "user=%c&message=", ME);
        Serial.println(strlen(result_cam));
        strcat(body, result_cam);
        int body_len = strlen(body);
        sprintf(request_buffer_cam, "POST http://608dev.net/sandbox/sc/whu2704/images.py HTTP/1.1\r\n");
        strcat(request_buffer_cam, "Host: 608dev.net\r\n");
        strcat(request_buffer_cam, "Content-Type: application/x-www-form-urlencoded\r\n");
        sprintf(request_buffer_cam + strlen(request_buffer_cam), "Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
        strcat(request_buffer_cam, "\r\n"); //new line from header to body
        strcat(request_buffer_cam, body); //body
        strcat(request_buffer_cam, "\r\n"); //header
        //Serial.println(request_buffer_cam);
        do_http_request("608dev.net", request_buffer_cam, 300, RESPONSE_TIMEOUT, true);
      }


      //Serial.println(request_buffer_cam);
      //  string2buffer(result_cam, 50);
      //  for (int i = 0; i < 50; i++) {
      //    Serial.println(buffer[i]);
      //  }
    }
    //////////////////////////////////////////////asdfa sdf ads fasd fa dsf df//////
    void receive() {
//      char requesting[200];
      tft.fillScreen(TFT_WHITE);
      tft.setCursor(0, 0, 1);
      tft.setTextColor(TFT_BLACK);
      tft.printf("Requesting picture...\n");
      char *requesting = request_buffer;
      for (int i = 0; i < 5; i++) {
//        memset(result_cam, '\0', sizeof(result_cam));
        //memset(request_buffer_cam, '\0', sizeof(request_buffer_cam));
//        memset(requesting, 0, sizeof(requesting));
        sprintf(requesting, "GET http://608dev.net/sandbox/sc/whu2704/images.py?user=%c&index=%d", ME, 4 - i);
        //  strcat(requesting,sentence);
        strcat(requesting, " HTTP/1.1\r\n");
        strcat(requesting, "Host: 608dev.net\r\n");
        strcat(requesting, "\r\n"); //add blank line!
        result_cam = 0;
        do {
//                  do_http_request("608dev.net", requesting, 2010, RESPONSE_TIMEOUT, true);  
          result_cam = do_http_request("608dev.net", requesting, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
          Serial.println("RESPONSE BUFFER BELOW\n\n");
          Serial.println(result_cam);
          Serial.println("--NOW YOU CAN DECIDE IF OK--");
        } while (!result_cam);
//        do_request("608dev.net", requesting, 2010, RESPONSE_TIMEOUT, true);

        string2buffer(result_cam, i);
      }

      tft.fillScreen(TFT_WHITE);
      tft.setCursor(0, 0, 1);
      tft.setTextColor(TFT_BLACK);
      tft.printf("Press %d to exit...\n", BUTTON_EXIT);
      delay(1000);

      tft.setTextColor(TFT_RED, TFT_LIGHTGREY);
      tft.fillScreen(TFT_LIGHTGREY);
      tft.drawCircle(10, 10, 10, ST7735_YELLOW);

      JpegDec.decodeArray(buffer, sizeof(buffer));
      // print information about the image to the serial port
      jpegInfo();

      // render the image onto the screen at coordinate 0,0
      renderJPEG(0, 0);

      // wait a little bit before clearing the screen to random color and drawing again
      //delay(4000);

      // clear screen
      //tft.fillScreen(random(0xFFFF));  // Alternative: tft.background(255, 255, 255);
      //  tft.drawBitmap(0, 0, buffer, 160,120, TFT_BLACK);
      //for(int i=0;i<160*120;i++){
      //tft.drawPixel(i/160,i%120,buffer[i]);
      //}
      Serial.println("counter");
      Serial.println(counter);

      while (exitButton.update() == 0);
      display_camera_home_screen();
    }

    void serverCapture() {
      delay(1000);
      start_capture();
      Serial.println(F("CAM Capturing"));

      int total_time = 0;

      total_time = millis();
      while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
      total_time = millis() - total_time;
      Serial.print(F("capture total_time used (in miliseconds):"));
      Serial.println(total_time, DEC);

      total_time = 0;

      Serial.println(F("CAM Capture Done."));
      total_time = millis();
      camCapture(myCAM);
      total_time = millis() - total_time;
      Serial.print(F("send total_time used (in miliseconds):"));
      Serial.println(total_time, DEC);
      Serial.println(F("CAM send Done."));
    }

//    WiFiClientSecure client; //global WiFiClient Secure object

    void display_camera_home_screen() {
      tft.fillScreen(TFT_WHITE);
      tft.setTextColor(TFT_BLACK, TFT_WHITE);
      tft.setCursor(0, 0, 1);
      tft.printf("CAMERA APP         \n\nPress %d to capture\n\nPress:\n-Joystick to receive\n-%d to exit\n", BUTTON_PIN1, BUTTON_EXIT);
    }

    void setup() {
      //  Serial.begin(115200); //initialize serial!
      tft.init(); //initialize the screen
      tft.setRotation(2); //set rotation for our layout
      loading();
      //  pinMode(BUTTON_PIN1, INPUT_PULLUP);
      //  pinMode(BUTTON_PIN2, INPUT_PULLUP);
      uint8_t vid, pid;
      uint8_t temp;
      //set the CS as an output:
      pinMode(CS, OUTPUT);
      pinMode(CAM_POWER_ON , OUTPUT);
      digitalWrite(CAM_POWER_ON, HIGH);
#if defined(__SAM3X8E__)
      Wire1.begin();
#else
      Wire.begin();
#endif
      Serial.begin(115200);
      Serial.println(F("ArduCAM Start!"));



      // initialize SPI:
      SPI.begin();
      SPI.setFrequency(4000000); //4MHz

      //Check if the ArduCAM SPI bus is OK
      myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
      temp = myCAM.read_reg(ARDUCHIP_TEST1);
      if (temp != 0x55) {
        Serial.println(F("SPI1 interface Error!"));
        while (1);
      }

      //Check if the ArduCAM SPI bus is OK
      myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
      temp = myCAM.read_reg(ARDUCHIP_TEST1);
      if (temp != 0x55) {
        Serial.println(F("SPI1 interface Error!"));
        while (1);
      }
#if defined (OV2640_MINI_2MP) || defined (OV2640_CAM)
      //Check if the camera module type is OV2640
      myCAM.wrSensorReg8_8(0xff, 0x01);
      myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
      myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
      if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 )))
        Serial.println(F("Can't find OV2640 module!"));
      else
        Serial.println(F("OV2640 detected."));
#elif defined (OV5640_MINI_5MP_PLUS) || defined (OV5640_CAM)
      //Check if the camera module type is OV5640
      myCAM.wrSensorReg16_8(0xff, 0x01);
      myCAM.rdSensorReg16_8(OV5640_CHIPID_HIGH, &vid);
      myCAM.rdSensorReg16_8(OV5640_CHIPID_LOW, &pid);
      if ((vid != 0x56) || (pid != 0x40))
        Serial.println(F("Can't find OV5640 module!"));
      else
        Serial.println(F("OV5640 detected."));
#elif defined (OV5642_MINI_5MP_PLUS) || defined (OV5642_MINI_5MP) || defined (OV5642_MINI_5MP_BIT_ROTATION_FIXED) ||(defined (OV5642_CAM))
      //Check if the camera module type is OV5642
      myCAM.wrSensorReg16_8(0xff, 0x01);
      myCAM.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
      myCAM.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
      if ((vid != 0x56) || (pid != 0x42)) {
        Serial.println(F("Can't find OV5642 module!"));
      }
      else
        Serial.println(F("OV5642 detected."));
#endif


      //Change to JPEG capture mode and initialize the OV2640 module
      myCAM.set_format(JPEG);
      myCAM.InitCAM();
#if defined (OV2640_MINI_2MP) || defined (OV2640_CAM)
      myCAM.OV2640_set_JPEG_size(OV2640_320x240);
#elif defined (OV5640_MINI_5MP_PLUS) || defined (OV5640_CAM)
      myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);   //VSYNC is active HIGH
      myCAM.OV5640_set_JPEG_size(OV5640_320x240);
#elif defined (OV5642_MINI_5MP_PLUS) || defined (OV5642_MINI_5MP) || defined (OV5642_MINI_5MP_BIT_ROTATION_FIXED) ||(defined (OV5642_CAM))
      myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);   //VSYNC is active HIGH
      myCAM.OV5640_set_JPEG_size(OV5642_320x240);
#endif

      myCAM.clear_fifo_flag();
      //  WiFi.begin(NETWORK, PASSWORD); //attempt to connect to wifi
//      connect_mit_wifi();
      // Start the server
      //server.on("/capture", HTTP_GET, serverCapture);
      //server.on("/stream", HTTP_GET, serverStream);
      //  server.onNotFound(handleNotFound);
      //  server.begin();
      //  Serial.println(F("Server started"));
      display_camera_home_screen();
    }

    bool loop() {
      //server.handleClient();
      //  for (int i = 0; i < 2048; i++) {
      ////    Serial.println("buffering");
      ////    Serial.println(buffer[i]);
      //  }
      //  Serial.println("eee");
      //  start_capture();
      //  camCapture(myCAM);

      int bv = cameraButton.update();
      int bv2 = getButton.update();
      int bve = exitButton.update();

      if (bve >= 1) {
        return true;
      }

      if (bv2 == 1) {
        receive();
      }
      if (bv == 1) {
        counter = 0;
//        memset(result_cam, '\0', sizeof(result_cam));
//        memset(request_buffer_cam, '\0', sizeof(request_buffer_cam));
        uint8_t vid, pid;
        //set the CS as an output:
        pinMode(CS, OUTPUT);
        pinMode(CAM_POWER_ON , OUTPUT);
        digitalWrite(CAM_POWER_ON, HIGH);
        uint8_t temp = 0, temp_last = 0;
        i = 0;
        is_header = false;
        SPI.begin();
        SPI.setFrequency(4000000); //4MHz

        myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
        temp = myCAM.read_reg(ARDUCHIP_TEST1);
        if (temp != 0x55) {
          Serial.println(F("SPI1 interface Error!"));
          while (1);
        }

        //Check if the ArduCAM SPI bus is OK
        myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
        temp = myCAM.read_reg(ARDUCHIP_TEST1);
        if (temp != 0x55) {
          Serial.println(F("SPI1 interface Error!"));
          while (1);
        }

#if defined (OV2640_MINI_2MP) || defined (OV2640_CAM)
        //Check if the camera module type is OV2640
        myCAM.wrSensorReg8_8(0xff, 0x01);
        myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
        myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
        if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 )))
          Serial.println(F("Can't find OV2640 module!"));
        else
          Serial.println(F("OV2640 detected."));
#elif defined (OV5640_MINI_5MP_PLUS) || defined (OV5640_CAM)
        //Check if the camera module type is OV5640
        myCAM.wrSensorReg16_8(0xff, 0x01);
        myCAM.rdSensorReg16_8(OV5640_CHIPID_HIGH, &vid);
        myCAM.rdSensorReg16_8(OV5640_CHIPID_LOW, &pid);
        if ((vid != 0x56) || (pid != 0x40))
          Serial.println(F("Can't find OV5640 module!"));
        else
          Serial.println(F("OV5640 detected."));
#elif defined (OV5642_MINI_5MP_PLUS) || defined (OV5642_MINI_5MP) || defined (OV5642_MINI_5MP_BIT_ROTATION_FIXED) ||(defined (OV5642_CAM))
        //Check if the camera module type is OV5642
        myCAM.wrSensorReg16_8(0xff, 0x01);
        myCAM.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
        myCAM.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
        if ((vid != 0x56) || (pid != 0x42)) {
          Serial.println(F("Can't find OV5642 module!"));
        }
        else
          Serial.println(F("OV5642 detected."));
#endif


        //Change to JPEG capture mode and initialize the OV2640 module
        myCAM.set_format(JPEG);
        myCAM.InitCAM();
#if defined (OV2640_MINI_2MP) || defined (OV2640_CAM)
        myCAM.OV2640_set_JPEG_size(OV2640_320x240);
#elif defined (OV5640_MINI_5MP_PLUS) || defined (OV5640_CAM)
        myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);   //VSYNC is active HIGH
        myCAM.OV5640_set_JPEG_size(OV5640_320x240);
#elif defined (OV5642_MINI_5MP_PLUS) || defined (OV5642_MINI_5MP) || defined (OV5642_MINI_5MP_BIT_ROTATION_FIXED) ||(defined (OV5642_CAM))
        myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);   //VSYNC is active HIGH
        myCAM.OV5640_set_JPEG_size(OV5642_320x240);
#endif

        myCAM.clear_fifo_flag();

        serverCapture();
        display_camera_home_screen();

        //delay(5000);
      }

      return false;
    }

    void jpegInfo() {
      Serial.println(F("==============="));
      Serial.println(F("JPEG image info"));
      Serial.println(F("==============="));
      Serial.print(F(  "Width      :")); Serial.println(JpegDec.width);
      Serial.print(F(  "Height     :")); Serial.println(JpegDec.height);
      Serial.print(F(  "Components :")); Serial.println(JpegDec.comps);
      Serial.print(F(  "MCU / row  :")); Serial.println(JpegDec.MCUSPerRow);
      Serial.print(F(  "MCU / col  :")); Serial.println(JpegDec.MCUSPerCol);
      Serial.print(F(  "Scan type  :")); Serial.println(JpegDec.scanType);
      Serial.print(F(  "MCU width  :")); Serial.println(JpegDec.MCUWidth);
      Serial.print(F(  "MCU height :")); Serial.println(JpegDec.MCUHeight);
      Serial.println(F("==============="));
    }


    //====================================================================================
    //   Decode and paint onto the TFT screen
    //====================================================================================
    void renderJPEG(int xpos, int ypos) {

      // retrieve infomration about the image
      uint16_t *pImg;
      uint16_t mcu_w = JpegDec.MCUWidth;
      uint16_t mcu_h = JpegDec.MCUHeight;
      uint32_t max_x = JpegDec.width;
      uint32_t max_y = JpegDec.height;

      // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
      // Typically these MCUs are 16x16 pixel blocks
      // Determine the width and height of the right and bottom edge image blocks
      Serial.print("mcu_w");
      Serial.println(mcu_w);
      uint32_t min_w = minimum((uint32_t) mcu_w, (uint32_t) (max_x % (mcu_w + 1)));
      uint32_t min_h = minimum((uint32_t) mcu_h, (uint32_t) (max_y % (mcu_h + 1)));

      // save the current image block size
      uint32_t win_w = mcu_w;
      uint32_t win_h = mcu_h;

      // record the current time so we can measure how long it takes to draw an image
      uint32_t drawTime = millis();

      // save the coordinate of the right and bottom edges to assist image cropping
      // to the screen size
      max_x += xpos;
      max_y += ypos;

      // read each MCU block until there are no more
      while ( JpegDec.read()) {

        // save a pointer to the image block
        pImg = JpegDec.pImage;

        // calculate where the image block should be drawn on the screen
        int mcu_x = JpegDec.MCUx * mcu_w + xpos;
        int mcu_y = JpegDec.MCUy * mcu_h + ypos;

        // check if the image block size needs to be changed for the right and bottom edges
        if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
        else win_w = min_w;
        if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
        else win_h = min_h;

        // calculate how many pixels must be drawn
        uint32_t mcu_pixels = win_w * win_h;

        // draw image block if it will fit on the screen
        if ( ( mcu_x + win_w) <= tft.width() && ( mcu_y + win_h) <= tft.height()) {
          // open a window onto the screen to paint the pixels into
          //tft.setAddrWindow(mcu_x, mcu_y, mcu_x + win_w - 1, mcu_y + win_h - 1);
          tft.setAddrWindow(mcu_x, mcu_y, mcu_x + win_w - 1, mcu_y + win_h - 1);
          // push all the image block pixels to the screen
          while (mcu_pixels--) tft.pushColor(*pImg++); // Send to TFT 16 bits at a time
        }

        // stop drawing blocks if the bottom of the screen has been reached
        // the abort function will close the file
        else if ( ( mcu_y + win_h) >= tft.height()) JpegDec.abort();

      }

      // calculate how long it took to draw the image
      drawTime = millis() - drawTime; // Calculate the time it took

      // print the results to the serial port
      Serial.print  ("Total render time was    : "); Serial.print(drawTime); Serial.println(" ms");
      Serial.println("=====================================");

    }
};

void enter_camera() {
  Camera camera = Camera();
  Serial.println("------START CAMERA INTEGRATION----------");
  camera.setup();
  while (!camera.loop());
  //note that you need a reset to original screen too.
  Serial.println("-------END CAMERA INTEGRATION--------");
}
