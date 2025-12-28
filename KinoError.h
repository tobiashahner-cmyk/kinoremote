#pragma once
#include <Arduino.h>   // <-- WICHTIG

enum class KinoError : uint8_t {
    OK = 0,

    DeviceNotReady,
    PropertyNotSupported,
    InvalidType,
    InvalidValue,
    InternalError,
    InvalidProperty
};
