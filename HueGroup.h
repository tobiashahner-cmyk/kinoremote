#pragma once
#include <Arduino.h>
#include <optional>
#include <utility>

class HueBridge;


class HueGroup {
public:
    HueGroup(uint16_t id,
             const String& name,
             HueBridge& bridge,
             const std::vector<uint8_t>& lightIds);

    uint16_t getId() const;
    const String& getName() const;

    bool allOn() const;
    bool anyOn() const;

    bool setOn(bool value);
    bool setBri(uint8_t value);
    bool setCT(uint16_t value);

    bool applyChanges(HueBridge& bridge);

private:
    uint16_t _id;
    String _name;

    HueBridge& _bridge;
    std::vector<uint8_t> _lightIds;

    struct {
        std::optional<bool> on;
        std::optional<uint8_t> bri;
        std::optional<uint16_t> ct;
    } pending;

    void clearPending();
};
