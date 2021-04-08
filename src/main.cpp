#include <SoftwareSerial.h>
#include <Arduino.h>
#include <TinyGPS++.h>
#include <LiquidCrystal.h>
#include <Wire.h>

#define BAUTRATE 9600
#define GPS_NUMBER 2
#define WAITFORSIGNALTIME 5000
#define DISPLAY_MODIE 2
#define MULTIMODETIME 2000
#define MULTIDISPLAYS 4

#define ANALOGPIN A0

//Debug level
#define DEBUG 0

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
    unsigned long lastTimeSinceData;
    bool missing;
    bool lcdOnMissing;
};
GPSStruct gpsstruct[GPS_NUMBER];

// Variables for gpsseclection
int gpsselected = 0;
int displayMode = 0;
int multiMode = 0;
unsigned long modeTime;

int smothAnalog = 1024;

//method declaration
void modeSelection();
void gpsSeclection();

void readSoftSerail();
void updateDisplay();

void display_oneline(TinyGPSPlus gps, short row);
void display_multi(TinyGPSPlus gps, short row);
void display_multi_1(TinyGPSPlus gps, short row);
void display_multi_2(TinyGPSPlus gps, short row);
void display_multi_3(TinyGPSPlus gps, short row);
void display_multi_4(TinyGPSPlus gps, short row);

//underlined caracters
byte underlinedTwo[] = {
  0x0E,
  0x11,
  0x01,
  0x02,
  0x04,
  0x08,
  0x1F,
  0x1F
};

byte underlinedOne[] = {
  0x04,
  0x0C,
  0x04,
  0x04,
  0x04,
  0x04,
  0x0E,
  0x1F
};

void setup() {
  //lower wire speed to improve lcd accurency
  Wire.setClock(10000);
  
  //GPS1
  gpsstruct[0].soft = &port1;
  gpsstruct[0].encode = gps1;
  gpsstruct[0].lastTimeSinceData = millis();
  gpsstruct[0].missing = true;
  gpsstruct[0].lcdOnMissing = false;

  //GPS2
  gpsstruct[1].soft = &port2;
  gpsstruct[1].encode = gps2;
  gpsstruct[1].lastTimeSinceData = millis();
  gpsstruct[1].missing = true;
  gpsstruct[1].lcdOnMissing = false;

  // LCD
  lcd.begin(16, 2);
  lcd.clear();

  lcd.createChar(0,underlinedOne);
  lcd.createChar(1,underlinedTwo);

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

  modeTime = millis();
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
  displayMode = (displayMode+1) % DISPLAY_MODIE;
  multiMode = 0;

  gpsstruct[0].lcdOnMissing = false;
  gpsstruct[1].lcdOnMissing = false;
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
      if(displayMode == 0){
        gpsSeclection();
      }
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

      if(displayMode == 0){
        gpsSeclection();
      }
    }
  }
}

/*### display #################################################################
 * update display
*/
void updateDisplay(){
  switch(displayMode){
    case 0:
      for (short i = 0; i < GPS_NUMBER; i++){
        //check if update is needed
        if(!gpsstruct[i].missing){
          gpsstruct[i].lcdOnMissing = false;
          if(gpsstruct[i].encode.location.isUpdated() || gpsstruct[i].encode.satellites.isUpdated()){
            display_oneline(gpsstruct[i].encode, i);
          }
        }else{
          if(!gpsstruct[i].lcdOnMissing){
            gpsstruct[i].lcdOnMissing = true;
            lcd.setCursor(0,i);
            lcd.print("Missing         ");
          }
        }
      }
      lcd.setCursor(0,gpsselected);
      break;

    case 1:
      if(!gpsstruct[gpsselected].missing){
        display_multi(gpsstruct[gpsselected].encode, gpsselected);
      }else{
        //first line
        lcd.setCursor(0, 0);
        lcd.print(gpsselected+1);
        lcd.print(" missing       ");

        lcd.setCursor(0, 1);
        lcd.print("                ");
      }
      break;
  }
}

//diesplay gps per line
void display_oneline(TinyGPSPlus gps, short row){
  //name
  lcd.setCursor(0, row);
  if(row == gpsselected){
    lcd.write(row);
  }else{
    lcd.print(row+1);
  }
  lcd.print(":");

  //satellites
  lcd.print(gps.satellites.value());
  if(gps.satellites.value() < 10){
    lcd.print(" ");
  }
  lcd.print(" ");

  //location
  lcd.setCursor(5, row);
  lcd.print(gps.location.lat(),2);
  lcd.print("  ");
  lcd.setCursor(10, row);
  lcd.print(" ");
  lcd.print(gps.location.lng(),2);
  lcd.print(" ");
}

//display multimode with more collums
void display_multi(TinyGPSPlus gps, short row){
  int mode = multiMode;

  smothAnalog = (smothAnalog*0.8)+(analogRead(ANALOGPIN)*0.2);

  if(smothAnalog>900){
    mode = multiMode;
  }else if(smothAnalog>830){
    mode = 0;
  }else if(smothAnalog>680){
    mode = 0;
  }else if(smothAnalog>510){
    mode = 1;
  }else if(smothAnalog>340){
    mode = 2;
  }else if(smothAnalog>170){
    mode = 3;
  }

  switch(mode){
    case 0:
      display_multi_1(gps, row+1);
      break;
    case 1:
      display_multi_2(gps, row+1);
      break;
    case 2:
      display_multi_3(gps, row+1);
      break;
    case 3:
      display_multi_4(gps, row+1);
      break;
  }

  if(millis()-modeTime >MULTIMODETIME){
    modeTime = millis();
    multiMode = (multiMode+1) % MULTIDISPLAYS;
  }
}

void display_multi_1(TinyGPSPlus gps, short name){
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

void display_multi_2(TinyGPSPlus gps, short name){
  //first line
  lcd.setCursor(0, 0);
  lcd.print(name);

  lcd.print("Date ");
  if(gps.date.day() < 10){
    lcd.print(" ");
  }
  lcd.print(gps.date.day());
  lcd.print(":");
  if(gps.date.month() < 10){
    lcd.print(" ");
  }
  lcd.print(gps.date.month());
  lcd.print(":");
  lcd.print(gps.date.year());
  lcd.print("        ");

  lcd.setCursor(0, 1);
  lcd.print(" Time ");
  lcd.print(gps.time.hour());
  lcd.print(":");
  lcd.print(gps.time.minute());
  lcd.print(":");
  lcd.print(gps.time.second());
  lcd.print("       ");

}

void display_multi_3(TinyGPSPlus gps, short name){
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
  lcd.print(gps.course.deg(), 3);

  lcd.setCursor(0, 1);
  lcd.print(" kmh ");
  lcd.print(gps.speed.kmph(),2);
  lcd.print("       ");
}

void display_multi_4(TinyGPSPlus gps, short name){
  //first line
  lcd.setCursor(0, 0);
  lcd.print(name);

  lcd.print("Alt  ");
  lcd.print(gps.altitude.meters(), 4);
  lcd.print("          ");

  lcd.setCursor(0,1);
  lcd.print(" Hdop ");
  lcd.print(gps.hdop.value());
  lcd.print("          ");
}