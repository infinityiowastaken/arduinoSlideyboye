# 1 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino"
# 1 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino"
# 2 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino" 2
# 3 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino" 2

# 5 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino" 2

// #include "FastLED.h"      // led lib
# 28 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino"
uint32_t timings[6];
// timings[] array stores timings information
// 0  set at start of cycle
// 1  set after slider is read
// 2  set after interrupt is read
// 3  set when button0 succeeds
// 4  set when button1 succeeds
// 5  set when button2 succeeds
// 6  set at start of debug print

// char last[12]; // no longer in use
// last[] array stores last operations performed:
// 0  most recent value
// 15 least recent value

int delta, rotation, fillBut, fillBut2, mode, unsmoothed;

int count, stdev;
long sum, sumSq;

int slide[10];
// slide[] array stores data relating to the slider:
// 0     current position, calibrated and smoothed
// 1     current value
// 2     last position, calibrated and smoothed
// 3     last value
// 4 - 7 last 4 readings, calibrated but not smoothed
// 8     fancy position (percentage)
// 9     unsmoothed, uncalibrated position

bool updateRot, lastDir;

SSD1306AsciiAvrI2c oled;

void setup() {
  // Serial.begin(9600);              // init serial at 9600bps, no longer used

  Keyboard.begin(); // init keyboard

  oled.begin(&Adafruit128x32, 0x3C); // init display
  oled.displayRemap ( true ); // flip display 180deg
  oled.invertDisplay( 0 ); // invert if defined

  pinMode(1 /* tx pin, blocks serial out unfortunately*/, 0x2); // init encoder pins
  pinMode(7, 0x2);
  attachInterrupt(3 /* pin 1 is interrupt pin 3 because reasons*/, rotate, 0x0);

  pinMode(6, 0x2); // init button pins
  pinMode(5, 0x2);
  pinMode(4, 0x2);

  slide[4] = analogRead(A0); // init slider readings
  slide[8] = 2 * (((float) slide[4] * 0.09765625 * 0.5)>=0?(long)(((float) slide[4] * 0.09765625 * 0.5)+0.5):(long)(((float) slide[4] * 0.09765625 * 0.5)-0.5));

  fullRefresh(0, slide[1], slide[6], 0); // default values for display
}

