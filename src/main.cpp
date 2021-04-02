#include <SoftwareSerial.h>
#include <Arduino.h>
#include <TinyGPS++.h>
#include <LiquidCrystal.h>

#define BAUTRATE 9600

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 6, en = 7, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
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

//method declaration
void printGPS(TinyGPSPlus);
void displayGPS(TinyGPSPlus, int, String);
void readSoftSerail(GPSStruct);

void setup() {
  gpsstruct[0].soft = &port1;
  gpsstruct[0].gps = gps1;
  gpsstruct[0].name = "GPS1";
  gpsstruct[0].row = 0;

  gpsstruct[1].soft = &port2;
  gpsstruct[1].gps = gps2;
  gpsstruct[1].name = "GPS2";
  gpsstruct[1].row = 1;

  // LCD
  lcd.begin(16, 2);

  //Serial communication
  Serial.begin(BAUTRATE);
  while (!Serial) {}

  //software serial
  port1.begin(BAUTRATE);
  port2.begin(BAUTRATE);
}

long long time = millis();
bool change = false;

void loop() {
  if(change){
    readSoftSerail(gpsstruct[0]);
  }else{
    readSoftSerail(gpsstruct[1]);
  }
  if(millis()-time > 10000){
    time = millis();
    change =! change;
    Serial.println("----------------------------------------------------------------------------------");
  }
}

void readSoftSerail(GPSStruct gps_struct){
  //Serial.print(gps_struct.name + ": ");

  gps_struct.soft->listen();
  if(gps_struct.soft->available() > 0){
    while (gps_struct.soft->available() > 0) {
      char inByte = gps_struct.soft->read();
      gps_struct.gps.encode(inByte);
      Serial.print(inByte);
    }
    Serial.println(gps_struct.gps.charsProcessed());
    if(gps_struct.gps.charsProcessed() > 600){
      displayGPS(gps_struct.gps, gps_struct.row, gps_struct.name);
    }
    printGPS(gps_struct.gps);
  }
}



void displayGPS(TinyGPSPlus gps, int row, String name){
  GPSSats = gps.satellites.value();
    if(gps.satellites.isValid()){

    lcd.setCursor(0, row);
    lcd.print(name);

    lcd.setCursor(5, row);
    lcd.print(GPSSats);

    GPSLat = gps.location.lat();
    GPSLon = gps.location.lng();

    lcd.setCursor(7, row);
    lcd.print(GPSLat);

    lcd.setCursor(11, row);
    lcd.print(" ");
    lcd.print(GPSLon);
  } 
}

void printGPS(TinyGPSPlus gps){
  float tempfloat;

  
  GPSLat = gps.location.lat();
  GPSLon = gps.location.lng();
  GPSAlt = gps.altitude.meters();
  GPSSats = gps.satellites.value();
  GPSHdop = gps.hdop.value();
  tempfloat = ( (float) GPSHdop / 100);

  Serial.print(F("Lat,"));
  Serial.print(GPSLat, 6);
  Serial.print(F(",Lon,"));
  Serial.print(GPSLon, 6);
  Serial.print(F(",Alt,"));
  Serial.print(GPSAlt, 1);
  Serial.print(F("m,Sats,"));
  Serial.print(GPSSats);
  Serial.print(F(",HDOP,"));
  Serial.print(tempfloat, 2);
  Serial.println();

}