#include <mpu9255_esp32.h>
#include <WiFi.h> //Connect to WiFi Network
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h> //Used in support of TFT Display
#include <time.h>
#include <vector>

void do_request (const char* server_filename, const char* body, bool suppress_loading) {
  if (!suppress_loading) {
    loading();
  }
  //This is done after the body.
  sprintf(request_buffer,"POST http://608dev.net/sandbox/sc/whu2704/%s HTTP/1.1\r\n", server_filename);
  strcat(request_buffer,"Host: 608dev.net\r\n");
  strcat(request_buffer,"Content-Type: application/x-www-form-urlencoded\r\n");
  sprintf(request_buffer+strlen(request_buffer),"Content-Length: %d\r\n", strlen(body)); //append string formatted to end of request buffer
  strcat(request_buffer,"\r\n"); //new line from header to body
  strcat(request_buffer,body); //body
  strcat(request_buffer,"\r\n"); //header
  Serial.println("---------START REQUEST BUFFER----------------");
  Serial.println(request_buffer);
  Serial.println("---------END REQUEST BUFFER----------------");
  response_buffer[0] = 0;
  do_http_request("608dev.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT,true);
}

void do_request (const char* server_filename, const char* body) {
  do_request(server_filename, body, false);
}
