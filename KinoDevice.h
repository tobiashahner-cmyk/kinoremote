#pragma once
#include <Arduino.h>

#include "KinoError.h"
#include "KinoVariant.h"

struct KinoPropertyOption {
  const char* id;     // stabil, API-intern (z.B. "HDMI_1")
  const char* value;  // Wert für set() am echten Gerät ("HDMI 1")
  const char* label;  // Anzeige ("BluRay")
};

struct KinoPropertyParam {
  const char* getsetPath;
  const char* label;
  int access;
  std::optional<int> minvalue;
  std::optional<int> maxvalue;
  std::optional<int> valuestep;
};

enum KinoPropertyFlags : uint16_t {
  Prop_None        = 0,
  Prop_Read        = 1 << 0,
  Prop_Write       = 1 << 1,
  Prop_Query       = 1 << 2,   // hat query/queryCount
  Prop_Internal    = 1 << 3,   // nicht für UI
  Prop_Status      = 1 << 4,   // Anzeige-only
  Prop_hasLabel    = 1 << 14,  // Es gibt Anzeigelabel (für jede Option, falls vorhanden (Prop_Query)
  Prop_hasParams   = 1 << 15
};

struct KinoPropertyInfo {
  const char* key;          // API-Key (z.B. "input" => liefert bei Query sowas wie "HDMI1")
  const char* label;        // UI-Label (z.B. "Input")
  uint16_t flags;

  // optional
  std::optional<int> minValue;
  std::optional<int> maxValue;
  std::optional<int> valueStp;
};



class KinoDevice {
public:
    virtual ~KinoDevice() = default;

    virtual const char* deviceType() const = 0;

    virtual KinoError init() {
      return KinoError::OK;
    }

    virtual KinoError tick() {
      return KinoError::NothingToDo;
    }

    virtual KinoError get(const char* property, KinoVariant& out) {
        (void)property;
        (void)out;
        return KinoError::PropertyNotSupported;
    }

    virtual KinoError set(const char* property, const KinoVariant& value) {
        (void)property;
        (void)value;
        return KinoError::PropertyNotSupported;
    }

    virtual KinoError queryCount(const char* property, uint16_t &out ) {
        return KinoError::PropertyNotSupported;
    }

    virtual KinoError query(const char* property, uint16_t index, KinoVariant& out) {
        return KinoError::PropertyNotSupported;
    }

    virtual bool needsCommit() {
      return false;
    }

    virtual bool commit() {
      return true;
    }

    // Properties
    virtual size_t getPropertyCount() const {
      return 0;
    }

    virtual const KinoPropertyInfo* getPropertyInfo(size_t index) const {
      return nullptr;
    }

    virtual bool hasProperty(const char* id) {
      return false;
    }

    void showMemory() {
      unsigned long freeHeap = ESP.getFreeHeap();
      uint16_t maxFreeBlockSize = ESP.getMaxFreeBlockSize();
      uint8_t heapFragmentation = ESP.getHeapFragmentation();
      unsigned long freeStack = ESP.getFreeContStack();
      Serial.print(this->deviceType());
      Serial.print(F("   FreeHeap: "));
      Serial.print(freeHeap);
      Serial.print(F(" | MaxBlock: "));
      Serial.print(maxFreeBlockSize);
      Serial.print(F(" | Fragmentation: "));
      Serial.print(heapFragmentation);
      Serial.print(F(" | Stack: "));
      Serial.println(freeStack);
    }

protected:
  bool _refreshing = false;
      
};
