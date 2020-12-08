#ifndef _NEOPIXEL_EFFECTS
#define _NEOPIXEL_EFFECTS

// Struct to hold all data about an effect. Each effect routine will pull parameters from a pased reference
// to the structure. One filed effect_mem is a pointer to which malloced memory can be attached if larger arrays
// are needed.

typedef struct effectData {
  bool slotActive;           // Does this slot in the queue have an active effect in it?
  bool isOverlay;            // Does this effect expect to appear infront of other effects?
  bool (*effectPtr)(effectData&); // used by management to remember which effect owns this data
  unsigned int firstPixel;   // first pixel involved in effect
  unsigned int lastPixel;    // last pixel involved in effect
  unsigned int effectDelay;  // Allow effect to have 2 delay times to compare against
  byte         r;            // What color effect should use if selectable
  byte         g;            // What color effect should use if selectable
  byte         b;            // What color effect should use if selectable
  byte         w;            // What color effect should use if selectable
  int          effectVar[5]; // 5 integers for effect to play with before having to allocate memory
  void *       effectMemory; // receiver of pointer from memory allocation
  int          intParam[4];  // defined per effect
  bool         applyBrightness;
  unsigned int effectState;  // State 0 is alway init, allocate memory, set defaults
  // State 1 is always end, release memory if allocated
  // States 2+ defined per effect
  char *effectName;
} effectData;

// Mainroom lights are pixels 0-421 and 612-1379
// Kitchen lights are pixels 422-1199
// Bedroom lights are pixels 1380-2323
// Need to virtualy remap main room to one continous range
//   Lengths of segments of the LED strip of interest (Mr is main room, Kt is Kitchen, Br is Bedroom
#define lengthMr1 422
#define lengthMr2 768
#define lengthKt 590
#define lengthBr 543
#define lengthMr (lengthMr1 + lengthMr2)
// lengthMR1 + lengthMR2 + lengthKt + lengthBr must equal ledCount (2323)
// Calculate location of segments in real strip
#define startMr1 0
#define endMr1   (startMr1 + lengthMr1 - 1)
#define startKt  (endMr1 + 1)
#define endKt    (startKt + lengthKt - 1)
#define startMr2 (endKt + 1)
#define endMr2   (startMr2 + lengthMr2 - 1)
#define startBr  (endMr2 + 1)
#define endBr    ledCount

// Calculate remapped locations of rooms (so virtual strip? with contignous Mr)
#define vStartBr startBr
#define vEndBr   endBr
#define vStartMr 0
#define vEndMr   (vStartMr + lengthMr - 1)
#define vStartKt (vEndMr + 1)
#define vEndKt   (vStartKt + lengthKt - 1)

// vStartMr = 0
// vEndMr   = 1189
// vStartKt = 1190
// vEndKt   = 1779
// vStartBr = 1780
// vEndBr   = 2322

// see if one pixel is inside the range of a room
bool insideroom (unsigned int check, unsigned int first, unsigned int last) {
  return ((first <= check) && (check <= last));
}

void mapPixel (unsigned int virtualPixel, unsigned int &stripNumber, unsigned int &actualPixel) {
  // Unwrap 2nd level of virtual that joins 2 segements of main room into one and slides kitched to after that segment
  stripNumber = 0;
  actualPixel = 0;
  if (insideroom(virtualPixel, vStartMr, vEndMr)) {
    if (insideroom(virtualPixel, endMr1 + 1, vEndMr)) { // in Mr2 so remap to actual pixel
      virtualPixel = virtualPixel - lengthMr1 + startMr2; // offset into second part of virtual mr applied to actual mr2
    }
  } else if (insideroom(virtualPixel, vStartKt, vEndKt)) {
    virtualPixel = virtualPixel - vStartKt + startKt; // offset into virutal kitchen applied to real kitchen range
  }

  // Now need to unmap 1 large virtual strip into individual real strips
  // Find the correct strip to work with out of virtual all led strip
  for (int i = 0; i < NUMSTRIPS; i++) {
    if ((virtualPixel >= stripStart[i]) && (virtualPixel <= stripEnd[i])) {
      stripNumber = i;
      break; // found strip so don't check rest
    }
  }

  // Find the correct LED in the selected strip
  if (stripReversed[stripNumber]) { // is strip reversed?
    actualPixel = stripEnd[stripNumber] - virtualPixel; // offset from end
  } else {
    actualPixel = virtualPixel - stripStart[stripNumber]; // offset from start
  }
  actualPixel = constrain (actualPixel, 0, LED_COUNT_MAXIMUM);
}

///**************************** START EFFECTS *****************************************/
// Effects from: https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/

// This function's need will go away as effects are transformed into state machines rather then loops.
//bool shouldAbortEffect() {
//  yield(); // Watchdog timer
//  client.loop(); // Update from MQTT
//  return transitionAbort;
//}

void showDirty() { // Send updated data to all led strips that need updates
  for (int i = 0; i < NUMSTRIPS; i++) {
    if (stripDirty[i]) {
      pixelStrings[i].show();
      stripDirty[i] = false;
    }
  }
}

void showStrip() {
  showDirty();
}

/*!
   @brief   Set virtual pixel to given color.
   @param   pixel which pixel across entire virtual strip to set (uint)
   @param   r red colors value (byte)
   @param   g green colors value (byte)
   @param   b blue colors value (byte)
   @param   w white colors value (byte)
   @param   applyBrightness (bool) should global brightnes effect pixel
   @return  void
*/
void setPixel(unsigned int pixel, byte r, byte g, byte b, byte w, bool applyBrightness) {
  if (applyBrightness) {
    r = map(r, 0, 255, 0, brightness);
    g = map(g, 0, 255, 0, brightness);
    b = map(b, 0, 255, 0, brightness);
    w = map(w, 0, 255, 0, brightness);
  }

  unsigned int stripNumber = 0;
  unsigned int ledNumber = 0;

  // translate from virutal pixel number to physical strip information
  mapPixel(pixel, stripNumber, ledNumber);
  /*
    if (insideroom(pixel, vStartMr, vEndMr)) {
    if (insideroom(pixel, endMr1+1, vEndMr)) { // in Mr2 so remap to actual pixel
      pixel = pixel - lengthMr1 + startMr2; // offset into second part of virtual mr applied to actual mr2
    }
    } else if (insideroom(pixel,vStartKt, vEndKt)) {
    pixel = pixel - vStartKt + startKt; // offset into virutal kitchen applied to real kitchen range
    }

    // Find the correct strip to work with out of virtual all led strip
    for (int i = 0; i < NUMSTRIPS; i++) {
    if ((pixel >= stripStart[i]) && (pixel <= stripEnd[i])) {
      stripNumber = i;
      break; // found strip so don't check rest
    }
    }

    // Find the correct LED in the selected strip
    if (stripReversed[stripNumber]) { // is strip reversed?
    ledNumber = stripEnd[stripNumber] - pixel; // offset from end
    } else {
    ledNumber = pixel - stripStart[stripNumber]; // offset from start
    }
    ledNumber = constrain (ledNumber, 0, LED_COUNT_MAXIMUM);
  */
  // Note the strip is dirty because we set a value in it and updating is needed
  stripDirty[stripNumber] = true;

  // Actually set the pixel in the strip
  pixelStrings[stripNumber].setPixelColor(ledNumber, r, g, b, w); // faster to not build packed color word
}

void getPixelColor(unsigned int pixel, byte &red, byte &green, byte &blue, byte &white) {
  unsigned int stripNumber = 0;
  unsigned int ledNumber = 0;
  unsigned int currentColor;

  mapPixel (pixel, stripNumber, ledNumber);
  currentColor = pixelStrings[stripNumber].getPixelColor(ledNumber);
  white = (currentColor >> 24) & 0xFF;
  red   = (currentColor >> 16) & 0xFF;
  green = (currentColor >> 8) & 0xFF;
  blue  = (currentColor) & 0xFF;
}

// fade given pixel by ratio of fade value. Return true if pixel became black else return false;
bool fadeToBlack (unsigned int pixel, byte fadeValue) {
  unsigned int stripNumber = 0;
  unsigned int ledNumber = 0;
  unsigned int currentColor;
  int red, green, blue, white;

  // Get current color
  mapPixel (pixel, stripNumber, ledNumber);
  currentColor = pixelStrings[stripNumber].getPixelColor(ledNumber);
  if (currentColor != 0) {
    white = (currentColor >> 24) & 0xFF;
    red   = (currentColor >> 16) & 0xFF;
    green = (currentColor >> 8)  & 0xFF;
    blue  = (currentColor)       & 0xFF;
    // Do fade
    red =   (red <= fadeValue)   ? 0 : (int) red   - (red   * fadeValue / 256 + 1);
    green = (green <= fadeValue) ? 0 : (int) green - (green * fadeValue / 256 + 1);
    blue =  (blue <= fadeValue)  ? 0 : (int) blue  - (blue  * fadeValue / 256 + 1);
    white = (white <= fadeValue) ? 0 : (int) white - (white * fadeValue / 256 + 1);
    pixelStrings[stripNumber].setPixelColor(ledNumber, red, green, blue, white); // faster to not build packed color word
    // Note the strip is dirty because we set a value in it and updating is needed
    stripDirty[stripNumber] = true;
    return ((red == 0) && (blue == 0) && (green == 0) && (white == 0));
  } else {
    return true;
  }

}

void correctPixel (unsigned int pixel, byte r, byte g, byte b, byte w, bool applyBrightness) {
  unsigned int currentColor;
  byte currentRed, currentBlue, currentGreen, currentWhite;
  if (applyBrightness) {
    r = map(r, 0, 255, 0, brightness);
    g = map(g, 0, 255, 0, brightness);
    b = map(b, 0, 255, 0, brightness);
    w = map(w, 0, 255, 0, brightness);
  }

  unsigned int stripNumber = 0;
  unsigned int ledNumber = 0;

  mapPixel(pixel, stripNumber, ledNumber);

  /*
    if (insideroom(pixel, vStartMr, vEndMr)) {
    if (insideroom(pixel, endMr1+1, vEndMr)) { // in Mr2 so remap to actual pixel
      pixel = pixel - lengthMr1 + startMr2; // offset into second part of virtual mr applied to actual mr2
    }
    } else if (insideroom(pixel,vStartKt, vEndKt)) {
    pixel = pixel - vStartKt + startKt; // offset into virutal kitchen applied to real kitchen range
    }

    // Find the correct strip to work with out of virtual all led strip
    for (int i = 0; i < NUMSTRIPS; i++) {
    if ((pixel >= stripStart[i]) && (pixel <= stripEnd[i])) {
      stripNumber = i;
      break; // found strip so don't check rest
    }
    }

    // Find the correct LED in the selected strip
    if (stripReversed[stripNumber]) { // is strip reversed?
    ledNumber = stripEnd[stripNumber] - pixel; // offset from end
    } else {
    ledNumber = pixel - stripStart[stripNumber]; // offset from start
    }
    ledNumber = constrain (ledNumber, 0, LED_COUNT_MAXIMUM);
  */

  currentColor = pixelStrings[stripNumber].getPixelColor(ledNumber);
  currentWhite = (currentColor >> 24) & 0xFF;
  currentRed   = (currentColor >> 16) & 0xFF;
  currentGreen = (currentColor >> 8) & 0xFF;
  currentBlue  = (currentColor) & 0xFF;
  if ((currentWhite != w) || (currentRed != r) || (currentGreen != g) || (currentBlue != b)) {
    // Note the strip is dirty because we set a value in it and updating is needed
    stripDirty[stripNumber] = true;

    // Actually set the pixel in the strip
    pixelStrings[stripNumber].setPixelColor(ledNumber, r, g, b, w); // faster to not build packed color word
  }
}

