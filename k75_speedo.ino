#include <SPI.h>
#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

Adafruit_8x8matrix matrix = Adafruit_8x8matrix();

// speedo variables
int pinSpeedoSensor = A3;
int reedCounter = 0;
int sensorValue = 0;        // raw sensor value, 0 to 1023
int sensorState = 0;       // the current sensorState from the input pin
int lastSensorState = 0;   // the previous sensorState from the input pin

static const uint8_t PROGMEM
  logo_bmp[] = {
        B00011100,
        B00001100,
        B00111100,
        B00000000,
        B00111100,
        B00011000,
        B11011011,
        B11000011
      },
    one[] = {
        B001,
        B001,
        B001,
        B001,
        B001,
      },
      two[] = {
        B111,
        B001,
        B111,
        B100,
        B111,
      },
      three[] = {
        B111,
        B001,
        B111,
        B001,
        B111,
      },
      four[] = {
        B101,
        B101,
        B111,
        B001,
        B001,
      },
      five[] = {
        B111,
        B100,
        B111,
        B001,
        B111,
      },
      six[] = {
        B111,
        B100,
        B111,
        B101,
        B111,
      },
      seven[] = {
        B111,
        B001,
        B001,
        B001,
        B001,
      },
      eight[] = {
        B111,
        B101,
        B111,
        B101,
        B111,
      },
      nine[] = {
        B111,
        B101,
        B111,
        B001,
        B001,
      },
      zero[] = {
        B111,
        B101,
        B101,
        B101,
        B111,
      }
    ;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(57600);
  Serial.println("BMW K75 at your service");

  // pinMode(pinSpeedoSensor, INPUT);
  analogReference(INTERNAL); // set reference voltage to 1.1v. 1023 = 1.1v, 0 = 0v. Peak voltage of bmw sensor should be ± 400.

  // pinMode(A2, INPUT);
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

  configureDisplay();
 
  logo();

  configureInterupts();
}

int counter = 0;

void loop() {
  matrix.clear();
  speedo();
  delay(500);
}

void configureDisplay() {
  matrix.begin(0x70);  // pass in the address
  matrix.setTextSize(1);     // size 1 == 8 pixels high
  matrix.setTextWrap(false); // Don't wrap at end of line - will do ourselves
  matrix.setBrightness(5); // 10 is max to to exceed 20m amps
}

void configureInterupts() {
  cli(); //stop interrupts
  //set timer1 interrupt at 1kHz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;
  // set timer count for 1khz increments
  OCR1A = 1999;// = (1/1000) / ((1/(16*10^6))*8) - 1
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS11 bit for 8 prescaler
  TCCR1B |= (1 << CS11);   
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);

  sei();//allow interrupts
  //END TIMER SETUP
}

void logo(){
  matrix.clear();
  matrix.drawBitmap(0, 0, logo_bmp, 8, 8, LED_ON);
  matrix.fillRect(0, 6, 2, 2, LED_ON);
  matrix.fillRect(3, 6, 2, 2, LED_ON);
  matrix.fillRect(6, 6, 2, 2, LED_ON);
  matrix.writeDisplay();
  
  delay(1000); 

  matrix.fillRect(0, 0, 8, 8, LED_OFF);
  matrix.writeDisplay();
}

void speedo(){
  Serial.println(reedCounter);
  printnr(reedCounter, 0, 0);
  reedCounter = 0;
}

// display functions
void printnr(int number, int xoffset, int yoffset){
  int first = (number % 10);
  printnr_digit(first, xoffset, yoffset);  
  if (number > 9){
    int second = (int)((number / 10) % 10);
    xoffset -= 4;
    printnr_digit(second, xoffset, yoffset);  
  }

  if (number > 99){
    int third = (int)((number / 100) % 10);
    xoffset -= 4;
    printnr_digit(third, xoffset, yoffset);  
  }
}

void printnr_digit(int number, int xoffset, int yoffset){
 switch (number) {
    case 0:
      matrix.drawBitmap(xoffset, yoffset, zero, 8, 5, LED_ON);
      break;
    case 1:
      matrix.drawBitmap(xoffset, yoffset, one, 8, 5, LED_ON);
      break;
    case 2:
      matrix.drawBitmap(xoffset, yoffset, two, 8, 5, LED_ON);
      break;
    case 3:
      matrix.drawBitmap(xoffset, yoffset, three, 8, 5, LED_ON);
      break;
    case 4:
      matrix.drawBitmap(xoffset, yoffset, four, 8, 5, LED_ON);
      break;
    case 5:
      matrix.drawBitmap(xoffset, yoffset, five, 8, 5, LED_ON);
      break;
    case 6:
      matrix.drawBitmap(xoffset, yoffset, six, 8, 5, LED_ON);
      break;
    case 7:
      matrix.drawBitmap(xoffset, yoffset, seven, 8, 5, LED_ON);
      break;
    case 8:
      matrix.drawBitmap(xoffset, yoffset, eight, 8, 5, LED_ON);
      break;
    case 9:
      matrix.drawBitmap(xoffset, yoffset, nine, 8, 5, LED_ON);
      break;
    break;
  }
 
  matrix.writeDisplay();
}

// timer functions
ISR(TIMER1_COMPA_vect) {//Interrupt at freq of 1kHz to measure reed switch
  readSpeedoSensor();
}

int signalcount = 0;

void readSpeedoSensor() {
  sensorValue = analogRead(pinSpeedoSensor);

  if (sensorValue > 120) {
      sensorState = 1;
  } else {
      sensorState = 0;
  }

  // If the sensor changed, due to noise or signal:
  if (sensorState != lastSensorState) {
    signalcount = 0;
  } else if (sensorState == 1){
    signalcount ++;
  }

  if (signalcount == 3) { // 3 consecutive ms seems reliable. at idle speed, we have 5 to 7 1ms > 100mv
    reedCounter ++;
  }
  
//  if (logcount % 100 == 0){
//    Serial.print(sensorState); 
//    Serial.print(" - ");
//    Serial.print(sensorValue);
//    Serial.print(" - ");
//    Serial.print(signalcount);
//    Serial.print(" - ");
//    Serial.println(reedCounter);
//  }

  // save the sensorState. Next time through the loop, it'll be the lastSensorState:
  lastSensorState = sensorState;
}
