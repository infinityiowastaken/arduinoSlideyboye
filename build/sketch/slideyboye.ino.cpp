#include <Arduino.h>
#line 1 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino"
#line 1 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino"
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
  uint32_t debounce[4]; // set by all functions that require a debounce
  /*
    debounce[] array stores the last time functions were triggered such that async 
    debouncing can be done for when they are triggered
      0 - 2 store the values for button0 through button2
      3     stores the value for the encoder
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

bool updateRot, lastDir, hideLed, hideDisp;

SSD1306AsciiAvrI2c oled;
CRGB leds[1];

#line 87 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino"
void setup();
#line 113 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino"
void loop();
#line 10 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\functions.ino"
void readButtons(timingsS &timings, int &mode);
#line 69 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\functions.ino"
void readSlide(slideS &slide, int mode);
#line 113 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\functions.ino"
void readRotate();
#line 155 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\functions.ino"
void showDebug();
#line 216 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\functions.ino"
void addToStdev(stdevS &data, int value);
#line 228 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\functions.ino"
void resetStdev(stdevS &data);
#line 234 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\functions.ino"
void calcStdev(stdevS &data);
#line 252 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\functions.ino"
void fullRefresh(int mode, int value, int position, int rotation);
#line 258 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\functions.ino"
void dispMode(int mode);
#line 304 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\functions.ino"
void dispSlide(int value, int position, int mode);
#line 322 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\functions.ino"
void dispEncod(int value);
#line 329 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\functions.ino"
void dispInd(int mode);
#line 338 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\functions.ino"
void doUpdates(int mode, bool &updateRot, int &countdown0, int &countdown1);
#line 372 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\functions.ino"
int calib(int input);
#line 383 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\functions.ino"
int smooth(int values[4]);
#line 87 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino"
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
  timings.start = millis();
  
  readButtons(timings, mode); // button readings
  readSlide(slide, mode);   // slider readings
  
  timings.readings = millis();

  doUpdates(mode, updateRot, fillBut, fillBut2);

  if (mode == -1) showDebug();
  dispInd(mode);

  delay(1); // allows uploads to happen without resets
}
#line 1 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\functions.ino"
// functions.ino
// all of the functions used within the main program, separated out for simplicity

/*
  things i let functions do:
  - read from gpio internally
  - set and read from structures, although they have to be passed to the functions
*/

