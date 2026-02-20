#include "KinoAPI.h"
#include "KinoDevice.h"
#include "KinoDeviceFactory.h"
#include "KinoMacroEngine.h"



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

  KinoError getMacroCount(size_t& out) {
    size_t ct = macroEngine.getMacroCount();
    out = ct;
    return KinoError::OK;
  }
  
  KinoError getMacroNameByIndex(size_t index, KinoVariant& out) {
    return macroEngine.getMacroNameByIndex(index, out);
  }

  KinoError getMacroIndexByName(const char* macroName, size_t& out) {
    out = macroEngine.getMacroIndex(macroName);
    if (out != (size_t)-1) return KinoError::OK;
    return KinoError::OutOfRange;
  }

  size_t getMacroLineCount(const char* macroName) {
    return macroEngine.getMacroLineCount(macroName);
  }
  
  KinoError getMacroLineByIndex(const char* macroName, size_t index, char* buf, size_t bufLen) {
    if (macroEngine.getMacroLineByIndex(macroName, index, buf, bufLen)) {
      return KinoError::OK;
    }
    if (bufLen > 0) buf[0] = '\0';
    return KinoError::OutOfRange;
  }


  bool prepareMacroJsonString(const char* cmd, const char* deviceName, const char* action, const KinoVariant& value, char* json, size_t jsonLen) {
    char valStr[64]; // Puffer etwas vergrößert für Sicherheit bei RGB/Strings

    if      (value.type == KinoVariant::BOOL)       snprintf(valStr, sizeof(valStr), "%s", (value.b) ? "true" : "false");
    else if (value.type == KinoVariant::INT)        snprintf(valStr, sizeof(valStr), "%d", value.i);
    else if (value.type == KinoVariant::FLOAT)      snprintf(valStr, sizeof(valStr), "%.2f", value.f);
    else if (value.type == KinoVariant::STRING)     snprintf(valStr, sizeof(valStr), "\"%s\"", value.s);
    else if (value.type == KinoVariant::RGB_COLOR)  snprintf(valStr, sizeof(valStr), "[%d,%d,%d]", value.color.r, value.color.g, value.color.b);
    else return false;

    int written = snprintf(json, jsonLen, "{\"cmd\":\"%s\",\"dev\":\"%s\",\"val\":{\"%s\":%s}}",
                           cmd, deviceName, action, valStr);
    
    // Prüfen, ob der String in den Zielpuffer gepasst hat
    return (written > 0 && (size_t)written < jsonLen);
  }

  size_t getMacroCommandCount() {
    return macroEngine.getMacroCommandCount();
  }
  
  KinoError getMacroCommand(size_t index, char* out, size_t outLen) {
    return macroEngine.getMacroCommand(index, out, outLen) ? KinoError::OK : KinoError::OutOfRange;
  }

  bool addMacroCommand(const char* macroName, size_t index, const char* jsonActionElement) {
    return macroEngine.addCommand(macroName, index, jsonActionElement);
  }

  bool addMacroCommand(const char* macroName, size_t linenr, const char* cmd, const char* devName, const char* action, const KinoVariant& value) {
    char json[128];
    if (!prepareMacroJsonString(cmd, devName, action, value, json, sizeof(json))) return false;
    return macroEngine.addCommand(macroName, linenr, json);
  }

  bool deleteMacroCommand(const char* macroName, size_t index) {
    return macroEngine.deleteCommand(macroName, index);
  }

  bool updateMacroCommand(const char* macroName, size_t index, const char* jsonActionElement) {
    return macroEngine.updateCommand(macroName, index, jsonActionElement);
  }

  bool updateMacroCommand(const char* macroName, size_t index, const char* cmd, const char* deviceName, const char* action, const KinoVariant& value) {
    char jsonActionString[128];
    if (!prepareMacroJsonString(cmd, deviceName, action, value, jsonActionString, sizeof(jsonActionString))) return false;
    return macroEngine.updateCommand(macroName, index, jsonActionString);
  }
  
  bool executeMacro(const char* name,MacroFinishedCallback cb/*=nullptr*/) {
    return macroEngine.startMacro(name, cb);
  }

  bool testMacro(const char* name, MacroFinishedCallback cb/*=nullptr*/) {
    return macroEngine.testMacro(name, cb);
  }

  bool getCurrentMacroName(char* out, size_t outLen) {
    macroEngine.getName(out, outLen);
    return (strlen(out)>0);
  }

  bool handleMacroTicks() {
    macroEngine.tick();
    return (macroEngine.errorCount() == 0);
  }

  bool createMacro(const char* macroName) {
    return macroEngine.createMacro(macroName);
  }

  bool renameMacro(const char* oldName, const char* newName) {
    return macroEngine.renameMacro(oldName, newName);
  }

  bool addOrUpdateMacro(const char* json) {
    return macroEngine.addOrUpdateMacro(json);
  }

  bool deleteMacro(const char* macroName) {
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
    return KinoDeviceFactory::getDeviceByName(deviceName);
  }

