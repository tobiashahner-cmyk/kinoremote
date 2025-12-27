#pragma once

#include <Arduino.h>

#include "YamahaReceiver.h"
#include "WLEDDevice.h"
#include "HueBridge.h"
#include "HyperionDevice.h"
#include "OptomaBeamer.h"


// --------------------
// Konfigurations-Typen
// --------------------

struct YamahaConfig {
  const char* ip;
};

struct WLEDConfig {
  const char* ip;
};

struct HueConfig {
  const char* ip;
  const char* username;
};

struct HyperionConfig {
  const char* ip;
};

struct OptomaConfig {
  const char* ip;
  uint8_t beamerId;
};

// --------------------
// Factory
// --------------------

class KinoDeviceFactory {
  public:
      static YamahaReceiver* createYamaha(const YamahaConfig& cfg);
      static WLEDDevice*     createWLED(const WLEDConfig& cfg);
      static HueBridge*      createHue(const HueConfig& cfg);
      static OptomaBeamer*   createOptoma(const OptomaConfig& cfg);
      static HyperionDevice* createHyperion(const HyperionConfig& cfg);
};
