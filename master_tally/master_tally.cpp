/*
Tally CR
MODUL 0 MASTER
Aquest modul te els connectors GPIO

Equip A: VIA
Equip B: Taula QL

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
#define BOTO_VERMELL_PIN 13
#define BOTO_VERD_PIN 14
#define LED_VERMELL_PIN 2
#define LED_VERD_PIN 12
#define MATRIX_PIN 4

// Define Quantitat de leds
#define LED_COUNT 72 // 8x8 + 8

// Define sensor battery
#define BATTERY_PIN 39

// Equip A (VIA?)
// I Input O Output
/*
#define GPIA1_PIN 28
#define GPIA2_PIN 29
#define GPIA3_PIN 30
#define GPIA4_PIN 31
#define GPIA5_PIN 32
#define GPIA6_PIN 33 // Sensor 5V Per saber si esta conectat el VIA
#define GPOA1_PIN 34
#define GPOA2_PIN 35
#define GPOA3_PIN 36
#define GPOA4_PIN 37
#define GPOA5_PIN 38
*/
const uint8_t GPIA_PIN[] = {28, 29, 30, 31, 32, 33};
const uint8_t GPOA_PIN[] = {34, 35, 36, 37, 38};

// Equip B (QL?)
// I Input O Output
/*
#define GPIB1_PIN 15
#define GPIB2_PIN 16
#define GPIB3_PIN 17
#define GPIB4_PIN 18
#define GPIB5_PIN 19
#define GPIB6_PIN 20 // Sensor 5V Per saber si esta conectada la taula
#define GPOB1_PIN 23
#define GPOB2_PIN 24
#define GPOB3_PIN 25
#define GPOB4_PIN 26
#define GPOB5_PIN 27
*/
uint8_t const GPIB_PIN[] = {15, 16, 17, 18, 19, 20};
uint8_t const GPOB_PIN[] = {23, 24, 25, 26, 27};

// Define display
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Declarem neopixels
Adafruit_NeoPixel llum(LED_COUNT, MATRIX_PIN, NEO_GRB + NEO_KHZ800);

// Definim els colors GRB
uint8_t const COLOR[][6] = {{0, 0, 0},        // 0- NEGRE
                            {0, 255, 0},      // 1- VERMELL
                            {0, 0, 255},      // 2- BLAU
                            {255, 0, 0},      // 3- VERD
                            {255, 255, 0},    // 4- GROC
                            {128, 255, 0},    // 5- TARONJA
                            {255, 255, 255}}; // 6- BLANC

uint8_t color_matrix = 0; // Per determinar color local

// Cal configurar amb les Mac dels diferents receptors
// tally 0 {0xC4, 0xDD, 0x57, 0xEB, 0xF2D, 0xDC} tally 0 (master)
uint const broadcastAddress1[] = {0xC4, 0xDD, 0x57, 0xEB, 0xF39, 0x8C}; // tally 1 (conductor)
uint const broadcastAddress2[] = {0xC4, 0xDD, 0x57, 0xEB, 0xF43, 0x2C}; // tally 2 (productor)
// uint8_t broadcastAddressX[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; /tally x

// Variable to store if sending data was successful
String success;

// Bool per veure missatges de debug
bool debug = true;

// Structure dades rebudes dels tallys
typedef struct message_from_tally
{
  uint8_t id;     // Identificador del tally
  uint8_t funcio; // Identificador de la funcio del tally
  bool boto_vermell;
  bool boto_verd;
  uint16_t battery;
} message_from_tally;

// Structure dades que enviarem als tallys
typedef struct message_to_tally
{
  bool cond_led_vermell; // llum confirmació cond polsador vermell
  bool cond_led_verd;    // llum confirmació cond polsador verd
  bool prod_led_vermell; // llum confirmació prod polsador vermell
  bool prod_led_verd;    // llum confirmació prod polsador verd
  // tallyOUT.cond_text
  // tallyOUT.prod_text
  uint8_t color_tally; // Color indexat del tally
  // hora - Es la mateixa per tos els tally's
} message_to_tally;

