#include <SoftwareSerial.h>
#include <Arduino.h>
#include <TinyGPS++.h>
#include <LiquidCrystal.h>
#include <Wire.h>

#define BAUDRATE 9600

//gps
#define GPSNUMBER 2
#define WAITFORSIGNALTIME 5000

//units
// 0 = metric
// 1 = imperial
#define METRICS 0

//display
#define DISPLAYMODE 2
#define MULTIMODETIME 2000
#define MULTIDISPLAYS 4

//defines timeshift from
#define TIMEOFFSET 1
//0 = Wintertime(no offset), 1=Summertime(1 hour ofsset additional)
#define SUMMERTIME 1

//Analog Pin
#define ANALOGPIN A0

//Debug level
#define DEBUG 0

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 6, en = 7, d4 = 5, d5 = 4, d6 = 13, d7 = 12;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//software and gps instances
//rx, tx
SoftwareSerial port0(10, 11);
SoftwareSerial port1(8, 9);

//struct to manage gps module
struct GPSStruct {
    SoftwareSerial *soft;
    TinyGPSPlus encode;
    unsigned long lastTimeSinceData;  //used to check if a gps module is connected
    bool missing;                     //stores  if missing
    bool lcdOnMissing;                //stores if missing is displayed (stor this information to prefent refres scylus of the lcd)
};
GPSStruct gpsstruct[GPSNUMBER];

// Variables for gpsseclection
int gpsselected = 0;
short initGPSIndex = 0;

//display variables
int displayMode = 0;
int multiMode = 0;
unsigned long modeTime;
int smoothAnalog = 1024;

//setup struct
void initGPSStruct(SoftwareSerial*);

//method declaration
void modeSelection();
void gpsSeclection();

//process methods
void readSoftSerail();
void updateDisplay();

//display methods
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
  
  //GPS
  initGPSStruct(&port0);
  initGPSStruct(&port1);

  // LCD
  lcd.begin(16, 2);
  lcd.clear();
  lcd.createChar(0,underlinedOne);
  lcd.createChar(1,underlinedTwo);

  //Serial communication
  #if DEBUG > 0
    Serial.begin(BAUDRATE);
    while (!Serial) {}

    Serial.println("setup");
  #endif

  //pins interrupt
  pinMode(2, INPUT);
  attachInterrupt(digitalPinToInterrupt(2), modeSelection, RISING);
  pinMode(3, INPUT);
  attachInterrupt(digitalPinToInterrupt(3), gpsSeclection, RISING);

  //set mode timer
  modeTime = millis();
}

//init struct
void initGPSStruct(SoftwareSerial* soft){
  //initialice gpsModule
  gpsstruct[initGPSIndex].soft = soft;
  gpsstruct[initGPSIndex].soft->begin(BAUDRATE);
  gpsstruct[initGPSIndex].encode = TinyGPSPlus();
  gpsstruct[initGPSIndex].lastTimeSinceData = millis();
  gpsstruct[initGPSIndex].missing = true;
  gpsstruct[initGPSIndex].lcdOnMissing = false;

  //update index
  initGPSIndex++;
}

//### Loop #######################################################################
void loop() {
  //loop is splitted into two parts: Read Data and update display
  readSoftSerail();
  updateDisplay();
}

/*### interrupt #################################################################
 * For both buttons
*/
void modeSelection(){
  //change display mode
  displayMode = (displayMode+1) % DISPLAYMODE;
  multiMode = 0;

  //remove lcdOnMissing so lcd could be updated
  gpsstruct[0].lcdOnMissing = false;
  gpsstruct[1].lcdOnMissing = false;
}

void gpsSeclection(){
  //change selected gps module
  gpsselected = (gpsselected+1) % GPSNUMBER;
}

