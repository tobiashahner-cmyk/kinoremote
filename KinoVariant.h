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
