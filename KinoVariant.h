#pragma once
#include <Arduino.h>
#include "RGBColor.h"
#include <ArduinoJson.h>

struct KinoVariant {
  enum Type : uint8_t {
    NONE,
    INT,
    BOOL,
    FLOAT,
    STRING,
    RGB_COLOR
  };

  Type type = NONE;

  union {
    int32_t  i;
    bool     b;
    float    f;
    RGBColor color;
  };

  char* s = nullptr;

  KinoVariant() = default;

  ~KinoVariant() {
    if (type == STRING && s) {
      free(s);
    }
  }

  KinoVariant(const KinoVariant& other) {
    *this = other;
  }

  KinoVariant& operator=(const KinoVariant& other) {
    if (this == &other) return *this;

    if (type == STRING && s) {
      free(s);
      s = nullptr;
    }

    type = other.type;

    switch (type) {
      case STRING:
          s = other.s ? strdup(other.s) : nullptr;
          break;
      case INT:
          i = other.i;
          break;
      case BOOL:
          b = other.b;
          break;
      case FLOAT:
          f = other.f;
          break;
      case RGB_COLOR:
          color = other.color;
          break;
      default:
          break;
    }

    return *this;
  }

  // ---------- Factory-Methoden ----------

  static KinoVariant fromString(const char* v) {
    KinoVariant k;
    k.type = STRING;
    k.s = v ? strdup(v) : nullptr;
    return k;
  }

  static KinoVariant fromInt(int32_t v) {
    KinoVariant k;
    k.type = INT;
    k.i = v;
    return k;
  }

  static KinoVariant fromBool(bool v) {
    KinoVariant k;
    k.type = BOOL;
    k.b = v;
    return k;
  }

  static KinoVariant fromFloat(float v) {
    KinoVariant k;
    k.type = FLOAT;
    k.f = v;
    return k;
  }

  static KinoVariant fromColor(uint8_t r, uint8_t g, uint8_t b) {
    KinoVariant k;
    k.type = RGB_COLOR;
    k.color = { r, g, b };
    return k;
  }

  static KinoVariant fromColor(const RGBColor& c) {
    KinoVariant k;
    k.type = RGB_COLOR;
    k.color = c;
    return k;
  }

  static KinoVariant fromJsonVariant(JsonVariantConst j) {
    if (j.is<bool>()) {
      return fromBool(j.as<bool>());
    } 
    else if (j.is<int32_t>()) { // Spezifisch auf 32-bit Int prüfen
      return fromInt(j.as<int32_t>());
    } 
    else if (j.is<float>()) {
      return fromFloat(j.as<float>());
    } 
    else if (j.is<const char*>()) {
      return fromString(j.as<const char*>());
    }
    else if (j.is<JsonArrayConst>()) {
      JsonArrayConst arr = j.as<JsonArrayConst>();
      if (arr.size() == 3) {
        return fromColor(arr[0], arr[1], arr[2]);
      }
    }
    return KinoVariant(); // Gibt NONE zurück
  }

  bool asBool() const {
    switch (type) {
      case BOOL:   return b;
      case INT:    return i != 0;
      case FLOAT:  return f != 0.0f;
      case STRING: 
          if (!s) return false;
          if (strcasecmp(s, "true") == 0 || strcmp(s, "1") == 0) return true;
          return false;
      default:     return false;
    }
  }

  int32_t asInt() const {
    switch (type) {
      case INT:    return i;
      case BOOL:   return b ? 1 : 0;
      case FLOAT:  return (int32_t)f;
      case STRING: return s ? atoi(s) : 0;
      default:     return 0;
    }
  }

  float asFloat() const {
    switch (type) {
      case FLOAT:  return f;
      case INT:    return (float)i;
      case BOOL:   return b ? 1.0f : 0.0f;
      case STRING: return s ? atof(s) : 0.0f;
      default:     return 0.0f;
    }
  }

  const char* asString() const {
    // Hinweis: Liefert bei Typ STRING den Pointer, sonst nullptr.
    // Für eine echte Konvertierung Zahl -> String müsste man einen Puffer verwalten (komplexer).
    return (type == STRING) ? s : nullptr;
  }

