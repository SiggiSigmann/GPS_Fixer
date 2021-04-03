#include <SoftwareSerial.h>
#include <Arduino.h>
#include <TinyGPS++.h>
#include <LiquidCrystal.h>

#define BAUTRATE 9600
#define SERIAL_PRITN 3

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
    TinyGPSPlus gps;
    String name;
    int row;
};
GPSStruct gpsstruct[2];

//other variables
float GPSLat;
float GPSLon;
float GPSAlt;
uint8_t GPSSats;
uint32_t GPSHdop;
uint32_t GPSstarttimemS;
uint32_t GPSendtimemS;

//variabel for SoftSerial change
long long time = millis();
bool change = false;
//variabel for display mode
int displayMode = 0;
int oldmode = 0;
// Variables for gpsseclection
int gpsselected = 0;

//method declaration
void printGPS(TinyGPSPlus);
void displayGPS(TinyGPSPlus, int, String);
void readSoftSerail(GPSStruct);
void display_multi(TinyGPSPlus gps, String name);
void display_oneline(TinyGPSPlus gps, int row, String name);
void modeSelection();
void gpsSeclection();

void setup() {
  gpsstruct[0].soft = &port1;
  gpsstruct[0].gps = gps1;
  gpsstruct[0].name = "1";
  gpsstruct[0].row = 0;

  gpsstruct[1].soft = &port2;
  gpsstruct[1].gps = gps2;
  gpsstruct[1].name = "2";
  gpsstruct[1].row = 1;

  // LCD
  lcd.begin(16, 2);

  //Serial communication
  Serial.begin(BAUTRATE);
  while (!Serial) {}

  //software serial
  port1.begin(BAUTRATE);
  port2.begin(BAUTRATE);

  //pins interrupt
  pinMode(2, INPUT);
  attachInterrupt(digitalPinToInterrupt(2), modeSelection, RISING);
  pinMode(3, INPUT);
  attachInterrupt(digitalPinToInterrupt(3), gpsSeclection, RISING);
}

//### interrupt
void modeSelection(){
  displayMode++;
  if(displayMode>1) displayMode = 0;
}
void gpsSeclection(){
  change =! change;
}

//### Loop
void loop() {
  if(change){
    readSoftSerail(gpsstruct[0]);
  }else{
    readSoftSerail(gpsstruct[1]);
  }

  if(oldmode == 0){
    if(millis()-time > 10000){
      time = millis();
      change =! change;
      Serial.println("Change");
    }
  }
}

//### Serial
void readSoftSerail(GPSStruct gps_struct){
  gps_struct.soft->listen();
  if(gps_struct.soft->available() > 0){
    Serial.print(gps_struct.name + ":   \n");

    while (gps_struct.soft->available() > 0) {
      char inByte = gps_struct.soft->read();
      gps_struct.gps.encode(inByte);
        Serial.print(inByte);
    }

    Serial.println("Processed: "+gps_struct.gps.charsProcessed());


    if(gps_struct.gps.charsProcessed() > 500){
      displayGPS(gps_struct.gps, gps_struct.row, gps_struct.name);
    }
  }
}

//### lcd
void displayGPS(TinyGPSPlus gps, int row, String name){
  if(oldmode != displayMode){
    oldmode = displayMode;
    lcd.clear();
  }
  switch(displayMode){
    case 0:
      display_oneline(gps, row, name);
      break;
    case 1:
      display_multi(gps, name);
      break;
  }
}

void display_multi(TinyGPSPlus gps, String name){
  GPSSats = gps.satellites.value();
  if(gps.satellites.isValid()){
    lcd.setCursor(0, 0);
    lcd.print(name);
    lcd.print(":");

    lcd.setCursor(2, 0);
    lcd.print(GPSSats);
    if(GPSSats < 10){
      lcd.print(" ");
    }

    lcd.setCursor(5,0);
    lcd.print(gps.speed.mph());

    lcd.setCursor(10,0);
    lcd.print(gps.altitude.meters());

    GPSLat = gps.location.lat();
    GPSLon = gps.location.lng();

    lcd.setCursor(0, 1);
    lcd.print(GPSLat,7);

    lcd.setCursor(8, 1);
    lcd.print(" ");

    lcd.setCursor(9, 1);
    lcd.print(GPSLon,7);
  } 
}

void display_oneline(TinyGPSPlus gps, int row, String name){
  GPSSats = gps.satellites.value();
  if(gps.satellites.isValid()){

    lcd.setCursor(0, row);
    lcd.print(name);
    lcd.print(":");

    lcd.setCursor(2, row);
    lcd.print(GPSSats);
    if(GPSSats < 10){
      lcd.print(" ");
    }

    GPSLat = gps.location.lat();
    GPSLon = gps.location.lng();

    lcd.setCursor(5, row);
    lcd.print(GPSLat);

    lcd.setCursor(10, row);
    lcd.print(" ");
    lcd.print(GPSLon);
  } 
}
