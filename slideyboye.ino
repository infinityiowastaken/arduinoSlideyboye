#include "SSD1306Ascii.h" // display lib
#include "SSD1306AsciiAvrI2c.h"

#include "HID-Project.h"  // input lib

#include <FastLED.h>      // led lib

#define addr  0x3C // i2c addr of display

#define slider  A0

#define rotPin   1 // tx pin, blocks serial out unfortunately
#define rotInt   3 // pin 1 is interrupt pin 3 because reasons
#define button2  4
#define button1  5
#define button0  6
#define dirPin   7
#define ledPin   9 // single led on board
#define ledPin2 19 // expansion strip, on A1 which maps to pin 19

#define divisor 20 // gives 51.2 steps across 0-1023 range
#define doDebug  1 // shows timings (and last instructions) as a mode
#define invert   0

#define numLeds 30
#define bright 127 // limits for ext. led strip as max brightness is not possible

#define buttonDebounce  300 // time between accepted readings
#define changeDebounce  150 // time for rotation in different directions
#define turningDebounce  13 // time for multiple rotations in same direction

struct timingsS {
  uint32_t start;       // set at start of cycle
  uint32_t readings;    // set after slider/buttons are read
  uint32_t debug;       // set at start of debug print
  uint32_t lastChange;  // set when any variable changes significantly
  uint32_t debounce[5]; // set by all functions that require a debounce
  /*
    debounce[] array stores the last time functions were triggered such that async 
    debouncing can be done for when they are triggered
      0 - 2 store the values for button0 through button2
      3     stores the value for the encoder
      4     stores the value for the slider (why? because it makes my life easy)
  */ 
};
struct slideS {
  int reading[4];
  int position[3];
  int value[2];
  byte fancyPosition;
  /*
    reading[] array stores last 4 raw readings of the slider:
      0     most recent
      3     oldest
    position[] array stores data relating to the raw values:
      0     current position, calibrated and smoothed
      1     current position, uncalibrated and smoothed
      2     last position, calibrated and smoothed
    value[] array stores the current and previous notch for volume
      0     current
      1     previous
  */
};
struct stdevS {
  bool doStdev;
  int  count; // number of items in sample
  int  stdev; // working standard deviation in sample
  long sum;   // sum of all sample elements
  long sumSq; // sum of squares of all sample elements
};

timingsS timings;
slideS slide;
stdevS stdev;
// char last[12]; // no longer in use
/*
  last[] array stores last operations performed:
    0  most recent value
    15 least recent value 
*/

int delta, rotation, fillBut, fillBut2, mode;

bool updateRot, lastDir, hideLed, hideDisp, doTimeout = true, hidden;

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

  slide.reading[0] = analogRead(slider);      // init slider readings
  slide.fancyPosition = 2 * round((float) slide.reading[0] * 0.09765625 * 0.5);

  fullRefresh(0, slide.reading[0], slide.fancyPosition, 0); // default values for display
}

void loop() {
  timings.lastChange = max(max(max(timings.debounce[0], timings.debounce[1]), timings.debounce[2]),  max(timings.debounce[3], timings.debounce[4]));
  timings.start = millis();

  readButtons(timings, mode); // button readings
  readSlide(slide, mode);     // slider readings
  
  timings.readings = millis();

  doUpdates(mode, updateRot, fillBut, fillBut2);

  if (mode == -1) showDebug();
  if (!hideDisp)  dispInd(mode);

  if (timings.start - timings.lastChange > 360000 && doTimeout) { // timeout because light is annoying
    hideDisp = true;
    hideLed  = true;

    if (!hidden) oled.clear();
    if (!hidden) leds[0] = CRGB::Black;
    hidden = true;
  } else {
    hideDisp = false;
    hideLed  = false;

    if (hidden) fullRefresh(mode, slide.position[0], slide.fancyPosition, rotation); 
    hidden = false;
  }

  delay(1); // allows uploads to happen without resets
}