/*
Tally CR
MODUL 1 REMOTE (CONDUCTOR O PRODUCTOR)
Aquest modul NO te els connectors GPIO

OLED Display
GND
VCC VIN
SCL GPIO 22
SDA GPIO 21
En teoria es pot connectar a 5V
*/

#include <esp_now.h>
#include <WiFi.h>

#include <Wire.h> //OLED comunicació

#include <Adafruit_GFX.h>     //OLED Display
#include <Adafruit_SSD1306.h> //OLED Display

#include <Adafruit_NeoPixel.h> //Control neopixels

#define VERSIO 1 // Versió del software

// Define PINS
// Botons i leds locals
#define BOTO_VERMELL_PIN GPIO_NUM_13 //???
#define BOTO_VERD_PIN 14
#define LED_VERMELL_PIN 2
#define LED_VERD_PIN 12
#define MATRIX_PIN 4

// Define Quantitat de leds
#define LED_COUNT 72 // 8x8 + 8

// Define sensor battery
#define BATTERY_PIN 39

// Define display
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Declarem neopixels
Adafruit_NeoPixel llum(LED_COUNT, MATRIX_PIN, NEO_GRB + NEO_KHZ800);

// Definim els colors GRB
const uint8_t COLOR[][6] = { {0, 0, 0},     // 0- NEGRE
                             {0, 255, 0},   // 1- VERMELL
                             {0, 0, 255},   // 2- BLAU
                             {255, 0, 0},   //3- VERD
                             {255, 255, 0}, //4- GROC
                             {128, 255, 0}, //5- TARONJA
                             {255, 255, 255} }; //6- BLANC

uint8_t color_matrix = 0; //Per determinar color local

// Cal configurar amb les Mac dels diferents receptors
uint8_t const broadcastAddress0[] = {0xC4, 0xDD, 0x57, 0xEB, 0x2D, 0xDC}; // tally 0 (master)
// uint8_t broadcastAddress1[] = {0xC4, 0xDD, 0x57, 0xEB, 0xF39, 0x8C}; // tally 1 (conductor)
// uint8_t broadcastAddress2[] = {0xC4, 0xDD, 0x57, 0xEB, 0xF43, 0x2C}; // tally 2 (productor)
// uint8_t broadcastAddressX[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; /tally x

// Variable to store if sending data was successful
String success;

// Bool per veure missatges de debug
bool debug = true;

// Identificador del tally (s'hauria de poder modificar)
uint8_t id_local = 0; //Identificador equip - aixi podem intercanviar equips
// Funcio del tally (s'hauria de poder modificar)
uint8_t funcio_local = 0; // De moment li posem el 1 (conductor). 0 es per master. 2 Productor
// Aquesta variable recull el valor en funció de la funció que realirza al sistema
// 0 = MASTER
// 1 = CONDUCTOR
// 2 = PRODUCTOR
// 3 = SENSE BOTONS


// Structure dades a enviar cap al master
typedef struct message_to_master
{
  uint8_t id;     // Identificador del tally
  uint8_t funcio;               // Identificador de la funcio que realitza
  bool lectura_polsador_vermell;    // Posició boto verd
  bool lectura_polsador_verd; // Posició boto vermell
  uint16_t battery;              // Nivell bateria
} message_to_master;

// Structure dades rebudes del master
typedef struct message_from_master
{
  bool cond_led_verd;    // llum confirmació cond polsador verd
  bool cond_led_vermell; // llum confirmació cond polsador vermell
  bool prod_led_verd;    // llum confirmació prod polsador verd
  bool prod_led_vermell; // llum confirmació prod polsador vermell
  //cond_text
  //prod_text
  uint8_t color_tally; // Color indexat del tally
  // hora - Es la mateixa per tos els tally's
} message_from_master;

// Creem una estructura per guardar els missatges rebuts
message_from_master tallyIN;

// Create una estructura per enviar els missatges al master
message_to_master tallyOUT;

// Variables
// Fem arrays de dos valors la 0 és anterior la 1 actual
bool BOTO_LOCAL_VERMELL[] = {false, false};
bool BOTO_LOCAL_VERD[] = {false, false};
uint16_t BATTERY_LOCAL_READ[] = {0, 0};

// Valor dels leds dels polsadors
bool LED_LOCAL_VERMELL = false;
bool LED_LOCAL_VERD = false;

// Variables de gestió
bool LOCAL_CHANGE = false; // Per saber si alguna cosa local ha canviat

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status == 0)
  {
    success = "Delivery Success :)";
  }
  else
  {
    success = "Delivery Fail :(";
  }
}

// Posar llum a un color
void escriure_matrix (uint8_t color)
{
  //GBR 
  uint8_t G = COLOR[color][0];
  uint8_t B = COLOR[color][1];
  uint8_t R = COLOR[color][2];
  for (int i = 0; i < LED_COUNT; i++)
  {
    llum.setPixelColor(i, llum.Color(G, B, R));
  }
  llum.show();
}

void comunicar_valors()
{
 tallyOUT.id = id_local;
 tallyOUT.funcio = funcio_local;
 tallyOUT.lectura_polsador_vermell = BOTO_LOCAL_VERMELL[1];
 tallyOUT.lectura_polsador_verd = BOTO_LOCAL_VERD[1];
 tallyOUT.battery = 1200; // CAL POSAR UNA VARIABLE

 // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress0, (uint8_t *)&tallyOUT, sizeof(tallyOUT));

  if (result == ESP_OK)
  {
    Serial.println("Sent with success");
  }
  else
  {
    Serial.println("Error sending the data");
  }
}

