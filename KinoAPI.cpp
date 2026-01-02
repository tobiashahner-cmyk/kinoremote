#include "KinoAPI.h"
#include "KinoDevice.h"
#include "YamahaReceiver.h"
#include "WLEDDevice.h"
#include "OptomaBeamer.h"
#include "HyperionDevice.h"
#include "HueBridge.h"
#include "KinoMacroEngine.h"

// Externe Geräte aus dem Sketch

extern KinoDevice* _yamaha;
extern KinoDevice* _canvas;
extern KinoDevice* _sound;
extern KinoDevice* _beamer;
extern KinoDevice* _hyperion;
extern KinoDevice* _hue;


namespace KinoAPI {
  bool isPause = false;
  bool isAlarm = false;

  // =================================================
  //            MACROS
  // =================================================

  // Macros sind ein Teil der API, also ist das hier genau richtig aufgehoben
  static KinoMacroEngine macroEngine;

  bool startMacroEngine() {
    if (macroEngine.isReady()) return true;
    return macroEngine.begin();
  }

  std::vector<String> listMacros() {
    return macroEngine.listMacros();
  }

  bool getMacroLines(const String& macroName, std::vector<String>& lines) {
    return macroEngine.getMacroLines(macroName, lines);
  }

  bool prepareMacroJsonString(const String& cmd, const String& deviceName, const String& action, const KinoVariant& value, String& jsonString) {
    char jsonActionString[128];
    String valStr;
    valStr.reserve(32);
    if      (value.type == KinoVariant::BOOL)      valStr += ((value.b) ? "true" : "false");
    else if (value.type == KinoVariant::INT)       valStr += value.i;
    else if (value.type == KinoVariant::FLOAT)     valStr += value.f;
    else if (value.type == KinoVariant::STRING)    valStr += "\"" + String(value.s) + "\"";
    else if (value.type == KinoVariant::RGB_COLOR) valStr += "["+String(value.color.r)+","+String(value.color.g)+","+String(value.color.b)+"]";
    else return false;
    snprintf(jsonActionString,128,"{\"cmd\":\"%s\",\"dev\":\"%s\",\"val\":{\"%s\":%s}}",
                    cmd.c_str(),
                    deviceName.c_str(),
                    action.c_str(),
                    valStr.c_str());
    jsonString = String(jsonActionString);
    return true;
  }


  bool addMacroCommand(const String& macroName, size_t index, const String& jsonActionElement) {
    return macroEngine.addCommand(macroName, index, jsonActionElement);
  }

  bool addMacroCommand(const String& macroName, size_t index, const String& cmd, const String& deviceName, const String& action, const KinoVariant& value) {
    String jsonActionString;
    jsonActionString.reserve(128);
    if (!prepareMacroJsonString(cmd, deviceName, action, value, jsonActionString)) return false;
    return macroEngine.addCommand(macroName, index, String(jsonActionString));
  }

  bool deleteMacroCommand(const String& macroName, size_t index) {
    return macroEngine.deleteCommand(macroName, index);
  }

  bool updateMacroCommand(const String& macroName, size_t index, const String& jsonActionElement) {
    return macroEngine.updateCommand(macroName, index, jsonActionElement);
  }

  bool updateMacroCommand(const String& macroName, size_t index, const String& cmd, const String& deviceName, const String& action, const KinoVariant& value) {
    String jsonActionString;
    jsonActionString.reserve(128);
    if (!prepareMacroJsonString(cmd, deviceName, action, value, jsonActionString)) return false;
    return macroEngine.updateCommand(macroName, index, String(jsonActionString));
  }
  
  bool executeMacro(const String& name,MacroFinishedCallback cb/*=nullptr*/) {
    return macroEngine.startMacro(name, cb);
  }

  bool testMacro(const String& name, MacroFinishedCallback cb/*=nullptr*/) {
    return macroEngine.testMacro(name, cb);
  }