/*### Serial read #################################################################
 * encode signal from modules
*/
void readSoftSerail(){
  gpsstruct[gpsselected].soft->listen();

  if(gpsstruct[gpsselected].soft->available() > 0){
    //update last update time
    gpsstruct[gpsselected].missing = false;
    gpsstruct[gpsselected].lastTimeSinceData = millis();

    #if DEBUG > 1
      Serial.print(gpsstruct[gpsselected].name);
      Serial.println(":");
    #endif

    //read data
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

    //check if satellites is valid
    if(gpsstruct[gpsselected].encode.satellites.isValid()){
      #if DEBUG > 2
        Serial.println("  satellites valid");
      #endif

      //change Module if valid in one_line mode
      if(displayMode == 0){
        gpsSeclection();
      }
    }

    #if DEBUG > 1
      Serial.println();
    #endif
  }else{
    //if in WAITFORSIGNALTIME no signal is detected change gps Module
    if(millis()-gpsstruct[gpsselected].lastTimeSinceData > WAITFORSIGNALTIME){
      gpsstruct[gpsselected].missing = true;
      gpsstruct[gpsselected].lastTimeSinceData = millis();

      //restart software serial
      gpsstruct[gpsselected].soft->begin(BAUDRATE);

      #if DEBUG > 2
        Serial.print(gpsstruct[gpsselected].name);
        Serial.println(":");
        Serial.println("  missing");
      #endif

      //change gpsModule in one_line mode
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
      //oneline mode
      for (short i = 0; i < GPSNUMBER; i++){
        //check if update is needed
        if(!gpsstruct[i].missing){
          gpsstruct[i].lcdOnMissing = false;

          //check if update is needed
          if(gpsstruct[i].encode.location.isUpdated() || gpsstruct[i].encode.satellites.isUpdated()){
            display_oneline(gpsstruct[i].encode, i);
          }
        }else{
          //no module is connected
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
      //multiline mode
      if(!gpsstruct[gpsselected].missing){
        display_multi(gpsstruct[gpsselected].encode, gpsselected);
      }else{
        //no module is connected
        lcd.setCursor(0, 0);
        lcd.print(gpsselected+1);
        lcd.print(" missing       ");

        lcd.setCursor(0, 1);
        lcd.print("                ");
      }
      break;
  }
}

//display gps per line
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
  if(gps.satellites.isValid()){
    lcd.print(gps.satellites.value());
    if(gps.satellites.value() < 10){
      lcd.print(" ");
    }
  }else{
    lcd.print("--");
  }
  lcd.print(" ");

  //location
  if(gps.location.isValid()){
    lcd.setCursor(5, row);
    lcd.print(gps.location.lat(),2);
    lcd.print("  ");
    lcd.setCursor(10, row);
    lcd.print(" ");
    lcd.print(gps.location.lng(),2);
  }else{
    lcd.setCursor(5, row);
    lcd.print("--.--");
    lcd.print("  ");
    lcd.setCursor(10, row);
    lcd.print(" ");
    lcd.print("--.--");
  }
  lcd.print(" ");
}

//display multimode with more columns
void display_multi(TinyGPSPlus gps, short row){
  int mode = multiMode;

  //check analog value and dislay corresponding display
  smoothAnalog = (smoothAnalog*0.8)+(analogRead(ANALOGPIN)*0.2);
  if(smoothAnalog>820){
    mode = multiMode;
  }else if(smoothAnalog>615){
    mode = 0;
  }else if(smoothAnalog>412){
    mode = 1;
  }else if(smoothAnalog>205){
    mode = 2;
  }else {
    mode = 3;
  }

  //choosing display
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

  //change display after MULTIMODETIME
  if(millis()-modeTime >MULTIMODETIME){
    modeTime = millis();
    multiMode = (multiMode+1) % MULTIDISPLAYS;
  }
}

//location display
void display_multi_1(TinyGPSPlus gps, short name){
  //first line
  lcd.setCursor(0, 0);
  lcd.print(name);

  if( gps.location.isValid()){
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
  }else{
    lcd.print("La--.----------");
    lcd.setCursor(0, 1);
    lcd.print(" Lo--.----------");
  }
}

//time and date display
void display_multi_2(TinyGPSPlus gps, short name){
  short hour, dayoffset = 0;

  //calc offsets
  if(gps.time.isValid()){
    hour = gps.time.hour() + TIMEOFFSET + SUMMERTIME;
    dayoffset = hour/24;
    if(dayoffset) hour-=24;
  }

  //first line
  lcd.setCursor(0, 0);
  lcd.print(name);

  //date
  lcd.print("Date ");
  if(gps.date.isValid()){
    if(gps.date.day()+dayoffset < 10){
      lcd.print(" ");
    }
    lcd.print(gps.date.day()+dayoffset);
    lcd.print(".");
    if(gps.date.month() < 10){
      lcd.print("0");
    }
    lcd.print(gps.date.month());
    lcd.print(".");
    lcd.print(gps.date.year());
    lcd.print("        ");
  }else{
    lcd.print("--.--.----");
  }

  //time
  lcd.setCursor(0, 1);
  lcd.print(" Time ");
  if(gps.time.isValid()){
    if(hour < 10){
      lcd.print(" ");
    }
    lcd.print(hour);
    lcd.print(":");
    if(gps.time.minute() < 10){
      lcd.print("0");
    }
    lcd.print(gps.time.minute());
    lcd.print(":");
    if(gps.time.second() < 10){
      lcd.print("0");
    }
    lcd.print(gps.time.second());
    lcd.print("  ");
  }else{
    lcd.print("--:--:--  ");
  }
}

//satellites, cor, speed
void display_multi_3(TinyGPSPlus gps, short name){
  //first line
  lcd.setCursor(0, 0);
  lcd.print(name);

  //satellites
  lcd.print("SA ");
  if(gps.satellites.isValid()){
    lcd.print(gps.satellites.value());
    if(gps.satellites.value() < 10){
      lcd.print(" ");
    }
  }else{
    lcd.print("--");
  }

  //course degree
  lcd.print(" COR ");
    if(gps.course.isValid()){
    if(gps.course.deg() < 10){
      lcd.print(" ");
    }
    lcd.print(gps.course.deg(), 2);
    lcd.print("   ");
  }else{
    lcd.print("------");
  }

  //speed
  lcd.setCursor(0, 1);
  if(gps.speed.isValid()){
    #if METRICS == 0
      lcd.print(" kmh ");
      if(gps.speed.kmph() < 100){
        lcd.print(" ");
      }
      if(gps.speed.kmph() < 10){
        lcd.print(" ");
      }
      lcd.print(gps.speed.kmph(),2);
    #elif METRICS ==1
      lcd.print(" mph ");
      if(gps.speed.mph() < 100){
        lcd.print(" ");
      }
      if(gps.speed.mph() < 10){
        lcd.print(" ");
      }
      lcd.print(gps.speed.mph(),2);
    #endif
  }else{
    lcd.print("---.--");
  }
  lcd.print("     ");
}

void display_multi_4(TinyGPSPlus gps, short name){
  //first line
  lcd.setCursor(0, 0);
  lcd.print(name);

  //altitude
  lcd.print("Alt ");
  if(gps.altitude.isValid()){
    #if METRICS == 0
      if(gps.altitude.meters() < 1000){
        lcd.print(" ");
      }
      if(gps.altitude.meters() < 100){
        lcd.print(" ");
      }
      if(gps.altitude.meters() < 10){
        lcd.print(" ");
      }
      lcd.print(gps.altitude.meters(), 1);
      lcd.print("m    ");
    #elif METRICS == 1
      if(gps.altitude.feet() < 1000){
        lcd.print(" ");
      }
      if(gps.altitude.feet() < 100){
        lcd.print(" ");
      }
      if(gps.altitude.feet() < 10){
        lcd.print(" ");
      }
      lcd.print(gps.altitude.feet(), 1);
      lcd.print("feet ");
    #endif
  }else{
    lcd.print("----.-");
    #if METRICS == 0
      lcd.print("m    ");
    #elif METRICS == 1
      lcd.print("feet ");
    #endif
  }

  //hdop
  lcd.setCursor(0,1);
  lcd.print(" Hdop ");
  if(gps.hdop.isValid()){
    if(gps.hdop.value() < 100){
      lcd.print(" ");
    }
    if(gps.hdop.value() < 10){
      lcd.print(" ");
    }
    lcd.print(gps.hdop.value());
  }else{
    lcd.print("----");
  }
  lcd.print("       ");
}
