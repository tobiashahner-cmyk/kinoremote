#pragma once
#include <Arduino.h>
#include <optional>
#include <utility>

class HueBridge;


class HueGroup {
public:
    HueGroup(uint16_t id,
             const char* name,
             HueBridge& bridge,
             const std::vector<uint8_t>& lightIds);

    uint16_t getId() const;
    const char* getName() const;
    uint16_t getTT() const;
    const std::vector<uint8_t>& getLightIds() const;

    bool allOn() const;
    bool anyOn() const;

    bool setOn(bool value);
    bool setBri(uint8_t value);
    bool setCT(uint16_t value);
    bool setTT(uint16_t value);

    void updateValues(const char* name, const std::vector<uint8_t> lightIds);
    bool applyChanges(HueBridge* bridge);
    bool isDirty();
    void setDirty(uint8_t lightId);
    void clearDirty();

private:
    bool _dirty;
    uint16_t _id;
    char _name[32];

    HueBridge& _bridge;
    std::vector<uint8_t> _lightIds;

    struct {
        std::optional<bool> on;
        std::optional<uint8_t> bri;
        std::optional<uint16_t> ct;
        std::optional<uint16_t> tt;
    } pending;
    static char _payload[128];
    void clearPending();
};
