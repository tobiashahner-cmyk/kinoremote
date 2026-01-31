#pragma once
#include <Arduino.h>
#include <vector>

class HueBridge;

class HueScene {
public:
    HueScene(const String& id,
             const String& name,
             const std::vector<uint8_t>& lightIds);

    const String& getId() const;
    const String& getName() const;
    const uint16_t getTT() const;
    const std::vector<uint8_t>& getLightIds() const;

    bool setActive(HueBridge* bridge);
    bool captureLightStates(HueBridge* bridge);
    bool setTT(uint16_t value);

private:
    String _id;
    String _name;
    uint16_t _tt = 4;
    std::vector<uint8_t> _lightIds;
};
