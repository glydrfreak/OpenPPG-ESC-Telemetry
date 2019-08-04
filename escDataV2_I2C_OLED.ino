#include <Wire.h>
#include <Adafruit_SSD1306.h>
//#include <Fonts/FreeSans9pt7b.h>

#define VBATPIN         9
#define FAKE_GROUND     23  //MOSI pin used as general IO
#define ESC_BAUD_RATE   19200
#define ESC_DATA_SIZE   24
#define HEADER_BYTE     0x9B

byte escData[ESC_DATA_SIZE];
byte prevData[ESC_DATA_SIZE];
unsigned long displayMillis = 0;

float inputPWM = 0;
float outputPWM = 0;
float rpm = 0;
float amps = 0;
float volts = 0;
float capTempC = 0;
float fetTempC = 0;

Adafruit_SSD1306 display(128, 64, &Wire, 4);

void setup() {
  pinMode(FAKE_GROUND, OUTPUT);
  pinMode(VBATPIN, INPUT);
  digitalWrite(FAKE_GROUND, LOW);
  Serial.begin(ESC_BAUD_RATE);
  Serial1.begin(ESC_BAUD_RATE);
  initDisplay();  
  delay(1000);
}

void loop() {

  prepareSerialRead();
  Serial1.readBytes(escData, ESC_DATA_SIZE);

  
//  //check to see if 9B is in position 0, revert to previous data if not:
//  for(int i=0; i<ESC_DATA_SIZE; i++){
//    if (escData[i] == HEADER_BYTE){
//      //Serial.print(HEADER_BYTE, HEX);
//      //Serial.print(" in position ");
//      //Serial.println(i);
//      if(i != 0){
//        //Serial.println("********************************************************ERROR");
//        for(int j=0; j<ESC_DATA_SIZE; j++){
//          escData[j] = prevData[j];
//        }
//      }
//    }
//    prevData[i] = escData[i];
//  }

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
    Serial.println("_________________________________________________CHECKSUM FAILED!");
    for(int i=0; i<ESC_DATA_SIZE; i++){
      escData[i] = prevData[i];
    }
  }
  for(int i=0; i<ESC_DATA_SIZE; i++){
    prevData[i] = escData[i];
  }
  
  
  
  Serial.print("DATA: ");
  for(int i=0; i<ESC_DATA_SIZE; i++){
    Serial.print(escData[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // 0 Header byte
  // 1 Length of data packet
  // 2 ESC firmware version code
  // 3 Reserved byte
  // 4 Packet number MSB
  // 5 Packet number LSB
  
  // 6 Rx Throttle value MSB
  // 7 Rx Throttle value LSB ... throttle percent = (6 concat 7) * 100 / 1024.0
  inputPWM = word(escData[6], escData[7])*100/1024.0;
  Serial.print("INPUT PWM: ");
  Serial.println(inputPWM);
  
  // 8 Actual output PWM MSB
  // 9 Actual output PWM LSB ... output throttle percent = (8 concat 9) * 100 / 1024.0
  outputPWM = word(escData[8], escData[9])*100/1024.0;
  Serial.print("OUTPUT PWM: ");
  Serial.println(outputPWM);

  // 10 Actual rpm cycle MSB
  // 11 Actual rpm cycle LSB ... RPM = (10 concat 11) * 10 / (pole pairs)
  rpm = word(escData[10], escData[11])*10/7.0;
  Serial.print("RPM: ");
  Serial.println(rpm);
  
  // volts = (12 concat 13) * 11 / 650
  volts = word(escData[12], escData[13])*11/650.0;  
  Serial.print("VOLTS: ");
  Serial.println(volts);

  // amps = (14 concat 15) * 33 / 2020.0
  amps = word(escData[14], escData[15])*33/2020.0;
  Serial.print("AMPS: ");
  Serial.println(amps);

  // 16 Reserved byte
  // 17 Reserved byte
  
  // 18 CAP temp MSB 
  // 19 CAP temp LSB ... value coorelates to resistance of thermal sensor, which coorelates with temperature
  //3244 @ 22.722 C
  //3288 @ 22.167 C
  capTempC = -0.0126 * (word(escData[18], escData[19])) + 63.641;
  Serial.print("CAP TEMP: ");
  Serial.println(capTempC);
  

  // 20 FET temp MSB 
  // 21 FET temp LSB ... value coorelates to resistance of thermal sensor, which coorelates with temperature
  //3267 @ 22.722 C
  //3310 @ 22.167 C
  fetTempC = -0.0129 * (word(escData[20], escData[21])) + 64.889;
  Serial.print("FET TEMP: ");
  Serial.println(fetTempC); // FET Temp (C)
  
  // 22 Checksum LSB
  // 23 Checksum MSB ... Add bytes 0 through 21 to get the sum
  // sum 0 through 21:
  // 9B 16 3 2 3A 4A 0 0 0 0 0 0 B 84 0 0 0 0 C 78 C 8A E3 2
  // sum 9B through 8A = 739 = 0x02E3
    
  Serial.println();

  if(millis()-displayMillis>=1000){
    delay(500);
    updateDisp();
    delay(500);
    displayMillis = millis();
  }
}


void prepareSerialRead(){  
  while(Serial1.available()>0){
    byte t = Serial1.read();
  }
}


void initDisplay() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // initialize with the I2C addr 0x3C (for the 128x32)
  // Clear the buffer.
  display.clearDisplay();
  display.setRotation(0);
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(F("OpenPPG"));
  display.display();
  display.clearDisplay();
}


void updateDisp(){
  display.setCursor(36, 24);
  display.setTextSize(1);
  //display.print("RPM: ");
  //rpm = 9000;
  display.print(rpm,0);
  display.display();
  display.clearDisplay();
}


void updateDisplay() {

//inputPWM
//outputPWM
//rpm
//amps
//volts
//capTempC
//fetTempC
  
  display.setCursor(0, 0);
  display.setTextSize(2);  
  //display.setFont(&FreeSans9pt7b);
  //volts = 50.0;
  if(volts<10) display.print(" ");
  if(volts==0) volts=getBatteryLvl();
  display.print(volts,1);
  
  //display.setFont(NULL);
  display.setCursor(50, 8);
  display.setTextSize(1);
  display.print("V");

  display.setCursor(70,0);
  display.setTextSize(2);
  //amps = 38.0;
  if(amps<10) display.print(" ");
  display.print(amps,1);
  
  display.setCursor(120, 8);
  display.setTextSize(1);
  display.println("A");

  
  display.setCursor(36, 24);
  display.print("RPM: ");
  //rpm = 9000;
  display.println(rpm,0);

  display.setCursor(12, 39);
  display.print("TEMP");

  display.setCursor(0, 48);
  display.print("CAP: ");
  display.print(capTempC,0);

  display.setCursor(0, 56);
  display.print("FET: ");
  display.println(fetTempC,0);

  display.setCursor(90, 39);
  display.print("PWM");

  display.setCursor(78, 48);
  display.print("IN: ");
  //inputPWM = 1023;
  display.print(inputPWM,0);

  display.setCursor(72, 56);
  display.print("OUT: ");
  //outputPWM = 1023;
  display.println(outputPWM,0);
  
  display.display();
  display.clearDisplay();
}


float getBatteryLvl(){  
  float measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024.0; // convert to voltage
  return measuredvbat;
}