void readButtons(timingsS &timings, int &mode) { // button reading
  // button 0 is on the encoder, button 1 is centre, button 2 is far left

  if (digitalRead(button0) == LOW && millis() - timings.debounce[0] > buttonDebounce) {
    timings.debounce[0] = millis();         // ratelimit/debounce for mode change

    if (mode < 2)   mode ++;
    else            mode = -1 * doDebug;
    
    if (mode == 0)  fullRefresh(mode, slide.position[0], slide.fancyPosition, rotation); // fix for minor debug issue
    else            dispMode(mode);
  } // button 0 changes the mode

  if (digitalRead(button1) == LOW && millis() - timings.debounce[1] > buttonDebounce) {
    timings.debounce[1] = millis();    // ratelimit/debounce for mode change

    if      (mode == 0) Keyboard.write(KEY_F24);
    else if (mode == 1) Keyboard.write(KEY_DOWN);
    else if (mode == 2) {
      Keyboard.press(KEY_LEFT_CTRL);
      Keyboard.press(KEY_LEFT_ALT);
      Keyboard.press(KEY_U);
      delay(17);
      Keyboard.releaseAll();
    }
    else if (mode == -1) resetStdev(stdev);

    log(47); // log o
    fillBut = 5; // on screen button animation
  } // button 1 performs an operation based upon the mode

  if (digitalRead(button2) == LOW && millis() - timings.debounce[2] > buttonDebounce) {
    timings.debounce[2] = millis();    // ratelimit/debounce for mode change

    if      (mode == 0) Keyboard.write(KEY_F21);
    else if (mode == 1) {
      Keyboard.write(KEY_UP);
      delay(17);
      Keyboard.press(KEY_LEFT_CTRL);
      Keyboard.press(KEY_0);
      delay(17);
      Keyboard.releaseAll();
    }
    else if (mode == 2) {
      Keyboard.press(KEY_LEFT_CTRL);
      Keyboard.press(KEY_H);
      delay(17);
      Keyboard.releaseAll();
    } 
    else if (mode == -1) {
      hideDisp = !hideDisp;
      hideLed  = !hideLed;
      oled.clear();
    }

    log(48);
    fillBut2 = 5;
  } // button 2 performs an operation based upon the mode
}
void readSlide(slideS &slide, int mode) { // slider reading
  for (int i = 2; i >= 0; i--) slide.reading[i + 1] = slide.reading[i]; // move last 3 values back one place in array
  slide.reading[0] = analogRead(slider);                                // update most recent value to current

  slide.position[1] = smooth(slide.reading);                           // take weighted mean of last 4 samples to reduce error
  slide.position[0] = calib(slide.position[1]);                        // apply correction to smoothed value
  slide.value[0] = round(((float) 512 - slide.position[0]) / divisor); // turn smoothed & corrected value into one of 51 positions

  delta = slide.value[1] - slide.value[0]; // work out change in values over last cycle

  if (abs(slide.position[2] - slide.position[0]) < 20) delta = 0;      // slight hysteresis to reduce jitter - only allows changes 
                                                                       // if last two values have significant change between them

  if (delta != 0) {
    for (int i = 0; i < abs(delta); i++) {
      if (delta > 0) {
        if (mode == 0) {
          Consumer.write(MEDIA_VOL_UP);
          slide.fancyPosition += 2;      // change percentage for volume
        }
        else if (mode == 1) Keyboard.write(KEY_EQUAL);
        else if (mode == 2) Keyboard.write(KEY_UP);
        if (doDebug) log(43); // log '+'
      } else {
        if (mode == 0) {
          Consumer.write(MEDIA_VOL_DOWN);
          slide.fancyPosition -= 2; // change percentage
        }
        else if (mode == 1) Keyboard.write(KEY_MINUS);
        else if (mode == 2) Keyboard.write(KEY_DOWN);

        if (doDebug) log(45); // log '-'
      }
    }
    if (slide.fancyPosition > 100) { // if percentage outside bounds, remake the value
      slide.fancyPosition = 2 * round( (float) slide.position[0] * 0.09765625 * 0.5 );
    }

    dispSlide(slide.position[0], slide.fancyPosition, mode); // display slider on screen

    slide.position[2] = slide.position[0]; // set previous position
    slide.value[1] = slide.value[0];       // set previous value
  }
}
void readRotate() { // encoder reading, interrupt-called function
  uint32_t t = millis();
  if (t - timings.debounce[3] > turningDebounce) { // 13ms debounce on input, 76hz max speed
    if (digitalRead(dirPin) == digitalRead(rotPin)) { // rotation to the right
      if (lastDir || !lastDir && t - timings.debounce[3] > changeDebounce) { // if direction change, more delay is needed
        rotation++;
        if      (mode == 0) Keyboard.write(KEY_F23);
        else if (mode == 1) Keyboard.write(KEY_RIGHT);
        else if (mode == 2) {
          Keyboard.press(KEY_LEFT_CTRL);
          Keyboard.press(KEY_Y);
          delay(17);
          Keyboard.releaseAll();
        }
        lastDir = true;
        if (doDebug) log(44); // log '>'
      }
    }
    else { // rotation to the left
      if (!lastDir || lastDir && t - timings.debounce[3] > changeDebounce) { // if direction change, more delay is needed
        rotation--;
        if      (mode == 0) Keyboard.write(KEY_F22);
        else if (mode == 1) Keyboard.write(KEY_LEFT);
        else if (mode == 2) {
          Keyboard.press(KEY_LEFT_CTRL);
          Keyboard.press(KEY_Z);
          delay(17);
          Keyboard.releaseAll();
        }
        if (doDebug) log(46); // log '<'
        lastDir = false;
      }
    }

    updateRot = true; // because this can (and will) interrupt other lcd writes, it instead sets a flag and
                      // will be updated when the cycle next ends
    
    timings.debounce[3] = t;
  } 
}

