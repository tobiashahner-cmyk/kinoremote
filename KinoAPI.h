#pragma once
#include <Arduino.h>
#include "KinoDevice.h"
#include "KinoMacroEngine.h"

namespace KinoAPI {
  // Macros
  bool startMacroEngine();
  bool handleMacroTicks();
  bool executeMacro(const String& name, MacroFinishedCallback cb=nullptr);
  bool testMacro(const String& name, MacroFinishedCallback cb=nullptr);
  String getCurrentMacroName();
  bool addOrUpdateMacro(const String& json);
  bool deleteMacro(const String& macroName);
  std::vector<String> listMacros();
  bool getMacroLines(const String& macroName, std::vector<String>& lines);
  bool addMacroCommand(const String& macroName, size_t index, const String& jsonActionElement);
  bool addMacroCommand(const String& macroName, size_t index, const String& cmd, const String& deviceName, const String& action, const KinoVariant& value);
  bool deleteMacroCommand(const String& macroName, size_t index);
  bool updateMacroCommand(const String& macroName, size_t index, const String& jsonActionElement);
  bool updateMacroCommand(const String& macroName, size_t index, const String& cmd, const String& deviceName, const String& action, const KinoVariant& value);
  size_t getMacroErrorCount();
  const MacroError& getMacroError(size_t i);

  // neue API: ein grosser Getter und Setter, und eine Query
  // als dynamischer Wrapper für alle KinoDevices
  std::vector<String> getDeviceNames();
  KinoError initDevice(const char* deviceName);
  KinoError getDeviceType(const char* deviceName, KinoVariant& out);
  KinoError getProperty(const char* deviceName, const char* property, KinoVariant& out);
  KinoError setProperty(const char* deviceName, const char* property, const KinoVariant& value);
  KinoError getQueryCount(const char* deviceName, const char* property, uint16_t& out);
  KinoError getQuery(const char* deviceName, const char* property, int index, KinoVariant& out);
  KinoError commit(const char* deviceName);

}
