#include "SSD1306Ascii.h" // display lib
#include "SSD1306AsciiAvrI2c.h"

#include "HID-Project.h"  // input lib

#include <FastLED.h>      // led lib

#define addr  0x3C

#define slider  A0

#define rotPin   1 // tx pin, blocks serial out unfortunately
#define rotInt   3 // pin 1 is interrupt pin 3 because reasons
#define button2  4
#define button1  5
#define button0  6
#define dirPin   7
#define ledPin   9 // single led on board
#define ledPin2 19 // expansion strip

#define divisor 20 // gives 51.2 steps across 0-1023 range
#define debug    1 // shows timings (and last instructions) as a mode
#define invert   0

#define numLeds 30
#define bright 127 // limits for ext. led strip as max brightness is not possible

uint32_t timings[7];
/* 
timings[] array stores timings information
  0  set at start of cycle
  1  set after slider is read
  2  set after interrupt is read
  3  set when button0 succeeds
  4  set when button1 succeeds
  5  set when button2 succeeds
  6  set at start of debug print 
*/

// char last[12]; // no longer in use
/*
last[] array stores last operations performed:
  0  most recent value
  15 least recent value 
*/

int delta, rotation, fillBut, fillBut2, mode, unsmoothed;

int  count, stdev; // debug modes for calculating stdev of slider
long sum, sumSq;

int slide[10];
/*
slide[] array stores data relating to the slider:
  0     current position, calibrated and smoothed
  1     current value
  2     last position, calibrated and smoothed
  3     last value
  4 - 7 last 4 readings, calibrated but not smoothed
  8     fancy position (percentage)
  9     unsmoothed, uncalibrated position
*/

bool updateRot, lastDir, doStdev, hideLed, hideDisp;

SSD1306AsciiAvrI2c oled;
CRGB leds[1];

void setup() {
  // Serial.begin(9600);              // init serial at 9600bps, no longer used as tx pin in use

  Keyboard.begin();                   // init keyboard

  oled.begin(&Adafruit128x32, addr);  // init display
  oled.displayRemap ( true   );       // flip display 180deg
  oled.invertDisplay( invert );       // invert if defined

  pinMode(rotPin,  INPUT_PULLUP);     // init encoder pins
  pinMode(dirPin,  INPUT_PULLUP);
  attachInterrupt(rotInt, readRotate, LOW);

  pinMode(button0, INPUT_PULLUP);     // init button pins
  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);

  FastLED.addLeds<WS2812B, ledPin, RGB>(leds, 1);
  leds[0] = CHSV(0, 0, 0);

  slide[4] = analogRead(slider);      // init slider readings
  slide[8] = 2 * round((float) slide[4] * 0.09765625 * 0.5);

  fullRefresh(0, slide[1], slide[6], 0); // default values for display
}

void loop() {
  timings[0] = millis();

  readButtons(); // button readings
  readSlide();   // slider readings
  
  timings[1] = millis();

  doUpdates(mode, updateRot, fillBut, fillBut2);

  if (mode == -1) showDebug();

  delay(1); // allows uploads to happen without resets
}