void loop() {
  timings[0] = millis();


  // button readings
  // button 0 is on the encoder, button 1 is to the left etc
  if (digitalRead(6) == 0x0 && millis() - timings[3] > 300) {
    timings[3] = millis(); // ratelimit/debounce for mode change

    if (mode < 2) mode ++;
    else mode = -1 * 1 /* shows timings (and last instructions)*/;

    if (mode == 0) fullRefresh(mode, slide[0], slide[8], rotation); // fix for minor debug issue
    else dispMode(mode);
  } // button 0 changes the mode
  if (digitalRead(5) == 0x0 && millis() - timings[4] > 300) {
    timings[4] = millis(); // ratelimit/debounce for mode change

    if (mode == 0) Keyboard.write(KEY_F24);
    else if (mode == 1) Keyboard.write(KEY_DOWN);
    else if (mode == 2) {
      Keyboard.press(KEY_LEFT_CTRL);
      Keyboard.press(KEY_LEFT_ALT);
      Keyboard.press(KEY_U);
      delay(17);
      Keyboard.releaseAll();
    }
    else if (mode == -1) resetStdev();

    log(47); // log o
    fillBut = 5; // on screen button animation
  } // button 1 performs an operation based upon the mode
  if (digitalRead(4) == 0x0 && millis() - timings[5] > 300) {
    timings[5] = millis(); // ratelimit/debounce for mode change

    if (mode == 0) Keyboard.write(KEY_MUTE);
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
    } //

    log(48);
    fillBut2 = 5;
  } // button 2 performs an operation based upon the mode


  // slider readings
  for (int i = 3; i > 0; i--) slide[4+i] = slide[3+i]; // move last 3 values back in memory
  slide[4] = analogRead(A0); // update most recent value

  slide[9] = (2 * slide[4] + slide[5] + slide[6] + slide[7]) / 5; // create smoothed (weighted) value based off of last 4 inputs
  slide[0] = calib(slide[9]); // apply correction to smoothed value
  slide[1] = ((((float) 512 - slide[0]) / 20 /* gives 51.2 steps across 0-1023 range*/)>=0?(long)((((float) 512 - slide[0]) / 20 /* gives 51.2 steps across 0-1023 range*/)+0.5):(long)((((float) 512 - slide[0]) / 20 /* gives 51.2 steps across 0-1023 range*/)-0.5)); // turn smoothed value into one of 51 positions

  delta = slide[2] - slide[1]; // work out change in positions over last cycle

  if (((slide[3] - slide[0])>0?(slide[3] - slide[0]):-(slide[3] - slide[0])) < 20 /* gives 51.2 steps across 0-1023 range*/ + 5) delta = 0; // slight hysteresis to reduce jitter - only allows changes 
                                                            // if last two values have significant change between them

  if (delta != 0) {
    for (int i = 0; i < ((delta)>0?(delta):-(delta)); i++) {
      if (delta > 0) {
        if (mode == 0) {
          Consumer.write(MEDIA_VOL_UP);
          slide[8] += 2; // change percentage for volume
        }
        else if (mode == 1) Keyboard.write(KEY_EQUAL);
        else if (mode == 2) Keyboard.write(KEY_UP);
        if (1 /* shows timings (and last instructions)*/) log(43); // log '+'
      } else {
        if (mode == 0) {
          Consumer.write(MEDIA_VOL_DOWN);
          slide[8] -= 2; // change percentage
        }
        else if (mode == 1) Keyboard.write(KEY_MINUS);
        else if (mode == 2) Keyboard.write(KEY_DOWN);

        if (1 /* shows timings (and last instructions)*/) log(45); // log '-'
      }
    }
    if (slide[8] < 0 || slide[8] > 100) { // if percentage outside bounds, remake the value
      slide[8] = 2 * (((float) slide[0] * 0.09765625 * 0.5)>=0?(long)(((float) slide[0] * 0.09765625 * 0.5)+0.5):(long)(((float) slide[0] * 0.09765625 * 0.5)-0.5));
    }

    dispSlide(slide[0], slide[8], mode); // display slider on screen

    slide[2] = slide[1]; // set previous position
    slide[3] = slide[0]; // set previous value
  }

  timings[1] = millis();


  if (mode == 0) {
    
# 189 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino" 3
   (*(volatile uint8_t *)((0x05) + 0x20)) 
# 189 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino"
   |= (1<<0);
    
# 190 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino" 3
   (*(volatile uint8_t *)((0x0B) + 0x20)) 
# 190 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino"
   |= (1<<5);
  } else if (mode == 1) {
    
# 192 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino" 3
   (*(volatile uint8_t *)((0x05) + 0x20)) 
# 192 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino"
   |= (1<<0);
    
# 193 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino" 3
   (*(volatile uint8_t *)((0x0B) + 0x20)) 
# 193 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino"
   &= ~(1<<5);
  } else if (mode == 2) {
    
# 195 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino" 3
   (*(volatile uint8_t *)((0x05) + 0x20)) 
# 195 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino"
   &= ~(1<<0);
    
# 196 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino" 3
   (*(volatile uint8_t *)((0x0B) + 0x20)) 
# 196 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino"
   |= (1<<5);
  } else if (mode == -1) {
    
# 198 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino" 3
   (*(volatile uint8_t *)((0x05) + 0x20)) 
# 198 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino"
   &= ~(1<<0);
    
# 199 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino" 3
   (*(volatile uint8_t *)((0x0B) + 0x20)) 
# 199 "c:\\Users\\nmail\\Documents\\Arduino\\slideyboye\\slideyboye.ino"
   &= ~(1<<5);
  }

  if (updateRot && mode != -1) {
    dispEncod(rotation);
    updateRot = false;
  }

  if (fillBut == 0 && mode != -1) {
    oled.setFont(circle);
    oled.setCursor(64,3);
    oled.print(9);
  }
  else if (fillBut > 0 && mode != -1) {
    oled.setFont(circle);
    oled.setCursor(64,3);
    oled.print(8);
    if (digitalRead(5)) fillBut--;
  }

  if (fillBut2 == 0 && mode != -1) {
    oled.setFont(circle);
    oled.setCursor(0,3);
    oled.print(9);
  }
  else if (fillBut2 > 0 && mode != -1) {
    oled.setFont(circle);
    oled.setCursor(0,3);
    oled.print(8);
    if (digitalRead(4)) fillBut2--;
  }

  if (mode == -1) { // debug
    timings[5] = millis();
    addToStdev(slide[0]);

    oled.setFont(Verdana12);
    oled.setCursor(0,0);
    oled.print("Debug");

    oled.setFont(Adafruit5x7);

    oled.setCursor(61,0);
    if (slide[9] < 10) oled.print(" ");
    if (slide[9] < 100) oled.print(" ");
    if (slide[9] < 1000) oled.print(" ");
    oled.print(slide[9]);
    oled.print(" / ");
    if (slide[0] < 10) oled.print(" ");
    if (slide[0] < 100) oled.print(" ");
    if (slide[0] < 1000) oled.print(" ");
    oled.print(slide[0]);

    oled.setCursor(103,1);
    if (rotation >= 0) oled.print(" ");
    if (((rotation)>0?(rotation):-(rotation)) < 10) oled.print(" ");
    if (((rotation)>0?(rotation):-(rotation)) < 100) oled.print(" ");
    oled.print(rotation);

    oled.setCursor(79,2);
    oled.print("sd: ");
    oled.print(calcStdev()/100);
    oled.print(".");
    if (calcStdev()%100 < 10) oled.print("0");
    oled.print(calcStdev()%100);

    oled.setCursor(67,3);
    if ((1 + millis() - timings[0]) < 10) oled.print(" ");
    oled.print((1 + millis() - timings[0])); // time for entire cycle
    oled.print("ms");

    oled.print(" (");
    oled.print(1 + timings[5] - timings[0]);
    oled.print("ms)");

    // oled.setFont(Adafruit5x7);
    // oled.setCursor(0,6);
    // oled.print("outputs: ");
    // oled.setFont(arrows);
    // oled.println(last);
  }


  delay(1); // allows uploads to happen without resets
}

