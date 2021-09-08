#include <SoftwareSerial.h>
#include <Arduino.h>
#include <TinyGPS++.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <EEPROM.h>

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
#define MULTIDISPLAYS 4

//Analog Pin
#define ANALOGPIN A0

//Debug level
#define DEBUG 0

//satelites timer definition
#define SATNUMBER1 5
#define SATNUMBER2 7
#define SATNUMBER3 10
#define SATNUMBER4 12

//settings
#define SETTINGSDISPLAY 2

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 6, en = 7, d4 = 5, d5 = 4, d6 = 13, d7 = 12;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//software and gps instances
//rx, tx
SoftwareSerial port0(10, 11);
SoftwareSerial port1(8, 9);

//struct to manage gps module
struct GPSStruct
{
  SoftwareSerial *soft;
  TinyGPSPlus encode;
  unsigned long lastTimeSinceData; //used to check if a gps module is connected
  bool missing;                    //stores  if missing
  bool lcdOnMissing;               //stores if missing is displayed (stor this information to prefent refres scylus of the lcd)
};
GPSStruct gpsstruct[GPSNUMBER];

//store time takes to discovers x satelites
struct SatTimer{
  unsigned long start = 0;
  unsigned long time1 = 0;
  unsigned long time2 = 0;
  unsigned long time3 = 0;
  unsigned long time4 = 0;
};
SatTimer gpsTime[2];

//autochange
int countValidpackages = 0;

// Variables for gpsseclection
int gpsselected = 0;
short initGPSIndex = 0;
int maxNumberOfPackages = 10;

//display variables
int displayMode = 0;
int multiMode = 0;
unsigned long modeTime;
int smoothAnalog = 1024;
unsigned long displayduration = 2000;

//settingstimer
unsigned long pressedtimer = 0;
bool showSettings = false;
bool changedSettings = false;
// timeshift from Timeszone
short timeoffset = 1;
//0 = Wintertime(no offset), 1=Summertime(1 hour ofsset additional)
short summertime = 1;
bool changeSettings = false;

//adresses
short settingsdisplay = 0;
int addressTimeOffset = 0;
int addressSummerTime = addressTimeOffset + sizeof(timeoffset);
int addressmaxNumberOfPackages = addressSummerTime + sizeof(summertime);
int addressdisplayduration = addressSummerTime + sizeof(maxNumberOfPackages);

//setup struct
void initGPSStruct(SoftwareSerial *);

//method declaration
void modeSelection();
void gpsSeclection();

//process methods
void readSoftSerail();

//display methods
void updateDisplay();

//display oneline
short oldOneLineMode = 0;
void display_oneline_handl();
void display_oneline(TinyGPSPlus gps, short row);
void display_time(TinyGPSPlus gps, short row, short time);
void dispaly_oneline_select(short i);
String formattetTime(unsigned long seconds);

//multimode displays
void display_multi(TinyGPSPlus gps, short row);
void display_multi_1(TinyGPSPlus gps, short row);
void display_multi_2(TinyGPSPlus gps, short row);
void display_multi_3(TinyGPSPlus gps, short row);
void display_multi_4(TinyGPSPlus gps, short row);

//settingsdisplay
void display_settings_init();
void display_settings();
void handelPoti();

//time calc
void calcTime();

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

void setup(){
  //eeprom
  EEPROM.get(addressTimeOffset, timeoffset);
  EEPROM.get(addressSummerTime, summertime);
  EEPROM.get(addressmaxNumberOfPackages, maxNumberOfPackages);
  EEPROM.get(addressdisplayduration, displayduration);

  //lower wire speed to improve lcd accurency
  Wire.setClock(10000);

  //GPS
  initGPSStruct(&port0);
  initGPSStruct(&port1);

  // LCD
  lcd.begin(16, 2);
  lcd.clear();
  lcd.createChar(0, underlinedOne);
  lcd.createChar(1, underlinedTwo);

  //Serial communication
  Serial.begin(BAUDRATE);
  Serial.println("setup");
  #if DEBUG > 0
    
    while (!Serial){}

    Serial.println("setup");
  #endif

  //pins interrupt
  pinMode(2, INPUT);
  attachInterrupt(digitalPinToInterrupt(2), modeSelection, RISING);
  pinMode(3, INPUT);
  attachInterrupt(digitalPinToInterrupt(3), gpsSeclection, RISING);

  //set mode timer
  modeTime = millis();
  Serial.println("done");
}