  String getCurrentMacroName() {
    return macroEngine.getName();
  }

  bool handleMacroTicks() {
    macroEngine.tick();
    return (macroEngine.errorCount() == 0);
  }

  bool addOrUpdateMacro(const String& json) {
    return macroEngine.addOrUpdateMacro(json);
  }

  bool deleteMacro(const String& macroName) {
    return macroEngine.deleteMacro(macroName);
  }

  size_t getMacroErrorCount() {
    return macroEngine.errorCount();
  }
  
  const MacroError& getMacroError(size_t i) {
    return macroEngine.getError(i);
  }
  
  // neue API: ein grosser Getter und Setter, und eine Query
  // als dynamischer Wrapper für alle KinoDevices
  KinoDevice* getDeviceByName(const char* deviceName) {
    KinoDevice* d = nullptr;
    if (strcmp(deviceName, "yamaha") == 0) d = _yamaha;
    if (strcmp(deviceName, "canvas") == 0) d = _canvas;
    if (strcmp(deviceName, "sound" ) == 0) d = _sound;
    if (strcmp(deviceName, "beamer") == 0) d = _beamer;
    if (strcmp(deviceName, "hyperion")==0) d = _hyperion;
    if (strcmp(deviceName, "hue")    == 0) d = _hue;
    return d;
  }

  std::vector<String> getDeviceNames() {
    std::vector<String> deviceNames;
    deviceNames.push_back("yamaha");
    deviceNames.push_back("canvas");
    deviceNames.push_back("sound");
    deviceNames.push_back("beamer");
    deviceNames.push_back("hyperion");
    deviceNames.push_back("hue");
    return deviceNames;
  }

  KinoError initDevice(const char* deviceName) {
    if (!deviceName) return KinoError::DeviceNotReady;
    KinoDevice* d = getDeviceByName(deviceName);
    if (!d) return KinoError::DeviceNotReady;
    KinoError e = d->init();
    return e;
  }

  KinoError getDeviceType(const char* deviceName, KinoVariant& out) {
    if (!deviceName) return KinoError::DeviceNotReady;
    KinoDevice* d = getDeviceByName(deviceName);
    if (!d) return KinoError::DeviceNotReady;
    out = KinoVariant::fromString(d->deviceType());
    return KinoError::OK;
  }
  
  KinoError getProperty(const char* deviceName, const char* property, KinoVariant& out) {
    if (!deviceName) return KinoError::DeviceNotReady;
    KinoDevice* d = getDeviceByName(deviceName);
    if (!d) return KinoError::DeviceNotReady;
    KinoError e = d->get(property, out);
    return e;
  }
  
  KinoError setProperty(const char* deviceName, const char* property, const KinoVariant& value) {
    if (!deviceName) return KinoError::DeviceNotReady;
    KinoDevice* d = getDeviceByName(deviceName);
    if (!d) return KinoError::DeviceNotReady;
    KinoError e = d->set(property, value);
    return e;
  }
  
  KinoError getQueryCount(const char* deviceName, const char* property, uint16_t& out) {
    if (!deviceName) return KinoError::DeviceNotReady;
    KinoDevice* d = getDeviceByName(deviceName);
    if (!d) return KinoError::DeviceNotReady;
    return d->queryCount(property, out);
  }
  
  KinoError getQuery(const char* deviceName, const char* property, int index, KinoVariant& out) {
    if (!deviceName) return KinoError::DeviceNotReady;
    KinoDevice* d = getDeviceByName(deviceName);
    if (!d) return KinoError::DeviceNotReady;
    KinoError e = d->query(property, index, out);
    return e;
  }
  
  KinoError commit(const char* deviceName) {
    if (!deviceName) return KinoError::DeviceNotReady;
    KinoDevice* d = getDeviceByName(deviceName);
    if (!d) return KinoError::DeviceNotReady;
    if (!d->commit()) return KinoError::InternalError;
    return KinoError::OK;
  }


}
