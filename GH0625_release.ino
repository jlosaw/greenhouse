
//JLO Notes

// 053114 Changed the threshold temperature to 85 degrees and cleaned up some comments.
// 050519 Updated temp threshold...deleted some commented garbage
// ADA OLED was being annoying so updated to .96" OLED
// 0324 updated OLED component.. changed button to pin 3

//Pins
//2 Sensor1
//3 Button
//4 Sensor 2
//6 serial LCD
//7 settings button
//8 shdown for the voltage booseter
//9 valve on pin
//10 DAQ pin
//11 valve off pin
// SCL and SDA for the OLED

//Libraries---------------------------------------------

//OLED-------

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16
static const unsigned char PROGMEM logo_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

//------------------------------------


//DHT------------
#include "DHT.h"              //dht library
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
//-------------------------


//#include <SoftwareSerial.h>      //for the LCD
//SoftwareSerial mySerial = SoftwareSerial(255, TxPin);
#include <Wire.h>

 
int setPin = 9;                    //valve open
int resetPin = 11;                 //valve close
int shdnPin = 8;                  //booster pin
int triggers = 0;                //count the # of valve openings
int setpoint = 96;              // temp threshold 95 and 96 not bad here
const int buttonPin = 3;     // the number of the pushbutton pin
int buttonState = 0;      // variable for reading the pushbutton status

// subs---------------------------------------
void boosterOff()
{
  digitalWrite(shdnPin, HIGH);
}

void boosterOn()
{
  digitalWrite(shdnPin, LOW);
}

void setup()
{

  Serial.begin(9600);


attachInterrupt(digitalPinToInterrupt(buttonPin),tempcounter,FALLING);

//OLED Code-------------------

// SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

// Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();


//DHT11 initialization-----------------------------------  
   
  dht.begin();

//Pinmodes--------------------------
  

  pinMode(setPin, OUTPUT);    //valve on
  pinMode(resetPin, OUTPUT);  //valve off
  pinMode(shdnPin, OUTPUT);   //booster pin
  pinMode(buttonPin, INPUT);   //button pin


//Cycle Valve to low so the DHT11 is not noisy----------------------  

  display.clearDisplay();
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.println("Initializing Mister");
  display.display();
  
 boosterOn();
  delay (1000);
  boosterOff();
  delay (2000);
  
  display.print("Closing Valve");     //let the user know
  display.display();
  
  digitalWrite(resetPin, HIGH);
  delay(1000);
  digitalWrite(resetPin, LOW);
  delay(1000);
//-----------------------------------------  

}

void loop(){


//Get sensor data--------------------------------------


  
 float temp = dht.readTemperature(true);
 float h = dht.readHumidity();

  Serial.print("Setpoint: ");
  Serial.println(setpoint);
  
  Serial.print("Humidity (%): ");
  Serial.println(h, DEC);

  Serial.print("Temperature °F): ");
  Serial.println(temp, DEC);
  

//Convert data to text that OLED can read--------

    char temp_buff[5]; char hum_buff[5];
    char temp_disp_buff[11] = "Temp:";
    char hum_disp_buff[11] = "Hum:";

    // appending temp/hum to buffers
    dtostrf(temp,2,1,temp_buff);
    strcat(temp_disp_buff,temp_buff);
    dtostrf(h,2,1,hum_buff);
    strcat(hum_disp_buff,hum_buff);
    
 //Write to OLED---------------------------------
 //Temp, humidity, setpoint, and triggers

  display.clearDisplay();
  display.setTextSize(1);     
  display.setTextColor(SSD1306_WHITE); 
  display.setCursor(0, 0);    
  display.print("Temp ");
  display.println(temp_buff);
  display.print("Humidity: ");
  display.println(hum_buff);
  display.print("Setpoint: ");
  display.println(setpoint);
  display.print("Triggers: ");
  display.println(triggers);
  display.println();
  display.print("V:GH0324_release");
  display.display();
  

//Compare temp value and trigger valve------------------------------------------------  

if ((temp)>setpoint){
   
 
  boosterOn();
  delay (1000);
  boosterOff();
  delay (2000);
  
  Serial.println("Cooling");        //Let the user know
  digitalWrite(setPin, HIGH);
  delay(1000);
  digitalWrite(setPin, LOW);

  //Write Misting Screen-------------------
  
  display.clearDisplay();
  display.setTextSize(2);      
  display.setTextColor(SSD1306_WHITE); 
  display.setCursor(0, 0);     
  display.write("Misting @");
  display.setCursor(0, 20);     //Go down a line
  display.write(temp_buff);     //Show the temp that triggered it
  display.display();

  delay(180000);                //water for 3 minutes,180,000 ms
  
  boosterOn();                // Shut-er down
  delay (1000);
  boosterOff();
  delay (2000);
  
  digitalWrite(resetPin, HIGH);
  delay(1000);
  digitalWrite(resetPin, LOW);
  delay(1000);
  
  triggers=triggers+1;        //count number of triggers
}

else {
      //digitalWrite(ledpin,LOW);
    }

if (triggers>100){
  triggers=0;
}
  
  delay(10000);            //time between reads: 10000=10 sec
  
  
}


void tempcounter(){
Serial.print("Button pressed");
 // Set temp with button-----------------
 // buttonState = digitalRead(buttonPin);
  //if (buttonState == HIGH) {
    setpoint=setpoint +1;
    // }
     
 // Look for button to set the temp

  if (setpoint> 100){
    setpoint =92;
  }

  /*
   * 
  //write new setting
  display.clearDisplay();
  display.setTextSize(2);     
  display.setCursor(0, 0);    
  display.write("New Setting");
  display.write(setpoint);
  display.display();
  */
}
