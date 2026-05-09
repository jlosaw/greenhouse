#include <Wire.h>
#include <U8g2lib.h>
#include "DHT.h"

// OLED -------------------------------------------------
U8G2_SSD1309_128X64_NONAME0_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// DHT --------------------------------------------------
#define DHTPIN 7
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Pins -------------------------------------------------
const int manualValveButtonPin = 2;
const int setpointButtonPin = 3;

const int shdnPin = 8;
const int setPin = 9;
const int resetPin = 11;

// Variables --------------------------------------------
volatile bool setpointButtonFlag = false;
volatile bool manualValveButtonFlag = false;

unsigned long lastSetpointPress = 0;
unsigned long lastManualPress = 0;
const unsigned long debounceDelay = 250;

int triggers = 0;
int setpoint = 96;
bool valveOpen = false;

// Function Prototypes ----------------------------------
void boosterOff();
void boosterOn();
void openValve();
void closeValve();

void setpointButtonISR();
void manualValveButtonISR();

void handleSetpointButton();
void handleManualValveButton();

void drawInitializingScreen();
void drawClosingValveScreen();
void drawManualValveScreen();
void drawMainStatusScreen(float tempF, float humidity);
void drawMistingScreen(float tempF);

// Booster control --------------------------------------
void boosterOff() {
  digitalWrite(shdnPin, HIGH);
}

void boosterOn() {
  digitalWrite(shdnPin, LOW);
}

// Interrupts -------------------------------------------
void setpointButtonISR() {
  setpointButtonFlag = true;
}

void manualValveButtonISR() {
  manualValveButtonFlag = true;
}

// Setup -------------------------------------------------
void setup() {
  Serial.begin(9600);

  pinMode(setPin, OUTPUT);
  pinMode(resetPin, OUTPUT);
  pinMode(shdnPin, OUTPUT);

  pinMode(setpointButtonPin, INPUT_PULLUP);
  pinMode(manualValveButtonPin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(setpointButtonPin), setpointButtonISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(manualValveButtonPin), manualValveButtonISR, FALLING);

  u8g2.begin();
  dht.begin();

  drawInitializingScreen();

  boosterOn();
  delay(1000);
  boosterOff();
  delay(2000);

  drawClosingValveScreen();

  digitalWrite(resetPin, HIGH);
  delay(1000);
  digitalWrite(resetPin, LOW);
  delay(1000);

  valveOpen = false;
}

// Loop --------------------------------------------------
void loop() {
  handleSetpointButton();
  handleManualValveButton();

  float temp = dht.readTemperature(true);
  float h = dht.readHumidity();

  Serial.print(F("Setpoint: "));
  Serial.println(setpoint);

  Serial.print(F("Humidity (%): "));
  Serial.println(h, DEC);

  Serial.print(F("Temperature F: "));
  Serial.println(temp, DEC);

  drawMainStatusScreen(temp, h);

  if (temp > setpoint) {
    openValve();

    Serial.println(F("Cooling"));

    drawMistingScreen(temp);

    delay(180000);

    closeValve();

    triggers++;
  }

  if (triggers > 100) {
    triggers = 0;
  }

  delay(10000);
}

// Button Handlers --------------------------------------
void handleSetpointButton() {
  if (setpointButtonFlag) {
    setpointButtonFlag = false;

    unsigned long now = millis();

    if (now - lastSetpointPress > debounceDelay) {
      lastSetpointPress = now;

      setpoint++;

      if (setpoint > 100) {
        setpoint = 92;
      }

      Serial.print(F("New setpoint: "));
      Serial.println(setpoint);
    }
  }
}

void handleManualValveButton() {
  if (manualValveButtonFlag) {
    manualValveButtonFlag = false;

    unsigned long now = millis();

    if (now - lastManualPress > debounceDelay) {
      lastManualPress = now;

      if (valveOpen) {
        closeValve();
      } else {
        drawManualValveScreen();
        openValve();
      }
    }
  }
}

