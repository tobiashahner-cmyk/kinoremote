#pragma once
#include <Arduino.h>
#include <map>
#include <ArduinoJson.h>

class HueBridge;

class HueSensor {
public:
    HueSensor(uint16_t id,
              const String& name,
              const String& type);

    uint16_t getId() const;
    const String& getName() const;
    const String& getType() const;

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
    String _name;
    String _type;

    StaticJsonDocument<256> _state;

    StaticJsonDocument<64> _pending;

    void clearPending();
};