// things related to debug mode
void showDebug() { // if doDebug is one, mode -1 can be selected, which will run this each loop
  timings.debug = millis();

  if (!hideLed) leds[0] = CHSV(timings.debug / 10 , rotation, (slide.position[0] * 7 / 32) + 32);
  else          leds[0] = CHSV(0, 0, 0);
  FastLED.show();

  if(!hideDisp) {
  oled.setFont(Verdana12);
  oled.setCursor(0,0);
  oled.print("Debug");

  oled.setFont(Adafruit5x7);

  oled.setCursor(61,0);
  oled.clearToEOL();
  if (slide.reading[0] < 1000) oled.print(" ");
  if (slide.reading[0] < 100)  oled.print(" ");
  if (slide.reading[0] < 10)   oled.print(" ");
  oled.print(slide.reading[0]);
  oled.print(" / ");
  if (slide.position[0] < 1000) oled.print(" ");
  if (slide.position[0] < 100)  oled.print(" ");
  if (slide.position[0] < 10)   oled.print(" ");
  oled.print(slide.position[0]);

  oled.setCursor(103,1);
  if (abs(rotation) < 10)  oled.print(" ");
  if (abs(rotation) < 100) oled.print(" ");
  if (rotation > 0)        oled.print("+");
  if (rotation == 0)       oled.print(" ");
  oled.print(rotation);

  if (stdev.doStdev) {
    addToStdev(stdev, slide.position[0]);
    calcStdev(stdev);
    oled.setCursor(79,2);
    oled.print("sd: ");
    oled.print(stdev.stdev / 100);
    oled.print(".");
    if (stdev.stdev % 100 < 10) oled.print("0");
    oled.print(stdev.stdev % 100);
    oled.print(" ");
  }
  
  oled.setCursor(61, 3);
  if ((1 + timings.debug - timings.start) < 10)   oled.print(" ");
  oled.print((1 + timings.debug - timings.start)); // time for entire cycle
  oled.print("ms");

  oled.print(" (");
  oled.print(millis() - timings.debug);
  oled.print("ms) ");

  // oled.setFont(Adafruit5x7);
  // oled.setCursor(0,6);
  // oled.print("outputs: ");
  // oled.setFont(arrows);
  // oled.println(last); // no longer used
  }
}
void addToStdev(stdevS &data, int value) { // adds an integer value to the summary statistics
  if (data.count >= 500) {
    data.doStdev = false;
    return;
  };
  data.count ++;
  data.sum += value;
  data.sumSq += pow(value, 2); // sumSq is a long so this shouldn't pose too many issues unless 
                          // count > 2000, in which case an online algorithm may be needed
                          // for efficiency, but 2k samples should be more than enough for 
                          // anything i can think of where stdev could be used
}
void resetStdev (stdevS &data) { // resets the summary statistics
  data.count = 0;
  data.sum = 0;
  data.sumSq = 0;
  data.doStdev = true;
}
void calcStdev(stdevS &data) { // updates calculation for standard deviation
  if (data.count > 500) return;

  double mean       = (double) data.sum   / (data.count); // calc summary statistics
  double meansq     = (double) data.sumSq / (data.count); // calc summary statistics

  double variance   = (double) meansq - (pow(mean, 2));        // var = mean of sqares - square of means
  double u_variance = (double) variance * data.count / (data.count - 1); // correction for sample var vs real var
  double u_stdev    = (double) sqrt(u_variance);

  data.stdev = round(u_stdev * 100);
}
/* void log(int action) {
  memmove(&last[1], &last[0], (11) * sizeof(char));
  last[0] = action;
} */ // no longer in use

