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
#include <SPI.h>
#include <Wire.h> //OLED comunicació

#include <Adafruit_GFX.h>     //OLED Display
#include <Adafruit_SSD1306.h> //OLED Display

#include <Adafruit_NeoPixel.h> //Control neopixels

#define VERSIO 1 // Versió del software

// Define PINS
// Botons i leds locals
#define BOTO_VERMELL_PIN 16
#define BOTO_VERD_PIN 5
#define LED_VERMELL_PIN 17
#define LED_VERD_PIN 18
#define MATRIX_PIN 4

// Define Quantitat de leds
#define LED_COUNT 72 // 8x8 + 8

// Define sensor battery
#define BATTERY_PIN 36

// Define display
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); // El -1 per el reset compartit amb placa

// Declarem neopixels
Adafruit_NeoPixel llum(LED_COUNT, MATRIX_PIN, NEO_GRB + NEO_KHZ800);

// Definim els colors GRB
const uint8_t COLOR[][6] = {{0, 0, 0},        // 0- NEGRE
                            {255, 0, 0},      // 1- VERMELL
                            {0, 0, 255},      // 2- BLAU
                            {255, 0, 0},      // 3- VERD
                            {255, 128, 0},    // 4- GROC
                            {128, 128, 0},    // 5- TARONJA
                            {255, 255, 255}}; // 6- BLANC

uint8_t color_matrix = 0; // Per determinar color local

// Cal configurar amb les Mac dels diferents receptors
uint8_t const broadcastAddress0[] = {0xC4, 0xDD, 0x57, 0xEB, 0x2D, 0xDC}; // tally 0 (master)
// uint8_t broadcastAddress1[] = {0xC4, 0xDD, 0x57, 0xEB, 0xF39, 0x8C}; // tally 1 (conductor)
// uint8_t broadcastAddress2[] = {0xC4, 0xDD, 0x57, 0xEB, 0xF43, 0x2C}; // tally 2 (productor)
// uint8_t broadcastAddressX[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; /tally x

// Variable to store if sending data was successful
String success;

// Bool per veure missatges de debug
bool debug = true;

// Structure dades a enviar cap al master
typedef struct message_to_master
{
  uint8_t id;        // Identificador del tally
  uint8_t funcio;    // Identificador de la funcio que realitza
  bool boto_vermell; // Posició boto verd
  bool boto_verd;    // Posició boto vermell
  uint16_t battery;  // Nivell bateria
} message_to_master;

