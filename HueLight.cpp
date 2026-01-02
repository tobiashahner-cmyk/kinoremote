#include "HueLight.h"
#include "HueBridge.h"
#include <ArduinoJson.h>

HueLight::HueLight(uint8_t id,
                   const String& name,
                   bool on,
                   uint8_t bri,
                   bool hasXY,
                   float x,
                   float y,
                   bool hasCT,
                   uint16_t ct,
                   bool hasBri)
: _id(id),
  _name(name),
  _on(on),
  _bri(bri),
  _hasBri(hasBri),
  _hasXY(hasXY),
  _x(x),
  _y(y),
  _hasCT(hasCT),
  _ct(ct) {}

// --- Getter ---
uint8_t HueLight::getId() const { return _id; }
const String& HueLight::getName() const { return _name; }

bool HueLight::isOn() const { return _on; }
uint8_t HueLight::getBrightness() const { return _bri; }
bool HueLight::isDimmable() const { return _hasBri; }

bool HueLight::hasXYColor() const { return _hasXY; }
bool HueLight::hasCTColor() const { return _hasCT; }

float HueLight::getX() const { return _x; }
float HueLight::getY() const { return _y; }
RgbColor HueLight::getRGB() { return xyToRgb(_x, _y, _bri); }
uint16_t HueLight::getCT() const { return _ct; }

// --- Setter ---
bool HueLight::setOn(bool value)      { pending.on = value; return true;}
bool HueLight::setBri(uint8_t value)  { if(_hasBri) pending.bri = value; return true;}
bool HueLight::setCT(uint16_t value)  { if(_hasCT) { pending.ct = value;  return true; } return false;}
bool HueLight::setXY(float x, float y){ if(_hasXY) { pending.xy = std::make_pair(x, y);  return true;}  return false;}
bool HueLight::setRGB(uint8_t r, uint8_t g, uint8_t b) { 
  RgbColor col = {r,g,b};
  XyPoint xyp = rgbToXy(col); 
  return setXY(xyp.x, xyp.y);
  }

// --- spezielle Setter für direkten Zugriff ohne "pending"
void HueLight::forceOn(bool value)              { _on = value; }
void HueLight::forceBri(uint8_t value)          { if (_hasBri) _bri = value; }
void HueLight::forceCT(uint16_t value)          { if (_hasCT)  _ct = value; }

// --- applyChanges ---
bool HueLight::applyChanges(HueBridge* bridge) {
    // Prüfen, ob überhaupt Änderungen vorliegen
    bool hasChanges = pending.on.has_value() || 
                      pending.bri.has_value() || 
                      pending.ct.has_value() || 
                      pending.xy.has_value();
    if (!hasChanges) return true;

    StaticJsonDocument<256> doc;

    if(pending.on.has_value()) doc["on"] = pending.on.value();
    if(pending.bri.has_value()) doc["bri"] = pending.bri.value();
    if(pending.ct.has_value()) doc["ct"] = pending.ct.value();
    if(pending.xy.has_value()) {
        JsonArray xyArr = doc.createNestedArray("xy");
        xyArr.add(pending.xy->first);
        xyArr.add(pending.xy->second);
    }

    String payload;
    serializeJson(doc, payload);

    if(!bridge->sendLightState(_id, payload)) return false;

    // Lokale Werte aktualisieren
    if(pending.on.has_value()) _on = pending.on.value();
    if(pending.bri.has_value()) _bri = pending.bri.value();
    if(pending.ct.has_value()) _ct = pending.ct.value();
    if(pending.xy.has_value()) { 
      _x = pending.xy->first; 
      _y = pending.xy->second; 
    }

    clearPending();
    return true;
}


// Umrechnung von RGB nach XY
// Struktur zum Halten der RGB-Werte (0-255)


// --- Hilfsfunktionen ---

// 1. Gamma-Korrektur
float HueLight::gammaCorrection(uint8_t color) {
    float component = color / 255.0f;
    if (component > 0.04045f) {
        return pow(component + 0.055f, 2.4f) / 1.055f;
    } else {
        return component / 12.92f;
    }
}

// 2. Farbraum-Begrenzung (Color Gamut Mapping für Farbraum B)
XyPoint HueLight::getClosestPoint(XyPoint p, XyPoint a, XyPoint b) {
    XyPoint ap = {p.x - a.x, p.y - a.y};
    XyPoint ab = {b.x - a.x, b.y - a.y};
    float ab2 = ab.x * ab.x + ab.y * ab.y;
    float ap_ab = ap.x * ab.x + ap.y * ab.y;
    float t = ap_ab / ab2;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    XyPoint newPoint = {a.x + ab.x * t, a.y + ab.y * t};
    return newPoint;
}

