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
#define REFRESHRATE 2000
#define MULTIDISPLAYS 4
#define MULTIDISPLAYSREFRESHRATE 5000

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
    int wasSuccessfull;
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
int multiMode = 0;
unsigned long multichangeInterval = millis();
// Variables for gpsseclection
int gpsselected = 0;

//method declaration
void printGPS(TinyGPSPlus);
void displayGPS();
void readSoftSerail();
void display_multi(TinyGPSPlus gps, int row, char name);
void display_oneline(TinyGPSPlus gps, int row, char name);
void modeSelection();
void gpsSeclection();
void changeGPSModule();
void display_multi_1(TinyGPSPlus gps, char name);
void display_multi_2(TinyGPSPlus gps, char name);
void display_multi_3(TinyGPSPlus gps, char name);
void display_multi_4(TinyGPSPlus gps, char name);

void setup() {
  Wire.setClock(10000);
  //GPS1
  gpsstruct[0].soft = &port1;
  gpsstruct[0].encode = gps1;
  gpsstruct[0].name = '1';
  gpsstruct[0].row = 0;
  gpsstruct[0].wasSuccessfull = 0;

  //GPS2
  gpsstruct[1].soft = &port2;
  gpsstruct[1].encode = gps2;
  gpsstruct[1].name = '2';
  gpsstruct[1].row = 1;
  gpsstruct[1].wasSuccessfull = 0;

  // LCD
  lcd.begin(16, 2);

  //Serial communication
  #if DEBUG > 1
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

  displayGPS();
}

//### Loop #######################################################################
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

  if(millis()-multichangeInterval > MULTIDISPLAYSREFRESHRATE){

    if(displayMode == 1){
      multiMode = (multiMode+1) % MULTIDISPLAYS;
      multichangeInterval = millis();
      displayGPS();
    }

  }
}

//### interrupt #################################################################
void modeSelection(){
  displayMode = (displayMode+1) % DISPLAY_MODIE;

  if(displayMode == 1){
    multiMode = 0;
    multichangeInterval = millis();
  }
  displayGPS();
}

void gpsSeclection(){
  changeGPSModule();
}

//### Serial ###################################################################
void readSoftSerail(){
  gpsstruct[gpsselected].wasSuccessfull = 0;

  gpsstruct[gpsselected].soft->listen();
  if(gpsstruct[gpsselected].soft->available() > 0){
    gpsstruct[gpsselected].wasSuccessfull = 1;

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

      //change gps module if encoding was sucessfull
      if(gpsstruct[gpsselected].encode.satellites.isValid() && displayMode == 0){
        changeGPSModule();
      }
      
    }
  }
}

void changeGPSModule(){
  gpsselected = (gpsselected+1) % GPS_Number;
  time = millis();

  displayGPS();
}

//### lcd ############################################################################
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
      lcd.noCursor();
      display_multi(gpsstruct[gpsselected].encode, gpsstruct[gpsselected].row, gpsstruct[gpsselected].name);
      break;
    default:
      lcd.cursor();
      for(int i = 0; i < GPS_Number;i++){
        display_oneline(gpsstruct[i].encode, gpsstruct[i].row, gpsstruct[i].name);
      }

      //set curser under gps name
      lcd.setCursor(0, gpsstruct[gpsselected].row);
      break;
  }
}

//display multimode with more collums
void display_multi(TinyGPSPlus gps, int row, char name){
  if(gpsstruct[row].wasSuccessfull){
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("                ");

    switch(multiMode){
      case 0:
        display_multi_1(gps, name);
        break;
      case 1:
        display_multi_2(gps, name);
        break;
      case 2:
        display_multi_3(gps, name);
        break;
      case 3:
        display_multi_4(gps, name);
        break;
    }

  }else{
    lcd.setCursor(0, 0);
    lcd.print("Missing         ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
  }
}

void display_multi_1(TinyGPSPlus gps, char name){
  //first line
  lcd.setCursor(0, 0);
  lcd.print(name);

  //location
  lcd.print("La");
  float lat = gps.location.lat();
  if(lat < 10.0){
    lcd.print(" ");
  }
  lcd.print(gps.location.lat(),10);

  lcd.setCursor(0, 1);
  lcd.print(" Lo");
  float lng = gps.location.lng();
  if(lng < 10.0){
    lcd.print(" ");
  }
  lcd.print(lng,10);
}

void display_multi_2(TinyGPSPlus gps, char name){
  //first line
  lcd.setCursor(0, 0);
  lcd.print(name);

  lcd.print("Date ");
  lcd.print(gps.date.day());
  lcd.print(":");
  lcd.print(gps.date.month());
  lcd.print(":");
  lcd.print(gps.date.year());

  lcd.setCursor(0, 1);
  lcd.print(" Time ");
  lcd.print(gps.time.hour());
  lcd.print(":");
  lcd.print(gps.time.minute());
  lcd.print(":");
  lcd.print(gps.time.second());

}

void display_multi_3(TinyGPSPlus gps, char name){
  //first line
  lcd.setCursor(0, 0);
  lcd.print(name);

  lcd.print("SA ");
  int sat = gps.satellites.value();
  lcd.print(sat);
  if(sat < 10){
    lcd.print(" ");
  }
  lcd.print(" COR ");
  lcd.print(gps.course.deg());

  lcd.setCursor(0, 1);
  lcd.print(" kmh ");
  lcd.print(gps.speed.kmph());
}

void display_multi_4(TinyGPSPlus gps, char name){
  //first line
  lcd.setCursor(0, 0);
  lcd.print(name);

  lcd.print("Alt  ");
  lcd.print(gps.altitude.meters());

  lcd.setCursor(0,1);
  lcd.print(" Hdop ");
  lcd.print(gps.hdop.value());
}

//diesplay gps per line
void display_oneline(TinyGPSPlus gps, int row, char name){
  if(gpsstruct[row].wasSuccessfull){
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

    lcd.setCursor(4, row);
    lcd.print(" ");

    GPSLat = gps.location.lat();
    GPSLon = gps.location.lng();

    //location
    lcd.setCursor(5, row);
    lcd.print(GPSLat);
    lcd.setCursor(10, row);
    lcd.print(" ");
    lcd.print(GPSLon);
  }else{
    lcd.setCursor(0, row);
    lcd.print("Missing         ");
  }
}
