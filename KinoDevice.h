#pragma once

class KinoDevice {
public:
    virtual ~KinoDevice() {}

    // Reiner Identifikator, KEINE Logik
    virtual const char* deviceType() const = 0;
};
