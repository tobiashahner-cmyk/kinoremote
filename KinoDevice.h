#pragma once
#include <Arduino.h>

#include "KinoError.h"
#include "KinoVariant.h"

class KinoDevice {
public:
    virtual ~KinoDevice() = default;

    virtual const char* deviceType() const = 0;

    virtual KinoError init() {
      return KinoError::OK;
    }

    virtual KinoError get(const char* property, KinoVariant& out) {
        (void)property;
        (void)out;
        return KinoError::PropertyNotSupported;
    }

    virtual KinoError set(const char* property, const KinoVariant& value) {
        (void)property;
        (void)value;
        return KinoError::PropertyNotSupported;
    }

    virtual KinoError queryCount(const char* property, uint16_t &out ) {
        return KinoError::PropertyNotSupported;
    }

    virtual KinoError query(const char* property, uint16_t index, KinoVariant& out) {
        return KinoError::PropertyNotSupported;
    }

    virtual bool commit() {
      return true;
    }
};
