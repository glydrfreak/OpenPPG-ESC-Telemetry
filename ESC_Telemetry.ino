/*
 * Single Prop OpenPPG ESC telemetry reader - Written by Braedin Butler
 * Written for use with ADAFRUIT FEATHER M0 and APD HV Pro ESC
 * As long as the transmission intervals are no faster than
 * 24ms, the data transfer success rate should be 100%.
 */

#include <ResponsiveAnalogRead.h>  // smoothing for throttle
#include <Servo.h> // to control ESCs

#define POT_PIN       A0  // throttle pot input
#define ESC_PIN       12  // for PWM signal to the ESC
#define LED_BUILTIN   13  // built in LED on the feather M0, HIGH when armed, LOW when disarmed
#define SWITCH_PIN    A1  // slide switch to arm the ESC, HIGH to arm, LOW to disarm
#define ESC_BAUD_RATE   256000
#define ESC_DATA_SIZE   20
#define READ_INTERVAL    0
#define ESC_TIMEOUT     10

byte escData[ESC_DATA_SIZE];
byte prevData[ESC_DATA_SIZE];
unsigned long readMillis = 0;
unsigned long transmitted = 0;
unsigned long failed = 0;
bool receiving = false;
bool armed = false;

uint16_t _volts = 0;
uint16_t _temperatureC = 0;
int16_t _amps = 0;
uint32_t _eRPM = 0;
uint16_t _inPWM = 0;
uint16_t _outPWM = 0;

float volts = 0;
float temperatureC = 0;
float amps = 0;
float eRPM = 0;
float inPWM = 0;
float outPWM = 0;

ResponsiveAnalogRead pot(POT_PIN, false);
Servo esc; // Creating a servo class with name of esc


void setup() {
  delay(250);  // power-up safety delay
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(SWITCH_PIN, INPUT);
  analogReadResolution(12);     // M0 chip provides 12bit resolution
  pot.setAnalogResolution(4096);
  esc.attach(ESC_PIN);
  esc.writeMicroseconds(0); // make sure motors off
  Serial1.begin(ESC_BAUD_RATE); 
  Serial1.setTimeout(ESC_TIMEOUT);
  delay(1000);
}

void loop() {

  int switchVal = digitalRead(SWITCH_PIN);
  if (switchVal && !armed){
    armSystem();
  }
  else if (!switchVal && armed){
    disarmSystem();
  }
  
  if (armed){ 
    handleThrottle(); 
  }
  

  if(millis()-readMillis>=READ_INTERVAL){
    readMillis = millis();
    prepareSerialRead();
    Serial1.readBytes(escData, ESC_DATA_SIZE);
    transmitted++;
    if(transmitted>=ESC_TIMEOUT) {
      transmitted = 1;
      failed = 0;
    }
  }

  //enforceChecksum();
  
  printRawSentence();
 
  parseData();

}


void prepareSerialRead(){  
  while(Serial1.available()>0){
    byte t = Serial1.read();
  }
}


void handleThrottle() {
  pot.update();
  int potVal = pot.getValue();
  int val = map(potVal, 0, 4095, 1110, 2000); // mapping val to minimum and maximum
  esc.writeMicroseconds(val); // using val as the signal to esc
  Serial.print(F("WRITING: "));
  Serial.println(val);
}


void armSystem(){
  Serial.println(F("Sending Arm Signal"));
  esc.writeMicroseconds(1000); // initialize the signal to 1000
  armed = true;
  digitalWrite(LED_BUILTIN, HIGH);
}


void disarmSystem() {
  esc.writeMicroseconds(0);
  armed = false;
  Serial.println(F("disarmed"));
  digitalWrite(LED_BUILTIN, LOW);
  delay(1500); // dont allow immediate rearming
}


void enforceChecksum(){
  //Check checksum, revert to previous data if bad:
  word checksum = word(escData[19], escData[18]);
  int sum = 0;
  for(int i=0; i<ESC_DATA_SIZE-2; i++){
    sum += escData[i];
  }
  //Serial.print("     SUM: ");
  //Serial.println(sum);
  //Serial.print("CHECKSUM: ");
  //Serial.println(checksum);
  if(sum != checksum){
    Serial.println("_________________________________________________CHECKSUM FAILED!");
    failed++;
    if(failed>=1000) {
    transmitted = 1;
    failed = 0;
  }
    for(int i=0; i<ESC_DATA_SIZE; i++){
      escData[i] = prevData[i];
    }
  }
  for(int i=0; i<ESC_DATA_SIZE; i++){
    prevData[i] = escData[i];
  }
}


void printRawSentence(){
  Serial.print("DATA: ");
  for(int i=0; i<ESC_DATA_SIZE; i++){
    Serial.print(escData[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}


void parseData(){
  // LSB First
  
  //_volts = word(escData[1], escData[0]);
  _volts = ((unsigned int)escData[1] << 8) + escData[0];
  volts = _volts/100.0;
  //reading 23.00 = 22.7 actual
  //reading 16.00 = 15.17 actual
  Serial.print("Volts: ");
  Serial.println(volts);
  
  _temperatureC = word(escData[3], escData[2]);
  temperatureC = _temperatureC/100.0;
  //reading 17.4C = 63.32F in 84F ambient?
  Serial.print("TemperatureC: ");
  Serial.println(temperatureC);

  _amps = word(escData[5], escData[4]);
  amps = _amps/10.0;
  Serial.print("Amps: ");
  Serial.println(amps);

  // 7 and 6 are reserved bytes
  
  _eRPM = escData[11];     // 0 
  _eRPM << 8;
  _eRPM += escData[10];    // 0
  _eRPM << 8;
  _eRPM += escData[9];     // 30
  _eRPM << 8;
  _eRPM += escData[8];     // b4
  eRPM = _eRPM/6.0/2.0;
  Serial.print("eRPM: ");
  Serial.println(eRPM);
  
  _inPWM = word(escData[13], escData[12]);
  inPWM = _inPWM/100.0;
  Serial.print("inPWM: ");
  Serial.println(inPWM);

  _outPWM = word(escData[15], escData[14]);
  outPWM = _outPWM/100.0;
  Serial.print("outPWM: ");
  Serial.println(outPWM);
  
  // 17 and 16 are reserved bytes
  // 19 and 18 is checksum
  
}
