#pragma once
#include <Arduino.h>
#include <map>
#include <ArduinoJson.h>
#include <time.h>

class HueBridge;

struct HueSensorValue {
  char key[16];
  float value;
  uint8_t type;// 0: None, 1: Bool, 2: Int, 3: Float
};

class HueSensor {
public:
    HueSensor(uint16_t id,
              const char* name,
              const char* type);

    uint16_t getId() const;
    const char* getName() const;
    const char* getType() const;

    // Lesen
    bool hasValue(const char* key) const;
    //JsonVariantConst getValue(const String& key) const;
    float getValue(const char* key) const;
    HueSensorValue& getRawValue(const char* key);
    uint32_t getLastUpdated();
    int getStateSize() const;
    //JsonObjectConst getState() const;
    const HueSensorValue& getValueAt(int index) const;

    // Schreiben (nur für CLIPGenericStatus)
    //bool setValue(const String& key, int value);
    bool setValue(const char* key, int value);

    // intern
    void updateState(const JsonObject& state);
    bool isWritable() const;
    bool applyChanges(HueBridge* bridge);

    bool isDirty();
    void clearDirty();

private:
    bool _dirty;
    uint16_t _id;
    char _name[48];
    char _type[32];
    uint32_t _lastupdated;   // Unix Timestamp

    HueSensorValue _value[3];
    //StaticJsonDocument<256> _state;
    
    HueSensorValue _pending[3];
    HueSensorValue _tmpValue; // Hilfsmember, um "frei erfundene" values zu liefern
    //StaticJsonDocument<64> _pending;

    void clearPending();
    uint32_t isoToTimestamp(const char* isoTime);
};