//init struct
void initGPSStruct(SoftwareSerial *soft){
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
void loop(){
  if (!digitalRead(2)){
    pressedtimer = 0;
    //Serial.println("löschen");
    changedSettings = false;
  }else{
    if ((!changedSettings) && (pressedtimer != 0) && ((millis() - pressedtimer) > 2000)){
      showSettings = !showSettings;
      changedSettings = true;
      if (showSettings){
        display_settings_init();
      }else{
        EEPROM.put(addressTimeOffset, timeoffset);
        EEPROM.put(addressSummerTime, summertime);
        EEPROM.put(addressmaxNumberOfPackages, maxNumberOfPackages);
        EEPROM.put(addressdisplayduration, displayduration);
      }
    }
      
    Serial.println("setup");

  }

  if (showSettings){
    //display and change settings
    display_settings();
    handelPoti();
    lcd.display();
  }else{
    //loop is splitted into three parts: Read Data, update times and update display
    readSoftSerail();
    calcTime();
    updateDisplay();
  }
}

/*### interrupt #################################################################
 * For both buttons
*/
void modeSelection(){
  //change display mode
  displayMode = (displayMode + 1) % DISPLAYMODE;
  multiMode = 0;
  modeTime = millis();

  //remove lcdOnMissing so lcd could be updated
  gpsstruct[0].lcdOnMissing = false;
  gpsstruct[1].lcdOnMissing = false;
  pressedtimer = millis();

  //settings
  settingsdisplay = (settingsdisplay + 1) % SETTINGSDISPLAY;
}

void gpsSeclection(){
  //change selected gps module
  gpsselected = (gpsselected + 1) % GPSNUMBER;
  multiMode = 0;
  modeTime = millis();
  changeSettings = false;
  countValidpackages = 0;
}

/*### Serial read #################################################################
 * encode signal from modules
*/
void readSoftSerail(){
  gpsstruct[gpsselected].soft->listen();

  if (gpsstruct[gpsselected].soft->available() > 0){
    //update last update time
    if (gpsstruct[gpsselected].missing){
      gpsTime[gpsselected].start = millis();
    }
    gpsstruct[gpsselected].missing = false;
    gpsstruct[gpsselected].lastTimeSinceData = millis();

    #if DEBUG > 1
        Serial.print(gpsselected);
        Serial.println(":");
    #endif

    //read data
    while (gpsstruct[gpsselected].soft->available() > 0){
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
    if (gpsstruct[gpsselected].encode.satellites.isValid()){
      #if DEBUG > 2
            Serial.println("  satellites valid");
      #endif

      //change Module if valid in one_line mode
      if (displayMode == 0){
        if (countValidpackages++ == maxNumberOfPackages){
          gpsSeclection();
          countValidpackages = 0;
        }
      }
    }

    #if DEBUG > 1
        Serial.println();
    #endif
  }else{
    //if in WAITFORSIGNALTIME no signal is detected change gps Module
    if (millis() - gpsstruct[gpsselected].lastTimeSinceData > WAITFORSIGNALTIME){
      gpsstruct[gpsselected].missing = true;
      gpsstruct[gpsselected].lastTimeSinceData = millis();

      //restart software serial
      gpsstruct[gpsselected].soft->begin(BAUDRATE);

      #if DEBUG > 2
            Serial.print(gpsselected);
            Serial.println(":");
            Serial.println("  missing");
      #endif

      //change gpsModule in one_line mode
      if (displayMode == 0){
        gpsSeclection();
      }
    }
  }
}

/* ### timeing #######################################################################
 * calc times for sateiltes
*/
void calcTime(){
  if (gpsstruct[gpsselected].missing){
    gpsTime[gpsselected].start = 0;
    gpsTime[gpsselected].time1 = 0;
    gpsTime[gpsselected].time2 = 0;
    gpsTime[gpsselected].time3 = 0;
    gpsTime[gpsselected].time4 = 0;
  }else{
    int sats = gpsstruct[gpsselected].encode.satellites.value();
    if (sats >= SATNUMBER1 && gpsTime[gpsselected].time1 == 0){
      gpsTime[gpsselected].time1 = millis() - gpsTime[gpsselected].start;
    }else if (sats >= SATNUMBER2 && gpsTime[gpsselected].time2 == 0){
      gpsTime[gpsselected].time2 = millis() - gpsTime[gpsselected].start;
    }else if (sats >= SATNUMBER3 && gpsTime[gpsselected].time3 == 0){
      gpsTime[gpsselected].time3 = millis() - gpsTime[gpsselected].start;
    }else if (sats >= SATNUMBER4 && gpsTime[gpsselected].time4 == 0){
      gpsTime[gpsselected].time4 = millis() - gpsTime[gpsselected].start;
    }
  }
}

/*### display #################################################################
 * update display
*/
void updateDisplay(){
  switch (displayMode){
    case 0:
      display_oneline_handl();
      break;

    case 1:
      //multiline mode
      if (!gpsstruct[gpsselected].missing){
        display_multi(gpsstruct[gpsselected].encode, gpsselected);
      }else{
        //no module is connected
        lcd.setCursor(0, 0);
        lcd.print(gpsselected + 1);
        lcd.print(" missing       ");

        lcd.setCursor(0, 1);
        lcd.print("                ");
      }
      break;
  }
}

/*### display one line ########################################################
 * update display
*/
//handel one line mode display stuff
void display_oneline_handl(){
  //oneline mode
  for (short i = 0; i < GPSNUMBER; i++){
    //check if update is needed
    if (!gpsstruct[i].missing){
      gpsstruct[i].lcdOnMissing = false;

      //check if update is needed
      dispaly_oneline_select(i);
    }else{
      //no module is connected
      if (!gpsstruct[i].lcdOnMissing){
        gpsstruct[i].lcdOnMissing = true;
        lcd.setCursor(0, i);
        lcd.print("Missing         ");
      }
    }
  }
  lcd.setCursor(0, gpsselected);
}

//select mothod to display from poti
void dispaly_oneline_select(short i){
  //check analog value and dislay corresponding display
  smoothAnalog = (smoothAnalog * 0.8) + (analogRead(ANALOGPIN) * 0.2);
  if (smoothAnalog > 820){
    oldOneLineMode = 0;
    display_time(gpsstruct[i].encode, i, 3);
  }else if (smoothAnalog > 615){
    oldOneLineMode = 1;
    display_time(gpsstruct[i].encode, i, 2);
  }else if (smoothAnalog > 412){
    oldOneLineMode = 2;
    display_time(gpsstruct[i].encode, i, 1);
  }else if (smoothAnalog > 205){
    oldOneLineMode = 3;
    display_time(gpsstruct[i].encode, i, 0);
  }else{
    if (oldOneLineMode != 4){
      display_oneline(gpsstruct[0].encode, 0);
      display_oneline(gpsstruct[1].encode, 1);
    }else{
      display_oneline(gpsstruct[i].encode, i);
    }
    oldOneLineMode = 4;
  }
}

//display gps per line
void display_oneline(TinyGPSPlus gps, short row){
  //name
  lcd.setCursor(0, row);
  if (row == gpsselected){
    lcd.write(row);
  }else{
    lcd.print(row + 1);
  }
  lcd.print(":");

  //satellites
  if (gps.satellites.isValid()){
    lcd.print(gps.satellites.value());
    if (gps.satellites.value() < 10){
      lcd.print(" ");
    }
  }else{
    lcd.print("--");
  }
  lcd.print(" ");

  //location
  if (gps.location.isValid()){
    lcd.setCursor(5, row);
    lcd.print(gps.location.lat(), 2);
    lcd.print("  ");
    lcd.setCursor(10, row);
    lcd.print(" ");
    lcd.print(gps.location.lng(), 2);
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

//display gps per line
void display_time(TinyGPSPlus gps, short row, short time){
  //name
  lcd.setCursor(0, row);
  if (row == gpsselected){
    lcd.write(row);
  }else{
    lcd.print(row + 1);
  }
  lcd.print(":");

  //print time it took to find x satelites)
  switch (time){
    case 0:
      lcd.print(SATNUMBER1);
      if (SATNUMBER1 < 10){
        lcd.print(" ");
      }
      lcd.print(" ");
      lcd.print(formattetTime(gpsTime[row].time1));
      break;

    case 1:
      lcd.print(SATNUMBER2);
      if (SATNUMBER2 < 10){
        lcd.print(" ");
      }
      lcd.print(" ");
      lcd.print(formattetTime(gpsTime[row].time2));
      break;

    case 2:
      lcd.print(SATNUMBER3);
      if (SATNUMBER3 < 10){
        lcd.print(" ");
      }
      lcd.print(" ");
      lcd.print(formattetTime(gpsTime[row].time3));
      break;

    case 3:
      lcd.print(SATNUMBER4);
      if (SATNUMBER4 < 10){
        lcd.print(" ");
      }
      lcd.print(" ");
      lcd.print(formattetTime(gpsTime[row].time4));
      break;
  }
}

//format time
String formattetTime(unsigned long seconds){
  String formattet;
  unsigned long secondstemp = seconds;

  short hours = secondstemp / 3600000;
  secondstemp = secondstemp % 3600000;
  if (hours > 99){
    formattet = "+9:";
  }else{
    formattet = String(hours) + ":";
  }

  short minutes = secondstemp / 60000;
  secondstemp = secondstemp % 60000;
  if (minutes < 10){
    formattet += "0" + String(minutes) + ":";
  }else{
    formattet += String(minutes) + ":";
  }

  short sec = secondstemp / 1000;
  secondstemp = secondstemp % 1000;
  if (sec < 10){
    formattet += "0" + String(sec) + ".";
  }else{
    formattet += String(sec) + ".";
  }

  if (secondstemp < 10){
    formattet += "00" + String(secondstemp);
  }else if (secondstemp < 100){
    formattet += "0" + String(secondstemp);
  }else{
    formattet += String(secondstemp);
  }

  short padding = 11 - formattet.length();
  while (padding-- != 0){
    formattet = " " + formattet;
  }

  return formattet;
}

/*### display multi mode ########################################################
 * update display
*/
//display multimode with more columns
void display_multi(TinyGPSPlus gps, short row){
  int mode = multiMode;

  //check analog value and dislay corresponding display
  smoothAnalog = (smoothAnalog * 0.8) + (analogRead(ANALOGPIN) * 0.2);
  if (smoothAnalog > 820){
    mode = multiMode;

    //change display after displayduration
    if (millis() - modeTime > displayduration){
      modeTime = millis();
      multiMode = (multiMode + 1) % MULTIDISPLAYS;
    }
  }else if (smoothAnalog > 615){
    mode = 0;
    multiMode = 0;
    modeTime = millis();
  }else if (smoothAnalog > 412){
    mode = 1;
  }else if (smoothAnalog > 205){
    mode = 2;
  }else{
    mode = 3;
  }

  //choosing display
  switch (mode){
  case 0:
    display_multi_1(gps, row + 1);
    break;
  case 1:
    display_multi_2(gps, row + 1);
    break;
  case 2:
    display_multi_3(gps, row + 1);
    break;
  case 3:
    display_multi_4(gps, row + 1);
    break;
  }
}

//location display
void display_multi_1(TinyGPSPlus gps, short name){
  //first line
  lcd.setCursor(0, 0);
  lcd.print(name);

  if (gps.location.isValid()){
    //location
    lcd.print("La");
    float lat = gps.location.lat();
    if (lat < 10.0){
      lcd.print(" ");
    }
    lcd.print(gps.location.lat(), 10);

    lcd.setCursor(0, 1);
    lcd.print(" Lo");
    float lng = gps.location.lng();
    if (lng < 10.0){
      lcd.print(" ");
    }
    lcd.print(lng, 10);
  }else{
    lcd.print("La--.----------");
    lcd.setCursor(0, 1);
    lcd.print(" Lo--.----------");
  }
}

//calculate day per month
int getNumberOfDays(int month, int year)
{
  //leap year condition, if month is 2
  if (month == 2){
    if ((year % 400 == 0) || (year % 4 == 0 && year % 100 != 0)){
      return 29;
    }else{
      return 28;
    }
  }
  //months which has 31 days
  else if (month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12){
    return 31;
  }else{
    return 30;
  }
}

//time and date display
void display_multi_2(TinyGPSPlus gps, short name){
  short hour, dayoffset = 0;

  //calc offsets
  if (gps.time.isValid()){
    hour = gps.time.hour() + timeoffset + summertime;
    dayoffset = hour / 24;
    if (dayoffset){
      hour -= 24;
    }
  }

  short month = gps.date.month();
  short year = gps.date.year();
  short days = gps.date.day() + dayoffset;
  short dayPerMonth = getNumberOfDays(month, year);

  //check if day fits to month and offset month
  if (dayPerMonth < days){
    month++;
    days = days - dayPerMonth;
  }

  //offset year
  if (month > 12){
    month = 1;
    year++;
  }

  //first line
  lcd.setCursor(0, 0);
  lcd.print(name);

  //date
  lcd.print("Date ");
  if (gps.date.isValid()){
    if (days < 10){
      lcd.print(" ");
    }
    lcd.print(days);
    lcd.print(".");
    if (month < 10){
      lcd.print("0");
    }
    lcd.print(month);
    lcd.print(".");
    lcd.print(year);
    lcd.print("        ");
  }else{
    lcd.print("--.--.----");
  }

  //time
  lcd.setCursor(0, 1);
  lcd.print(" Time ");
  if (gps.time.isValid()){
    if (hour < 10){
      lcd.print(" ");
    }
    lcd.print(hour);
    lcd.print(":");
    if (gps.time.minute() < 10){
      lcd.print("0");
    }
    lcd.print(gps.time.minute());
    lcd.print(":");
    if (gps.time.second() < 10){
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
  if (gps.satellites.isValid()){
    lcd.print(gps.satellites.value());
    if (gps.satellites.value() < 10){
      lcd.print(" ");
    }
  }else{
    lcd.print("--");
  }

  //course degree
  lcd.print(" COR ");
  if (gps.course.isValid()){
    if (gps.course.deg() < 10){
      lcd.print(" ");
    }
    lcd.print(gps.course.deg(), 2);
    lcd.print("   ");
  }else{
    lcd.print("------");
  }

  //speed
  lcd.setCursor(0, 1);
  if (gps.speed.isValid()){
    #if METRICS == 0
        lcd.print(" kmh ");
        if (gps.speed.kmph() < 100){
          lcd.print(" ");
        }
        if (gps.speed.kmph() < 10){
          lcd.print(" ");
        }
        lcd.print(gps.speed.kmph(), 2);
    #elif METRICS == 1
        lcd.print(" mph ");
        if (gps.speed.mph() < 100){
          lcd.print(" ");
        }
        if (gps.speed.mph() < 10){
          lcd.print(" ");
        }
        lcd.print(gps.speed.mph(), 2);
    #endif
  }else{
    lcd.print("---.--");
  }
  lcd.print("     ");
}

//hight, hdop
void display_multi_4(TinyGPSPlus gps, short name){
  //first line
  lcd.setCursor(0, 0);
  lcd.print(name);

  //altitude
  lcd.print("Alt ");
  if (gps.altitude.isValid()){
    #if METRICS == 0
        if (gps.altitude.meters() < 1000){
          lcd.print(" ");
        }
        if (gps.altitude.meters() < 100){
          lcd.print(" ");
        }
        if (gps.altitude.meters() < 10){
          lcd.print(" ");
        }
        lcd.print(gps.altitude.meters(), 1);
        lcd.print("m    ");
    #elif METRICS == 1
        if (gps.altitude.feet() < 1000){
          lcd.print(" ");
        }
        if (gps.altitude.feet() < 100){
          lcd.print(" ");
        }
        if (gps.altitude.feet() < 10){
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
  lcd.setCursor(0, 1);
  lcd.print(" Hdop ");
  if (gps.hdop.isValid()){
    if (gps.hdop.value() < 100){
      lcd.print(" ");
    }
    if (gps.hdop.value() < 10){
      lcd.print(" ");
    }
    lcd.print(gps.hdop.value());
  }else{
    lcd.print("----");
  }
  lcd.print("       ");
}

/*### display settings ########################################################
 * update display
*/
void display_settings_init(){
  settingsdisplay = 0;

  for (short row = 0; row < 2; row++){
    //name
    lcd.setCursor(0, row);

    lcd.write(row);

    lcd.print(" ");
    if (row == 0){
      lcd.print("TS: ");
      lcd.print(timeoffset);
      lcd.print(" ");
    }else{
      lcd.print("SW: ");
      lcd.print(summertime);
      lcd.print(" ");
    }
  }
}

void display_settings(){
  if (settingsdisplay == 0){
    for (short row = 0; row < 2; row++){
      //name
      lcd.setCursor(0, row);
      if (row == gpsselected){
        lcd.write(row);
      }else{
        lcd.print(row + 1);
      }
      lcd.print(" ");
      if (row == 0){
        lcd.print("TS: ");
      }else{
        lcd.print("SW: ");
      }
    }
  }else{
    for (short row = 0; row < 2; row++){
      //name
      lcd.setCursor(0, row);
      if (row == gpsselected){
        lcd.write(row);
      }else{
        lcd.print(row + 1);
      }
      lcd.print(" ");
      if (row == 0){
        lcd.print("packets: ");
      }else{
        lcd.print("nexttime: ");
      }
    }
  }
}

void handelPoti(){
  int newsmoothAnalog = (smoothAnalog * 0.8) + (analogRead(ANALOGPIN) * 0.2);
  if (abs(newsmoothAnalog - smoothAnalog) > 10){
    changeSettings = true;
  }
  smoothAnalog = newsmoothAnalog;

  if (settingsdisplay == 0){
    lcd.setCursor(6, gpsselected);
    if (gpsselected == 0){
      //sw time 0 / 1019
      if (changeSettings){
        short parsedAnalog = smoothAnalog / 39;
        parsedAnalog -= 12;
        timeoffset = parsedAnalog;
      }
      lcd.print(timeoffset);
      lcd.print("                 ");
    }else{
      //sw time 0 / 1019
      if (changeSettings){
        short parsedAnalog = smoothAnalog / 512;
        summertime = parsedAnalog;
      }
      lcd.print(summertime);
      lcd.print("                 ");
    }
  }else{
    lcd.setCursor(11, gpsselected);
    if (gpsselected == 0){
      //sw time 0 / 1019
      if (changeSettings){
        short parsedAnalog = smoothAnalog / 10;
        maxNumberOfPackages = parsedAnalog;
      }
      lcd.print(maxNumberOfPackages);
      lcd.print("                 ");
    }else{
      //sw time 0 / 1019
      if (changeSettings){
        short parsedAnalog = smoothAnalog / 10;
        displayduration = parsedAnalog * 40;
      }
      lcd.print(displayduration);
      lcd.print("                 ");
    }
  }
}