void setAll(byte r, byte g, byte b, byte w, bool refreshStrip = true) {
  for (int j = 0; j < LED_COUNT_MAXIMUM; j++) {
    setPixel(j, r, g, b, w, false);
  }

  if (refreshStrip) {
    showDirty();
  }
}

void FillPixels(unsigned int firstPixel, unsigned int lastPixel, byte r, byte g, byte b, byte w, bool refreshStrip = true) {
  int stepDir = (lastPixel > firstPixel) ? 1 : -1;
  for (unsigned int j = firstPixel; j <= lastPixel; j += stepDir) {
    setPixel(j, r, g, b, w, false);
  }
  if (refreshStrip) {
    showDirty();
  }
}
void correctPixels(unsigned int firstPixel, unsigned int lastPixel, byte r, byte g, byte b, byte w) {
  int stepDir = (lastPixel > firstPixel) ? 1 : -1;
  for (unsigned int j = firstPixel; j <= lastPixel; j += stepDir) {
    correctPixel(j, r, g, b, w, false);
  }
}

/*
  // Example effect. Should return true if effect has finished, false if effect wants more itterations
  bool SampleEffect (effectData &myData) {
  // intParam[0] is delay between blinking // always use [0] for delay setting
  // intParam[1] is how many leds to blink on
  bool returnValue = false;
  byte * effectMemory = (byte *)myData.effectMemory; // cast void pointer to correct type
  switch (myData.effectState) {
    case 0: // init or startup
      myData.effectVar[0] = 1; // for example this is an itteration count
      // grab a byte of memory for each led to be modified
      myData.effectMemory = (void *)malloc((myData.lastPixel - myData.firstPixel + 1) * sizeof(byte));
      effectMemory = (byte *)myData.effectMemory; // update since we just allocated memory
      myData.effectState = 2; // display effect
      FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      if (myData.effectMemory == NULL) {
        returnValue = true; // no memory to do effect
      }
      break;

    case 2: // Display data
      if (myData.effectVar[0] < myData.intParam[1]) {
        myData.effectVar[0] = constrain(myData.effectVar[0], myData.firstPixel, myData.lastPixel);
        setPixel( myData.effectVar[0], myData.r, myData.g, myData.b, myData.w, myData.applyBrightness);
        setPixel( myData.effectVar[0], 0, 0, 0, 0, myData.applyBrightness);
        ++myData.effectVar[0];
        myData.effectState = 3; // delay
        myData.effectDelay = currentMilliSeconds + myData.intParam[0];
      } else {
        myData.effectState = 1; // Effect has finished.
      }
      break;

    case 3: // delay
      if (currentMilliSeconds >= myData.effectDelay) myData.effectState = 2;
      break;

    default: // state has been lost so end effect
    case 1: // end effect and release memory
      if (myData.effectMemory != NULL) {
        free(myData.effectMemory);
      }
      returnValue = true;
  }
  return returnValue; // true if effect is finished. False for more itterations.
  }
*/

// NoEffect effect. Should return true if effect has finished, false if effect wants more itterations
bool NoEffect (effectData &myData) {
  // Do nothing and finish right away.
  return true;  // true if effect is finished. False for more itterations.
}

// clear effect. Should return true if effect has finished, false if effect wants more itterations
bool ClearEffect (effectData &myData) {
  bool returnValue = false;
  switch (myData.effectState) {
    case 0: // init or startup;
      FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      myData.effectDelay = currentMilliSeconds + 1000;
      myData.effectState = 3; // delay
      break;

    case 2: // refresh effect;
      correctPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0);
      myData.effectDelay = currentMilliSeconds + 1000;
      myData.effectState = 3; // delay
      break;

    case 3: // delay
      if (currentMilliSeconds >= myData.effectDelay) myData.effectState = 2;
      break;

    default: // state has been lost so end effect
    case 1: // end effect and release memory
      FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      returnValue = true;
      break;
  }
  return returnValue;
}

// Solid effect. Should return true if effect has finished, false if effect wants more itterations
bool SolidEffect (effectData &myData) {
  bool returnValue = false;
  switch (myData.effectState) {
    case 0: // init or startup;
      FillPixels (myData.firstPixel, myData.lastPixel, myData.r, myData.g, myData.b, myData.w, false);
      myData.effectDelay = currentMilliSeconds + 1000;
      myData.effectState = 3; // delay
      break;

    case 2: // refresh effect;
      //      FillPixels (myData.firstPixel, myData.lastPixel, myData.r, myData.g, myData.b, myData.w, false);
      correctPixels (myData.firstPixel, myData.lastPixel, myData.r, myData.g, myData.b, myData.w);
      myData.effectDelay = currentMilliSeconds + 100;
      myData.effectState = 3; // delay
      break;

    case 3: // delay
      if (currentMilliSeconds >= myData.effectDelay) myData.effectState = 2;
      break;

    default: // state has been lost so end effect
    case 1: // end effect and release memory
      FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      returnValue = true;
      break;
  }
  return returnValue;
}

// effect. Should return true if effect has finished, false if effect wants more itterations
bool TwinkleEffect (effectData &myData) {
  // intParam[0] is delay between blinking
  // intParam[1] is how many pixels to have twinkling at the same time
  // intParam[2] is background white brightness
  bool returnValue = false;
  unsigned int i, j;
  unsigned int *effectMemory = (unsigned int *)myData.effectMemory;
  byte backgroundWhite = myData.intParam[2];
  switch (myData.effectState) {
    case 0: // init or startup
      myData.effectVar[0] = 0; // Which twinkle pixel to change next
      myData.effectVar[1] = myData.lastPixel - myData.firstPixel + 1; // length of pixels to twinkle over
      // grab some memory for each pixel to be twinkled
      myData.effectMemory = (void *)malloc(myData.intParam[1] * sizeof(unsigned int));
      effectMemory = (unsigned int *)myData.effectMemory;
      if (myData.effectMemory == NULL) {
        returnValue = true; // no memory to do effect so abort
        break;
      }
      // erase zone we are to twinkle in
      FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, backgroundWhite, false);

      for (i = 0; i < myData.intParam[1]; ++i) {
        j = random(myData.effectVar[1] - 1);
        effectMemory[i] = j; // build initial list of twinkles and light them
        setPixel (j + myData.firstPixel, myData.r, myData.g, myData.b, myData.w, myData.applyBrightness);
      }
      myData.effectDelay = currentMilliSeconds + myData.intParam[0];
      myData.effectState = 3; // delay
      break;

    case 2: // Display next twinkle
      i = myData.effectVar[0]; // grab pixel index to change;
      j = effectMemory[i]; // grab location of pixel on strip
      setPixel (j + myData.firstPixel, 0, 0, 0, 0, myData.applyBrightness); // Turn off current twinkle
      j = random(myData.effectVar[1] - 1);
      setPixel (j + myData.firstPixel, myData.r, myData.g, myData.b, myData.w, myData.applyBrightness);
      effectMemory[i] = j; // store new twinkle location
      i = (i + 1) % myData.intParam[1]; // advance to next pixel looping back to zero when needed
      myData.effectDelay = currentMilliSeconds + myData.intParam[0];
      myData.effectState = 3; // delay
      break;

    case 3: // delay
      if (currentMilliSeconds >= myData.effectDelay) myData.effectState = 2;
      break;

    default: // state has been lost so end effect
    case 1: // end effect and release memory
      FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      if (myData.effectMemory != NULL) {
        for (i = 0; i < myData.intParam[1]; ++i) {
          j = effectMemory[i]; // grab location of pixel on strip
          setPixel (j + myData.firstPixel, 0, 0, 0, 0, myData.applyBrightness); // Turn off current twinkle
        }
        free(myData.effectMemory);
      }
      returnValue = true;
  }
  return returnValue; // true if effect is finished. False for more itterations.
}

// effect. Should return true if effect has finished, false if effect wants more itterations
bool TwinkleRandomEffect (effectData &myData) {
  // intParam[0] is delay between blinking
  // intParam[1] is how many pixels to have twinkling at the same time
  // intParam[2] is background white brightness
  bool returnValue = false;
  unsigned int i, j;
  unsigned int *effectMemory = (unsigned int *)myData.effectMemory;
  byte backgroundWhite = myData.intParam[2];
  switch (myData.effectState) {
    case 0: // init or startup
      myData.effectVar[0] = 0; // Which twinkle pixel to change next
      myData.effectVar[1] = myData.lastPixel - myData.firstPixel + 1; // length of pixels to twinkle over
      // grab some memory for each pixel to be twinkled
      myData.effectMemory = (void *)malloc(myData.intParam[1] * sizeof(unsigned int));
      effectMemory = (unsigned int *)myData.effectMemory;
      if (myData.effectMemory == NULL) {
        returnValue = true; // no memory to do effect so abort
        break;
      }
      // erase zone we are to twinkle in
      FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, backgroundWhite, false);

      for (i = 0; i < myData.intParam[1]; ++i) {
        j = random(myData.effectVar[1] - 1);
        effectMemory[i] = j; // build initial list of twinkles and light them
        setPixel (j + myData.firstPixel, random(0, 256), random(0, 256), random(0, 256), 0, myData.applyBrightness);
      }
      myData.effectDelay = currentMilliSeconds + myData.intParam[0];
      myData.effectState = 3; // delay
      break;

    case 2: // Display next twinkle
      i = myData.effectVar[0]; // grab pixel index to change;
      j = effectMemory[i]; // grab location of pixel on strip
      setPixel (j + myData.firstPixel, 0, 0, 0, backgroundWhite, myData.applyBrightness); // Turn off current twinkle
      j = random(myData.effectVar[1] - 1);
      setPixel (j + myData.firstPixel, random(0, 256), random(0, 256), random(0, 256), 0, myData.applyBrightness);
      effectMemory[i] = j; // store new twinkle location
      i = (i + 1) % myData.intParam[1]; // advance to next pixel looping back to zero when needed
      myData.effectDelay = currentMilliSeconds + myData.intParam[0];
      myData.effectState = 3; // delay
      break;

    case 3: // delay
      if (currentMilliSeconds >= myData.effectDelay) myData.effectState = 2;
      break;

    default: // state has been lost so end effect
    case 1: // end effect and release memory
      FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      if (myData.effectMemory != NULL) {
        for (i = 0; i < myData.intParam[1]; ++i) {
          j = effectMemory[i]; // grab location of pixel on strip
          setPixel (j + myData.firstPixel, 0, 0, 0, 0, myData.applyBrightness); // Turn off current twinkle
        }
        free(myData.effectMemory);
      }
      returnValue = true;
  }
  return returnValue; // true if effect is finished. False for more itterations.
}

