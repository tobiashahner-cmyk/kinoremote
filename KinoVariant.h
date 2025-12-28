#pragma once
#include <Arduino.h>   // <-- WICHTIG

class KinoVariant {
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
