#ifndef WIFI_STUFF_H
#define WIFI_STUFF_H

#include <mpu9255_esp32.h>
#include <WiFi.h> //Connect to WiFi Network
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
//#include <SPI.h> //Used in support of TFT Display
#include <WiFiClientSecure.h>
#include <time.h>
#include <vector>
#include "support_functions.h"

char ME = 'B';
const int IN_BUFFER_SIZE = 10000;
TFT_eSPI tft = TFT_eSPI();
WiFiClientSecure client; //global WiFiClient Secure object

bool connect_to_wifi (const char *network, const char *password) {
  Serial.println("CONNECT TO WIFI!!!");
  //change based on location!
  //6s08
  //  char network[] = "6s08";
  //  char password[] = "iesc6s08";
  if (password) {
    WiFi.begin(network, password);
  } else {
    WiFi.begin(network);
  }

  //MIT
  //  char network[] = "MIT";
  //  WiFi.begin(network);

  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count < 6) {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.println(WiFi.localIP().toString() + " (" + WiFi.macAddress() + ") (" + WiFi.SSID() + ")");
    delay(500);
    return true;
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    //    ESP.restart(); // restart the ESP (proper way)
    return false;
  }
}

void connect_mit_wifi() {
  while (true) {
    if (connect_to_wifi("MIT", NULL)) {
      break;
    }
    if (connect_to_wifi("6s08", "iesc6s08")) {
      break;
    }
  }
}

void couldnt_connect (char *nmsg) {
  Serial.println("Couldn't connect");
  //  sprintf(nmsg, "Couldn't connect\r\n");
}

static constexpr uint16_t OUT_BUFFER_SIZE = 20000; //size of buffer to hold HTTP response
char body[6000];
static char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response

char* do_http_request(char* host, char* request, uint16_t response_size, uint16_t response_timeout, uint8_t serial) {
  char *response = response_buffer;
  WiFiClient client; //instantiate a client object
  if (client.connect(host, 80)) { //try to connect to host on port 80
    if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces
    client.print(request);
    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      client.readBytesUntil('\n', response, response_size);
      if (serial) Serial.println(response);
      if (strcmp(response, "\r") == 0) { //found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis() - count > response_timeout) break;
    }
    memset(response, 0, response_size);
    count = millis();
    int len = 0;
    while (client.available()) { //read out remaining text (body of response)
//      static constexpr uint16_t OUT_BUFFER_SIZE = 300; //size of buffer to hold HTTP response
      if (len < OUT_BUFFER_SIZE - 1) {
        response[len++] = client.read();
        response[len] = 0;
      }
//      char_append(response, client.read(), OUT_BUFFER_SIZE);
    }
    if (serial) {
      Serial.printf("RESPONSE IS (in do_http_request function): %s\r\n", response);
    }
    client.stop();
    if (serial) Serial.println("-----------");
    return response;
  } else {
    if (serial) Serial.println("connection failed :/");
    if (serial) Serial.println("wait 0.5 sec...");
    client.stop();
    return 0;
  }
}

void loading() {
  Serial.println("here, loading.");
  tft.fillScreen(TFT_WHITE);
  Serial.println("Fill screen white");
  tft.setCursor(0, 0, 1);
  tft.print("Loading...");
}

//static constexpr uint16_t IN_BUFFER_SIZE = 3000; //size of buffer to hold HTTP request
static char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request

void do_request (const char* server_filename, const char* body, char* &response_buffer, bool suppress_loading = false) {
  const int RESPONSE_TIMEOUT = 6000;
  if (!suppress_loading) {
    loading();
  }
  //This is done after the body.
  sprintf(request_buffer, "POST http://608dev.net/sandbox/sc/whu2704/%s HTTP/1.1\r\n", server_filename);
  strcat(request_buffer, "Host: 608dev.net\r\n");
  strcat(request_buffer, "Content-Type: application/x-www-form-urlencoded\r\n");
  sprintf(request_buffer + strlen(request_buffer), "Content-Length: %d\r\n", strlen(body)); //append string formatted to end of request buffer
  strcat(request_buffer, "\r\n"); //new line from header to body
  strcat(request_buffer, body); //body
  strcat(request_buffer, "\r\n"); //header
  Serial.println("---------START REQUEST BUFFER----------------");
  Serial.println(request_buffer);
  Serial.println("---------END REQUEST BUFFER----------------");
  response_buffer = 0;
  do {
    response_buffer = do_http_request("608dev.net", request_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
    Serial.println("RESPONSE BUFFER BELOW\n\n");
    Serial.println(response_buffer);
    Serial.println("--NOW YOU CAN DECIDE IF OK--");
  } while (!response_buffer);
}

//void do_request (const char* server_filename, const char* body, char* &response_buffer) {
//  do_request(server_filename, body, response_buffer, false);
//}

#endif
