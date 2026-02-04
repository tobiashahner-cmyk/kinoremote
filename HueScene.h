#pragma once
#include <Arduino.h>
#include <vector>

class HueBridge;

class HueScene {
public:
    HueScene(const char* id,
             const char* name,
             const std::vector<uint8_t>& lightIds);

    const char* getId() const;
    const char* getName() const;
    const uint16_t getTT() const;
    const std::vector<uint8_t>& getLightIds() const;

    bool setActive(HueBridge* bridge);
    bool captureLightStates(HueBridge* bridge);
    bool setTT(uint16_t value);

private:
    char _id[48];
    char _name[32];
    uint16_t _tt = 4;
    std::vector<uint8_t> _lightIds;
};