// Create a struct_message called localReadings to hold local readings
message_to_tally tallyOUT; // Podriem fer una array per si tenim 2 o més tally
// tally[n] n és igual als tally -1 - els arrays comencen amb 0

// Create a struct_message to hold incoming values readings
message_from_tally tallyIN;
// message_from_tally tallyIN1;
// message_from_tally tallyIN2;

// Podriem fer una array per si tenim 2 o més tally
// tally[n] n és igual als tally -1 - els arrays comencen amb 0

// Variables locals
// Fem arrays de dos valors la 0 és anterior la 1 actual
bool BOTO_LOCAL_VERMELL[] = {false, false};
bool BOTO_LOCAL_VERD[] = {false, false};
uint16_t BATTERY_LOCAL_READ[] = {0, 0};
uint8_t id_local = 0;     // Definim Master
uint8_t funcio_local = 0; // Definim Master

// Valor dels leds dels polsadors
bool LED_LOCAL_VERMELL = false;
bool LED_LOCAL_VERD = false;

// Valors remots
uint16_t battery_level[] = {0, 0, 0};
; // Array per als 3 nivells de battery 0 Master
// 1 Esp1 (no confondre amb el funcio)
// 2 Esp2 (no confondre amb el funcio)
bool CONF_COND_2_PROD = false; // Valor led confirmacio vermell CONDUCTOR
bool CONF_COND_2_ESTU = false; // Valor led confirmacio verd CONDUCTOR
bool CONF_PROD_2_COND = false; // Valor led confirmacio vermell PRODUCTOR
bool CONF_PROD_2_ESTU = false; // Valor led confirmacio verd CONDUCTOR

// Variables GPIO
// Fem arrays de dos valors la 0 és anterior la 1 actual
// El primer valor es el PIN 0 = 1, 1=2..
// Els GPIx tenen dos valors per veure si han canviat.
bool GPIA[][7] = {{false, false, false, false, false, false},
                  {false, false, false, false, false, false}}; // GPI que venen del equip A INPUTS (VIA) 5 pq mirem alimentació
bool GPIB[][7] = {{false, false, false, false, false, false},
                  {false, false, false, false, false, false}}; // GPI que venen del equip B INPUTS (QL) 5 pq mirem alimentació
bool GPOA[] = {false, false, false, false, false};             // GPO que van al equip A OUTPUTS (VIA)
bool GPOB[] = {false, false, false, false, false};             // GPO que van al equip B OUTPUTS (QL)

// Variables de gestió
bool LOCAL_CHANGE = false; // Per saber si alguna cosa local ha canviat
bool GPI_CHANGE = false;   // Per saber si algun GPIO ha canviat

unsigned long nextsend = 0;           // Variable per determinar quan hem d'enviar espnow (rellotge)
const unsigned long tempsdelay = 200; // Temps entre envios

/*
uint8t_t color_tally = 0; //Color indexat del tally
                          // 0- NEGRE
                          // 1- VERMELL
                          // 2- BLAU
                          // 3- VERD
                          // 4- GROC
                          // 5- TARONJA
                          // 6- BLANC

*/
void logica_GPOB()
{
  // Prioritzem les ordres del conductor al productor
  if (GPOB[1])
  {
    GPOB[0] = true; // Fem mute del conductor
    GPOB[2] = false;
    GPOB[3] = false;
    GPOB[4] = false;
  }
  // Prioritzem les ordres del conductor al estudi
  if (!GPOB[1] && GPOB[2])
  {
    GPOB[0] = true; // Fem mute del conductor
    GPOB[3] = false;
    GPOB[4] = false;
  }
  // Prioritzem les ordres del productor al conductor
  if (!GPOB[1] && !GPOB[2] && GPOB[3])
  {
    GPOB[0] = false; // No fem mute del Conductor
    GPOB[4] = false;
  }
  if (!GPOB[1] && !GPOB[2] && !GPOB[3] && GPOB[4])
  {
    GPOB[0] = false; // No fem mute del Conductor
  }
  if (!GPOB[1] && !GPOB[2] && !GPOB[3] && !GPOB[4])
  {
    GPOB[0] = false; // No fem mute del Conductor
    // Si no apretem cap botó ens assegurem unmute del conductor
  }
}

