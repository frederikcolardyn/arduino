#include <SPI.h>
#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

// speedo variables
int pinSpeedoSensor = A3; // analog 3
int displayPinHigh = 11; // digital 11

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
      },
      one_small[] = {
        B01,
        B01,
        B01,
        B01,
        B01,
      },
      two_small[] = {
        B11,
        B01,
        B11,
        B10,
        B11,
      },
      three_small[] = {
        B11,
        B01,
        B11,
        B01,
        B11,
      },
      four_small[] = {
        B11,
        B11,
        B11,
        B01,
        B01,
      },
      five_small[] = {
        B11,
        B10,
        B11,
        B01,
        B11,
      },
      six_small[] = {
        B11,
        B10,
        B11,
        B11,
        B11,
      },
      seven_small[] = {
        B11,
        B01,
        B01,
        B01,
        B01,
      },
      eight_small[] = {
        B11,
        B11,
        B00,
        B11,
        B11,
      },
      nine_small[] = {
        B11,
        B11,
        B11,
        B01,
        B01,
      },
      zero_small[] = {
        B11,
        B11,
        B11,
        B11,
        B11,
      }
    ;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(57600);
  Serial.println("BMW K75 at your service");

  // pinMode(pinSpeedoSensor, INPUT);
  analogReference(INTERNAL); // set reference voltage to 1.1v. 1023 = 1.1v, 0 = 0v. Peak voltage of bmw sensor should be ± 400.

  // pinMode(A2, INPUT);
  pinMode(displayPinHigh, OUTPUT);
  digitalWrite(displayPinHigh, HIGH);

  configureDisplay();
 
  logo();

  configureInterupts();
}

int counter = 0;

void loop() {
  // matrix.clear();
  // speedo();
  demo();
  delay(2000);
}

void demo(){  
  
}

void configureDisplay() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);  // initialize with the I2C addr 0x3D (for the 128x64)
  display.display();
  delay(2000);

  // Clear the buffer.
  display.clearDisplay();

  // draw a single pixel
  // text display tests
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("K75 SKRAMBLER");
  display.display();
  delay(2000);
  display.clearDisplay();

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

int count = 0;
void logo(){
  display.println(count++);
  display.display();
}

unsigned long high0 = 0;
unsigned long high1 = 0;
unsigned long high2 = 0;
unsigned long high3 = 0;
unsigned long high4 = 0;
unsigned long high5 = 0;
int kmhprev = 0;

unsigned long highprev = 0;

void speedo(){
  int kmh = 0;

  if (high0 == highprev){
    kmh = 0;
  } else {
    int lapse5 = high4 - high5;
    int lapse4 = high3 - high4;
    int lapse3 = high2 - high3;
    int lapse2 = high1 - high2;
    int lapse1 = high0 - high1;

    int lapses[] = {high4 - high5, high3 - high4, high2 - high3, high1 - high2, high0 - high1};
    int count = 0;
    float total = 0;
    int i;
    for (i = 0; i < 5; i = i + 1) {
      if (lapses[i] < 2 * lapses[0] && lapses[i] > lapses[0] / 2){
          total += lapses[i];
          count ++;
      }
    }
    
    // a lapse should be between 5ms and 150ms
    // 180kmh = 50m/sec = 150 pulses / sec = 6ms lapse
    // 120kmh = 33.3m/sec = 100 pulses / sec = 10ms lapse
    // 7kmh = 2m/sec = 8 pulses / sec = 125ms lapse
    
    // take avg
    if (count >= 4){
      float lapse = total / count;
      kmh = 1200 / lapse;  
    } else {
      kmh = kmhprev;
    }
  }
  display.println(kmh);
  display.display();
  kmhprev = kmh;
  highprev = high0;
}

int prevnr1 = 0 ;
int prevnr2 = 0 ;

// timer functions
ISR(TIMER1_COMPA_vect) {//Interrupt at freq of 1kHz to measure reed switch
  readSpeedoSensor();
}

int signalcount = 0;
int sensorValue = 0;        // raw sensor value, 0 to 1023
int sensorState = 0;       // the current sensorState from the input pin
int lastSensorState = 0;   // the previous sensorState from the input pin
// 4800 pulses per mile -> 2983 pulses per km -> approx 3 pulses per meter
unsigned long clockcount = 0;

void readSpeedoSensor() {
  clockcount ++;
  sensorValue = analogRead(pinSpeedoSensor);

  if (sensorValue > 100) {
      sensorState = 1;
  } else if (sensorValue == 0) {
      sensorState = 0;
  } else {
      sensorState = 0;
  }

  // If the sensor changed, due to noise or signal:
  if (sensorState != lastSensorState) {
    signalcount = 0;
  } else if (sensorState == 1){
    signalcount ++;
  }

  if (signalcount == 4) { // 3 consecutive ms seems reliable. at idle speed, we have 5 to 7 1ms > 100mv
    high5 = high4;
    high4 = high3;
    high3 = high2;
    high2 = high1;
    high1 = high0;
    high0 = clockcount;
  }
  
  // save the sensorState. Next time through the loop, it'll be the lastSensorState:
  lastSensorState = sensorState;
  if (clockcount >= 4294967290){
      clockcount = 0;
  }
  
}
