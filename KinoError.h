#ifndef KINO_ERROR_H
#define KINO_ERROR_H
#include <Arduino.h>   // <-- WICHTIG

enum class KinoError : uint8_t {
    OK = 0,

    DeviceNotReady,
    PropertyNotSupported,
    InvalidType,
    InvalidValue,
    InternalError,
    InvalidProperty,
    OutOfRange,
    DeviceUnknown,
};


inline const __FlashStringHelper* kinoErrorToString(KinoError error) {
    switch (error) {
        case KinoError::OK:                   return F("OK");
        case KinoError::DeviceNotReady:       return F("Device Not Ready");
        case KinoError::PropertyNotSupported: return F("Property Not Supported");
        case KinoError::InvalidType:          return F("Invalid Type");
        case KinoError::InvalidValue:         return F("Invalid Value");
        case KinoError::InternalError:        return F("Internal Error");
        case KinoError::InvalidProperty:      return F("Invalid Property");
        case KinoError::OutOfRange:           return F("Out Of Range");
        case KinoError::DeviceUnknown:        return F("Device Unknown");
        default:                              return F("Unknown Error");
    }
}


#endif