void escriure_gpo()
{
  // Que passa si apretem tots els botons alhora. La QL intententara caregar 4
  // memories al mateix temps -> Cosa que la pot liar parda.
  // La idea es mirar que tan sols un GPOB escrigui els valors.
  // Per no complicar el codi ho farem amb un void.

  logica_GPOB(); // Filtre per enviar només un GPO i fer logica Mute COND

  for (uint8_t i = 0; i < 5; i++)
  {
    digitalWrite(GPOA_PIN[i], GPOA[i]);
    digitalWrite(GPOB_PIN[i], GPOB[i]);
  }
}

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

// Callback when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  memcpy(&tallyIN, incomingData, sizeof(tallyIN));
  if (debug)
  {
    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.print("Id: ");
    Serial.println(tallyIN.id);
    Serial.print("Funcio: ");
    Serial.println(tallyIN.funcio);
    Serial.print("Vermell: ");
    Serial.println(tallyIN.boto_vermell);
    Serial.print("Verd: ");
    Serial.println(tallyIN.boto_verd);
    Serial.print("Bateria: ");
    Serial.println(tallyIN.battery);
  }

  // Escriure a la pantalla alguna cosa
  // missatge rebut de tallyIN.id
  if (tallyIN.funcio == 1)
  {
    GPOB[1] = tallyIN.boto_vermell; // Si funcio=1 conductor enviem bit a gpo2 vermell
    GPOB[2] = tallyIN.boto_verd;    // si funcio=1 conductor enviem bit a gpo2 verd
    // Hauriem de indicar en display bit rebut
  }
  if (tallyIN.funcio == 2)
  {
    GPOB[3] = tallyIN.boto_vermell; // Si funcio=2 productor enviem el bit a gpo4 vermell
    GPOB[4] = tallyIN.boto_verd;    // Si funcio=2 productor enviem el bit a gpo5 verd
    // Hauriem de indicar en display bit rebut
  }
  battery_level[tallyIN.id] = tallyIN.battery; // Guardem al array corresponent al ID
                                               // el nivell de bateria

  escriure_gpo(); // Escribim als gpo de la QL els valors rebuts
}

// Posar llum a un color
void escriure_matrix(uint8_t color)
{
  // GBR
  uint8_t G = COLOR[0][color];
  uint8_t B = COLOR[1][color];
  uint8_t R = COLOR[2][color];
  for (int i = 0; i < LED_COUNT; i++)
  {
    llum.setPixelColor(i, llum.Color(G, B, R));
  }
  llum.show();
}

