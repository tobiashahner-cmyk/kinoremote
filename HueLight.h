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
             const char* name,
             bool on,
             bool hasBri,
             uint8_t bri,
             bool hasXY,
             float x,
             float y,
             bool hasCT,
             uint16_t ct,
             uint16_t minct,
             uint16_t maxct);

    bool isDirty();
    void clearDirty();

    uint8_t getId() const;
    const char* getName() const;

    bool isOn() const;
    uint8_t getBrightness() const;

    bool hasXYColor() const;
    bool hasCTColor() const;
    bool isDimmable() const;

    float getX() const;
    float getY() const;
    RgbColor getRGB();
    uint16_t getCT() const;
    uint16_t getMinCT() const;
    uint16_t getMaxCT() const;
    uint16_t getTT() const;

    // --- Setter für Änderungen ---
    bool setOn(bool value);
    bool setBri(uint8_t value);
    bool setCT(uint16_t value);
    bool setXY(float x, float y);
    bool setRGB(uint8_t r, uint8_t g, uint8_t b);
    bool setTT(uint16_t value);

    XyPoint rgbToXy(RgbColor color);
    

    // --- Setter für direkte Änderungen ohne Bridge-Kommunikation
    // NUR für Änderungen über GROUPS
    void forceOn(bool value);
    void forceBri(uint8_t value);
    void forceCT(uint16_t value);
    // Setter für Änderungen, die beim Refresh von der Bridge kommen
    void updateValues(const char* name, bool on, bool hasBri, uint8_t bri, bool hasXY, float x, float y, bool hasCT, uint16_t ct, uint16_t minct, uint16_t maxct);
    // --- Änderungen anwenden ---
    bool applyChanges(HueBridge* bridge);

private:
    bool _dirty;
    uint8_t _id;
    char _name[32];

    bool _on;
    uint8_t _bri;
    bool _hasBri;

    bool _hasXY;
    float _x, _y;

    bool _hasCT;
    uint16_t _ct;
    uint16_t _minct;
    uint16_t _maxct;

    void checkAndCorrectXY(XyPoint& p);
    XyPoint getClosestPoint(XyPoint p, XyPoint a, XyPoint b);
    float gammaCorrection(uint8_t color);
    RgbColor xyToRgb(float x, float y, uint8_t brightness);
    uint8_t reverseGamma(float factor);
    

    // Container für noch nicht übertragene Änderungen
    struct PendingChanges {
        std::optional<bool> on;
        std::optional<uint8_t> bri;
        std::optional<uint16_t> ct;
        std::optional<std::pair<float,float>> xy;
        std::optional<uint16_t> tt;
    } pending;

    void clearPending() { pending = PendingChanges(); }
};
