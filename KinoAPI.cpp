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

  std::vector<String> listMacros() {
    return macroEngine.listMacros();
  }

  KinoError getMacroCount(size_t& out) {
    size_t ct = macroEngine.getMacroCount();
    out = ct;
    return KinoError::OK;
  }
  
  KinoError getMacroNameByIndex(size_t index, KinoVariant& out) {
    //String KinoMacroEngine::getMacroName(size_t index)
    String mName = macroEngine.getMacroName(index);
    if (mName == "") return KinoError::OutOfRange;
    out = KinoVariant::fromString(mName.c_str());
    return KinoError::OK;
  }

  KinoError getMacroIndexByName(const String& macroName, size_t& out) {
    out = macroEngine.getMacroIndex(macroName);
    if (out >= 0) return KinoError::OK;
    return KinoError::OutOfRange;
  }

  bool getMacroLines(const String& macroName, std::vector<String>& lines) {
    return macroEngine.getMacroLines(macroName, lines);
  }

  KinoError getMacroLineCount(const String& macroName, size_t &out) {
    //bool getMacroLineCount(const String& macroName, size_t& out);
    //bool getMacroLineByIndex(const String& macroName, size_t index, String& out);
    size_t ct;
    if (macroEngine.getMacroLineCount(macroName, ct)) {
      out = ct;
      return KinoError::OK;
    }
    out = 0;
    return KinoError::DeviceNotReady;
  }
  
  KinoError getMacroLineByIndex(const String& macroName, size_t index, String& out) {
    String tmpline;
    if (macroEngine.getMacroLineByIndex(macroName, index, tmpline)) {
      out = tmpline;
      return KinoError::OK;
    }
    out = "";
    return KinoError::OutOfRange;
  }

  KinoError getMacroLineByIndex(const char* macroName, size_t index, char* buf, size_t bufLen) {
    String tmpLine;
    if (macroEngine.getMacroLineByIndex(macroName, index, tmpLine)) {
      //out = tmpline;
      strncpy(buf, tmpLine.c_str(), bufLen);
      buf[bufLen-1] = '\0';
      return KinoError::OK;
    }
    //buf = "";
    buf[0] = '\0';
    return KinoError::OutOfRange;
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

  /*std::vector<String> getAvailableMacroCommands() {
    return macroEngine.getAvailableMacroCommands();
  }*/
  std::vector<const char*> getAvailableMacroCommands() {
    return macroEngine.getAvailableMacroCommands();
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

  bool createMacro(const String& macroName) {
    return macroEngine.createMacro(macroName);
  }

  bool renameMacro(const String& oldName, const String& newName) {
    return macroEngine.renameMacro(oldName, newName);
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
    return KinoDeviceFactory::getDeviceByName(deviceName);
  }

  std::vector<String> getDeviceNames() {
    return KinoDeviceFactory::getDeviceNames();
  }

  KinoError getDeviceCount(size_t& out) {
    out = (size_t)KinoDeviceFactory::getDeviceCount();
    return KinoError::OK;
  }

  KinoError getDeviceName(size_t devIndex, KinoVariant& out) {
    if (devIndex >= KinoDeviceFactory::getDeviceCount()) return KinoError::OutOfRange;
    String devName = KinoDeviceFactory::getDeviceNameByIndex(devIndex);
    if (devName.length()==0) return KinoError::InternalError;
    out = KinoVariant::fromString(devName.c_str());
    return KinoError::OK;
  }

  KinoError initDevice(const char* deviceName) {
    KinoDevice* d = KinoDeviceFactory::getDeviceByName(deviceName);
    if (!d) return KinoError::DeviceNotReady;
    KinoError e = d->init();
    return e;
  }

  void showMemory() {
    unsigned long freeHeap = ESP.getFreeHeap();
    uint16_t maxFreeBlockSize = ESP.getMaxFreeBlockSize();
    uint8_t heapFragmentation = ESP.getHeapFragmentation();
    Serial.print(F("FreeHeap: "));
    Serial.print(freeHeap);
    Serial.print(F(" | MaxBlock: "));
    Serial.print(maxFreeBlockSize);
    Serial.print(F(" | Fragmentation: "));
    Serial.println(heapFragmentation);
  }

  // This method will repeatedly cycle through all available devices and ask them
  // to tick(). Each device will handle its own ticks according to its tickInterval
  // A static int "runner" ensures that only ONE device will tick() in a loop() cycle
  //KinoError handleDeviceTicks(std::function<void(String devname)> cb) {
  KinoError handleDeviceTicks(std::function<void(const String& devname)> cb) {
    static int runner = 0;
    int devCount = KinoDeviceFactory::getDeviceCount();
    KinoDevice* d = KinoDeviceFactory::getDeviceByIndex(runner);
    KinoError tickresult = KinoError::DeviceUnknown;
    if (d) {
      tickresult = d->tick();
      if ((tickresult == KinoError::OK) && (cb != nullptr)) {
        cb(KinoDeviceFactory::getDeviceNameByIndex(runner));
      } 
    }
    runner++;
    if (runner >= devCount) runner=0;
    return tickresult;
  }


  KinoError getDeviceType(const char* deviceName, KinoVariant& out) {
    if (!deviceName) return KinoError::DeviceNotReady;
    KinoDevice* d = KinoDeviceFactory::getDeviceByName(deviceName);
    //if (!d) return KinoError::DeviceNotReady;
    if (!d) { out.setNone(); return KinoError::DeviceNotReady; }
    //out = KinoVariant::fromString(d->deviceType());
    out.setString(d->deviceType());
    return KinoError::OK;
  }
  
  KinoError getProperty(const char* deviceName, const char* property, KinoVariant& out) {
    if (!deviceName) return KinoError::DeviceNotReady;
    KinoDevice* d = KinoDeviceFactory::getDeviceByName(deviceName);
    if (!d) return KinoError::DeviceNotReady;
    /*KinoError e = d->get(property, out);
    return e;*/
    return d->get(property, out);
  }
  
  KinoError setProperty(const char* deviceName, const char* property, const KinoVariant& value) {
    if (!deviceName) return KinoError::DeviceNotReady;
    //KinoDevice* d = getDeviceByName(deviceName);
    KinoDevice* d = KinoDeviceFactory::getDeviceByName(deviceName);
    if (!d) return KinoError::DeviceNotReady;
    /*KinoError e = d->set(property, value);
    return e;*/
    return d->set(property, value);
  }
  
  KinoError getQueryCount(const char* deviceName, const char* property, uint16_t& out) {
    if (!deviceName) return KinoError::DeviceNotReady;
    //KinoDevice* d = getDeviceByName(deviceName);
    KinoDevice* d = KinoDeviceFactory::getDeviceByName(deviceName);
    if (!d) return KinoError::DeviceNotReady;
    return d->queryCount(property, out);
  }
  
  KinoError getQuery(const char* deviceName, const char* property, int index, KinoVariant& out) {
    if (!deviceName) return KinoError::DeviceNotReady;
    //KinoDevice* d = getDeviceByName(deviceName);
    KinoDevice* d = KinoDeviceFactory::getDeviceByName(deviceName);
    if (!d) return KinoError::DeviceNotReady;
    if (!property) return KinoError::InvalidProperty;
    /*KinoError e = d->query(property, index, out);
    return e;*/
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
    if (!deviceName) return KinoError::DeviceNotReady;
    //KinoDevice* d = getDeviceByName(deviceName);
    KinoDevice* d = KinoDeviceFactory::getDeviceByName(deviceName);
    if (!d) return KinoError::DeviceNotReady;
    if (!d->commit()) return KinoError::InternalError;
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