void enviar_esp_now()
{
  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(0, (uint8_t *)&tallyOUT, sizeof(tallyOUT));

  if (result == ESP_OK)
  {
    Serial.println("Sent with success");
  }
  else
  {
    Serial.println("Error sending the data");
  }
  // updateDisplay();
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

void llegir_gpi()
{
  for (uint8_t i = 0; i < 6; i++)
  {
    GPIA[1][i] = digitalRead(GPIA_PIN[i]);
    GPIB[1][i] = digitalRead(GPIB_PIN[i]);
  }
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
  if (funcio_local == 0)
  {
    display.print("MASTER");
  }
  if (funcio_local == 1)
  {
    display.print("CONDUCTOR");
  }
  if (funcio_local == 2)
  {
    display.print("PRODUCTOR");
  }
  display.setCursor(0, 35);
  display.print("COND Vermell ");
  if (GPOB[1] == true)
  {
    display.print("ON");
  }
  else
  {
    display.print("OFF");
  }
  /*
  display.setCursor(0, 35);
  display.print("COND Verd");
  if (GPOB[2] == true){
    display.print("ON");
  } else {
    display.print("OFF");
  }
  display.setCursor(0, 35);
  display.print("PROD Vermell");
  if (GPOB[3] == true){
    display.print("ON");
  } else {
    display.print("OFF");
  }
  display.setCursor(0, 35);
  display.print("PROD Verd");
  if (GPOB[4] == true){
    display.print("ON");
  } else {
    display.print("OFF");
  }
  */
  display.setCursor(0, 56);
  display.print(success);
  display.display();
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
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  esp_now_peer_info_t peerInfo;
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Register peer 1
  memcpy(peerInfo.peer_addr, broadcastAddress1, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer 1");
    return;
  }
  // Register peer 2
  memcpy(peerInfo.peer_addr, broadcastAddress2, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer 2");
    return;
  }

  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);

  // Configurem els pins BOTONS/LEDS
  pinMode(BOTO_VERMELL_PIN, INPUT);
  pinMode(BOTO_VERD_PIN, INPUT);
  pinMode(LED_VERMELL_PIN, OUTPUT);
  pinMode(LED_VERD_PIN, OUTPUT);
  // Definim els pins de GPIO
  // EQUIP A (VIA)
  /*
  pinMode(GPIA1_PIN, INPUT);
  pinMode(GPIA2_PIN, INPUT);
  pinMode(GPIA3_PIN, INPUT);
  pinMode(GPIA4_PIN, INPUT);
  pinMode(GPIA5_PIN, INPUT);
  pinMode(GPIA6_PIN, INPUT);
  pinMode(GPOA1_PIN, OUTPUT);
  pinMode(GPOA2_PIN, OUTPUT);
  pinMode(GPOA3_PIN, OUTPUT);
  pinMode(GPOA4_PIN, OUTPUT);
  pinMode(GPOA5_PIN, OUTPUT);
  */
  // EQUIP B (QL)
  /*
  pinMode(GPIB1_PIN, INPUT);
  pinMode(GPIB2_PIN, INPUT);
  pinMode(GPIB3_PIN, INPUT);
  pinMode(GPIB4_PIN, INPUT);
  pinMode(GPIB5_PIN, INPUT);
  pinMode(GPIB6_PIN, INPUT);
  pinMode(GPOB1_PIN, OUTPUT);
  pinMode(GPOB2_PIN, OUTPUT);
  pinMode(GPOB3_PIN, OUTPUT);
  pinMode(GPOB4_PIN, OUTPUT);
  pinMode(GPOB5_PIN, OUTPUT);
  */
  for (uint8_t i = 0; i < 6; i++)
  {
    pinMode(GPIA_PIN[i], INPUT);
    pinMode(GPIB_PIN[i], INPUT);
  }
  for (uint8_t i = 0; i < 5; i++)
  {
    pinMode(GPOA_PIN[i], OUTPUT);
    pinMode(GPOB_PIN[i], OUTPUT);
  }
  // Esborrem llum
  llum.clear();
  // Identificador del tally (s'hauria de poder modificar)
  id_local = 0; // Identificador equip - aixi podem intercanviar equips
  // Funcio del tally (s'hauria de poder modificar)
  funcio_local = 2; // De moment li posem el 1 (conductor). 0 es per master. 2 Productor
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
  llegir_gpi();    // Llegim les entrades
  LOCAL_CHANGE = false;
  GPI_CHANGE = false;

  // Detecció canvi de botons locals
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

  // Deteccio de canvis GPI
  for (uint8_t i = 0; i < 6; i++)
  {
    if (GPIA[0][i] != GPIA[1][i])
    {
      // GPI CANVIAT a VIA
      // FES ALGUNA COSA A
      GPI_CHANGE = true;
      GPIA[0][i] = GPIA[1][i];
    }
    if (GPIB[0][i] != GPIB[1][i])
    {
      // GPI CANVIAT a QL
      // FES ALGUNA COSA B
      GPI_CHANGE = true;
      GPIA[0][i] = GPIA[1][i];
    }
  }

  // Si hem fet un canvi local
  if (LOCAL_CHANGE)
  {
    if (tallyIN.funcio == 1) //SOC CONDUCTOR
    {
      GPOB[1] = BOTO_LOCAL_VERMELL[0]; // Si funcio=1 conductor enviem bit a gpo2 vermell
      GPOB[2] = BOTO_LOCAL_VERD[0];    // si funcio=1 conductor enviem bit a gpo2 verd
      // Hauriem de indicar en display bit rebut
    }
    if (tallyIN.funcio == 2) //SOC PRODUCTOR
    {
      GPOB[3] = BOTO_LOCAL_VERMELL[0]; // Si funcio=2 productor enviem el bit a gpo4 vermell
      GPOB[4] = BOTO_LOCAL_VERD[0];    // Si funcio=2 productor enviem el bit a gpo5 verd
      // Hauriem de indicar en display bit rebut
    }
    escriure_gpo(); // Escribim als gpo de la QL els valors locals
  }

  // Si hi hagut un canvi dels GPI
  if (GPI_CHANGE)
  {
    // CANVIS DE COLOR DE TALLYS
    if (GPIA[5] && GPIB[5] && GPIA[0] && GPIB[0])
    {
      // A conectat, B connectat, ON air, Mic obert
      color_matrix = 1; // Vermell
    }

    if (GPIA[5] && GPIB[5] && !GPIA[0] && GPIB[0])
    {
      // A conectat, B connectat, NO ON air, Mic obert
      color_matrix = 4; // Groc
    }

    if (GPIA[5] && GPIB[5] && GPIA[0] && !GPIB[0])
    {
      // A conectat, B connectat, ON air, NO Mic obert
      color_matrix = 5; // Taronja
    }

    if (GPIA[5] && GPIB[5] && !GPIA[0] && !GPIB[0])
    {
      // A conectat, B connectat, NO ON air, NO Mic obert
      color_matrix = 0; // Negre
    }

    if (GPIA[5] && !GPIB[5] && GPIA[0])
    {
      // A conectat, B NO connectat, ON air
      color_matrix = 1; // Vermell
    }

    if (GPIA[5] && !GPIB[5] && !GPIA[0])
    {
      // A conectat, B NO connectat, ON air
      color_matrix = 0; // Negre
    }

    if (!GPIA[5] && GPIB[5] && GPIB[0])
    {
      // A NO conectat, B connectat, Micro Obert
      color_matrix = 3; // Verd - Gravacio local
    }

    if (!GPIA[5] && GPIB[5] && !GPIB[0])
    {
      // A NO conectat, B connectat, NO Micro Obert
      color_matrix = 0; // Negre
    }
    // CANVIS DE COLOR DE TALLYS
    escriure_matrix(color_matrix);

    // CONFIRMACIONS TAULA
    // Com que hem detectat canvi de GPI assignem als Booleans
    // el valor del bit rebut de la taula. Ja siguin true o false
    // ja que es convertiran en leds de confirmació verds o vermells
    // En cas que la taula B GPIB[5] no sigui conectat, assignem tot
    // a off i aixi deixem els leds de confirmació apagats.
    if (GPIB[5])
    {
      CONF_COND_2_PROD = GPIB[1];
      CONF_COND_2_ESTU = GPIB[2];
      CONF_PROD_2_COND = GPIB[3];
      CONF_PROD_2_ESTU = GPIB[4];
    }
    else
    {
      CONF_COND_2_PROD = false;
      CONF_COND_2_ESTU = false;
      CONF_PROD_2_COND = false;
      CONF_PROD_2_ESTU = false;
    }

    // Carreguem els enviaments al Id 1 Conductor
    tallyOUT.cond_led_vermell = CONF_COND_2_PROD; // Confirmacio ordres cond a prod
    tallyOUT.cond_led_verd = CONF_COND_2_ESTU;    // Confirmacio ordres cond a estu
    tallyOUT.prod_led_vermell = CONF_PROD_2_COND; // Confirmacio ordres prod a cond
    tallyOUT.prod_led_verd = CONF_PROD_2_ESTU;    // Confirmacio ordres prod a estu
    // tallyOUT.cond_text
    // tallyOUT.prod_text
    tallyOUT.color_tally = color_matrix; // Enviem el color (el mateix per tots)
    // tallyOUT.hora

    enviar_esp_now();
  }
}