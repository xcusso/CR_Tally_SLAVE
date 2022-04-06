#include <Arduino.h>
/*
  Daniel Carrasco
  This and more tutorials at https://www.electrosoftcloud.com/
*/
#include <esp_now.h>
#include <WiFi.h>
// Set the SLAVE MAC Address
// C4:DD:57:EB:39:8C
uint8_t slaveAddress[] = {0xC4, 0xDD, 0x57, 0xEB, 0x39, 0x8C};
// Structure to keep the temperature and humidity data from a DHT sensor
typedef struct temp_humidity {
  float temperature;
  float humidity;
} temp_humidity;
// Create a struct_message called myData
temp_humidity dhtData;
// Callback to have a track of sent messages
void OnSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nSend message status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sent Successfully" : "Sent Failed");
}
 
void setup() {
  // Init Serial Monitor
  Serial.begin(115200);
 
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("There was an error initializing ESP-NOW");
    return;
  }
  // We will register the callback function to respond to the event
  esp_now_register_send_cb(OnSent);
  
  // Register the slave
  esp_now_peer_info_t slaveInfo;
  memcpy(slaveInfo.peer_addr, slaveAddress, 6);
  slaveInfo.channel = 0;  
  slaveInfo.encrypt = false;
  
  // Add slave        
  if (esp_now_add_peer(&slaveInfo) != ESP_OK){
    Serial.println("There was an error registering the slave");
    return;
  }
}
void loop() {
  // Set values to send
  // To simplify the code, we will just set two floats and I'll send it 
  dhtData.temperature = 12.5;
  dhtData.humidity = 58.9;
  // Is time to send the messsage via ESP-NOW
  esp_err_t result = esp_now_send(slaveAddress, (uint8_t *) &dhtData, sizeof(dhtData));
   
  if (result == ESP_OK) {
    Serial.println("The message was sent sucessfully.");
  }
  else {
    Serial.println("There was an error sending the message.");
  }
  delay(2000);
}