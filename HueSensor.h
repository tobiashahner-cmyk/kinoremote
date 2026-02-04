#pragma once
#include <Arduino.h>
#include <map>
#include <ArduinoJson.h>

class HueBridge;

class HueSensor {
public:
    HueSensor(uint16_t id,
              const char* name,
              const char* type);

    uint16_t getId() const;
    const char* getName() const;
    const char* getType() const;

    // Lesen
    bool hasValue(const String& key) const;
    JsonVariantConst getValue(const String& key) const;
    int getStateSize() const;
    JsonObjectConst getState() const;

    // Schreiben (nur für CLIPGenericStatus)
    bool setValue(const String& key, int value);

    // intern
    void updateState(const JsonObject& state);
    bool isWritable() const;
    bool applyChanges(HueBridge* bridge);

private:
    uint16_t _id;
    char _name[48];
    char _type[32];

    StaticJsonDocument<256> _state;

    StaticJsonDocument<64> _pending;

    void clearPending();
};