  String toString() const {
    switch (type) {
        case BOOL:   return b ? "true" : "false";
        case INT:    return String(i);
        case FLOAT:  return String(f, 2); // 2 Nachkommastellen
        case STRING: return s ? String(s) : "";
        case RGB_COLOR: {
            char buf[8];
            snprintf(buf, sizeof(buf), "#%02X%02X%02X", color.r, color.g, color.b);
            return String(buf);
        }
        default:     return "none";
    }
}

  RGBColor asColor() const {
    if (type == RGB_COLOR) return color;
    
    if (type == STRING && s != nullptr && s[0] == '#') {
      return parseHexColor(s);
    }
    
    // Falls die Farbe als Zahl gespeichert wurde (z.B. Hex-Int)
    if (type == INT) {
      return { (uint8_t)((i >> 16) & 0xFF), (uint8_t)((i >> 8) & 0xFF), (uint8_t)(i & 0xFF) };
    }

    return {0, 0, 0};
  }

  private:
  // Hilfsfunktion zum Parsen von "#RRGGBB" oder "#RGB"
  RGBColor parseHexColor(const char* hex) const {
    if (!hex || hex[0] != '#') return {0, 0, 0};
    
    uint32_t rgb = (uint32_t)strtol(hex + 1, nullptr, 16);
    size_t len = strlen(hex);
    
    if (len <= 4) { // Format #RGB
      uint8_t r = (rgb >> 8) & 0xF;
      uint8_t g = (rgb >> 4) & 0xF;
      uint8_t b = rgb & 0xF;
      return { (uint8_t)(r | (r << 4)), (uint8_t)(g | (g << 4)), (uint8_t)(b | (b << 4)) };
    } else { // Format #RRGGBB
      return { (uint8_t)((rgb >> 16) & 0xFF), (uint8_t)((rgb >> 8) & 0xFF), (uint8_t)(rgb & 0xFF) };
    }
  }

};








// Version bis 29.12.25, direkt vor Einbau von RGBColor-Support
/*
#pragma once
#include <Arduino.h>   // <-- WICHTIG

struct KinoVariant {
    enum Type : uint8_t {
        NONE,
        INT,
        BOOL,
        FLOAT,
        STRING
    };

    Type type = NONE;

    union {
        int32_t i;
        bool b;
        float f;
    };

    char* s = nullptr;

    KinoVariant() = default;

    ~KinoVariant() {
        if (type == STRING && s) {
            free(s);
        }
    }

    KinoVariant(const KinoVariant& other) {
        *this = other;
    }

    KinoVariant& operator=(const KinoVariant& other) {
        if (this == &other) return *this;

        if (type == STRING && s) {
            free(s);
        }

        type = other.type;

        switch (type) {
            case STRING:
                if (other.s) {
                    s = strdup(other.s);
                }
                break;
            case INT:   i = other.i; break;
            case BOOL:  b = other.b; break;
            case FLOAT: f = other.f; break;
            default: break;
        }
        return *this;
    }

    static KinoVariant fromString(const char* v) {
        KinoVariant k;
        k.type = STRING;
        k.s = v ? strdup(v) : nullptr;
        return k;
    }

    static KinoVariant fromInt(int32_t v) {
        KinoVariant k;
        k.type = INT;
        k.i = v;
        return k;
    }

    static KinoVariant fromBool(bool v) {
        KinoVariant k;
        k.type = BOOL;
        k.b = v;
        return k;
    }

    static KinoVariant fromFloat(float v) {
        KinoVariant k;
        k.type = FLOAT;
        k.f = v;
        return k;
    }
};
*/

// erste Version:
/*class KinoVariant {
public:
    enum Type : uint8_t {
        NONE,
        INT,
        BOOL,
        FLOAT,
        STRING
    };

    Type type;

    union {
        int32_t     i;
        bool        b;
        float       f;
        const char* s;
    };

    KinoVariant() : type(NONE), i(0) {}

    static KinoVariant fromInt(int32_t v) {
        KinoVariant k;
        k.type = INT;
        k.i = v;
        return k;
    }

    static KinoVariant fromBool(bool v) {
        KinoVariant k;
        k.type = BOOL;
        k.b = v;
        return k;
    }

    static KinoVariant fromFloat(float v) {
        KinoVariant k;
        k.type = FLOAT;
        k.f = v;
        return k;
    }

    static KinoVariant fromString(const char* v) {
        KinoVariant k;
        k.type = STRING;
        k.s = v;
        return k;
    }
};
*/
