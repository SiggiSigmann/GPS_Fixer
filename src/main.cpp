#include <SoftwareSerial.h>
#include <Arduino.h>
#include <TinyGPS++.h>
#include <LiquidCrystal.h>
#include <Wire.h>

#define BAUTRATE 9600
#define GPS_NUMBER 2
#define WAITFORSIGNALTIME 5000

//Debug level
#define DEBUG 21

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 6, en = 7, d4 = 5, d5 = 4, d6 = 13, d7 = 12;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//rx, tx
SoftwareSerial port1(10, 11);
TinyGPSPlus gps1;

SoftwareSerial port2(8, 9);
TinyGPSPlus gps2;

struct GPSStruct {
    SoftwareSerial *soft;
    TinyGPSPlus encode;
    char name;
    unsigned long lastTimeSinceData;
    bool missing;
    bool lcdOnMissing;
};
GPSStruct gpsstruct[GPS_NUMBER];

// Variables for gpsseclection
int gpsselected = 0;

//method declaration
void modeSelection();
void gpsSeclection();

void readSoftSerail();
void updateDisplay();

void display_oneline(TinyGPSPlus, short, char);

void setup() {
  //lower wire speed to improve lcd accurency
  Wire.setClock(10000);
  
  //GPS1
  gpsstruct[0].soft = &port1;
  gpsstruct[0].encode = gps1;
  gpsstruct[0].name = '1';
  gpsstruct[0].lastTimeSinceData = millis();
  gpsstruct[0].missing = true;
  gpsstruct[0].lcdOnMissing = false;

  //GPS2
  gpsstruct[1].soft = &port2;
  gpsstruct[1].encode = gps2;
  gpsstruct[1].name = '2';
  gpsstruct[1].lastTimeSinceData = millis();
  gpsstruct[1].missing = true;
  gpsstruct[1].lcdOnMissing = false;

  // LCD
  lcd.begin(16, 2);
  lcd.clear();
  lcd.cursor();

  //Serial communication
  #if DEBUG > 0
    Serial.begin(BAUTRATE);
    while (!Serial) {}

    Serial.println("setup");
  #endif

  //software serial
  port1.begin(BAUTRATE);
  port2.begin(BAUTRATE);

  //pins interrupt
  pinMode(2, INPUT);
  attachInterrupt(digitalPinToInterrupt(2), modeSelection, RISING);
  pinMode(3, INPUT);
  attachInterrupt(digitalPinToInterrupt(3), gpsSeclection, RISING);
}

//### Loop #######################################################################
void loop() {
  //loop is splitted into two parts: Read Data and display
  readSoftSerail();
  updateDisplay();
}

/*### interrupt #################################################################
 * For both buttons
*/
void modeSelection(){
  //displayMode = (displayMode+1) % DISPLAY_MODIE;
}

void gpsSeclection(){
  gpsselected = (gpsselected+1) % GPS_NUMBER;
}

/*### Serial read #################################################################
 * encode sinal from modules
*/
void readSoftSerail(){
  gpsstruct[gpsselected].soft->listen();
  if(gpsstruct[gpsselected].soft->available() > 0){
    //update time
    gpsstruct[gpsselected].missing = false;
    gpsstruct[gpsselected].lastTimeSinceData = millis();

    #if DEBUG > 1
      Serial.print(gpsstruct[gpsselected].name);
      Serial.println(":");
    #endif

    while (gpsstruct[gpsselected].soft->available() > 0) {
      char inByte = gpsstruct[gpsselected].soft->read();
      gpsstruct[gpsselected].encode.encode(inByte);
      
      #if DEBUG > 10
        Serial.print(inByte);
      #endif
    }

    #if DEBUG > 3
      Serial.print("  processed Chars:");
      Serial.println(gpsstruct[gpsselected].encode.charsProcessed());
    #endif

    //check if satellites is value
    if(gpsstruct[gpsselected].encode.satellites.isValid()){
      #if DEBUG > 2
        Serial.println("  satellites valid");
      #endif

      //change Module if valid
      gpsSeclection();
    }

    #if DEBUG > 1
      Serial.println();
    #endif
  }else{
    //if in WAITFORSIGNALTIME no signal is detected change satelitte
    if(millis()-gpsstruct[gpsselected].lastTimeSinceData > WAITFORSIGNALTIME){
      gpsstruct[gpsselected].missing = true;
      gpsstruct[gpsselected].lastTimeSinceData = millis();

      gpsstruct[gpsselected].soft->begin(BAUTRATE);


      #if DEBUG > 2
        Serial.print(gpsstruct[gpsselected].name);
        Serial.println(":");
        Serial.println("  missing");
      #endif

      gpsSeclection();
    }
  }
}

/*### display #################################################################
 * update display
*/
bool checkForUpdate(TinyGPSPlus gps){
  return (gps.location.isUpdated() || gps.satellites.isUpdated());
}

void updateDisplay(){
  for (short i = 0; i < GPS_NUMBER; i++){
    //check if update is needed
    if(!gpsstruct[i].missing){
      gpsstruct[i].lcdOnMissing = false;
      if(checkForUpdate(gpsstruct[i].encode)){
        display_oneline(gpsstruct[i].encode, i, gpsstruct[i].name);
      }
    }else{
      if(!gpsstruct[i].lcdOnMissing){
        gpsstruct[i].lcdOnMissing = true;
        Serial.println("update missing");
        lcd.setCursor(0,i);
        lcd.print("Missing         ");
      }
    }
  }
  lcd.setCursor(0,gpsselected);
}

//diesplay gps per line
void display_oneline(TinyGPSPlus gps, short row, char name){
  //name
  lcd.setCursor(0, row);
  lcd.print(name);
  lcd.print(":");

  //satellites
  lcd.print(gps.satellites.value());
  if(gps.satellites.value() < 10){
    lcd.print(" ");
  }
  lcd.print(" ");

  //location
  lcd.setCursor(5, row);
  lcd.print(gps.location.lat());
  lcd.setCursor(10, row);
  lcd.print(" ");
  lcd.print(gps.location.lng());
}