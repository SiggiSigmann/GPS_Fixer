#include <SoftwareSerial.h>
#include <Arduino.h>
#include <TinyGPS++.h>
#include <LiquidCrystal.h>

#define BAUTRATE 9600
#define GPS_Number 2
#define DISPLAY_MODIE 2

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 6, en = 7, d4 = 5, d5 = 4, d6 = 13, d7 = 12;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//rx, tx
SoftwareSerial port1(10, 11);
TinyGPSPlus gps1;
SoftwareSerial port2(8, 9);
TinyGPSPlus gps2;

TinyGPSPlus gpsConverter;

struct GPSStruct {
    SoftwareSerial *soft;
    TinyGPSPlus gps;
    char name;
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
void displayGPS(TinyGPSPlus, int, char);
void readSoftSerail(GPSStruct);
void display_multi(TinyGPSPlus gps, char name);
void display_oneline(TinyGPSPlus gps, int row, char name);
void modeSelection();
void gpsSeclection();

void setup() {
  gpsstruct[0].soft = &port1;
  gpsstruct[0].gps = gps1;
  gpsstruct[0].name = '1';
  gpsstruct[0].row = 0;

  gpsstruct[1].soft = &port2;
  gpsstruct[1].gps = gps2;
  gpsstruct[1].name = '2';
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
  displayMode = (displayMode+1) % DISPLAY_MODIE;
  switch (displayMode){
    case 1:
      displayGPS(gpsstruct[gpsselected].gps, gpsstruct[gpsselected].row, gpsstruct[gpsselected].name);
      break;
    default:
      for(int i = 0; i < GPS_Number;i++){
        displayGPS(gpsstruct[i].gps, gpsstruct[i].row, gpsstruct[i].name);
      }
      break;
  }
}
void gpsSeclection(){
  gpsselected = (gpsselected+1) % GPS_Number;
  switch (displayMode){
    case 1:
      displayGPS(gpsstruct[gpsselected].gps, gpsstruct[gpsselected].row, gpsstruct[gpsselected].name);
      break;
    default:
      time = millis();
      break;
  }
}

//### Loop
void loop() {
  readSoftSerail(gpsstruct[gpsselected]);

  //change only when displaymode is online mode (mode 0)
  if(oldmode == 0){
    if(millis()-time > 10000){
      time = millis();
      gpsselected = gpsselected+1 % GPS_Number;
      //Serial.println("Change");
    }
  }
}

//### Serial
void readSoftSerail(GPSStruct gps_struct){
  gps_struct.soft->listen();
  if(gps_struct.soft->available() > 0){

    while (gps_struct.soft->available() > 0) {
      char inByte = gps_struct.soft->read();
      gpsConverter.encode(inByte);
      //Serial.print(inByte);
    }

   // Serial.println("Processed: "+gpsConverter.charsProcessed());

    if(gpsConverter.charsProcessed() > 400){
      //Serial.print(gps_struct.name + " :");

      TinyGPSPlus temp = gps_struct.gps;
      gps_struct.gps = gpsConverter;
      gpsConverter = temp;

      displayGPS(gps_struct.gps, gps_struct.row, gps_struct.name);
    }
  }
}

//### lcd
void displayGPS(TinyGPSPlus gps, int row, char name){
  //Serial.print(displayMode);
  if(oldmode != displayMode){
    oldmode = displayMode;
    lcd.clear();
    //Serial.println("clear");
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

void display_multi(TinyGPSPlus gps, char name){
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

void display_oneline(TinyGPSPlus gps, int row, char name){
  GPSSats = gps.satellites.value();
  if(gps.satellites.isValid()){
    lcd.setCursor(0, row);
    lcd.print(name);
    lcd.setCursor(1, row);
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
