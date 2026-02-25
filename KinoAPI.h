#pragma once
#include <Arduino.h>
#include "KinoDevice.h"
#include "KinoMacroEngine.h"

namespace KinoAPI {
  // Macros
  KinoError getMacroCount(size_t& out);
  KinoError getMacroNameByIndex(size_t index, KinoVariant& out);
  KinoError getMacroIndexByName(const char* macroName, size_t& out);
  size_t getMacroLineCount(const char* macroName);
  KinoError getMacroLineByIndex(const char* macroName, size_t index, char* buf, size_t bufLen);
  size_t getMacroCommandCount();
  KinoError getMacroCommand(size_t index, char* out, size_t outLen);
  bool startMacroEngine();
  bool handleMacroTicks();
  bool executeMacro(const char* name, MacroFinishedCallback cb=nullptr, MacroErrorCallback e=nullptr);
  bool testMacro(const char* name, MacroFinishedCallback cb=nullptr, MacroErrorCallback e=nullptr);
  bool getCurrentMacroName(char* out, size_t outLen);
  bool createMacro(const char* macroName);
  bool addOrUpdateMacro(const char* json);
  bool deleteMacro(const char* macroName);
  bool renameMacro(const char* oldName, const char* newName);
  bool prepareMacroJsonString(const char* cmd, const char* devName, const char* action, const KinoVariant& value, char* json, size_t jsonLen);
  bool addMacroCommand(const char* macroName, size_t index, const char* jsonActionElement);
  bool addMacroCommand(const char* macroName, size_t index, const char* cmd, const char* deviceName, const char* action, const KinoVariant& value);
  bool deleteMacroCommand(const char* macroName, size_t index);
  bool updateMacroCommand(const char* macroName, size_t index, const char* jsonActionElement);
  bool updateMacroCommand(const char* macroName, size_t index, const char* cmd, const char* deviceName, const char* action, const KinoVariant& value);
  size_t getMacroErrorCount();
  const MacroError& getMacroError(size_t i);

  // neue API: ein grosser Getter und Setter, und eine Query
  // als dynamischer Wrapper für alle KinoDevices
  KinoError getDeviceCount(size_t& out);
  KinoError getDeviceName(size_t devIndex, KinoVariant& out);
  //std::vector<String> getDeviceNames();
  KinoError initDevice(const char* deviceName);
  void showMemory();
  bool startTicks(int ti=0);
  bool stopTicks();
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
