//const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const int GETTING_PERIOD = 5000; //periodicity of getting a number fact.
const uint16_t IN_BUFFER_SIZE = 2000; //size of buffer to hold HTTP request
//const uint16_t OUT_BUFFER_SIZE = 2000; //size of buffer to hold HTTP response
char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response
char body[IN_BUFFER_SIZE];

void do_request (const char* server_filename, const char* body) {
  loading();
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

void send_msg (char *speech) {
  Serial.printf("speech = %s\n", speech);
  bool ok = false;
  do {
    Serial.println("send_msg() sending message...");
    sprintf(body, "user=%s&message=%s", USER, speech);
    do_request("query_speech.py", body);
    nlines = 0;
    sprintf(output[nlines++], "Message sent.");
    sprintf(output[nlines++], "%s\n", speech);
    display_all_lines();
    
    ok = true;
  } while (!ok);
}

void get_msgs() {
  bool ok = false;
  do {
    Serial.println("Getting last messages");
    sprintf(body, "");
    do_request("query_speech.py", body);

    char *str_msgs = "@MESSAGES: ";
    char *ptr_msgs = strstr(response_buffer, str_msgs) + strlen(str_msgs);
    nlines = 0;
    strcpy(output[nlines++], ptr_msgs);

    display_all_lines();
    ok = true;
  } while (!ok);
}
