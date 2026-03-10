#pragma once
#include <Arduino.h>
#include "KinoAPI.h"

#define ROT_CLK D7
#define ROT_DT D6

namespace rotary {

  struct RotaryState {
    char deviceName[32];
    char propKey[64];
    bool moving;
    bool changed;
    int pos;
  };

  // Diese müssen außerhalb des Structs bleiben, da die ISR schnell darauf zugreifen muss
  static volatile int virtPos = -500;
  static volatile int minval = -800;
  static volatile int maxval = -200;
  static volatile int steps = 5;
  static volatile int8_t lrsum = 0;
  static uint32_t lastchange = 0;

  static RotaryState currentRotary = { "", "", false, 0 };

  // Vorwärtsdeklaration für die ISR (wegen ICACHE_RAM_ATTR)
  void ICACHE_RAM_ATTR countRotation();

  void begin();
  bool setDeviceProperty(const char* deviceName, const char* propKey);
  const char* getDeviceName();
  const char* getPropKey(); 
  int getPosition();
  bool isMoving();
  bool hasChanged();
  void clearChanged();
  bool isRemapped();
  void clearRemapped();
  bool isInMenuMode();
  void setMenuMode(int minval, int maxval, int newpos);
  void clearMenuMode();
  void setPosition(int pos);
  void setRotaryState(int cur, int minv, int maxv, int steps);
  void handleRotation();

  inline void ICACHE_RAM_ATTR countRotation() {
    static uint8_t lrmem = 3;
    //static int8_t lrsum = 0;
    static const int8_t TRANSITION[] = {0,-1,1,14,1,0,14,-1,-1,14,0,1,14,1,-1,0};    
  
    int8_t l = digitalRead(ROT_CLK);
    int8_t r = digitalRead(ROT_DT);
  
    lrmem = ((lrmem & 0x03) << 2) + 2 * l + r;
    lrsum += TRANSITION[lrmem];
  
    if (lrsum % 4 != 0) return;
  
    if (lrsum == 4) {
      lrsum = 0;
      if (virtPos+steps <= maxval) virtPos+=steps;
    } else if (lrsum == -4) {
      lrsum = 0;
      if (virtPos-steps >= minval) virtPos-=steps;
    } else {
      lrsum = 0; // Reset bei ungültigen Sprüngen
    }
  }
}




// ALTE VERSION ALS BACKUP
/*#pragma once

#define ROT_CLK D7
#define ROT_DT D6

#define MINVAL 0
#define MAXVAL 100

namespace rotary {
  struct RotaryState {
    const char* deviceName;
    const char* propKey;
    int steps;
    bool moving;
    bool changed;
    int pos;
  };
  
  volatile int virtPos = 50;
  volatile int minval = MINVAL;
  volatile int maxval = MAXVAL;


  static uint32_t lastchange = 0;
  static RotaryState currentRotary = { "yamaha", "volume", 5, false, false, 0 };

  void begin() {
    pinMode(ROT_CLK, INPUT_PULLUP);
    pinMode(ROT_DT, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ROT_CLK),rotary::countRotation,CHANGE);
    attachInterrupt(digitalPinToInterrupt(ROT_DT), rotary::countRotation,CHANGE);
  }

  void setDevice(const char* devName) {
    currentRotary.deviceName = devName;
  }

  void setPropKey(const char* propKey) {
    currentRotary.propKey = propKey;
  }

  const char* getDeviceName() {
    return currentRotary.deviceName;
  }

  const char* getPropKey() {
    return currentRotary.propKey;
  }

  bool hasChanged() {
    return currentRotary.changed;
  }

  bool isMoving() {
    return currentRotary.moving;
  }

  int getPosition() {
    return currentRotary.pos;
  }

  void handleRotation() {
    rmin = minval;
    rmax = maxval;
    if (currentRotary.pos != virtPos) {
      lastchange = millis();
      currentRotary.pos = virtPos;
      currentRotary.moving = true;
      currentRotary.changed = true;
    }
    if (currentRotary.changed && (millis()-lastchange > 500)) {
      currentRotary.moving = false;
    }
  }
  
  void ICACHE_RAM_ATTR countRotation() {
    // this is a modified version of the rotary counting presented by Ralph S Beacon
    // in https://www.youtube.com/watch?v=sQNPAsZKnDw&t=1171s
    // which is in turn a modified implementation of https://www.pinteric.com/rotary.html
    // there is absolutely no debouncing, in fact all bounces are counted and will eventually sum up to a complete 4-step-cycle
  
    static uint8_t lrmem = 3;   // will be previous and current pin states, combined to a 4digit number
    static int8_t lrsum = 0;    // the local transition counter. during a full transition between 2 detent states, all lrmems will be added
                                    
    // every possible combination of previous and current pin states
    // the 14 is carefully selected!
    static int8_t TRANSITION[] = {0,-1,1,14,1,0,14,-1,-1,14,0,1,14,1,-1,0};    
  
    // read current pin states (could speed this up by port reading)
    int8_t l = digitalRead(ROT_CLK);    // 1 for HIGH, 0 for LOW
    int8_t r = digitalRead(ROT_DT);
  
    // move previous pin states 2 bits left and append current pin states
    lrmem = ((lrmem & 0x03)<<2)+2*l+r;
  
    // append this state change to the current counting
    lrsum += TRANSITION[lrmem];
  
    // check if encoder is in detent state. this happens when there were 4 counts. 4 counts ALWAYS sum up to a multiple of 4
    // (this is the reason for the 14)
    if (lrsum % 4 != 0) {
      return;
    }
  
    if (lrsum == 4) { // had counted 4*1, plus bounces which eliminated themselves
      lrsum = 0;
      virtPos++;
      if (virtPos > maxval) { virtPos = maxval; }
      return;
    }
    if (lrsum == -4) { // had 4* -1, plus bounces which eliminated themselves
      lrsum = 0;
      virtPos--;
      if (virtPos < minval) { virtPos = minval; }
      return;
    }
  
    // still here and no valid sum? reset for the next cycle
    // this will happen when there was a 14, which means a change of direction. Most likely because of too fast rotation
    lrsum = 0;
  }
  
  // zum Setzen neuer Werte von aussen
  void setRotaryPos(int cur, int minv, int maxv) {
    noInterrupts();
    virtPos = cur;
    minval = minv;
    maxval = maxv;
    interrupts();
    rotarypos = virtPos;
  }
}



*/