// Structure dades rebudes del master
typedef struct message_from_master
{
  bool cond_led_verd;    // llum confirmació cond polsador verd
  bool cond_led_vermell; // llum confirmació cond polsador vermell
  bool prod_led_verd;    // llum confirmació prod polsador verd
  bool prod_led_vermell; // llum confirmació prod polsador vermell
  // cond_text
  // prod_text
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
uint8_t id_local = 1;     // Definim com a tally1
uint8_t funcio_local = 1; // Definim conductor

// Valor dels leds dels polsadors
bool LED_LOCAL_VERMELL = false;
bool LED_LOCAL_VERD = false;

// Variables de gestió
bool LOCAL_CHANGE = false; // Per saber si alguna cosa local ha canviat

unsigned long temps_set_config = 0;      // Temps que ha d'estar apretat per configuracio
const unsigned long temps_config = 5000; // Temps per disparar opció config
bool pre_mode_configuracio = false;      // Inici mode configuració
bool mode_configuracio = false;          // Mode configuració

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
void escriure_matrix(uint8_t color)
{
  // GBR
  uint8_t G = COLOR[color][1];
  uint8_t B = COLOR[color][2];
  uint8_t R = COLOR[color][0];
  for (int i = 0; i < LED_COUNT; i++)
  {
    llum.setPixelColor(i, llum.Color(G, B, R));
  }
  llum.show();
  if (debug)
  {
    Serial.print("Color: ");
    Serial.println(color);
    Serial.print("R: ");
    Serial.println(R);
    Serial.print("G: ");
    Serial.println(G);
    Serial.print("B: ");
    Serial.println(B);
  }
}

void comunicar_valors()
{
  tallyOUT.id = id_local;
  tallyOUT.funcio = funcio_local;
  tallyOUT.boto_vermell = BOTO_LOCAL_VERMELL[1];
  tallyOUT.boto_verd = BOTO_LOCAL_VERD[1];
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
  BOTO_LOCAL_VERMELL[1] = !digitalRead(BOTO_VERMELL_PIN); // Els botons son PULLUP per tant els llegirem al revés
  BOTO_LOCAL_VERD[1] = !digitalRead(BOTO_VERD_PIN);
  // Detecció canvi de botons locals
  if (BOTO_LOCAL_VERMELL[0] != BOTO_LOCAL_VERMELL[1])
  {
    /// HEM POLSAT EL BOTO VERMELL
    LOCAL_CHANGE = true;
    BOTO_LOCAL_VERMELL[0] = BOTO_LOCAL_VERMELL[1];
    if (debug)
    {
      Serial.print("Boto local VERMELL: ");
      Serial.println(BOTO_LOCAL_VERMELL[0]);
    }
  }

  if (BOTO_LOCAL_VERD[0] != BOTO_LOCAL_VERD[1])
  {
    /// HEM POLSAT EL BOTO VERD
    LOCAL_CHANGE = true;
    BOTO_LOCAL_VERD[0] = BOTO_LOCAL_VERD[1];
    if (debug)
    {
      Serial.print("Boto local VERD: ");
      Serial.println(BOTO_LOCAL_VERD[0]);
    }
  }
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
  switch (funcio_local)
  {
  case 0:
    display.println("FUNCIO MASTER");
    break;
  case 1:
    display.println("FUNCIO CONDUCTOR");
    break;
  case 2:
    display.println("FUNCIO PRODUCTOR");
    break;
  case 3:
    display.println("SENSE BUTONS");
    break;
  }
  if (mode_configuracio)
  {
    display.println("MODE CONFIGURACIO");
  }
  display.display();
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
  if (funcio_local == 1) // Conductor
  {
    LED_LOCAL_VERMELL = tallyIN.cond_led_vermell;
    LED_LOCAL_VERD = tallyIN.cond_led_verd;
    // text = tallyIN.text;
  }
  if (funcio_local == 2) // Productor
  {
    LED_LOCAL_VERMELL = tallyIN.prod_led_vermell;
    LED_LOCAL_VERD = tallyIN.prod_led_verd;
    // text = tallyIN.text;
  }
  color_matrix = tallyIN.color_tally;
  escriure_leds();               // Engeguem o parem els leds
  escriure_matrix(color_matrix); // Construim el color per al matrix
  updateDisplay();               // Escrivim el texte
}

void detectar_mode_configuracio()
{
  if (LOCAL_CHANGE)
  {
    if (BOTO_LOCAL_VERMELL[0] && BOTO_LOCAL_VERD[0] && !pre_mode_configuracio)
    {
      // Tenim els dos polsadors apretats i no estem en pre_mode_configuracio
      // Entrarem al mode CONFIG
      temps_set_config = millis();
      pre_mode_configuracio = true;
      if (debug)
      {
        Serial.print("PRE CONFIGURACIO MODE");
      }
    }

    if ((!BOTO_LOCAL_VERMELL[0] || !BOTO_LOCAL_VERD[0]) && pre_mode_configuracio)
    { // Si deixem de pulsar botons i estavem en pre_mode_de_configuracio
      if ((millis()) >= (temps_config + temps_set_config))
      {                                // Si ha pasat el temps d'activació
        mode_configuracio = true;      // Entrem en mode configuracio
        pre_mode_configuracio = false; // Sortim del mode preconfiguracio
        if (debug)
        {
          Serial.print("CONFIGURACIO MODE");
        }
      }
      else
      {
        pre_mode_configuracio = false; // Cancelem la preconfiguracio
        mode_configuracio = false;     // Cancelem la configuracio
        if (debug)
        {
          Serial.print("CANCELEM CONFIGURACIO MODE");
        }
      }
    }
  }
}

void setup()
{
  // Init Serial Monitor
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 TALLY SLAVE");
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
  pinMode(BOTO_VERMELL_PIN, INPUT_PULLUP);
  pinMode(BOTO_VERD_PIN, INPUT_PULLUP);
  pinMode(LED_VERMELL_PIN, OUTPUT);
  pinMode(LED_VERD_PIN, OUTPUT);

  // Esborrem llum
  llum.clear();

  // Identificador del tally (s'hauria de poder modificar)
  id_local = 1; // Identificador equip - aixi podem intercanviar equips
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
  if (debug)
  {
    Serial.println("Mode DEBUG activat");
  }
  // Provem d'escriure alguna cosa al Display
  updateDisplay();
}

void loop()
{
  LOCAL_CHANGE = false;
  llegir_botons(); // Funcio per llegir valors
  detectar_mode_configuracio(); // Mirem si estan els dos apretats per CONFIG
  if (LOCAL_CHANGE)
  {
    comunicar_valors(); // Funció per comunicar valors
  }
}
