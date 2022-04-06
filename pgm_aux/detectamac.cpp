#include <Arduino.h>
/*
  Daniel Carrasco
  This and more tutorials at https://www.electrosoftcloud.com/
*/
// Simple code to retreive the WiFi MAC address
#if defined(ESP32)
  #include "WiFi.h"
#elif defined(ESP8266)
  #include "ESP8266WiFi.h"
#else
  // Non supported board
  #error This board is not supported
#endif
void setup(){
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  Serial.println("WiFi MAC: ");
  Serial.println(WiFi.macAddress());
}
void loop(){
}