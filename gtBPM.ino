#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for SSD1306 display connected using software SPI (default case):
#define OLED_MOSI   9 // D1 label
#define OLED_CLK   10 // D0 label
#define OLED_DC    14 // DC label
#define OLED_CS    15 // CS label
#define OLED_RESET 16 // RES label

#define PWM_PIN 3
#define INTERRUPT_PIN 7 // connect 3 to 7 for internal interrupt clock
#define OUTCLOCK_PIN  8
#define WIPER_PIN A1

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

// set up the display
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
unsigned long lastChangedTime = 0;
int lastRawWiper    = 0;
const int debounceDelay = 500;

// States for sleepless output
unsigned long lastBeatTime  = 0;
unsigned long lastBeatTimeM = 0;
const int beatDelay  = 20;

// Pointless UI flare
//   "/\\", "||", "\\/", "--"
const char* showOff[] = {
  "//", "--", "\\\\", "||"
};
// I can calculate this but why bother!
const int showOffLength = (4);
int showOffIndex = 0;

void setup() {
  Serial.begin(9600);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Set up output
  pinMode(OUTCLOCK_PIN, OUTPUT);

  // Interrupt setup
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), countClock, RISING);
  drawIntro();
}

void loop() {
  analogWrite(PWM_PIN, 127); // Scope measures this as 976.5hz

  // Read the wiper
  int val = analogRead(WIPER_PIN);
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
  }

  // Check if we're on the beat and reset the interrupt counter
  if(interruptCount >= BPM_COUNT[bpmIndex]) {
    lastBeatTime  = millis();
    lastBeatTimeM = micros();
    interruptCount = 0;
  }
  // Output if we're on, or just after the beat
  // This actually matches for several clock cycles because milli's is not that precise
  // but it's no matter because we maintain the trigger pulse anyway
  if( millis() < (lastBeatTime + beatDelay)) {
    // send the beat external trigger
    digitalWrite(OUTCLOCK_PIN, 100);    
  } else {
    // pull it down
    digitalWrite(OUTCLOCK_PIN, 0);
  }

  display.clearDisplay();
  display.setTextSize(3);
  if (BPM_LABEL[bpmIndex] < 100) {
    display.setCursor(45, 15);
  } else {
    display.setCursor(35, 15);
  }
  display.setTextColor(SSD1306_WHITE);
  display.print(BPM_LABEL[bpmIndex]);
  display.setTextSize(2);
  display.setCursor(0,50);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.print(F(">BPM"));

  display.setCursor(100,50);
  display.setTextColor(SSD1306_WHITE);
  
  // Set up a beat vbell
  // If we allow multiple clocks to affect this we get a 'skipping' effect
  if( (millis() - lastBeatTime) <= 15 ) {
    showOffIndex++;
    if (showOffIndex == (showOffLength)) {
      showOffIndex = 0;
    }
  }
  display.print(showOff[showOffIndex]);
  display.display();

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
