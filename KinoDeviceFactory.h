#pragma once

#include <Arduino.h>
#include <vector>
#include <ArduinoJson.h>
#include "KinoDevice.h"



class KinoDeviceFactory {
public:
  // Initialisiert alle Devices aus /devices.json
  static bool initDevices();

  // Zugriff für KinoAPI
  static KinoDevice* getDeviceByName(const char* name);
  static KinoDevice* getDeviceByIndex(int index);
  static std::vector<String> getDeviceNames();
  static const String& getDeviceNameByIndex(int index);
  static int getDeviceCount();

private:
  struct DeviceEntry {
    String name;
    String className;
    KinoDevice* device = nullptr;
    bool initOk = false;
  };

  static std::vector<DeviceEntry> _devices;

  // JSON helpers
  static bool loadDevicesJson(DynamicJsonDocument& doc);
  static bool createDefaultDevicesFile();

  // Device-Erzeugung
  static KinoDevice* createDeviceFromJson(
    const String& className,
    JsonObject cfg
  );


  static const char DEFAULT_AVR[] PROGMEM;
  static const char DEFAULT_BEAMER[] PROGMEM;
  static const char DEFAULT_CANVAS[] PROGMEM;
  static const char DEFAULT_SOUND[] PROGMEM;
  static const char DEFAULT_HYPERION[] PROGMEM;
  static const char DEFAULT_HUEBRIDGE[] PROGMEM;
};








/*#pragma once

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
*/