/*
  // Twinkle(10, 100, false);
  void Twinkle(unsigned int Count, unsigned int SpeedDelay, boolean OnlyOne) {
  if (effectStart) {
    effectState = 0; // 0=startup, 1=setNextPixel, 2=delay
  }

  switch (effectState) {
    case 0: // startup
      setAll(0, 0, 0, 0, false);
      effectMemory[0] = 0; // current pixel location / how far into "Count" state machine is
      effectMemory[1] = 0; // LED to light up.
      effectState = 1; // set next pixel
    //      break; // don't wait for next loop, fall into set next pixel

    case 1: // set next pixel
      effectMemory[1] = random(ledCount); // pick next LED to light up.
      setPixel(effectMemory[1], red, green, blue, white, false);
      showStrip();
      if (OnlyOne) {
        setPixel(effectMemory[1], 0, 0, 0, 0, false);
      }
      effectState = 2;
      effectDelayStart = currentMilliSeconds;
      break;

    case 2: // delay
      if ((currentMilliSeconds - effectDelayStart) > SpeedDelay) {
        if (++effectMemory[0] >= Count) { // finished current 0-Count so start again
          effectState = 0;
        } else {
          effectState = 1;

        }
      }
      break;

    default:
      effectState = 0; // lost current state so restart effect
      break;
  }
  }
*/
// effect. Should return true if effect has finished, false if effect wants more itterations
bool SetOnePixelEffect (effectData &myData) {
  // Just turn on the firstPixel and finish
  setPixel(myData.firstPixel, myData.r, myData.g, myData.b, myData.w, false);
  return true; // true if effect is finished. False for more itterations.
}