void llegir_botons()
{
  BOTO_LOCAL_VERMELL[1] = digitalRead(BOTO_VERMELL_PIN);
  BOTO_LOCAL_VERD[1] = digitalRead(BOTO_VERD_PIN);
}

void escriure_leds()
{
  digitalWrite(LED_VERMELL_PIN, LED_LOCAL_VERMELL);
  digitalWrite(LED_VERD_PIN, LED_LOCAL_VERD);
}

void llegir_bateria()
{
  BATTERY_LOCAL_READ[1] = analogRead(BATTERY_PIN);
}

void updateDisplay()
{
  // Display Readings on OLED Display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("TALLY CR");

  display.setCursor(0, 15);
  display.print("ID: ");
  display.print(id_local);
  display.setCursor(0, 25);
  display.print("Funcio: ");
  if (funcio_local == 0) {
    display.print("MASTER");
  }
  if (funcio_local == 1) {
    display.print("CONDUCTOR");
  }
  if (funcio_local == 2) {
    display.print("PRODUCTOR");
  }
  /*
  display.setCursor(0, 15);
  display.print("Temperature: ");
  display.print(incomingTemp);
  display.cp437(true);
  display.write(248);
  display.print("C");
  display.setCursor(0, 25);
  display.print("Humidity: ");
  display.print(incomingHum);
  display.print("%");
  display.setCursor(0, 35);
  display.print("Pressure: ");
  display.print(incomingPres);
  display.print("hPa");
  display.setCursor(0, 56);
  display.print(success);
  display.display();

  // Display Readings in Serial Monitor
  Serial.println("INCOMING READINGS");
  Serial.print("Temperature: ");
  Serial.print(incomingReadings.temp);
  Serial.println(" ºC");
  Serial.print("Humidity: ");
  Serial.print(incomingReadings.hum);
  Serial.println(" %");
  Serial.print("Pressure: ");
  Serial.print(incomingReadings.pres);
  Serial.println(" hPa");
  Serial.println();
  */
}

// Callback when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  memcpy(&tallyIN, incomingData, sizeof(tallyIN));
  if (debug)
  {
    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.print("Conductor confirmació Vermell: ");
    Serial.println(tallyIN.cond_led_vermell);
    Serial.print("Conductor confirmació Verd: ");
    Serial.println(tallyIN.cond_led_verd);
    Serial.print("Productor confirmació Vermell: ");
    Serial.println(tallyIN.prod_led_vermell);
    Serial.print("Productor confirmació Verd: ");
    Serial.println(tallyIN.prod_led_verd);
   }
  if (funcio_local == 1) //Conductor
  {
    LED_LOCAL_VERMELL = tallyIN.cond_led_vermell;
    LED_LOCAL_VERD = tallyIN.cond_led_verd;
    // text = tallyIN.text;
  }
  if (funcio_local == 2) //Productor
  {
    LED_LOCAL_VERMELL = tallyIN.prod_led_vermell;
    LED_LOCAL_VERD = tallyIN.prod_led_verd;
    // text = tallyIN.text;
  }
  color_matrix = tallyIN.color_tally;
  escriure_leds(); //Engeguem o parem els leds
  escriure_matrix(color_matrix); //Construim el color per al matrix
  updateDisplay(); //Escrivim el texte

}

void setup()
{
  // Init Serial Monitor
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 TALLY");
  Serial.print("Versió: ");
  Serial.println(VERSIO);

  // Init OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  // Nota de Xavi CB = Callback
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress0, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return;
  }
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);

  // Configurem els pins BOTONS/LEDS
  pinMode(BOTO_VERMELL_PIN, INPUT);
  pinMode(BOTO_VERD_PIN, INPUT);
  pinMode(LED_VERMELL_PIN, OUTPUT);
  pinMode(LED_VERD_PIN, OUTPUT);

  // Esborrem llum
  llum.clear();

  // Identificador del tally (s'hauria de poder modificar)
  id_local = 1; //Identificador equip - aixi podem intercanviar equips
  // Funcio del tally (s'hauria de poder modificar)
  funcio_local = 1; // De moment li posem el 1 (conductor). 0 es per master. 2 Productor
        // Aquesta variable recull el valor en funció de la funció que realirza al sistema
        // 0 = MASTER
        // 1 = CONDUCTOR
        // 2 = PRODUCTOR
        // 3 = SENSE BOTONS
  Serial.print("ID: ");
  Serial.println(id_local);
  Serial.print("Funció: ");
  Serial.println(funcio_local);

}

void loop()
{
  llegir_botons(); // Funcio per llegir valors
  LOCAL_CHANGE = false;
  if (BOTO_LOCAL_VERMELL[0] != BOTO_LOCAL_VERMELL[1])
  {
    /// HEM POLSAT EL BOTO VERMELL
    // FES NO SE QUE
    LOCAL_CHANGE = true;
    BOTO_LOCAL_VERMELL[0] = BOTO_LOCAL_VERMELL[1];
  }

  if (BOTO_LOCAL_VERD[0] != BOTO_LOCAL_VERD[1])
  {
    /// HEM POLSAT EL BOTO VERD
    // FES NO SE QUE
    LOCAL_CHANGE = true;
    BOTO_LOCAL_VERD[0] = BOTO_LOCAL_VERD[1];
  }
  if (LOCAL_CHANGE) {
    comunicar_valors(); // Funció per comunicar valors
  } 
}
