#include "rotary.h"
#include "KinoAPI.h"

namespace rotary {
  // helper bool for detecting re-mapping
  bool remapped = false;
  bool menumode = false;
  
  void begin() {
    pinMode(ROT_CLK, INPUT_PULLUP);
    pinMode(ROT_DT, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ROT_CLK), countRotation, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ROT_DT), countRotation, CHANGE);
  }

  bool setDeviceProperty(const char* deviceName, const char* propKey) {
    Serial.print(F("Setting rotary up to control "));
    Serial.print(deviceName);
    Serial.print(F("."));
    Serial.println(propKey);
    // hole die passende KinoPropertyInfo aus KinoAPI
    const KinoPropertyInfo* pi;
    KinoError err = KinoAPI::getPropertyInfoByName(deviceName, propKey, pi);
    int minValue = 0;
    int maxValue = 100;
    int stpValue = 1;
    if (err != KinoError::OK) {
      // Keine passende KinoPropertyInfo vorhanden. Suche nach einem evtl passenden Parameter
      bool paramFound = false;
      size_t propCount;
      err = KinoAPI::getPropertyCount(deviceName, propCount);
      for (int i=0; i<propCount; i++) { // Gehe alle PropertyInfos durch
        KinoAPI::getPropertyInfo(deviceName, i, pi);
        if (!KinoAPI::hasParam(pi)) continue; // Keine Parameter zu dieser Property
        if (KinoAPI::hasQuery(pi)) {  // gehe alle Optionen für diese Property durch
          // gehe alle optionen durch und durchsuche ihre Parameter
          uint16_t optionCount;
          KinoVariant option;
          KinoVariant paramGetsetPath;
          KinoAPI::getQueryCount(deviceName, pi->key, optionCount);
          for (int opt=0; opt < optionCount; opt++) {
            KinoAPI::getQuery(deviceName, pi->key, opt, option);
            char path[64];
            snprintf(path, sizeof(path), "%s/%s/param", pi->key, option.c_str());
            uint16_t paramCount;
            KinoAPI::getQueryCount(deviceName, path, paramCount);
            for (int pc=0; pc < paramCount; pc++) {
              snprintf(path, sizeof(path), "%s/%s/param/%d", pi->key, option.c_str(), pc);
              path[sizeof(path)-1] = '\0';
              KinoAPI::getProperty(deviceName, path, paramGetsetPath);
              if (strcmp(paramGetsetPath.c_str(), propKey)==0) {
                KinoVariant tmp;
                snprintf(path, sizeof(path), "%s/%s/param/%d/access", pi->key, option.c_str(), pc);
                path[sizeof(path)-1] = '\0';
                KinoAPI::getProperty(deviceName, path, tmp);
                if (tmp.asInt() < 2) {
                  Serial.println(F("property is not writable"));
                  return false;
                }
                paramFound = true;
                snprintf(path, sizeof(path), "%s/%s/param/%d/minvalue", pi->key, option.c_str(), pc);
                path[sizeof(path)-1] = '\0';
                KinoAPI::getProperty(deviceName, path, tmp);
                minValue = tmp.asInt();
                snprintf(path, sizeof(path), "%s/%s/param/%d/maxvalue", pi->key, option.c_str(), pc);
                path[sizeof(path)-1] = '\0';
                KinoAPI::getProperty(deviceName, path, tmp);
                maxValue = tmp.asInt();
                snprintf(path, sizeof(path), "%s/%s/param/%d/valuestep", pi->key, option.c_str(), pc);
                path[sizeof(path)-1] = '\0';
                KinoAPI::getProperty(deviceName, path, tmp);
                stpValue = tmp.asInt();
              }
            }
          }
        }
      }
      if (!paramFound) {
        Serial.println(F("could not get PropertyInfo")); 
        return false; 
      }
    } else {
      minValue = pi->minValue.value_or(0);
      maxValue = pi->maxValue.value_or(100);
      stpValue = pi->valueStp.value_or(1);
    }
    KinoVariant val;
    err = KinoAPI::getProperty(deviceName, propKey, val);
    if (err != KinoError::OK) {Serial.println(F("could not read current value")); return false;}
    if (val.type != KinoVariant::INT) { Serial.println(F("rotary can only control integer values")); return false; }
    strlcpy(currentRotary.deviceName, deviceName, sizeof(currentRotary.deviceName));
    strlcpy(currentRotary.propKey, propKey, sizeof(currentRotary.propKey));
    currentRotary.pos = val.asInt();
    noInterrupts();
    minval = minValue;
    maxval = maxValue;
    steps  = stpValue;
    virtPos= val.asInt();
    lrsum = 0;
    interrupts();
    remapped = true;
    currentRotary.changed = true;
    menumode = false;
    return true;
  }

  const char* getDeviceName() {
    return currentRotary.deviceName;
  }

  const char* getPropKey() {
    return currentRotary.propKey;
  }

  int getPosition() {
    return currentRotary.pos;
  }

  bool isMoving() {
    return currentRotary.moving;
  }

  bool hasChanged() {
    return currentRotary.changed;
  }

  void clearChanged() {
    currentRotary.changed = false;
  }

  bool isRemapped() {
    return remapped;
  }

  void clearRemapped() {
    remapped = false;
  }

  void setPosition(int pos) {
    noInterrupts();
    virtPos = pos;
    lrsum = 0;
    interrupts();
    currentRotary.pos = pos;
    currentRotary.moving = false;
    currentRotary.changed = true;
  }

  void setRotaryState(int cur, int minv, int maxv, int steps) {
    noInterrupts();
    virtPos = cur;
    minval  = minv;
    maxval  = maxv;
    steps = steps;
    lrsum = 0;    
    interrupts();
  
    currentRotary.pos = cur;
    currentRotary.moving = false;
  }

  bool isInMenuMode() {
    return menumode;
  }

  void setMenuMode(int minvalue, int maxvalue, int currentpos) {
    menumode = true;
    noInterrupts();
    virtPos = currentpos;
    minval = minvalue;
    maxval = maxvalue;
    steps = 1;
    lrsum = 0;
    interrupts();
    currentRotary.pos = currentpos;
    currentRotary.moving = false;
    currentRotary.changed = true;
  }

  void clearMenuMode() {
    menumode = false;
    // re-init the values for direct property control
    setDeviceProperty(currentRotary.deviceName, currentRotary.propKey);
  }

  void handleRotation() {
    if ((currentRotary.deviceName[0]=='\0') || (currentRotary.propKey[0]=='\0')) {
      //Serial.println(F("[rotary::handleRotation] rotary not bound to any property"));
      return;
    }
    int vPos;
    noInterrupts();
    vPos = virtPos;
    interrupts();

    if (currentRotary.pos != vPos) {
      lastchange = millis();
      currentRotary.pos = vPos;
      currentRotary.moving = true;
      currentRotary.changed = true;
    }

    // Wenn 500ms keine Bewegung mehr, setze den aktuellen Wert
    if (currentRotary.moving && (millis() - lastchange > 500) && !menumode) {
      Serial.print(F("Setting ")); Serial.print(currentRotary.deviceName); Serial.print(F(".")); Serial.print(currentRotary.propKey); Serial.print(F(" to ")); Serial.println(currentRotary.pos);
      currentRotary.moving = false;
      KinoVariant val = KinoVariant::fromInt(currentRotary.pos);
      KinoError err = KinoAPI::setProperty(currentRotary.deviceName, currentRotary.propKey, val);
      KinoAPI::commit(currentRotary.deviceName);
      currentRotary.changed = false;
      if (err != KinoError::OK) {
        Serial.println(F("failed to set, resyncing"));
        // something went wrong, resync rotary position
        KinoAPI::getProperty(currentRotary.deviceName, currentRotary.propKey, val);
        rotary::setPosition(val.asInt());
      }
    }

    if (!currentRotary.moving && !currentRotary.changed && !menumode) {
      // keine Änderung am Encoder, aber vielleicht von ausserhalb?
      KinoVariant val;
      KinoError err = KinoAPI::getProperty(currentRotary.deviceName, currentRotary.propKey, val);
      if (err != KinoError::OK) return;
      if (val.asInt() != currentRotary.pos) {
        setPosition(val.asInt());
      }
    }
  }
}