// effect. Should return true if effect has finished, false if effect wants more itterations
bool CylonBounceEffect (effectData &myData) {
  // intParam[0] is delay for moving eye
  // intParam[1] is center of eye pixel count
  // intParam[2] is edge of eye pixel count (1 or 2 are good)
  // intParam[3] is delay for eye to pause for at each end
  bool returnValue = false;
  unsigned int eyeLeftEdge = myData.effectVar[1];
  unsigned int eyeLeftCore = eyeLeftEdge + myData.intParam[2];
  unsigned int eyeRightCore = eyeLeftCore + myData.intParam[1];
  unsigned int eyeRightEdge = eyeRightCore + myData.intParam[2];
  switch (myData.effectState) {
    case 0: // init or startup
      // check if eye fits in pixel range to bounce in
      if ((myData.lastPixel - myData.firstPixel + 1) > (myData.intParam[2] * 2 + myData.intParam[1])) {
        myData.effectVar[0] = 1; // Direction of eye movement (-1 or 1)
        myData.effectVar[1] = myData.firstPixel; // Starting pixel of eye
        myData.effectState = 2;
        // erase zone we are to bounce in
        FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      } else {
        // flash to indicate error and then indicate finished.
        myData.effectState = 1;
        returnValue = true;
        FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 255, true);
        FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, true);
      }
      break;

    case 2: // Display eye
      // erase portion of eye that will go dark
      eyeLeftEdge = myData.effectVar[1];
      eyeLeftCore = eyeLeftEdge + myData.intParam[2];
      eyeRightCore = eyeLeftCore + myData.intParam[1];
      eyeRightEdge = eyeRightCore + myData.intParam[2];
      if (myData.effectVar[0] == 1) { // moving up
        setPixel(eyeLeftEdge, 0, 0, 0, 0, false); // turn off pixel away from which eye will move.
      } else {
        setPixel(eyeRightEdge, 0, 0, 0, 0, false); // turn off pixel away from which eye will move.
      }

      myData.effectVar[1] += myData.effectVar[0]; // move eye start to next pixel

      // Display eye in new locatipon
      eyeLeftEdge = myData.effectVar[1];
      eyeLeftCore = eyeLeftEdge + myData.intParam[2];
      eyeRightCore = eyeLeftCore + myData.intParam[1];
      eyeRightEdge = eyeRightCore + myData.intParam[2];
      FillPixels(eyeLeftEdge, eyeLeftCore - 1, myData.r / 5, myData.g / 5, myData.b / 5, myData.w / 5, false); // dim
      FillPixels(eyeLeftCore, eyeRightCore, myData.r, myData.g, myData.b, myData.w, false);  // bright
      FillPixels(eyeRightCore + 1, eyeRightEdge, myData.r / 5, myData.g / 5, myData.b / 5, myData.w / 5, false); // dim

      // Figure out how long to wait for
      if ((eyeRightEdge >= myData.lastPixel) || (eyeLeftEdge <= myData.firstPixel)) { // have we hit edge?
        myData.effectDelay = currentMilliSeconds + myData.intParam[3]; // pause at end
        myData.effectVar[0] *= -1;  // flip direction of movement
      } else {
        myData.effectDelay = currentMilliSeconds + myData.intParam[0]; // pause in middle
      }
      myData.effectState = 3; // delay
      break;

    case 3: // delay
      if (currentMilliSeconds >= myData.effectDelay) myData.effectState = 2;
      break;

    default: // state has been lost so end effect
    case 1: // end effect
      FillPixels(myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      returnValue = true;
      break;
  }
  return returnValue; // true if effect is finished. False for more itterations.
}
/*
  // CylonBounce(4, 10, 50);
  void CylonBounce(int EyeSize, int SpeedDelay, int ReturnDelay) {
  if (effectStart) {
    effectState = 0; // 0=startup, 1=display eye, 2=MoveDelay, 3=EndDelay

  }

  switch (effectState) {
    case 0: // startup
      effectMemory[0] = 1; // step increment 1 or -1 for direction to move
      effectMemory[1] = firstPixel; // eye starting position
      setAll(0, 0, 0, 0, false);
      effectState = 1; // Display eye
      break;

    case 1: // Display eye
      // Build eye in pixel memory
      setPixel(effectMemory[1], red / 5, green / 5, blue / 5, white / 5, false); // fade in
      FillPixels(effectMemory[1] + 1, effectMemory[1] + EyeSize, red, green, blue, white, false); // core bright
      setPixel(effectMemory[1] + EyeSize + 1, red / 5, green / 5, blue / 5, white / 5, false); // fade out

      // Send pixel memory to strip
      showStrip();

      // Erase eye in pixel memory, sending defered until next build of eye
      FillPixels(effectMemory[1], effectMemory[1] + EyeSize + 2, 0, 0, 0, 0, false);
      effectMemory[1] += effectMemory[0]; // Move eye to next starting location

      if (effectMemory[1] >= (lastPixel - EyeSize - 2) ) {
        // At high end of eye sweep, So reverse direction and do end delay
        effectMemory[0] = -1;
        effectState = 3; // end delay
      } else if (effectMemory[1] <= firstPixel) {
        // At low end of eye sweep, so reverse direction and do end delay
        effectMemory[0] = 1;
        effectState = 3; // end delay
      } else {
        effectState = 2; // move delay
      }
      effectDelayStart = currentMilliSeconds;
      break;

    case 2: // move delay
      if ((currentMilliSeconds - effectDelayStart) > SpeedDelay) {
        effectState = 1;
      }
      break;

    case 3: // end delay
      if ((currentMilliSeconds - effectDelayStart) > ReturnDelay) {
        effectState = 1;
      }
      break;

    default:
      effectState = 0; // lost current state so restart effect
      break;
  }
  //  for (int i = 0; i < (ledCount - EyeSize - 2); i++) {
  //    if (shouldAbortEffect()) {
  //      return;
  //    }
  //    setAll(0, 0, 0, 0, false);
  //    setPixel(i, red / 10, green / 10, blue / 10, white / 10, false);
  //    for (int j = 1; j <= EyeSize; j++) {
  //      setPixel(i + j, red, green, blue, white, false);
  //    }
  //    setPixel(i + EyeSize + 1, red / 10, green / 10, blue / 10, white / 10, false);
  //    showStrip();
  //    delay(SpeedDelay);
  //  }
  //
  //  delay(ReturnDelay);
  //
  //  for (int i = (ledCount - EyeSize - 2); i > 0; i--) {
  //    if (shouldAbortEffect()) {
  //      return;
  //    }
  //    setAll(0, 0, 0, 0, false);
  //    setPixel(i, red / 10, green / 10, blue / 10, white / 10, false);
  //    for (int j = 1; j <= EyeSize; j++) {
  //      setPixel(i + j, red, green, blue, white, false);
  //    }
  //    setPixel(i + EyeSize + 1, red / 10, green / 10, blue / 10, white / 10, false);
  //    showStrip();
  //    delay(SpeedDelay);
  //  }
  //
  //  delay(ReturnDelay);
  }
*/
// effect. Should return true if effect has finished, false if effect wants more itterations
bool FireEffect (effectData &myData) {
  // intParam[0] is delay between updates
  // intParam[1] is cooling rate
  // intParam[2] is sparking rate
  // intParam[3] is background white value
  bool returnValue = false;
  byte *heat = (byte *)myData.effectMemory;
  unsigned int cooldown;
  unsigned int i, j;
  unsigned int pixelCount = myData.lastPixel - myData.firstPixel + 1;
  switch (myData.effectState) {
    case 0: // init or startup
      // grab some memory for each led to be modified
      myData.effectMemory = (void *)malloc(pixelCount * sizeof(byte));
      heat = (byte *)myData.effectMemory; // update since memory was just allocated
      if (myData.effectMemory == NULL) {
        returnValue = true; // no memory to do effect so abort
        break;
      }
      // erase zone we are to fire in
      FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      break;

    case 2: // Display next fire
      // Step 1.  Cool down every cell a little
      for ( i = 0; i < pixelCount; i++) {
        cooldown = random(0, ((myData.intParam[1] * 10) / pixelCount) + 2);

        if (cooldown > heat[i]) {
          heat[i] = 0;
        } else {
          heat[i] -= cooldown;
        }
      }

      // Step 2.  Heat from each cell drifts 'up' and diffuses a little
      for ( i = pixelCount - 1; i >= 2; i--) {
        heat[i] = (heat[i - 1] + heat[i - 2] + heat[i - 2]) / 3;
      }

      // Step 3.  Randomly ignite new 'sparks'
      for (i = 0; i <= pixelCount % 200; i++) {
        if ( random(255) < myData.intParam[2] ) { //sparking
          j = random(7) + 200 * i; // a random start spark for every block of 200 pixels
          heat[j] += random(160, 255); // add heat to existing ember
        }
      }

      // Step 4.  Convert heat to LED colors and display
      // heat is a byte, heat * 3 will overflow twice for large values and that is desired.
      // This results in 3 linear ramps from 0 to 255.
      for ( i = 0; i < ledCount; i++) {
        // figure out which third of the spectrum we're in
        if (heat[i] > 170) {  // hottest
          setPixel(i + myData.firstPixel, 255, 255, heat[i] * 3, myData.intParam[3], true); // hottest
        } else if (heat[i] > 85) {  // middle
          setPixel(i + myData.firstPixel, 255, heat[i] * 3, 0, myData.intParam[3], true); // middle
        } else {
          setPixel(i + myData.firstPixel, heat[i] * 3, 0, 0, myData.intParam[3], true); // coolest
        }
      }
      myData.effectDelay = currentMilliSeconds + myData.intParam[0];
      myData.effectState = 3; // delay
      break;

    case 3: // delay
      if (currentMilliSeconds >= myData.effectDelay) myData.effectState = 2;
      break;

    default: // state has been lost so end effect
    case 1: // end effect and release memory
      FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      if (myData.effectMemory != NULL) {
        free(myData.effectMemory);
      }
      returnValue = true;
  }
  return returnValue; // true if effect is finished. False for more itterations.
}
/*
  void setPixelHeatColor (int Pixel, byte temperature) {
  // Scale 'heat' down from 0-255 to 0-191
  byte t192 = round((temperature / 255.0) * 191);

  // calculate ramp up from
  byte heatramp = t192 & 0x3F; // 0..63
  heatramp <<= 2; // scale up to 0..252

  // figure out which third of the spectrum we're in:
  if ( t192 > 0x80) {                    // hottest
    setPixel(Pixel, 255, 255, heatramp, 0, true);
  } else if ( t192 > 0x40 ) {            // middle
    setPixel(Pixel, 255, heatramp, 0, 0, true);
  } else {                               // coolest
    setPixel(Pixel, heatramp, 0, 0, 0, true);
  }
  }
  // Fire(55,120,15);
  void Fire(int Cooling, int Sparking, int SpeedDelay) {
  byte heat[ledCount];
  int cooldown;

  // Step 1.  Cool down every cell a little
  for ( int i = 0; i < ledCount; i++) {
    cooldown = random(0, ((Cooling * 10) / ledCount) + 2);

    if (cooldown > heat[i]) {
      heat[i] = 0;
    } else {
      heat[i] = heat[i] - cooldown;
    }
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for ( int k = ledCount - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' near the bottom
  if ( random(255) < Sparking ) {
    int y = random(7);
    heat[y] = heat[y] + random(160, 255);
    //heat[y] = random(160,255);
  }

  // Step 4.  Convert heat to LED colors
  for ( int j = 0; j < ledCount; j++) {
    setPixelHeatColor(j, heat[j] );
  }

  showStrip();
  delay(SpeedDelay);
  }
*/
// effect. Should return true if effect has finished, false if effect wants more itterations
bool FadeInOutEffect (effectData &myData) {
  // intParam[0] is delay between fade steps
  // intParam[1] is fade step size
  bool returnValue = false;
  byte r, g, b, w;
  switch (myData.effectState) {
    case 0: // init or startup
      if (myData.intParam[1] == 0) myData.intParam[1] = 1; // make sure there is some fading to do
      FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      myData.effectVar[0] = 1; //step increment 1 or -1 for direction of fade
      myData.effectVar[1] = 0; // Current level of fade
      myData.effectState = 2;
      break;

    case 2: {// Display next level of fade
        // Move to nexst level of fade
        myData.effectVar[1] = constrain (myData.effectVar[1] + myData.effectVar[0] * myData.intParam[1], 0, 255);
        // Reverse direction if fade level has maxed out
        if (myData.effectVar[1] == 255) {
          myData.effectVar[0] = -1;
        } else if (myData.effectVar[1] == 0) {
          myData.effectVar[0] = 1;
        }      // map (value, fromLow, fromHigh, toLow, toHigh)
        r = map(red,   0, 255, 0, myData.effectVar[1]);
        g = map(green, 0, 255, 0, myData.effectVar[1]);
        b = map(blue,  0, 255, 0, myData.effectVar[1]);
        w = map(white, 0, 255, 0, myData.effectVar[1]);
        FillPixels(myData.firstPixel, myData.lastPixel, r, g, b, w, false);
        myData.effectDelay = currentMilliSeconds + myData.intParam[0]; // pause at end
        myData.effectState = 3; // delay
      }
      break;

    case 3: // delay
      if (currentMilliSeconds >= myData.effectDelay) myData.effectState = 2;
      break;

    default: // state has been lost so end effect
    case 1: // end effect
      FillPixels(myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      returnValue = true;
      break;
  }
  return returnValue; // true if effect is finished. False for more itterations.
}
/*
  // FadeInOut();
  void FadeInOut() {
  byte r, g, b, w;
  if (effectStart) {
    effectState = 0; // 0=startup, 1=fade, 2=MoveDelay, 3=EndDelay

  }

  switch (effectState) {
    case 0: // startup
      effectMemory[0] = 1; // step increment 1 or -1 for direction of fade
      effectMemory[1] = 0; // current fade level
      setAll(0, 0, 0, 0, false);
      effectState = 1; // Display fade
    //      break;

    case 1: // display fade
      // map (value, fromLow, fromHigh, toLow, toHigh)
      r = map(red,   0, 255, 0, effectMemory[1]);
      g = map(green, 0, 255, 0, effectMemory[1]);
      b = map(blue,  0, 255, 0, effectMemory[1]);
      w = map(white, 0, 255, 0, effectMemory[1]);
      FillPixels(firstPixel, lastPixel, r, g, b, w, false);
      showStrip();
      effectMemory[1] += effectMemory[0]; // move to next fade level
      if (effectMemory[1] == 255) {
        effectMemory[0] = -1;
      } else if (effectMemory[1] == 0) {
        effectMemory[0] = 1;
      }
      effectState = 1;
      effectDelayStart = currentMilliSeconds;
      break;

    case 2: // delay if desired. Original had no delay
      if ((currentMilliSeconds - effectDelayStart) >= 0 ) {
        effectState = 1;
      }
      break;

    default:
      effectState = 0; // lost current state so restart effect
      break;
  }
  //  float r, g, b, w;
  //
  //  for (int k = 0; k < 256; k = k + 1) {
  //    if (shouldAbortEffect()) {
  //      return;
  //    }
  //    r = (k / 256.0) * red;
  //    g = (k / 256.0) * green;
  //    b = (k / 256.0) * blue;
  //    w = (k / 256.0) * white;
  //    setAll(r, g, b, w);
  //    showStrip();
  //  }
  //
  //  for (int k = 255; k >= 0; k = k - 2) {
  //    if (shouldAbortEffect()) {
  //      return;
  //    }
  //    r = (k / 256.0) * red;
  //    g = (k / 256.0) * green;
  //    b = (k / 256.0) * blue;
  //    w = (k / 256.0) * white;
  //    setAll(r, g, b, w);
  //    showStrip();
  //  }
  }
*/
// effect. Should return true if effect has finished, false if effect wants more itterations
bool StrobeEffect (effectData &myData) {
  // intParam[0] is delay for how long strobe is on
  // intParam[1] is delay for how long strobe is off
  // intParam[2] is number of strobe pulses to do
  bool returnValue = false;
  switch (myData.effectState) {
    case 0: // init or startup
      if (myData.intParam[2] == 0) myData.intParam[2] = 1; // make sure there is some strobing to do
      FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      myData.effectVar[0] = 0; //strobe on count
      myData.effectVar[1] = 0; // 1=strobe on, 0=strobe is off
      myData.effectState = 2; // display
      break;

    case 2: // Display next strobe or unstrobe cycle
      if (myData.effectVar[1] == 0) { // strobe is off so turn on
        myData.effectVar[1] = 1;
        FillPixels (myData.firstPixel, myData.lastPixel, myData.r, myData.g, myData.b, myData.w, false);
        ++myData.effectVar[0];
        myData.effectDelay = currentMilliSeconds + myData.intParam[0]; // pause for on time
        myData.effectState = 3; // delay
      } else {
        // strobe is on so turn off and check if finished strobing
        if (myData.effectVar[0] < myData.intParam[2]) { // more strobes to do
          myData.effectVar[1] = 0;
          FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
          myData.effectDelay = currentMilliSeconds + myData.intParam[1]; // pause for off time
          myData.effectState = 3; // delay
        } else {
          myData.effectState = 1; // Done which turns off pixels and returns
        }
      }
      break;

    case 3: // delay
      if (currentMilliSeconds >= myData.effectDelay) myData.effectState = 2;
      break;

    default: // state has been lost so end effect
    case 1: // end effect
      FillPixels(myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      returnValue = true;
      break;
  }
  return returnValue; // true if effect is finished. False for more itterations.
}
/*
  // Slower:
  // Strobe(10, 100);
  // Fast:
  // Strobe(10, 50);
  void Strobe(int StrobeCount, int FlashDelay) {
  for (int j = 0; j < StrobeCount; j++) {
    //    if (shouldAbortEffect()) {
    //      return;
    //    }
    setAll(red, green, blue, white);
    showStrip();
    delay(FlashDelay);
    setAll(0, 0, 0, 0);
    showStrip();
    delay(FlashDelay);
  }
  }
*/
// effect. Should return true if effect has finished, false if effect wants more itterations
bool TheaterChaseEffect (effectData &myData) {
  // intParam[0] is delay for how moving speed
  // intParam[1] is number of on pixels in a group
  // intParam[2] is number of off pixels between on groups
  // intParam[3] is how much the white leds should turn on for background light
  bool returnValue = false;
  unsigned int i;
  unsigned int frameSize = myData.effectVar[0];
  unsigned int frameOffset = myData.effectVar[1];
  byte backgroundWhite = myData.intParam[3];
  bool pattern[myData.intParam[1] + myData.intParam[2]];
  switch (myData.effectState) {
    case 0: // init or startup
      if (myData.intParam[1] == 0) myData.intParam[1] = 1; // make sure there is some on pixels
      if (myData.intParam[2] == 0) myData.intParam[2] = 2; // make sure there is some off pixels
      FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      myData.effectVar[0] = myData.intParam[1] + myData.intParam[2]; //frame size (on pixel count + off pixel count)
      myData.effectVar[1] = 0; // Current starting pixel within frame
      myData.effectState = 2; // display
      break;

    case 2: // Display next chase
      // build pattern (on followed by off pixels)
      for (i = 0; i < frameSize; ++i) {
        if (i < myData.intParam[1]) pattern[i] = true; // if i is less then size of on pixel count
        else pattern[i] = false;
      }
      for (i = myData.firstPixel; i <= myData.lastPixel; ++i) {
        if (pattern[(i + frameOffset) % frameSize]) { // is pixel to be on?
          setPixel(i, myData.r, myData.g, myData.b, myData.w, true);
        } else {
          setPixel(i, 0, 0, 0, backgroundWhite, false);
        }
      }
      // increment frame offset for next frame display
      myData.effectVar[1] = (myData.effectVar[1] + 1) % frameSize;
      myData.effectDelay = currentMilliSeconds + myData.intParam[0]; // pause for speed delay
      myData.effectState = 3; // delay
      break;

    case 3: // delay
      if (currentMilliSeconds >= myData.effectDelay) myData.effectState = 2;
      break;

    default: // state has been lost so end effect
    case 1: // end effect
      FillPixels(myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      returnValue = true;
      break;
  }
  return returnValue; // true if effect is finished. False for more itterations.
}
/*
  // theaterChase(50);
  void theaterChase(int SpeedDelay) {
  for (int q = 0; q < 3; q++) {
    //    if (shouldAbortEffect()) {
    //      return;
    //    }
    for (int i = 0; i < ledCount; i = i + 3) {
      setPixel(i + q, red, green, blue, white, false);  //turn every third pixel on
    }
    showStrip();

    delay(SpeedDelay);

    for (int i = 0; i < ledCount; i = i + 3) {
      setPixel(i + q, 0, 0, 0, 0, false);  //turn every third pixel off
    }
  }
  }
*/
// effect. Should return true if effect has finished, false if effect wants more itterations
bool RainbowCycleEffect (effectData &myData) {
  // intParam[0] is delay for how moving speed
  // intParam[1] is number of rainbows to do in range
  // intParam[2] is how much the white leds should turn on for background light
  bool returnValue = false;
  unsigned int i;
  unsigned int rainbowOffset = myData.effectVar[0];
  unsigned int rainbowSize = myData.effectVar[1];
  byte backgroundWhite = myData.intParam[2];
  byte wheelPos;
  byte increasing, decreasing;
  switch (myData.effectState) {
    case 0: // init or startup
      if (myData.intParam[1] == 0) myData.intParam[1] = 1; // at least 1 rainbow
      myData.effectVar[0] = 0; // rainbow offset
      myData.effectVar[1] = (myData.lastPixel - myData.firstPixel) / myData.intParam[1]; // pixel range of 1 rainbow
      myData.effectState = 2; // display
      break;

    case 2: // Display next rainbow
      for (i = myData.firstPixel; i <= myData.lastPixel; ++i) {
        // Calculate color wheel position by taking current pixel location within the frame of a rainbow
        // and mapping it's position number to the range of 0-255 for the color wheel.
        // Then add in the current rainbow offset to shift the colors around.
        // Final result is mod 256 to wrap around the color wheel.
        wheelPos = (map((i % rainbowSize), 0, rainbowSize, 0, 255) + rainbowOffset) % 256;
        increasing = wheelPos * 3; // overflow of byte is expected and desired.
        decreasing = 255 - increasing;
        if (wheelPos <= 85) {
          setPixel(i, increasing, decreasing, 0, backgroundWhite, false);
        } else if (wheelPos <= 170) {
          setPixel(i, decreasing, 0, increasing, backgroundWhite, false);
        } else {
          setPixel(i, 0, increasing, decreasing, backgroundWhite, false);
        }
      }
      // increment rainbow offset for next rainbow display
      myData.effectVar[0] = (myData.effectVar[0] + 1) % rainbowSize; // rainbowOffset + 1 wrapped to rainbow size
      myData.effectDelay = currentMilliSeconds + myData.intParam[0]; // pause for speed delay
      myData.effectState = 3; // delay
      break;

    case 3: // delay
      if (currentMilliSeconds >= myData.effectDelay) myData.effectState = 2;
      break;

    default: // state has been lost so end effect
    case 1: // end effect
      FillPixels(myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      returnValue = true;
      break;
  }
  return returnValue; // true if effect is finished. False for more itterations.
}
/*
  byte * Wheel(byte WheelPos) {
  static byte c[3];
  byte increasing = WheelPos * 3; // Yes overflow is expected and desired.
  byte decreasing = 255 - increasing;

  // Find color quadrent and fade to it
  if (WheelPos <= 85) {
    c[0] = increasing;
    c[1] = decreasing;
    c[2] = 0;
  } else if (WheelPos <= 170) {
    c[0] = decreasing;
    c[1] = 0;
    c[2] = increasing;
  } else {
    c[0] = 0;
    c[1] = increasing;
    c[2] = decreasing;
  }

  return c;
  }
  //  rainbowCycle(20);
  void rainbowCycle(int SpeedDelay) {
  byte *c;
  uint16_t i, j;

  for (j = 0; j < 256 * 2; j++) { // 2 cycles of all colors on wheel
    //    if (shouldAbortEffect()) {
    //      return;
    //    }
    for (i = 0; i < ledCount; i++) {
      c = Wheel(((i * 256 / ledCount) + j) & 255);
      setPixel(i, *c, *(c + 1), *(c + 2), 0, true);
    }
    showStrip();
    delay(SpeedDelay);
  }
  }
*/
// effect. Should return true if effect has finished, false if effect wants more itterations
bool ColorWipeEffect (effectData &myData) {
  // intParam[0] is delay for how moving speed
  bool returnValue = false;

  switch (myData.effectState) {
    case 0: // init or startup
      myData.effectVar[0] = myData.firstPixel; // Next pixel to light up
      myData.effectState = 2; // display
      break;

    case 2: // Display next pixel
      setPixel(myData.effectVar[0]++, myData.r, myData.g, myData.b, myData.w, false);
      returnValue = myData.effectVar[0] > myData.lastPixel;
      myData.effectDelay = currentMilliSeconds + myData.intParam[0]; // pause for speed delay
      myData.effectState = 3; // delay
      break;

    case 3: // delay
      if (currentMilliSeconds >= myData.effectDelay) myData.effectState = 2;
      break;

    default: // state has been lost so end effect
    case 1: // end effect
      FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      returnValue = true;
      break;
  }
  return returnValue; // true if effect is finished. False for more itterations.
}
/*
  //  colorWipe(50);
  void colorWipe(int SpeedDelay) {
  for (uint16_t i = 0; i < ledCount; i++) {
    //    if (shouldAbortEffect()) {
    //      return;
    //    }
    setPixel(i, red, green, blue, white, false);
    showStrip();
    delay(SpeedDelay);
  }
  transitionDone = true;
  }

  //  colorWipeOnce(50);
  void colorWipeOnce(int SpeedDelay) {
  colorWipe(SpeedDelay);

  // Reset back to previous color
  red = previousRed;
  green = previousGreen;
  blue = previousBlue;
  white = previousWhite;

  colorWipe(SpeedDelay);
  }
*/

// effect. Should return true if effect has finished, false if effect wants more itterations
bool RunningLightsEffect (effectData &myData) {
  // intParam[0] is delay for how moving speed
  // intParam[1] is the stretch factor on the sin wave
  bool returnValue = false;
  unsigned int i;
  byte strength, r, g, b, w;
  float offset = myData.effectVar[0];
  float stretch;
  switch (myData.effectState) {
    case 0: // init or startup
      myData.effectVar[0] = 0; // Offset in sin wave vs pixel position
      myData.effectState = 2; // display
      break;

    case 2: // Display next wave
      stretch = myData.intParam[1];
      offset = offset / stretch;
      for (i = myData.firstPixel; i <= myData.lastPixel; ++i) {
        float fi = i;
        strength = sin(fi / stretch + offset) * 127.0 + 128.0;
        r = map (strength, 0, 255, 0, myData.r);
        g = map (strength, 0, 255, 0, myData.g);
        b = map (strength, 0, 255, 0, myData.b);
        w = map (strength, 0, 255, 0, myData.w);
        setPixel (i, r, g, b, w, false);
      }
      myData.effectDelay = currentMilliSeconds + myData.intParam[0]; // pause for speed delay
      ++myData.effectVar[0];
      myData.effectState = 3; // delay
      break;

    case 3: // delay
      if (currentMilliSeconds >= myData.effectDelay) myData.effectState = 2;
      break;

    default: // state has been lost so end effect
    case 1: // end effect
      FillPixels(myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      returnValue = true;
      break;
  }
  return returnValue; // true if effect is finished. False for more itterations.
}

/*
  //  RunningLights(50);
  void RunningLights(int WaveDelay) {
  int Position = 0;

  for (int i = 0; i < ledCount; i++)
  {
    //    if (shouldAbortEffect()) {
    //      return;
    //    }
    Position++; // = 0; //Position + Rate;
    for (int i = 0; i < ledCount; i++) {
      // sine wave, 3 offset waves make a rainbow!
      //float level = sin(i+Position) * 127 + 128;
      //setPixel(i,level,0,0,false);
      //float level = sin(i+Position) * 127 + 128;
      setPixel(i, ((sin(i + Position) * 127 + 128) / 255)*red,
               ((sin(i + Position) * 127 + 128) / 255)*green,
               ((sin(i + Position) * 127 + 128) / 255)*blue,
               ((sin(i + Position) * 127 + 128) / 255)*white,
               false);
    }

    showStrip();
    delay(WaveDelay);
  }
  }
*/

// effect. Should return true if effect has finished, false if effect wants more itterations
bool SnowSparkleEffect (effectData &myData) {
  // intParam[0] is sparkle on delay
  // intParam[1] is delay between sparkles
  bool returnValue = false;

  switch (myData.effectState) {
    case 0: // init or startup
      myData.effectVar[0] = myData.firstPixel; // which pixel is sparkleing
      myData.effectVar[1] = 0; // is sparkle lit?
      myData.effectState = 2; // display
      FillPixels (myData.firstPixel, myData.lastPixel, myData.r, myData.g, myData.b, myData.w, false);
      break;

    case 2: // Display next sparkle or darkness
      if (myData.effectVar[1]) { // is sparkle lit?
        // yes so turn back to base color and delay
        setPixel (myData.effectVar[0], myData.r, myData.g, myData.b, myData.w, false);
        myData.effectVar[1] = 0; // note that pixel is off
        myData.effectDelay = currentMilliSeconds + myData.intParam[1];
      } else {
        // no so select a new pixel and lite then delay
        myData.effectVar[0] = random(myData.firstPixel, myData.lastPixel + 1); // which pixel is sparking
        setPixel (myData.effectVar[0], 0, 0, 0, 255, false);
        myData.effectVar[1] = 1; // note that pixel is on
        myData.effectDelay = currentMilliSeconds + myData.intParam[0];
      }
      myData.effectState = 3; // delay
      break;

    case 3: // delay
      if (currentMilliSeconds >= myData.effectDelay) myData.effectState = 2;
      break;

    default: // state has been lost so end effect
    case 1: // end effect
      FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      returnValue = true;
      break;
  }
  return returnValue; // true if effect is finished. False for more itterations.
}

/*
  //  SnowSparkle(20, random(100,1000));
  void SnowSparkle(int SparkleDelay, int SpeedDelay) {
  setAll(red, green, blue, white);

  int Pixel = random(ledCount);
  setPixel(Pixel, 0, 0, 0, 255, false);
  showStrip();
  delay(SparkleDelay);
  setPixel(Pixel, red, green, blue, white, false);
  showStrip();
  delay(SpeedDelay);
  }
*/

// effect. Should return true if effect has finished, false if effect wants more itterations
bool SparkleEffect (effectData &myData) {
  // intParam[0] is sparkle on delay
  // intParam[1] is delay between sparkles
  bool returnValue = false;

  switch (myData.effectState) {
    case 0: // init or startup
      myData.effectVar[0] = myData.firstPixel; // which pixel is sparkleing
      myData.effectVar[1] = 0; // is sparkle lit?
      myData.effectState = 2; // display
      FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      break;

    case 2: // Display next sparkle or darkness
      if (myData.effectVar[1]) { // is sparkle lit?
        // yes so turn back to base color and delay
        setPixel (myData.effectVar[0], 0, 0, 0, 0, false);
        myData.effectVar[1] = 0; // note that pixel is off
        myData.effectDelay = currentMilliSeconds + myData.intParam[1];
      } else {
        // no so select a new pixel and lite then delay
        myData.effectVar[0] = random(myData.firstPixel, myData.lastPixel + 1); // which pixel is sparking
        setPixel (myData.effectVar[0], myData.r, myData.g, myData.b, myData.w, false);
        myData.effectVar[1] = 1; // note that pixel is on
        myData.effectDelay = currentMilliSeconds + myData.intParam[0];
      }
      myData.effectState = 3; // delay
      break;

    case 3: // delay
      if (currentMilliSeconds >= myData.effectDelay) myData.effectState = 2;
      break;

    default: // state has been lost so end effect
    case 1: // end effect
      FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      returnValue = true;
      break;
  }
  return returnValue; // true if effect is finished. False for more itterations.
}

/*
  //  Sparkle(0);
  void Sparkle(int SpeedDelay) {
  setAll(0, 0, 0, 0);
  int Pixel = random(ledCount);
  setPixel(Pixel, red, green, blue, white, false);
  showStrip();
  delay(SpeedDelay);
  setPixel(Pixel, 0, 0, 0, 0, false);
  }
*/

/*
  //  TwinkleRandom(20, 100, false);
  void TwinkleRandom(int Count, int SpeedDelay, boolean OnlyOne) {
  setAll(0, 0, 0, 0);

  for (int i = 0; i < Count; i++) {
    //    if (shouldAbortEffect()) {
    //      return;
    //    }
    setPixel(random(ledCount), random(0, 255), random(0, 255), random(0, 255), 0, true);
    showStrip();
    delay(SpeedDelay);
    if (OnlyOne) {
      setAll(0, 0, 0, 0);
    }
  }

  delay(SpeedDelay);
  }
*/

typedef struct bouncingBallsData {
  float Height;
  float ImpactVelocity;
  float TimeSinceLastBounce;
  int   Position;
  long  ClockTimeSinceLastBounce;
  float Dampening;
} bouncingBallData;

// Bouncing balls effect. Should return true if effect has finished, false if effect wants more itterations
bool BouncingBallsEffect (effectData &myData) {
  // intParam[0] is delay between updates, may want 0 // always use [0] for delay setting
  // intParam[1] is how many balls to bounce
  bool returnValue = false;
  bouncingBallData * ballData = (bouncingBallData *)myData.effectMemory; // cast void pointer to correct type
  int ballCount = myData.intParam[1];
  int ledCount = myData.lastPixel  - myData.firstPixel + 1;
  const static float Gravity = -9.81;
  const static int StartHeight = 1;
  const static float ImpactVelocityStart = sqrt( -2 * Gravity * StartHeight );

  switch (myData.effectState) {
    case 0: // init or startup
      if (myData.intParam[1] < 1) myData.intParam[1] = 1;
      if (myData.intParam[1] > 100) myData.intParam[1] = 100;
      ballCount = myData.intParam[1];

      // grab memory for each ball to be bounced
      myData.effectMemory = (void *)malloc(ballCount * sizeof(bouncingBallData));
      ballData = (bouncingBallData *)myData.effectMemory; // update since we just allocated memory
      FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      if (myData.effectMemory == NULL) {
        returnValue = true; // no memory to do effect
      } else { // set up balls
        for (int i = 0 ; i < ballCount ; i++) {
          ballData[i].ClockTimeSinceLastBounce = millis();
          ballData[i].Height = StartHeight;
          ballData[i].Position = myData.firstPixel;
          ballData[i].ImpactVelocity = ImpactVelocityStart;
          ballData[i].TimeSinceLastBounce = 0;
          ballData[i].Dampening = 0.90 - float(i) / pow(ballCount, 2);
        }
      }
      myData.effectState = 2; // display effect
      break;

    case 2: // Display data
      for (int i = 0 ; i < ballCount ; i++) {
        setPixel(ballData[i].Position, 0, 0, 0, 0, false);
        ballData[i].TimeSinceLastBounce =  millis() - ballData[i].ClockTimeSinceLastBounce;
        ballData[i].Height  = 0.5 * Gravity * pow( ballData[i].TimeSinceLastBounce / 1000 , 2.0 ) + ballData[i].ImpactVelocity * ballData[i].TimeSinceLastBounce / 1000;

        if ( ballData[i].Height < 0 ) {
          ballData[i].Height = 0;
          ballData[i].ImpactVelocity = ballData[i].Dampening * ballData[i].ImpactVelocity;
          ballData[i].ClockTimeSinceLastBounce = millis();

          if ( ballData[i].ImpactVelocity < 0.01 ) {
            ballData[i].ImpactVelocity = ImpactVelocityStart;
          }
        }
        ballData[i].Position = round( ballData[i].Height * (ledCount - 1) / StartHeight) + myData.firstPixel;
        setPixel(ballData[i].Position, myData.r, myData.g, myData.b, myData.w, false);
      }
      break;

    case 3: // delay
      if (currentMilliSeconds >= myData.effectDelay) myData.effectState = 2;
      break;

    default: // state has been lost so end effect
    case 1: // end effect and release memory
      FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      if (myData.effectMemory != NULL) {
        free(myData.effectMemory);
      }
      returnValue = true;
  }
  return returnValue; // true if effect is finished. False for more itterations.
}

/*
  // BouncingBalls(3);
  void BouncingBalls(int BallCount) {
  float Gravity = -9.81;
  int StartHeight = 1;

  float Height[BallCount];
  float ImpactVelocityStart = sqrt( -2 * Gravity * StartHeight );
  float ImpactVelocity[BallCount];
  float TimeSinceLastBounce[BallCount];
  int   Position[BallCount];
  long  ClockTimeSinceLastBounce[BallCount];
  float Dampening[BallCount];

  for (int i = 0 ; i < BallCount ; i++) {
    ClockTimeSinceLastBounce[i] = millis();
    Height[i] = StartHeight;
    Position[i] = 0;
    ImpactVelocity[i] = ImpactVelocityStart;
    TimeSinceLastBounce[i] = 0;
    Dampening[i] = 0.90 - float(i) / pow(BallCount, 2);
  }

  while (true) {
    //    if (shouldAbortEffect()) {
    //      return;
    //    }
    for (int i = 0 ; i < BallCount ; i++) {
      TimeSinceLastBounce[i] =  millis() - ClockTimeSinceLastBounce[i];
      Height[i] = 0.5 * Gravity * pow( TimeSinceLastBounce[i] / 1000 , 2.0 ) + ImpactVelocity[i] * TimeSinceLastBounce[i] / 1000;

      if ( Height[i] < 0 ) {
        Height[i] = 0;
        ImpactVelocity[i] = Dampening[i] * ImpactVelocity[i];
        ClockTimeSinceLastBounce[i] = millis();

        if ( ImpactVelocity[i] < 0.01 ) {
          ImpactVelocity[i] = ImpactVelocityStart;
        }
      }
      Position[i] = round( Height[i] * (ledCount - 1) / StartHeight);
    }

    for (int i = 0 ; i < BallCount ; i++) {
      setPixel(Position[i], red, green, blue, white, false);
    }

    showStrip();
    setAll(0, 0, 0, 0);
  }
  }
*/

/**************************** START TRANSITION FADER *****************************************/
// From https://www.arduino.cc/en/Tutorial/ColorCrossfader
/* BELOW THIS LINE IS THE MATH -- YOU SHOULDN'T NEED TO CHANGE THIS FOR THE BASICS
  The program works like this:
  Imagine a crossfade that moves the red LED from 0-10,
    the green from 0-5, and the blue from 10 to 7, in
    ten steps.
    We'd want to count the 10 steps and increase or
    decrease color values in evenly stepped increments.
    Imagine a + indicates raising a value by 1, and a -
    equals lowering it. Our 10 step fade would look like:
    1 2 3 4 5 6 7 8 9 10
  R + + + + + + + + + +
  G   +   +   +   +   +
  B     -     -     -
  The red rises from 0 to 10 in ten steps, the green from
  0-5 in 5 steps, and the blue falls from 10 to 7 in three steps.
  In the real program, the color percentages are converted to
  0-255 values, and there are 1020 steps (255*4).
  To figure out how big a step there should be between one up- or
  down-tick of one of the LED values, we call calculateStep(),
  which calculates the absolute gap between the start and end values,
  and then divides that gap by 1020 to determine the size of the step
  between adjustments in the value.
*/
/*
  int calculateStep(int prevValue, int endValue) {
  int step = endValue - prevValue;  // What's the overall gap?
  if (step) {                       // If its non-zero,
    step = 1020 / step;            //   divide by 1020
  }

  return step;
  }
*/
/* The next function is calculateVal. When the loop value, i,
   reaches the step size appropriate for one of the
   colors, it increases or decreases the value of that color by 1.
   (R, G, and B are each calculated separately.)
*/
/*
  int calculateVal(int step, int val, int i) {
  if ((step) && i % step == 0) { // If step is non-zero and its time to change a value,
    if (step > 0) {              //   increment the value if step is positive...
      val += 1;
    }
    else if (step < 0) {         //   ...or decrement it if step is negative
      val -= 1;
    }
  }

  // Defensive driving: make sure val stays in the range 0-255
  if (val > 255) {
    val = 255;
  }
  else if (val < 0) {
    val = 0;
  }

  return val;
  }
*/
/*
  // Fade(50);
  void Fade(int SpeedDelay) {
  int redVal = previousRed;
  int grnVal = previousGreen;
  int bluVal = previousBlue;
  int whiVal = previousWhite;
  int stepR = calculateStep(redVal, red);
  int stepG = calculateStep(grnVal, green);
  int stepB = calculateStep(bluVal, blue);
  int stepW = calculateStep(whiVal, white);

  // If no change then exit
  if (stepR == 0 && stepG == 0 && stepB == 0 && stepW == 0) {
    setAll(redVal, grnVal, bluVal, whiVal); // Write current values to LED pins
    //transitionDone = true;
    return;
  }

  for (int i = 0; i < 1020; i++) {
    //    if (shouldAbortEffect()) {
    //      return;
    //    }

    redVal = calculateVal(stepR, redVal, i);
    grnVal = calculateVal(stepG, grnVal, i);
    bluVal = calculateVal(stepB, bluVal, i);
    whiVal = calculateVal(stepW, whiVal, i);

    if (i % 50 == 0) {
      setAll(redVal, grnVal, bluVal, whiVal); // Write current values to LED pins
      delay(SpeedDelay);
    }
  }

  setAll(redVal, grnVal, bluVal, whiVal); // Write current values to LED pins
  //transitionDone = true;
  }
*/

// Lightning effect. Should return true if effect has finished, false if effect wants more itterations
// Lightning works by starting in middle of range with some randomness and putting up a dim flash.
// Each following dim flash will use more of the range.
// After all dim passes there will be 1 bright flash and then effect will restart
bool LightingingEffect (effectData &myData) {
  // intParam[0] is delay scaler between dim lighting strikes // always use [0] for delay setting
  // intParam[1] is delay scaler between bright strike and next dim strike
  // intParam[2] is lower limit of dim strikes
  // intParam[3] is upper limit of dim strikes
  bool returnValue = false;
  int ledCount = myData.lastPixel  - myData.firstPixel + 1;
  int strikeLength = myData.effectVar[2] - myData.effectVar[1] + 1;
  unsigned int i;
  byte r, g, b, w, dimmer;

  switch (myData.effectState) {
    case 0: // init or startup
      myData.effectVar[0] = 0; // Dimmer prestrikes left
      myData.effectVar[1] = 0; // prestrike start
      myData.effectVar[2] = 0; // prestrike end
      myData.effectVar[3] = 0; // dim flash growth between dim flashes
      myData.effectVar[4] = 0; // prestrike length
      FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      myData.effectState = 2; // Dim Display
      break;

    case 2: // Display flash
      if (myData.effectVar[0] == 0) { // time to start next dim sequence
        myData.effectVar[0] = random(myData.intParam[2], myData.intParam[3] + 1); // Dimmer prestrikes left
        //Serial.print ("Dim flashes:"); Serial.print(myData.effectVar[0]);
        // Bright flash is entire pixel range. Each dim flash should grown in size.
        // Growth per flash should be (pixel range)/(dim flashes + 2)
        // The Plus two is fudge factor so bright flash has strip to grow into and to leave
        //   some room for random moving of dim flashes
        myData.effectVar[3] = (myData.lastPixel - myData.firstPixel + 1) / (myData.effectVar[0] + 2);
        // Serial.print ("  Dim growth step size:"); Serial.println(myData.effectVar[3]);
        myData.effectVar[4] = myData.effectVar[3]; // first dim flash is length of step size
      }

      if (--myData.effectVar[0] > 0) { // more dim flashes left
        int pixelRange = myData.lastPixel - myData.firstPixel + 1;
        int flashRange = myData.effectVar[4];
        // Erase prior flash incase next flash is far enough away so as to not overlap
        FillPixels (myData.effectVar[1], myData.effectVar[2], 0, 0, 0, 0, false);
        // Calculate new flash
        myData.effectVar[1] = random(0, ((pixelRange - flashRange) / 2)) + myData.firstPixel; // prestrike start
        myData.effectVar[2] = myData.effectVar[1] + flashRange - 1; // prestrike end
        myData.effectVar[4] += myData.effectVar[3]; // grow next flash size.
        // dimmer = random(10,brightness/4);
        dimmer = brightness / 10;
        r = map(myData.r, 0, 255, 0, dimmer);
        g = map(myData.g, 0, 255, 0, dimmer);
        b = map(myData.b, 0, 255, 0, dimmer);
        w = map(myData.w, 0, 255, 0, dimmer);
        FillPixels (myData.effectVar[1], myData.effectVar[2], r, g, b, w, false);
        //        Serial.print (myData.effectVar[1]); Serial.print(":"); Serial.print(myData.effectVar[2]);
        //        Serial.print (" - "); Serial.println(myData.effectVar[2] - myData.effectVar[1] + 1);
        myData.effectDelay = currentMilliSeconds + random(4, 15) * myData.intParam[0];
        myData.effectState = 3; // delay dim
      } else {
        FillPixels (myData.firstPixel, myData.lastPixel, myData.r, myData.g, myData.b, myData.w, false);
        //        Serial.println("Bright");
        myData.effectDelay = currentMilliSeconds + random(4, 15) * myData.intParam[0];
        myData.effectState = 4; // delay bright
      }
      break;

    case 3: // delay on dim
      if (currentMilliSeconds >= myData.effectDelay) {
        //FillPixels (myData.effectVar[1], myData.effectVar[2], 0, 0, 0, 0, false);
        //myData.effectDelay = currentMilliSeconds + 50 + random(100);
        //myData.effectState = 5; // delay off dim
        myData.effectState = 2; // Display next flash
      }
      break;

    case 5: // delay off
      if (currentMilliSeconds >= myData.effectDelay) {
        myData.effectState = 2; // display flash
      }
      break;

    case 4: // delay on bright
      if (currentMilliSeconds >= myData.effectDelay) {
        FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);  // end flash delay for dark time
        myData.effectDelay = currentMilliSeconds + 500 + (random(500, 1000) * myData.intParam[1]); // dark time between flashes
        myData.effectState = 5; // Delay off
      }
      break;

    default: // state has been lost so end effect
    case 1: // end effect and release memory
      FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      returnValue = true;
  }
  return returnValue; // true if effect is finished. False for more itterations.
}

/*
  void Lightning(int SpeedDelay) {
  setAll(0, 0, 0, 0);
  int ledstart = random(ledCount);           // Determine starting location of flash
  int ledlen = random(ledCount - ledstart);  // Determine length of flash (not to go beyond ledCount-1)
  for (int flashCounter = 0; flashCounter < random(1, 4); flashCounter++) {
    int dimmer = random(10, brightness);          // return strokes are brighter than the leader
    int rr = map(red, 0, 255, 0, dimmer);
    int gg = map(green, 0, 255, 0, dimmer);
    int bb = map(blue, 0, 255, 0, dimmer);
    int ww = map(white, 0, 255, 0, dimmer);

    for (int i = ledstart ; i < (ledstart + ledlen) ; i++) {
      setPixel(i, rr, gg, bb, ww, false);
    }
    showStrip();    // Show a section of LED's
    delay(random(4, 15));                // each flash only lasts 4-10 milliseconds
    for (int i = ledstart ; i < (ledstart + ledlen) ; i++) {
      setPixel(i, 0, 0, 0, 0, false);
    }
    showStrip();
    //if (flashCounter == 0) delay (130);   // longer delay until next flash after the leader
    delay(50 + random(100));             // shorter delay between strokes
  }
  delay(random(SpeedDelay) * 50);        // delay between strikes
  }
*/

// MeteorRain effect. Should return true if effect has finished, false if effect wants more itterations
bool MeteorRainEffect (effectData &myData) {
  // intParam[0] is delay for each movement of meteor// always use [0] for delay setting
  // intParam[1] is pixel size of meteor
  // intParam[2] is meteor trail decay (big =  short tail, range 0-255)
  // intParam[3] is meteor random decay (0 = smooth, 1+ = more jagged)
  bool returnValue = false;
  unsigned int i;
  unsigned int meteorHeadStop, meteorHeadStart;
  bool allBlack;

  switch (myData.effectState) {
    case 0: // init or startup
      myData.effectVar[0] = myData.firstPixel; // start of head of meteor
      myData.effectVar[1] = 0; // unused
      //Handle corner case of 0 where the math breaks
      if (myData.intParam[1] == 0) {
        myData.intParam[1] = 1;
      }
      myData.effectVar[2] = 0; // unused
      myData.effectVar[3] = 0; // unused
      myData.effectVar[4] = 0; // unused
      FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      myData.effectState = 2; // Display
      break;

    case 2: // Display meteor
      // fade all lit pixels
      allBlack = true;
      for (i = myData.firstPixel; i <= myData.lastPixel; ++i) {
        if (((myData.intParam[3] == 0) || (random(10) > 5))) {
          // fade current pixel
          allBlack &= fadeToBlack (i, myData.intParam[2]);
        }
      }

      // Draw meteor
      meteorHeadStart = myData.effectVar[0];
      if (meteorHeadStart < myData.lastPixel) {
        meteorHeadStop = meteorHeadStart + myData.intParam[1];
        if (meteorHeadStop > myData.lastPixel)
          meteorHeadStop = myData.lastPixel;
        FillPixels(meteorHeadStart, meteorHeadStop, myData.r, myData.g, myData.b, myData.w, false);
        ++myData.effectVar[0]; // advance meteor head
      } else if (allBlack) {
        // Meteor head is past end of pixel range and all faded pixels were black so start new meteor
        myData.effectVar[0] = 0;
      }
      myData.effectDelay = currentMilliSeconds + myData.intParam[0];
      myData.effectState = 3; // Delay
      break;

    case 3: // delay on dim
      if (currentMilliSeconds >= myData.effectDelay) {
        myData.effectState = 2; // Display next frame
      }
      break;

    default: // state has been lost so end effect
    case 1: // end effect and release memory
      FillPixels (myData.firstPixel, myData.lastPixel, 0, 0, 0, 0, false);
      returnValue = true;
  }
  return returnValue; // true if effect is finished. False for more itterations.
}


/*
  void ShowPixels() {
  // If there are only 2 items in the array then we are setting from and to othersise set each led in the array.
  if (pixelLen == 2) {
    //Serial.println(F("ShowPixels-Range"));

    int startL = max(0, min(pixelArray[0], pixelArray[1])); // choose smaller of array entries
    int endL = min(ledCount, max(pixelArray[0], pixelArray[1])); // choose larger of array entries

    for (int i = startL; i < endL; i++) {
      setPixel(i, red, green, blue, white, true);
    }

  } else {
    // For each item in array
    //Serial.println(F("ShowPixels-Array"));

    for (int i = 0; i < pixelLen; i++) {
      // constrain the pixel in the array to a valid pixel number in the strip
      setPixel(constrain(pixelArray[i], 0, ledCount), red, green, blue, white, true);
    }
  }
  showStrip();
  //transitionDone = true;
  }

*/






#endif
///**************************** END EFFECTS *****************************************/







//
///**************************** START STRIPLED PALETTE *****************************************/
//void setupStripedPalette( CRGB A, CRGB AB, CRGB B, CRGB BA) {
//  currentPalettestriped = CRGBPalette16(
//                            A, A, A, A, A, A, A, A, B, B, B, B, B, B, B, B
//                            //    A, A, A, A, A, A, A, A, B, B, B, B, B, B, B, B
//                          );
//}
//
//
//
///********************************** START FADE************************************************/
//void fadeall() {
//  for (int i = 0; i < ledCount; i++) {
//    leds[i].nscale8(250);  //for CYCLon
//  }
//}
//
//
//
///********************************** START FIRE **********************************************/
//void Fire2012WithPalette()
//{
//  // Array of temperature readings at each simulation cell
//  static byte heat[ledCount];
//
//  // Step 1.  Cool down every cell a little
//  for ( int i = 0; i < ledCount; i++) {
//    heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / ledCount) + 2));
//  }
//
//  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
//  for ( int k = ledCount - 1; k >= 2; k--) {
//    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
//  }
//
//  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
//  if ( random8() < SPARKING ) {
//    int y = random8(7);
//    heat[y] = qadd8( heat[y], random8(160, 255) );
//  }
//
//  // Step 4.  Map from heat cells to LED colors
//  for ( int j = 0; j < ledCount; j++) {
//    // Scale the heat value from 0-255 down to 0-240
//    // for best results with color palettes.
//    byte colorindex = scale8( heat[j], 240);
//    CRGB color = ColorFromPalette( gPal, colorindex);
//    int pixelnumber;
//    if ( gReverseDirection ) {
//      pixelnumber = (ledCount - 1) - j;
//    } else {
//      pixelnumber = j;
//    }
//    leds[pixelnumber] = color;
//  }
//}
//
//
//
///********************************** START ADD GLITTER *********************************************/
//void addGlitter( fract8 chanceOfGlitter)
//{
//  if ( random8() < chanceOfGlitter) {
//    leds[ random16(ledCount) ] += CRGB::White;
//  }
//}
//
//
//
///********************************** START ADD GLITTER COLOR ****************************************/
//void addGlitterColor( fract8 chanceOfGlitter, int red, int green, int blue)
//{
//  if ( random8() < chanceOfGlitter) {
//    leds[ random16(ledCount) ] += CRGB(red, green, blue);
//  }
//}
//













/***************************************************************************/
/***************************************************************************/
/***************************************************************************/


//
///********************************** GLOBALS for EFFECTS ******************************/
////RAINBOW
//uint8_t thishue = 0;                                          // Starting hue value.
//uint8_t deltahue = 10;
//
////CANDYCANE
//CRGBPalette16 currentPalettestriped; //for Candy Cane
//CRGBPalette16 gPal; //for fire
//
////NOISE
//static uint16_t dist;         // A random number for our noise generator.
//uint16_t scale = 30;          // Wouldn't recommend changing this on the fly, or the animation will be really blocky.
//uint8_t maxChanges = 48;      // Value for blending between palettes.
//CRGBPalette16 targetPalette(OceanColors_p);
//CRGBPalette16 currentPalette(CRGB::Black);
//
////TWINKLE
//#define DENSITY     80
//int twinklecounter = 0;
//
////RIPPLE
//uint8_t colour;                                               // Ripple colour is randomized.
//int center = 0;                                               // Center of the current ripple.
//int step = -1;                                                // -1 is the initializing step.
//uint8_t myfade = 255;                                         // Starting brightness.
//#define maxsteps 16                                           // Case statement wouldn't allow a variable.
//uint8_t bgcol = 0;                                            // Background colour rotates.
//int thisdelay = 20;                                           // Standard delay value.
//
////DOTS
//uint8_t   count =   0;                                        // Count up to 255 and then reverts to 0
//uint8_t fadeval = 224;                                        // Trail behind the LED's. Lower => faster fade.
//uint8_t bpm = 30;
//
////LIGHTNING
//uint8_t frequency = 50;                                       // controls the interval between strikes
//uint8_t flashes = 8;                                          //the upper limit of flashes per strike
//unsigned int dimmer = 1;
//uint8_t ledstart;                                             // Starting location of a flash
//uint8_t ledlen;
//int lightningcounter = 0;
//
////FUNKBOX
//int idex = 0;                //-LED INDEX (0 to ledCount-1
//int TOP_INDEX = int(ledCount / 2);
//int thissat = 255;           //-FX LOOPS DELAY VAR
//uint8_t thishuepolice = 0;
//int antipodal_index(int i) {
//  int iN = i + TOP_INDEX;
//  if (i >= TOP_INDEX) {
//    iN = ( i + TOP_INDEX ) % ledCount;
//  }
//  return iN;
//}
//
////FIRE
//#define COOLING  55
//#define SPARKING 120
//bool gReverseDirection = false;
//
////BPM
//uint8_t gHue = 0;







//  //EFFECT BPM
//  if (effect == "bpm") {
//    uint8_t BeatsPerMinute = 62;
//    CRGBPalette16 palette = PartyColors_p;
//    uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
//    for ( int i = 0; i < ledCount; i++) { //9948
//      leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
//    }
//    if (transitionTime == 0 or transitionTime == NULL) {
//      transitionTime = 30;
//    }
//    showleds();
//  }
//
//
//  //EFFECT Candy Cane
//  if (effect == "candy cane") {
//    static uint8_t startIndex = 0;
//    startIndex = startIndex + 1; /* higher = faster motion */
//    fill_palette( ledsRGB, ledCount,
//                  startIndex, 16, /* higher = narrower stripes */
//                  currentPalettestriped, 255, LINEARBLEND);
//    if (transitionTime == 0 or transitionTime == NULL) {
//      transitionTime = 0;
//    }
//    showleds();
//  }
//
//
////  //EFFECT CONFETTI
////  if (effect == "confetti" ) {
////    fadeToBlackBy( ledsRGB, ledCount, 25);
////    int pos = random16(ledCount);
////    leds[pos] += CRGBW(red + random8(64), green, blue, 0);
////    if (transitionTime == 0 or transitionTime == NULL) {
////      transitionTime = 30;
////    }
////    showleds();
////  }
//
//
//  //EFFECT CYCLON RAINBOW
//  if (effect == "cyclon rainbow") {                    //Single Dot Down
//    static uint8_t hue = 0;
//    // First slide the led in one direction
//    for (int i = 0; i < ledCount; i++) {
//      // Set the i'th led to red
//      leds[i] = CHSV(hue++, 255, 255);
//      // Show the leds
//      showleds();
//      // now that we've shown the leds, reset the i'th led to black
//      // leds[i] = CRGB::Black;
//      fadeall();
//      // Wait a little bit before we loop around and do it again
//      delay(10);
//    }
//    for (int i = (ledCount) - 1; i >= 0; i--) {
//      // Set the i'th led to red
//      leds[i] = CHSV(hue++, 255, 255);
//      // Show the leds
//      showleds();
//      // now that we've shown the leds, reset the i'th led to black
//      // leds[i] = CRGB::Black;
//      fadeall();
//      // Wait a little bit before we loop around and do it again
//      delay(10);
//    }
//  }
//
//
//  //EFFECT DOTS
//  if (effect == "dots") {
//    uint8_t inner = beatsin8(bpm, ledCount / 4, ledCount / 4 * 3);
//    uint8_t outer = beatsin8(bpm, 0, ledCount - 1);
//    uint8_t middle = beatsin8(bpm, ledCount / 3, ledCount / 3 * 2);
//    leds[middle] = CRGB::Purple;
//    leds[inner] = CRGB::Blue;
//    leds[outer] = CRGB::Aqua;
//    nscale8(ledsRGB, ledCount, fadeval);
//
//    if (transitionTime == 0 or transitionTime == NULL) {
//      transitionTime = 30;
//    }
//    showleds();
//  }
//
//
//  //EFFECT FIRE
//  if (effect == "fire") {
//    Fire2012WithPalette();
//    if (transitionTime == 0 or transitionTime == NULL) {
//      transitionTime = 150;
//    }
//    showleds();
//  }
//
//  random16_add_entropy( random8());
//
//
//  //EFFECT Glitter
//  if (effect == "glitter") {
//    fadeToBlackBy( ledsRGB, ledCount, 20);
//    addGlitterColor(80, red, green, blue);
//    if (transitionTime == 0 or transitionTime == NULL) {
//      transitionTime = 30;
//    }
//    showleds();
//  }
//
//
//  //EFFECT JUGGLE
//  if (effect == "juggle" ) {                           // eight colored dots, weaving in and out of sync with each other
//    fadeToBlackBy(ledsRGB, ledCount, 20);
//    for (int i = 0; i < 8; i++) {
//      ledsRGB[beatsin16(i + 7, 0, ledCount - 1  )] |= CRGB(red, green, blue);
//    }
//    if (transitionTime == 0 or transitionTime == NULL) {
//      transitionTime = 130;
//    }
//    showleds();
//  }
//
//
//  //EFFECT LIGHTNING
//  if (effect == "lightning") {
//    twinklecounter = twinklecounter + 1;                     //Resets strip if previous animation was running
//    if (twinklecounter < 2) {
//      FastLED.clear();
//      FastLED.show();
//    }
//    ledstart = random8(ledCount);           // Determine starting location of flash
//    ledlen = random8(ledCount - ledstart);  // Determine length of flash (not to go beyond ledCount-1)
//    for (int flashCounter = 0; flashCounter < random8(3, flashes); flashCounter++) {
//      if (flashCounter == 0) dimmer = 5;    // the brightness of the leader is scaled down by a factor of 5
//      else dimmer = random8(1, 3);          // return strokes are brighter than the leader
//      fill_solid(ledsRGB + ledstart, ledlen, CHSV(255, 0, 255 / dimmer));
//      showleds();    // Show a section of LED's
//      delay(random8(4, 10));                // each flash only lasts 4-10 milliseconds
//      fill_solid(ledsRGB + ledstart, ledlen, CHSV(255, 0, 0)); // Clear the section of LED's
//      showleds();
//      if (flashCounter == 0) delay (130);   // longer delay until next flash after the leader
//      delay(50 + random8(100));             // shorter delay between strokes
//    }
//    delay(random8(frequency) * 100);        // delay between strikes
//    if (transitionTime == 0 or transitionTime == NULL) {
//      transitionTime = 0;
//    }
//    showleds();
//  }
//
//
//  //EFFECT POLICE ALL
//  if (effect == "police all") {                 //POLICE LIGHTS (TWO COLOR SOLID)
//    idex++;
//    if (idex >= ledCount) {
//      idex = 0;
//    }
//    int idexR = idex;
//    int idexB = antipodal_index(idexR);
//    int thathue = (thishuepolice + 160) % 255;
//    leds[idexR] = CHSV(thishuepolice, thissat, 255);
//    leds[idexB] = CHSV(thathue, thissat, 255);
//    if (transitionTime == 0 or transitionTime == NULL) {
//      transitionTime = 30;
//    }
//    showleds();
//  }
//
//  //EFFECT POLICE ONE
//  if (effect == "police one") {
//    idex++;
//    if (idex >= ledCount) {
//      idex = 0;
//    }
//    int idexR = idex;
//    int idexB = antipodal_index(idexR);
//    int thathue = (thishuepolice + 160) % 255;
//    for (int i = 0; i < ledCount; i++ ) {
//      if (i == idexR) {
//        leds[i] = CHSV(thishuepolice, thissat, 255);
//      }
//      else if (i == idexB) {
//        leds[i] = CHSV(thathue, thissat, 255);
//      }
//      else {
//        leds[i] = CHSV(0, 0, 0);
//      }
//    }
//    if (transitionTime == 0 or transitionTime == NULL) {
//      transitionTime = 30;
//    }
//    showleds();
//  }
//
//
//  //EFFECT RAINBOW
//  if (effect == "rainbow") {
//    // FastLED's built-in rainbow generator
//    static uint8_t starthue = 0;    thishue++;
//    fill_rainbow(ledsRGB, ledCount, thishue, deltahue);
//    if (transitionTime == 0 or transitionTime == NULL) {
//      transitionTime = 130;
//    }
//    showleds();
//  }
//
//
//  //EFFECT RAINBOW WITH GLITTER
//  if (effect == "rainbow with glitter") {               // FastLED's built-in rainbow generator with Glitter
//    static uint8_t starthue = 0;
//    thishue++;
//    fill_rainbow(ledsRGB, ledCount, thishue, deltahue);
//    addGlitter(80);
//    if (transitionTime == 0 or transitionTime == NULL) {
//      transitionTime = 130;
//    }
//    showleds();
//  }
//
//
//  //EFFECT SIENLON
//  if (effect == "sinelon") {
//    fadeToBlackBy( ledsRGB, ledCount, 20);
//    int pos = beatsin16(13, 0, ledCount - 1);
//    leds[pos] += CRGB(red, green, blue);
//    if (transitionTime == 0 or transitionTime == NULL) {
//      transitionTime = 150;
//    }
//    showleds();
//  }
//
//

//
//void Twinkle2() {
//    twinklecounter = twinklecounter + 1;
//    if (twinklecounter < 2) {                               //Resets strip if previous animation was running
//      FastLED.clear();
//      FastLED.show();
//    }
//    const CRGBW lightcolor(8, 7, 1, 0);
//    for ( int i = 0; i < ledCount; i++) {
//      if ( !leds[i]) continue; // skip black pixels
//      if ( leds[i].r & 1) { // is red odd?
//        leds[i] -= lightcolor; // darken if red is odd
//      } else {
//        leds[i] += lightcolor; // brighten if red is even
//      }
//    }
//    if ( random8() < DENSITY) {
//      int j = random16(ledCount);
//      if ( !leds[j] ) leds[j] = lightcolor;
//    }
//
//    if (transitionTime == 0 or transitionTime == NULL) {
//      transitionTime = 0;
//    }
//    showleds();
//  }
//
//
//  EVERY_N_MILLISECONDS(10) {
//
//    nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);  // FOR NOISE ANIMATIon
//    {
//      gHue++;
//    }
//
//    //EFFECT NOISE
//    if (effect == "noise") {
//      for (int i = 0; i < ledCount; i++) {                                     // Just onE loop to fill up the LED array as all of the pixels change.
//        uint8_t index = inoise8(i * scale, dist + i * scale) % 255;            // Get a value from the noise function. I'm using both x and y axis.
//        leds[i] = ColorFromPalette(currentPalette, index, 255, LINEARBLEND);   // With that value, look up the 8 bit colour palette value and assign it to the current LED.
//      }
//      dist += beatsin8(10, 1, 4);                                              // Moving along the distance (that random number we started out with). Vary it a bit with a sine wave.
//      // In some sketches, I've used millis() instead of an incremented counter. Works a treat.
//      if (transitionTime == 0 or transitionTime == NULL) {
//        transitionTime = 0;
//      }
//      showleds();
//    }
//
//    //EFFECT RIPPLE
//    if (effect == "ripple") {
//      for (int i = 0; i < ledCount; i++) leds[i] = CHSV(bgcol++, 255, 15);  // Rotate background colour.
//      switch (step) {
//        case -1:                                                          // Initialize ripple variables.
//          center = random(ledCount);
//          colour = random8();
//          step = 0;
//          break;
//        case 0:
//          leds[center] = CHSV(colour, 255, 255);                          // Display the first pixel of the ripple.
//          step ++;
//          break;
//        case maxsteps:                                                    // At the end of the ripples.
//          step = -1;
//          break;
//        default:                                                             // Middle of the ripples.
//          leds[(center + step + ledCount) % ledCount] += CHSV(colour, 255, myfade / step * 2);   // Simple wrap from Marc Miller
//          leds[(center - step + ledCount) % ledCount] += CHSV(colour, 255, myfade / step * 2);
//          step ++;                                                         // Next step.
//          break;
//      }
//      if (transitionTime == 0 or transitionTime == NULL) {
//        transitionTime = 30;
//      }
//      showleds();
//    }
//
