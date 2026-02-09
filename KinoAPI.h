#pragma once
#include <Arduino.h>
#include "KinoDevice.h"
#include "KinoMacroEngine.h"

namespace KinoAPI {
  // Macros
  KinoError getMacroCount(size_t& out);
  KinoError getMacroNameByIndex(size_t index, KinoVariant& out);
  KinoError getMacroIndexByName(const String& macroName, size_t& out);
  KinoError getMacroLineCount(const String& macroName, size_t &out);
  KinoError getMacroLineByIndex(const String& macroName, size_t index, String& out);
  KinoError getMacroLineByIndex(const char* macroName, size_t index, char* buf, size_t bufLen);
  //std::vector<String> getAvailableMacroCommands();
  std::vector<const char*> getAvailableMacroCommands();
  size_t getMacroCommandCount();
  KinoError getMacroCommand(size_t index, char* out, size_t outLen);
  bool startMacroEngine();
  bool handleMacroTicks();
  bool executeMacro(const String& name, MacroFinishedCallback cb=nullptr);
  bool testMacro(const String& name, MacroFinishedCallback cb=nullptr);
  String getCurrentMacroName();
  bool createMacro(const String& macroName);
  bool addOrUpdateMacro(const String& json);
  bool deleteMacro(const String& macroName);
  bool renameMacro(const String& oldName, const String& newName);
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
  KinoError getDeviceCount(size_t& out);
  KinoError getDeviceName(size_t devIndex, KinoVariant& out);
  std::vector<String> getDeviceNames();
  KinoError initDevice(const char* deviceName);
  void showMemory();
  KinoError handleDeviceTicks(std::function<void(const char* devname, bool success)> cb = nullptr);
  KinoError getDeviceType(const char* deviceName, KinoVariant& out);
  KinoError getProperty(const char* deviceName, const char* property, KinoVariant& out);
  KinoError setProperty(const char* deviceName, const char* property, const KinoVariant& value);
  KinoError getQueryCount(const char* deviceName, const char* property, uint16_t& out);
  KinoError getQuery(const char* deviceName, const char* property, int index, KinoVariant& out);
  KinoError needsCommit(const char* deviceName, bool& out);
  KinoError commit(const char* deviceName);
  KinoError getPropertyCount(const char* deviceName, size_t &out);
  KinoError getPropertyInfo(const char* deviceName, size_t index, const KinoPropertyInfo*& out);
  KinoError getPropertyInfoByName(const char* deviceName, const char* propertyName, const KinoPropertyInfo*& out);
  bool hasLabel(const KinoPropertyInfo*& prop);
  bool hasQuery(const KinoPropertyInfo*& prop);
  bool hasParam(const KinoPropertyInfo*& prop);
  bool hasValue(const KinoPropertyInfo*& prop);
  bool isWritable(const KinoPropertyInfo*& prop);
  bool isInternal(const KinoPropertyInfo*& prop);
  bool isStatus(const KinoPropertyInfo*& prop);

  KinoError getJsonUpdates(JsonDocument& doc);

}
