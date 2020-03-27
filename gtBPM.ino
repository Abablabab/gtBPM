#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for SSD1306 display connected using software SPI (default case):
#define OLED_MOSI   9
#define OLED_CLK   10
#define OLED_DC    14
#define OLED_CS    15
#define OLED_RESET 16

#define INTERRUPT_PIN 7
#define OUTCLOCK_PIN  8

// BPM tables for the fast clock divisions
const int BPM_LABEL[] = {
  40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200, 210
};
const int BPM_COUNT[] = {
  1465, 1172, 977, 837, 732, 651, 586, 533, 488, 451, 419, 391, 366, 345, 326, 308, 293, 279
};
#define BMPS ((sizeof(BPM_COUNT)/sizeof(char *))-1) 
int bpmIndex = 0;
volatile int interruptCount = 0;

// Pindefs for IO
#define WIPER_PIN A1

Adafruit_SSD1306 display(
  SCREEN_WIDTH, 
  SCREEN_HEIGHT,
  OLED_MOSI, 
  OLED_CLK, 
  OLED_DC, 
  OLED_RESET, 
  OLED_CS
);

// Debounce variables
int lastChangedTime = 0;
int lastRawWiper    = 0;
const int debounceDelay = 500;


void setup() {
  Serial.begin(9600);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Button management
  //pinMode(BUTTON_DOWN, INPUT);
  //pinMode(BUTTON_UP,   INPUT);

  // Set up output
  pinMode(OUTCLOCK_PIN, OUTPUT);

  // Interrupt setup
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), countClock, RISING);
  drawIntro();
}

void loop() {
  analogWrite(3, 127); // Scope measures this as 976.5hz

  // Read the wiper
  int val = analogRead(WIPER_PIN);
  //lastRawWiper = val;
  int currentRawWiper = val;

  if (val > 780) {
    val = BMPS;
  } else if (val < 180) {
    val = 0;
  } else {
    val = map(val, 180, 780, 0, BMPS);
  }
  
  if (val != bpmIndex) {
    // Check for bouncing between two values!
    if ( 
         ((millis() - lastChangedTime) > debounceDelay) 
      && ((currentRawWiper > lastRawWiper+40) || (currentRawWiper < lastRawWiper-40))
    ){
      interruptCount = 0;
      bpmIndex=val;
      lastChangedTime = millis();
      lastRawWiper=currentRawWiper;
    }
    display.clearDisplay();
    display.setTextSize(3);
    if (BPM_LABEL[bpmIndex] < 100) {
      display.setCursor(45, 15);
    } else {
      display.setCursor(35, 15);
    }
    display.setTextColor(SSD1306_WHITE);
    display.println(BPM_LABEL[bpmIndex]);

    display.setTextSize(2);

    display.setCursor(0,50);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.print(F(">BPM"));

    display.setCursor(80,50);
    display.setTextColor(SSD1306_WHITE);
    display.print(currentRawWiper);
    display.display();
  } 

  // now we check the count
  if(interruptCount >= BPM_COUNT[bpmIndex]) {
    digitalWrite(OUTCLOCK_PIN, 100);
    delay(10);
    digitalWrite(OUTCLOCK_PIN, 0);
    interruptCount = 0;
  }
}

void drawIntro(void) {
  display.clearDisplay();
  display.setTextSize(1);                             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);                // Draw white text
  display.setCursor(0,0);                             // Start at top-left corner
  display.println(F("Chinesium Clock"));
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
  display.println(F("v0.1"));
  display.setTextSize(2);                             // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(30, 30);
  display.println(F("HELLO!"));
  display.display();
  delay(800);
  display.setCursor(45, 50);
  display.println(F("<3"));
  display.display();
  delay(2000);
}

void countClock() {
  // We're in interrupt country now
  // we'll keep this mega short and manage it in the loop
  interruptCount++;
}