void rotate() { // encoder reading
  uint32_t t = millis();
  if (t - timings[2] > 13) { // 13ms debounce on input, 76hz max speed
    if (digitalRead(7) == digitalRead(1 /* tx pin, blocks serial out unfortunately*/)) {
      if (lastDir || !lastDir && t - timings[2] > 125) { // if direction change, more delay is needed
        rotation++;
        if (mode == 0) Keyboard.write(KEY_F23);
        else if (mode == 1) Keyboard.write(KEY_RIGHT);
        else if (mode == 2) {
          Keyboard.press(KEY_LEFT_CTRL);
          Keyboard.press(KEY_Y);
          delay(17);
          Keyboard.releaseAll();
        }
        lastDir = true;
        if (1 /* shows timings (and last instructions)*/) log(44); // log '>'
      }
    }
    else {
      if (!lastDir || lastDir && t - timings[2] > 125) { // if direction change, more delay is needed
        rotation--;
        if (mode == 0) Keyboard.write(KEY_F22);
        else if (mode == 1) Keyboard.write(KEY_LEFT);
        else if (mode == 2) {
          Keyboard.press(KEY_LEFT_CTRL);
          Keyboard.press(KEY_Z);
          delay(17);
          Keyboard.releaseAll();
        }
        if (1 /* shows timings (and last instructions)*/) log(46); // log '<'
        lastDir = false;
      }
    }

    updateRot = true; // because this can (and will) interrupt other lcd writes, it instead sets a flag and
                      // will be updated when the cycle next ends

    timings[2] = t;
  }
}

// void log(int action) {
//   memmove(&last[1], &last[0], (11) * sizeof(char));
//   last[0] = action;
// } // no longer in use

void fullRefresh(int mode, int value, int position, int rotation) {
  oled.clear();
  dispMode(mode);
  dispSlide(value, position, mode);
  dispEncod(rotation);
}

void dispMode(int mode) {
  if (mode == -1) {
    oled.clear();
    return;
  }

  oled.setFont(Adafruit5x7);

  // slider
  oled.setCursor(0, 1);
  oled.clearToEOL();
  if (mode == 0) oled.print("Volume:");
  else if (mode == 1) oled.print("Zoom");
  else if (mode == 2) oled.print("Up / Down");

  // encoder
  oled.setCursor(10, 2);
  oled.clearToEOL();
  if (mode == 0) oled.print("Media Volume");
  else if (mode == 1) oled.print("Rotation");
  else if (mode == 2) oled.print("Undo / Redo");

  // button 2
  oled.setCursor(10,3);
  oled.clearToEOL();
  if (mode == 0) oled.print("Mute");
  else if (mode == 1) oled.print("Reset");
  else if (mode == 2) oled.print("Replace");

  // button 1
  oled.setCursor(74,3);
  if (mode == 0) oled.print("Output");
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

  for (int i = 0; i < value / 64; i++) oled.print("0");
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

int calib(int input) {
  if (input < 64) {
    return input*4;
  }
  else if (input < 959) {
    return (((float) (input * 0.607) + 217.2)>=0?(long)(((float) (input * 0.607) + 217.2)+0.5):(long)(((float) (input * 0.607) + 217.2)-0.5));
  }
  else {
    return (((float) (input * 3.54) - 2598.4)>=0?(long)(((float) (input * 3.54) - 2598.4)+0.5):(long)(((float) (input * 3.54) - 2598.4)-0.5));
  }
}

void addToStdev(int value) {
  if (count > 500) return;
  count ++;
  sum += value;
  sumSq += pow(value, 2);
}
void resetStdev () {
  count = 0;
  sum = 0;
  sumSq = 0;
}
int calcStdev() {
  if (count > 500) return stdev;
  double mean = (double) sum / (count);
  double meansq = (double) sumSq / (count);

  double variance = (double) meansq - (pow(mean, 2));
  double u_variance = (double) variance * count / (count - 1);
  double u_stdev = (double) sqrt(u_variance);
  stdev = ((u_stdev * 100)>=0?(long)((u_stdev * 100)+0.5):(long)((u_stdev * 100)-0.5));

  return stdev;
}