// Valve Actions ----------------------------------------
void openValve() {
  boosterOn();
  delay(1000);
  boosterOff();
  delay(2000);

  Serial.println(F("Opening valve"));

  digitalWrite(setPin, HIGH);
  delay(1000);
  digitalWrite(setPin, LOW);

  valveOpen = true;
}

void closeValve() {
  boosterOn();
  delay(1000);
  boosterOff();
  delay(2000);

  Serial.println(F("Closing valve"));

  digitalWrite(resetPin, HIGH);
  delay(1000);
  digitalWrite(resetPin, LOW);
  delay(1000);

  valveOpen = false;
}

// OLED Screens -----------------------------------------
void drawInitializingScreen() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(0, 14, "Initializing");
    u8g2.drawStr(0, 28, "Mister...");
    u8g2.drawFrame(0, 44, 128, 12);
    u8g2.drawBox(2, 46, 70, 8);
  } while (u8g2.nextPage());
}

void drawClosingValveScreen() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(0, 14, "System Startup");
    u8g2.drawLine(0, 18, 127, 18);
    u8g2.setFont(u8g2_font_9x15B_tr);
    u8g2.drawStr(0, 42, "Closing Valve");
  } while (u8g2.nextPage());
}

void drawManualValveScreen() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_6x12_tr);

    u8g2.drawStr(0, 10, "Manual Valve");
    u8g2.drawLine(0, 13, 127, 13);

    u8g2.drawStr(0, 28, "Opening Initialized");
    u8g2.drawStr(0, 44, "Press Green");
    u8g2.drawStr(0, 58, "Button to Close");

  } while (u8g2.nextPage());
}

void drawMainStatusScreen(float tempF, float humidity) {
  char tempBuff[8];
  char humBuff[8];
  char setpointBuff[6];


  dtostrf(tempF, 4, 1, tempBuff);
  dtostrf(humidity, 4, 1, humBuff);
  sprintf(setpointBuff, "%d", setpoint);
  

  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_6x12_tr);

    u8g2.drawStr(0, 10, "Mister Status");
    u8g2.drawLine(0, 13, 127, 13);

    u8g2.drawStr(0, 26, "Temp:");
    u8g2.drawStr(58, 26, tempBuff);
    u8g2.drawStr(96, 26, "F");

    u8g2.drawStr(0, 38, "Humidity:");
    u8g2.drawStr(58, 38, humBuff);
    u8g2.drawStr(96, 38, "%");

    u8g2.drawStr(0, 50, "Setpoint:");
    u8g2.drawStr(58, 50, setpointBuff);

   u8g2.drawStr(0, 62, "Valve Status:");

    if (valveOpen) {
     u8g2.drawStr(86, 62, "OPEN");
    } else {
     u8g2.drawStr(80, 62, "CLOSED");
    }
    }

   while (u8g2.nextPage());
}

void drawMistingScreen(float tempF) {
  char tempBuff[8];
  dtostrf(tempF, 4, 1, tempBuff);

  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_9x15B_tr);
    u8g2.drawStr(0, 14, "MISTING");

    u8g2.drawBox(0, 26, 16, 6);
    u8g2.drawTriangle(16, 23, 16, 35, 26, 29);

    u8g2.drawDisc(36, 24, 1);
    u8g2.drawDisc(48, 28, 1);
    u8g2.drawDisc(60, 24, 1);
    u8g2.drawDisc(72, 30, 1);
    u8g2.drawDisc(84, 25, 1);

    u8g2.drawDisc(40, 38, 1);
    u8g2.drawDisc(55, 42, 1);
    u8g2.drawDisc(70, 39, 1);
    u8g2.drawDisc(88, 43, 1);

    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(0, 62, "Temp:");
    u8g2.drawStr(42, 62, tempBuff);
    u8g2.drawStr(80, 62, "F");

  } while (u8g2.nextPage());
}
