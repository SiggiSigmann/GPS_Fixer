#include <SoftwareSerial.h>
#include <Arduino.h>
#include <TinyGPS++.h>
#include <LiquidCrystal.h>
#include <Wire.h>
//#include <MemoryFree.h>

#define BAUTRATE 9600
#define GPS_Number 2
#define DISPLAY_MODIE 2
//How often to change gps module. Shoulb be double the sending duration of the module to make 
//shure all gate could be read. (10000 => 10s)
#define REFRESHRATE 10000
#define DISPLAYRATE 1000

//Debug level
#define DEBUG 2

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
unsigned long time = millis();
bool change = false;
//variabel for display mode
int displayMode = 0;
int oldmode = 0;
// Variables for gpsseclection
int gpsselected = 0;
//Refres display variable
unsigned long displayRefresh = millis();

//method declaration
void printGPS(TinyGPSPlus);
void displayGPS();
void readSoftSerail();
void display_multi(TinyGPSPlus gps, char name);
void display_oneline(TinyGPSPlus gps, int row, char name);
void modeSelection();
void gpsSeclection();

void setup() {
  Wire.setClock(10000);
  //GPS1
  gpsstruct[0].soft = &port1;
  gpsstruct[0].encode = gps1;
  gpsstruct[0].name = '1';
  gpsstruct[0].row = 0;

  //GPS2
  gpsstruct[1].soft = &port2;
  gpsstruct[1].encode = gps2;
  gpsstruct[1].name = '2';
  gpsstruct[1].row = 1;

  // LCD
  lcd.begin(16, 2);
  lcd.cursor();

  //Serial communication
  #if DEBUG > 1
    Serial.begin(BAUTRATE);
    while (!Serial) {}
  #endif

  //software serial
  port1.begin(BAUTRATE);
  port2.begin(BAUTRATE);

  //pins interrupt
  pinMode(2, INPUT);
  attachInterrupt(digitalPinToInterrupt(2), modeSelection, RISING);
  pinMode(3, INPUT);
  attachInterrupt(digitalPinToInterrupt(3), gpsSeclection, RISING);

  displayGPS();
}

//### interrupt
void modeSelection(){
  displayMode = (displayMode+1) % DISPLAY_MODIE;

  displayGPS();
}

void gpsSeclection(){
  gpsselected = (gpsselected+1) % GPS_Number;

  time = millis();
  displayGPS();
}

//### Loop
void loop() {
  //Serial.println(freeMemory());
  //Serial.println(gpsselected);
  //read gps data and update display
  readSoftSerail();

  //change only when displaymode is online mode (mode 0)
  if(displayMode == 0){
    if(millis()-time > REFRESHRATE){
      time = millis();
      gpsselected = gpsselected+1 % GPS_Number;
    }
  }

  if(millis()-displayRefresh > DISPLAYRATE){
    //update screwn
    //Serial.println("display");
    displayGPS();
    displayRefresh = millis();
  }
}

//### Serial
void readSoftSerail(){
  gpsstruct[gpsselected].soft->listen();
  if(gpsstruct[gpsselected].soft->available() > 0){

    while (gpsstruct[gpsselected].soft->available() > 0) {
      char inByte = gpsstruct[gpsselected].soft->read();
      gpsstruct[gpsselected].encode.encode(inByte);
      
      #if DEBUG > 1
        //Serial.print(inByte);
      #endif
    }

    #if DEBUG > 1
      //Serial.print("Processed: ");
      //Serial.println(gpsstruct[gpsselected].encode.charsProcessed());
    #endif

    if(gpsstruct[gpsselected].encode.charsProcessed() > 400){
      #if DEBUG > 1
        //Serial.print(gps_struct.name + " :");
      #endif

      if(gpsstruct[gpsselected].encode.satellites.isValid()){
        gpsselected = (gpsselected+1) % GPS_Number;

        time = millis();
        //Serial.println(gpsstruct[gpsselected].encode.satellites.value());
        //Serial.println("ex");
        //TinyGPSPlus temp = gpsstruct[gpsselected].encode;
        //gpsstruct[gpsselected].toDisplay = gpsstruct[gpsselected].encode;
        //gpsstruct[gpsselected].encode = temp;
        //Serial.println(gpsstruct[gpsselected].toDisplay.satellites.value());
      }
      
    }
  }
}

//### lcd
void displayGPS(){
  //clean if new mode is selected
  if(oldmode != displayMode){
    oldmode = displayMode;
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
  }

  //update Display
  switch (displayMode){
    case 1:
      display_multi(gpsstruct[gpsselected].encode, gpsstruct[gpsselected].name);
      break;
    default:
      for(int i = 0; i < GPS_Number;i++){
        display_oneline(gpsstruct[i].encode, gpsstruct[i].row, gpsstruct[i].name);
      }

      //set curser under gps name
      lcd.setCursor(0, gpsstruct[gpsselected].row);
      break;
  }
}

//display multimode with more collums
void display_multi(TinyGPSPlus gps, char name){
  GPSSats = gps.satellites.value();

  //first line
  lcd.setCursor(0, 0);
  lcd.print(name);
  lcd.print(":");

  //satellites
  lcd.setCursor(2, 0);
  lcd.print(GPSSats);
  if(GPSSats < 10){
    lcd.print(" ");
  }

  //speed
  lcd.setCursor(5,0);
  lcd.print(gps.speed.kmph());

  //Altitute
  lcd.setCursor(10,0);
  lcd.print(gps.altitude.meters());

  //second line
  GPSLat = gps.location.lat();
  GPSLon = gps.location.lng();

  //location
  lcd.setCursor(0, 1);
  lcd.print(GPSLat,6);
  lcd.setCursor(7, 1);
  lcd.print(" ");
  lcd.setCursor(8, 1);
  lcd.print(GPSLon,6);
}

//diesplay gps per line
void display_oneline(TinyGPSPlus gps, int row, char name){
  GPSSats = gps.satellites.value();

  //name
  lcd.setCursor(0, row);
  lcd.print(name);
  lcd.setCursor(1, row);
  lcd.print(":");

  //satellites
  lcd.setCursor(2, row);
  lcd.print(GPSSats);
  if(GPSSats < 10){
    lcd.print(" ");
  }

  GPSLat = gps.location.lat();
  GPSLon = gps.location.lng();

  //location
  lcd.setCursor(5, row);
  lcd.print(GPSLat);
  lcd.setCursor(10, row);
  lcd.print(" ");
  lcd.print(GPSLon);
}
