#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeMonoBoldOblique9pt7b.h>

#define OLED_RESET  4
Adafruit_SSD1306 display(OLED_RESET);

#if (SSD1306_LCDHEIGHT != 64)
// #error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

// speedo variables
int pinSpeedoSensor = A3; // analog 3
int pinFuelSensor = A0;
int pinTemperatureSensor = A1;
int pinOilSensor = 2; // digital 2

int displayPinHigh = 11; // digital 11
int displayPinLow = 10; // digital 10

void setup() {
  // put your setup code here, to run once:
  Serial.begin(57600);
  Serial.println("K75 Speedo v0.2");

  // pinMode(pinSpeedoSensor, INPUT);
  // analogReference(INTERNAL); // set reference voltage to 1.1v. 1023 = 1.1v, 0 = 0v. Peak voltage of bmw sensor should be ± 400.

  pinMode(displayPinLow, OUTPUT);
  pinMode(displayPinHigh, OUTPUT);
  pinMode(pinOilSensor, INPUT_PULLUP);
  
  digitalWrite(displayPinHigh, LOW);
  digitalWrite(displayPinHigh, HIGH);

  delay(200); // boot display

  configureDisplay();
  configureInterupts();
}

int counter = 0;

void loop() {
  display.setFont();
  display.clearDisplay();
  display.setCursor(0,0);
  speedo();
  fuel();
  temperature();
  oil();
  display.display();
  // delay(300);
  delay(1000);
}

void configureDisplay() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32) 0x3D for 64?
  
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(255);
  display.ssd1306_command(SSD1306_INVERTDISPLAY);
  
  // Clear the buffer.
  display.clearDisplay();

  // draw a single pixel
  // text display tests
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(45,10);
  display.println("K75");
  display.display();
  delay(500);
  display.clearDisplay();
  display.setCursor(10,10);
  display.println("SKRAMBLER");
  display.display();
  delay(300);
  
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

// timer functions
ISR(TIMER1_COMPA_vect) {//Interrupt at freq of 1kHz to measure reed switch
  readSpeedoSensor();
  readFuelSensor();
//  readOilSensor();
//  readTemperatureSensor();
}

// speedo
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
      if (lapses[i] < 1.5 * lapses[0] && lapses[i] > lapses[0] / 1.5){
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
  display.setCursor(10,5);
  display.setTextSize(4); // 4
  display.print(kmh);
  display.setTextSize(2); // 2
  display.setFont();
  display.setCursor(86,17); // 86, 17 with default font
  display.println("kmh");
  kmhprev = kmh;
  highprev = high0;
}

int prevnr1 = 0 ;
int prevnr2 = 0 ;

int signalcount = 0;
int speedSensorValue = 0;        // raw sensor value, 0 to 1023
int speedSensorState = 0;       // the current speedSensorState from the input pin
int lastSensorState = 0;   // the previous speedSensorState from the input pin
// 4800 pulses per mile -> 2983 pulses per km -> approx 3 pulses per meter
unsigned long clockcount = 0;

void readSpeedoSensor() {
  clockcount ++;
  speedSensorValue = analogRead(pinSpeedoSensor);

  if (speedSensorValue > 100) {
      speedSensorState = 1;
  } else if (speedSensorValue == 0) {
      speedSensorState = 0;
  } else {
      speedSensorState = 0;
  }

  // If the sensor changed, due to noise or signal:
  if (speedSensorState != lastSensorState) {
    signalcount = 0;
  } else if (speedSensorState == 1){
    signalcount ++;
  }

  if (signalcount == 3) { // 3 consecutive ms seems reliable. at idle speed, we have 5 to 7 1ms > 100mv
    high5 = high4;
    high4 = high3;
    high3 = high2;
    high2 = high1;
    high1 = high0;
    high0 = clockcount;
  }
  
  // save the speedSensorState. Next time through the loop, it'll be the lastSensorState:
  lastSensorState = speedSensorState;
  if (clockcount >= 4294967290){
      clockcount = 0;
  }
  
}

int fuelSensorValue = 0;
void readFuelSensor() {
  fuelSensorValue = analogRead(pinFuelSensor);
}

void fuel(){
  display.setCursor(0,30);
  display.setTextSize(1);
  display.print(fuelSensorValue);

  Serial.print("FuelSensorValue: ");
  Serial.println(fuelSensorValue);
}

int oilSensorValue = 0;
void readOilSensor() {
  oilSensorValue = digitalRead(pinOilSensor);
}

void oil(){
  display.setCursor(35,30);
  display.setTextSize(1);

  if (oilSensorValue == HIGH) {
    display.print("OIL"); 
  } else {
    display.print(":)"); 
  }

  Serial.print("OilSensorValue: ");
  Serial.println(oilSensorValue);
}

int temperatureSensorValue = 0;
void readTemperatureSensor() {
  temperatureSensorValue = analogRead(pinTemperatureSensor);
}

void temperature(){
  display.setCursor(70,30);
  display.setTextSize(1);
  display.print(temperatureSensorValue);

  Serial.print("TemperatureSensorValue: ");
  Serial.println(temperatureSensorValue);
}

