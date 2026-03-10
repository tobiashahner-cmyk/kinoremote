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
  //static std::vector<String> getDeviceNames();
  static const bool getDeviceNameByIndex(int index, char* devName, size_t devNameLen);
  static int getDeviceCount();

private:
  struct DeviceEntry {
    char name[32];
    char className[32];
    KinoDevice* device = nullptr;
    bool initOk = false;
  };

  static std::vector<DeviceEntry> _devices;

  // JSON helpers
  static bool loadDevicesJson(JsonDocument& doc);
  static bool createDefaultDevicesFile();

  // Device-Erzeugung
  static KinoDevice* createDeviceFromJson(
    const char* className,
    JsonObject cfg
  );


  static const char DEFAULT_AVR[] PROGMEM;
  static const char DEFAULT_BEAMER[] PROGMEM;
  static const char DEFAULT_CANVAS[] PROGMEM;
  static const char DEFAULT_SOUND[] PROGMEM;
  static const char DEFAULT_HYPERION[] PROGMEM;
  static const char DEFAULT_HUEBRIDGE[] PROGMEM;
};