/*
  std::vector<String> getDeviceNames() {
    return KinoDeviceFactory::getDeviceNames();
  }
*/
  KinoError getDeviceCount(size_t& out) {
    out = (size_t)KinoDeviceFactory::getDeviceCount();
    return KinoError::OK;
  }

  KinoError getDeviceName(size_t devIndex, KinoVariant& out) {
    if (devIndex >= KinoDeviceFactory::getDeviceCount()) return KinoError::OutOfRange;
    char devName[32];
    if (!KinoDeviceFactory::getDeviceNameByIndex(devIndex, devName, sizeof(devName))) return KinoError::InternalError;
    out.setString(devName);
    return KinoError::OK;
  }

  KinoError initDevice(const char* deviceName) {
    Serial.print(F("trying to initialize device "));
    Serial.println(deviceName);
    KinoDevice* d = KinoDeviceFactory::getDeviceByName(deviceName);
    delay(500);
    yield();
    if (!d) { Serial.println("device not found"); return KinoError::DeviceNotReady;}
    KinoError e = d->init();
    Serial.println(kinoErrorToString(e));
    return e;
  }

  void showMemory() {
    unsigned long freeHeap = ESP.getFreeHeap();
    uint16_t maxFreeBlockSize = ESP.getMaxFreeBlockSize();
    uint8_t heapFragmentation = ESP.getHeapFragmentation();
    unsigned long freeStack = ESP.getFreeContStack();
    Serial.print(F("FreeHeap: "));
    Serial.print(freeHeap);
    Serial.print(F(" | MaxBlock: "));
    Serial.print(maxFreeBlockSize);
    Serial.print(F(" | Fragmentation: "));
    Serial.print(heapFragmentation);
    Serial.print(F(" | Stack: "));
    Serial.println(freeStack);
  }

  // This method will repeatedly cycle through all available devices and ask them
  // to tick(). Each device will handle its own ticks according to its tickInterval
  // A static int "runner" ensures that only ONE device will tick() in a loop() cycle
  //KinoError handleDeviceTicks(std::function<void(String devname)> cb) {
  KinoError handleDeviceTicks(std::function<void(const char* devname, bool success)> cb) {
    static int runner = 0;
    int devCount = KinoDeviceFactory::getDeviceCount();
    KinoDevice* d = KinoDeviceFactory::getDeviceByIndex(runner);
    KinoError tickresult = KinoError::DeviceUnknown;
    if (d) {
      tickresult = d->tick();
      if (tickresult != KinoError::NothingToDo) {
         char devName[32];
         KinoDeviceFactory::getDeviceNameByIndex(runner, devName, sizeof(devName));
         cb(devName, (tickresult == KinoError::OK));
      }
    }
    runner++;
    if (runner >= devCount) runner=0;
    return tickresult;
  }

  KinoError getJsonUpdates(JsonDocument& doc) {
    static size_t lastIdx = 0;
    size_t total = KinoDeviceFactory::getDeviceCount();
    
    for (size_t i = 0; i < total; i++) {
        size_t idx = (lastIdx + i) % total;
        KinoDevice* d = KinoDeviceFactory::getDeviceByIndex(idx);
        if (!d) return KinoError::InternalError;
        char devName[32];
        if (!KinoDeviceFactory::getDeviceNameByIndex(idx, devName, sizeof(devName))) return KinoError::InternalError;
        JsonObject root = doc.to<JsonObject>();
        if (d && d->getStatusUpdate(devName, root)) {
            lastIdx = (idx + 1) % total;
            return KinoError::OK;
        }
    }
    return KinoError::NothingToDo;
  }


  KinoError getDeviceType(const char* deviceName, KinoVariant& out) {
    if (!deviceName) return KinoError::DeviceNotReady;
    KinoDevice* d = KinoDeviceFactory::getDeviceByName(deviceName);
    if (!d) { out.setNone(); return KinoError::DeviceNotReady; }
    out.setString(d->deviceType());
    return KinoError::OK;
  }
  
  KinoError getProperty(const char* deviceName, const char* property, KinoVariant& out) {
    if (!deviceName) return KinoError::DeviceNotReady;
    KinoDevice* d = KinoDeviceFactory::getDeviceByName(deviceName);
    if (!d) return KinoError::DeviceNotReady;
    return d->get(property, out);
  }
  
  KinoError setProperty(const char* deviceName, const char* property, const KinoVariant& value) {
    if (!deviceName) return KinoError::DeviceNotReady;
    KinoDevice* d = KinoDeviceFactory::getDeviceByName(deviceName);
    if (!d) return KinoError::DeviceNotReady;
    return d->set(property, value);
  }
  
  KinoError getQueryCount(const char* deviceName, const char* property, uint16_t& out) {
    if (!deviceName) return KinoError::DeviceNotReady;
    KinoDevice* d = KinoDeviceFactory::getDeviceByName(deviceName);
    if (!d) return KinoError::DeviceNotReady;
    return d->queryCount(property, out);
  }
  
  KinoError getQuery(const char* deviceName, const char* property, int index, KinoVariant& out) {
    if (!deviceName) return KinoError::DeviceNotReady;
    KinoDevice* d = KinoDeviceFactory::getDeviceByName(deviceName);
    if (!d) return KinoError::DeviceNotReady;
    if (!property) return KinoError::InvalidProperty;
    return d->query(property, index, out);
  }

  KinoError needsCommit(const char* deviceName, bool& out) {
    if (!deviceName) return KinoError::DeviceNotReady;
    KinoDevice* d = KinoDeviceFactory::getDeviceByName(deviceName);
    if (!d) return KinoError::DeviceNotReady;
    out = d->needsCommit();
    return KinoError::OK;
  }
  
  KinoError commit(const char* deviceName) {
    bool ok = true;
    if (!deviceName) return KinoError::DeviceNotReady;
    KinoDevice* d = KinoDeviceFactory::getDeviceByName(deviceName);
    if (!d) return KinoError::DeviceNotReady;
    ok = d->commit();
    if (!ok) return KinoError::InternalError;
    return KinoError::OK;
  }

  KinoError getPropertyCount(const char* deviceName, size_t &out) {
    KinoDevice* d = KinoDeviceFactory::getDeviceByName(deviceName);
    if (!d) return KinoError::DeviceNotReady;
    out = d->getPropertyCount();
    return KinoError::OK;
  }

  KinoError getPropertyInfo(const char* deviceName, size_t index, const KinoPropertyInfo*& out) {
    KinoDevice* d = KinoDeviceFactory::getDeviceByName(deviceName);
    if (!d) return KinoError::DeviceNotReady;
  
    out = d->getPropertyInfo(index);
    if (!out) return KinoError::OutOfRange;
  
    return KinoError::OK;
  }

  KinoError getPropertyInfoByName(const char* deviceName, const char* propertyName, const KinoPropertyInfo*& out) {
    KinoDevice* d = KinoDeviceFactory::getDeviceByName(deviceName);
    if (!d) return KinoError::DeviceNotReady;
    size_t propCount = d->getPropertyCount();
    const KinoPropertyInfo* prop = nullptr;
    for (int i=0; i<propCount; i++) {
      prop = d->getPropertyInfo(i);
      if (strcmp(prop->key, propertyName)==0) {
        out = prop;
        return KinoError::OK;
      }
    }
    return KinoError::OutOfRange;
  }



  bool isInternal(const KinoPropertyInfo*& prop) {
    return ((prop->flags & KinoPropertyFlags::Prop_Internal) > 0);
  }
  bool isStatus(const KinoPropertyInfo*& prop) {
    return ((prop->flags & KinoPropertyFlags::Prop_Status) > 0);
  }
  bool hasValue(const KinoPropertyInfo*& prop) {
    return ((prop->flags & KinoPropertyFlags::Prop_Read) > 0);
  }
  bool isWritable(const KinoPropertyInfo*& prop) {
    return ((prop->flags & KinoPropertyFlags::Prop_Write) > 0);
  }
  bool hasLabel(const KinoPropertyInfo*& prop) {
    return ((prop->flags & KinoPropertyFlags::Prop_hasLabel) > 0);
  }
  bool hasQuery(const KinoPropertyInfo*& prop) {
    return ((prop->flags & KinoPropertyFlags::Prop_Query) > 0);
  }
  bool hasParam(const KinoPropertyInfo*& prop) {
    return ((prop->flags & KinoPropertyFlags::Prop_hasParams) > 0);
  }
}
