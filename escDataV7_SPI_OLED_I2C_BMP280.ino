/*
 * OpenPPG ESC telemetry reader - Written by Braedin Butler
 * Written for use with ADAFRUIT FEATHER M4 and 2.42" I2C OLED
 * and BMP280 altimeter in the I2C configuration.
 * As long as the transmission intervals are no faster than
 * 24ms, the data transfer success rate should be 100%.
 */

#include "BMP280.h"
#include <TimeLib.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH        128       // OLED display width, in pixels
#define SCREEN_HEIGHT       64        // OLED display height, in pixels
#define OLED_CS             10
#define OLED_RESET          12
#define OLED_DC             11
#define ESC_BAUD_RATE       19200
#define ESC_DATA_SIZE       24
#define DISPLAY_INTERVAL    250
#define pressureSeaLevel    1013.25
#define BMP280_OSR          16        // 0, 1, 2, 3, 4, 16
#define SERIAL1_TIMEOUT     1000

unsigned long displayMillis = 0;
unsigned long transmitted = 0;
unsigned long failed = 0;
bool invert = true;
unsigned long preTxMillis = 0;
unsigned long postTxMillis = 0;
bool armed = false;
bool throttledFlag = true;
bool throttled = false;
unsigned long throttledAtMillis = 0;
unsigned int throttleSecs = 0;

//ESC Telemetry:
byte escData[ESC_DATA_SIZE];
byte prevData[ESC_DATA_SIZE];
float inputPWM = 0;
float outputPWM = 0;
float rpm = 0;
float amps = 0;
float volts = 0;
float capTempC = 0;
float fetTempC = 0;

//BMP280 Measurements:
double temperatureC = 0;
double pressureMBar = 0;
double altitudeFt = 0;
float aglFt = 0;
float altiOffset = 0;
bool altiZeroed = false;

BMP280 bmp;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);


void setup() {
  initDisplay(); 
  bmp.begin();
  bmp.setOversampling(BMP280_OSR);
  Serial1.begin(ESC_BAUD_RATE); 
  Serial1.setTimeout(SERIAL1_TIMEOUT);
  delay(1000);
}


void loop() {
  if(millis()-displayMillis >= DISPLAY_INTERVAL){
    displayMillis = millis();
    updateDisplay();
    readBMP280();
    readESC();        
  }
}


void prepareSerialRead(){  
  while(Serial1.available()>0){
    byte t = Serial1.read();
  }
}


void initDisplay() {
  display.begin(SSD1306_SWITCHCAPVCC);
  display.clearDisplay();
  display.setRotation(0);
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(F("OpenPPG"));
  display.display();
  display.clearDisplay();
}


void updateDisplay() {
  if(invert == true){invert = false;}
  else{invert = true;}
  if(volts<42){display.invertDisplay(invert);}
  else{display.invertDisplay(false);}

  handleFlightTime();
  
  display.setCursor(1, 1);
  display.setTextSize(2);  
  if(volts<10) display.print(F(" "));
  display.print(volts,1);
  display.setCursor(50, 8);
  display.setTextSize(1);
  display.print(F("V"));

  display.setCursor(73,1);
  display.setTextSize(2);
  if(armed){
    if(amps<10) display.print(F(" "));
    display.print(amps,1);
    display.setCursor(122, 8);
    display.setTextSize(1);
    display.println(F("A"));
  }
  else{
    display.setTextSize(1);
    display.print(F("DISARMED"));
  }

  display.setCursor(72, 56);
  display.setTextSize (1);
  display.print(F("RPM: "));
  display.print(rpm,0);

  display.setCursor(68, 24);  // (35, 24) for centered
  display.setTextSize (2);
  displayTime(throttleSecs);
  
  display.setCursor(2, 24);
  display.print(aglFt,0);
  
  display.setCursor(1, 40);
  display.setTextSize(1);
  //display.print(transmitted-failed);
  //display.print(F("  "));
  display.print(failed);
  //display.print(F(" "));
  //display.print(freeMemory());

  display.setCursor(1, 48);
  display.print(F("CAP: "));
  display.print(capTempC,0);

  display.setCursor(1, 56);
  display.print(F("FET: "));
  display.println(fetTempC,0);

  display.setCursor(72, 48);
  display.print(F("PWM: "));
  display.print(inputPWM,0);

  display.display();
  display.clearDisplay();
}