void HueLight::checkAndCorrectXY(XyPoint& p) {
    // Definierte Eckpunkte für den Hue Farbraum B (Standard)
    const XyPoint red = {0.675f, 0.322f};
    const XyPoint green = {0.4091f, 0.518f};
    const XyPoint blue = {0.167f, 0.04f};

    // Prüfen, ob der Punkt innerhalb des Dreiecks liegt (Cross Product Method)
    float v1[] = {green.x - red.x, green.y - red.y};
    float v2[] = {blue.x - red.x, blue.y - red.y};
    float q[] = {p.x - red.x, p.y - red.y};
    float s = v1[1] * v2[0] - v2[1] * v1[0];
    float t1 = (q[0] * v2[1] - q[1] * v2[0]) / s;
    float t2 = (v1[0] * q[1] - v1[1] * q[0]) / s;

    if (t1 < 0.0f || t2 < 0.0f || (t1 + t2) > 1.0f) {
        // Punkt liegt außerhalb, muss korrigiert werden
        XyPoint pAB = getClosestPoint(p, red, green);
        XyPoint pAC = getClosestPoint(p, blue, red);
        XyPoint pBC = getClosestPoint(p, green, blue);

        // Prüfen, welcher Punkt am nächsten am Original liegt
        float dAB = pow(pAB.x - p.x, 2) + pow(pAB.y - p.y, 2);
        float dAC = pow(pAC.x - p.x, 2) + pow(pAC.y - p.y, 2);
        float dBC = pow(pBC.x - p.x, 2) + pow(pBC.y - p.y, 2);

        if (dAB < dAC && dAB < dBC) {
            p = pAB;
        } else if (dAC < dBC) {
            p = pAC;
        } else {
            p = pBC;
        }
    }
}


// --- Hauptfunktion ---

XyPoint HueLight::rgbToXy(RgbColor color) {
    // 1. Gamma-Korrektur (De-Gammaisierung)
    float r = gammaCorrection(color.r);
    float g = gammaCorrection(color.g);
    float b = gammaCorrection(color.b);

    // 2. RGB zu CIE XYZ Transformation (Standardmatrix)
    float X = r * 0.4124f + g * 0.3576f + b * 0.1805f;
    float Y = r * 0.2126f + g * 0.7152f + b * 0.0722f; // Y ist Luminanz
    float Z = r * 0.0193f + g * 0.1192f + b * 0.9505f;

    XyPoint xy = {0.0f, 0.0f};

    // 3. XYZ zu XY Normalisierung
    if (X + Y + Z > 0) {
        xy.x = X / (X + Y + Z);
        xy.y = Y / (X + Y + Z);
    }
    
    // 4. Farbraum-Filterung (begrenzt auf Hue-Gamut)
    checkAndCorrectXY(xy);

    return xy;
}


// Umrechnung von XY nach RGB
// Hilfsfunktion: Umgekehrte Gamma-Korrektur
uint8_t HueLight::reverseGamma(float factor) {
    if (factor <= 0.0031308f) {
        factor = 12.92f * factor;
    } else {
        factor = 1.055f * pow(factor, (1.0f / 2.4f)) - 0.055f;
    }
    
    // Auf 0.0 - 1.0 begrenzen und auf 0-255 skalieren
    int result = (int)(factor * 255.0f);
    if (result < 0) return 0;
    if (result > 255) return 255;
    return (uint8_t)result;
}

RgbColor HueLight::xyToRgb(float x, float y, uint8_t brightness) {
    // 1. Berechne XYZ Werte
    // Da wir nur x und y haben, nutzen wir die Helligkeit (0-254) als Y
    float Y = brightness / 254.0f; 
    float X = (Y / y) * x;
    float Z = (Y / y) * (1.0f - x - y);

    // 2. XYZ zu RGB Transformation (Wide RGB D65 Matrix)
    // Diese Koeffizienten entsprechen dem Farbraum der Hue Lampen
    float r =  X * 1.656492f - Y * 0.354851f - Z * 0.255038f;
    float g = -X * 0.707196f + Y * 1.655397f + Z * 0.036152f;
    float b =  X * 0.051713f - Y * 0.121364f + Z * 1.011530f;

    // 3. Maximale Komponente finden, um bei Überschreitung zu normalisieren
    if (r > 1.0f || g > 1.0f || b > 1.0f) {
        float maxVal = max(r, max(g, b));
        r /= maxVal;
        g /= maxVal;
        b /= maxVal;
    }

    // 4. Umgekehrte Gamma-Korrektur und Konvertierung nach uint8_t
    RgbColor result;
    result.r = reverseGamma(r);
    result.g = reverseGamma(g);
    result.b = reverseGamma(b);

    return result;
}
