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
    const std::vector<uint8_t>& getLightIds() const;

    bool setActive(HueBridge& bridge);
    bool captureLightStates(HueBridge& bridge);

private:
    String _id;
    String _name;
    std::vector<uint8_t> _lightIds;
};
