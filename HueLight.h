#pragma once
#include <Arduino.h>
#include <optional>
#include <utility>

struct RgbColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

// Struktur zum Halten der XY-Farbkoordinaten (0.0f - 1.0f)
struct XyPoint {
    float x;
    float y;
};


class HueBridge;

class HueLight {
public:
    HueLight(uint8_t id,
             const String& name,
             bool on,
             uint8_t bri,
             bool hasXY,
             float x,
             float y,
             bool hasCT,
             uint16_t ct,
             bool hasBri);

    uint8_t getId() const;
    const String& getName() const;

    bool isOn() const;
    uint8_t getBrightness() const;

    bool hasXYColor() const;
    bool hasCTColor() const;
    bool isDimmable() const;

    float getX() const;
    float getY() const;
    uint16_t getCT() const;

    // --- Setter für Änderungen ---
    bool setOn(bool value);
    bool setBri(uint8_t value);
    bool setCT(uint16_t value);
    bool setXY(float x, float y);
    bool setRGB(uint8_t r, uint8_t g, uint8_t b);

    XyPoint rgbToXy(RgbColor color);

    // --- Setter für direkte Änderungen ohne Bridge-Kommunikation
    // NUR für Änderungen über GROUPS
    void forceOn(bool value);
    void forceBri(uint8_t value);
    void forceCT(uint16_t value);

    // --- Änderungen anwenden ---
    bool applyChanges(HueBridge* bridge);

private:
    uint8_t _id;
    String _name;

    bool _on;
    uint8_t _bri;
    bool _hasBri;

    bool _hasXY;
    float _x, _y;

    bool _hasCT;
    uint16_t _ct;

    void checkAndCorrectXY(XyPoint& p);
    XyPoint getClosestPoint(XyPoint p, XyPoint a, XyPoint b);
    float gammaCorrection(uint8_t color);

    

    // Container für noch nicht übertragene Änderungen
    struct PendingChanges {
        std::optional<bool> on;
        std::optional<uint8_t> bri;
        std::optional<uint16_t> ct;
        std::optional<std::pair<float,float>> xy;
    } pending;

    void clearPending() { pending = PendingChanges(); }
};