void handleFlightTime(){
  if(!armed){
    throttledFlag = true;
    throttled = false;
  }
  if(armed){
    if(inputPWM>510 && throttledFlag){
      throttledAtMillis = millis();
      throttledFlag = false;
      throttled = true;
    }
    if(throttled){
      throttleSecs = (millis()-throttledAtMillis) / 1000.0;
    }
    else{
      throttleSecs = 0;
    }
  }
}


void displayTime(int val) {
  // displays number of minutes and seconds (since armed and throttled)
  int minutes = val / 60;
  int seconds = numberOfSeconds(val);
  if (minutes < 10) {
    display.print("0");
  }
  display.print(minutes);
  display.print(":");
  if (seconds < 10) {
    display.print("0");
  }
  display.print(seconds);
}


void setAltiOffset(){
  char result = bmp.startMeasurment();
  if(result!=0){
    delay(result);
    result = bmp.getTemperatureAndPressure(temperatureC, pressureMBar);
    if(result!=0){
      altiOffset = bmp.altitude(pressureMBar, pressureSeaLevel)*3.28;
    }
  }
}


void readBMP280(){
  char result = bmp.startMeasurment();
  if(result!=0){
    delay(result);
    result = bmp.getTemperatureAndPressure(temperatureC, pressureMBar);
    if(result!=0){
      altitudeFt = bmp.altitude(pressureMBar, pressureSeaLevel)*3.28;
      aglFt = altitudeFt - altiOffset;
    }
  }
}


void readESC(){
  prepareSerialRead();
  preTxMillis = millis();
  Serial1.readBytes(escData, ESC_DATA_SIZE);
  //delay(10);            //commented out and checksum errors went from 1-2 per second to occassionally upon throttle changes.
  transmitted++;
  postTxMillis = millis();
  if(postTxMillis-preTxMillis >= SERIAL1_TIMEOUT){
    armed = false;
    altiZeroed = false;
  }
  else{
    armed = true;
    if(!altiZeroed){
      setAltiOffset();  // to zero the altitude
      altiZeroed = true;  // to prevent from re-zeroing every time it's armed
    }
  }
  enforceChecksum();
  parseData();
}


void enforceChecksum(){
  //Check checksum, revert to previous data if bad:
  word checksum = word(escData[23], escData[22]);
  int sum = 0;
  for(int i=0; i<ESC_DATA_SIZE-2; i++){
    sum += escData[i];
  }
  //Serial.print("     SUM: ");
  //Serial.println(sum);
  //Serial.print("CHECKSUM: ");
  //Serial.println(checksum);
  if(sum != checksum){
    //Serial.println("_________________________________________________CHECKSUM FAILED!");
    failed++;
    if(failed>=1000) {
    transmitted = 1;
    //failed = 0;
  }
    for(int i=0; i<ESC_DATA_SIZE; i++){
      escData[i] = prevData[i];
    }
  }
  for(int i=0; i<ESC_DATA_SIZE; i++){
    prevData[i] = escData[i];
  }
}


void parseData(){
  inputPWM = word(escData[6], escData[7]);
  outputPWM = word(escData[8], escData[9]);
  rpm = word(escData[10], escData[11])*100/6.0;
  volts = word(escData[12], escData[13])*11/650.0;  
  amps = word(escData[14], escData[15])*330/2020.0;
  capTempC = -0.0126 * (word(escData[18], escData[19])) + 63.641;
  fetTempC = -0.0129 * (word(escData[20], escData[21])) + 64.889;
}