// things involving the display
void fullRefresh(int mode, int value, int position, int rotation) {
  oled.clear();
  dispMode(mode);
  dispSlide(value, position, mode);
  dispEncod(rotation);
}
void dispMode(int mode) {
  dispInd(mode);
  if (mode == -1) {
    oled.clear();
    return;
  }

  if (!hideLed) leds[0] = CHSV(90*mode+10, 255, 64);
  else          leds[0] = CHSV(0, 0, 0);
  FastLED.show();

  oled.setFont(Adafruit5x7);

  // slider
  oled.setCursor(0, 1);
  oled.clearToEOL();
  if      (mode == 0) oled.print("Volume:");
  else if (mode == 1) oled.print("Zoom");
  else if (mode == 2) oled.print("Up / Down");

  // encoder
  oled.setCursor(10, 2);
  oled.clearToEOL();
  if      (mode == 0) oled.print("Media Volume");
  else if (mode == 1) oled.print("Rotation");
  else if (mode == 2) oled.print("Undo / Redo");

  // button 2
  oled.setCursor(10,3);
  oled.clearToEOL();
  if      (mode == 0) oled.print("Input");
  else if (mode == 1) oled.print("Reset");
  else if (mode == 2) oled.print("Replace");

  // button 1
  oled.setCursor(74,3);
  if      (mode == 0) oled.print("Output");
  else if (mode == 1) oled.print("Flip");
  else if (mode == 2) oled.print("Upload");

  oled.setFont(circle);
  oled.setCursor(0,3);
  oled.print(9);
  oled.setCursor(64,3);
  oled.print(9);
}
void dispSlide(int value, int position, int mode) {
  if (mode == -1) return;

  oled.home();
  oled.setFont(lineboye); // 0 - 8 maps to line bits
  oled.setLetterSpacing(0);

  for (int i = 0; i < value / 64; i++)      oled.print("0");
  oled.print( (value / 8) % 8 + 1 );
  for (int i = value / 64 + 1; i < 16; i++) oled.print("0");

  if (mode == 0) {
    oled.setFont(Adafruit5x7);
    oled.setCursor(50, 1);
    oled.print(position);
    oled.print("%   ");
  }
}
void dispEncod(int value) {
  if (mode == -1) return;

  oled.setCursor(0, 2);
  oled.setFont(circle); // 0 - 7 maps to circle with bits removed
  oled.print(value % 8 >= 0 ? value % 8 : value % 8 + 8);
}
void dispInd(int mode) {
  if (mode == -1 && !hideLed) {
    RXLED0;
    TXLED0;
  } else {
    RXLED1;
    TXLED1;
  }
}
void doUpdates(int mode, bool &updateRot, int &countdown0, int &countdown1) {
  if (updateRot && mode != -1) {
    dispEncod(rotation);
    updateRot = false;
  }

  if (countdown0 == 0 && mode != -1) {
    oled.setFont(circle);
    oled.setCursor(64,3);
    oled.print(9);
  }

  else if (countdown0 > 0 && mode != -1) {
    oled.setFont(circle);
    oled.setCursor(64,3);
    oled.print(8);
    if (digitalRead(button1)) countdown0--;
  }

  if (countdown1 == 0 && mode != -1) {
    oled.setFont(circle);
    oled.setCursor(0,3);
    oled.print(9);
  }

  else if (countdown1 > 0 && mode != -1) {
    oled.setFont(circle);
    oled.setCursor(0,3);
    oled.print(8);
    if (digitalRead(button2)) countdown1--;
  }
}

// things involving improving inputs
int calib(int input) {
  if (input < 64) {
    return input*4;
  }
  else if (input < 959) {
    return round((float) (input * 0.607) + 217.2);
  }
  else {
    return round((float) (input * 3.54) - 2598.4);
  }
}
int smooth(int values[4]) {
  if (abs(1024 - values[0] - values[1]) < 200) {
    return (2 * values[0] + values[1] + values[2] + values[3]) / 5; 
  }
  else {
    return (values[0] + values[1] + values[2] + values[3]) / 4;
  }
} // create smoothed (weighted) value based off of last 4 inputs
