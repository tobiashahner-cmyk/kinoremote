#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "KinoDevice.h"
#include "OptomaSourceLookup.h"

class OptomaBeamer : public KinoDevice {
  public:
    using InputSource = OptomaSourceLookup::InputSource;
    
    const char* deviceType() const override {
        return "optomabeamer";
    }
  
    enum class DisplayMode : uint8_t {
      Presentation,
      Bright,
      Movie,
      sRGB,
      User,
      Blackboard,
      DICOM_SIM,
      Unknown
    };
  
    // Konstruktoren
    OptomaBeamer(const IPAddress& ip, uint8_t beamerId);
    OptomaBeamer(const String& ip, uint8_t beamerId);
  
    // Lifecycle
    bool begin();
    bool init();    // wie begin, nur andere Semantik
    bool getStatus();
    bool tick();
  
    // Getter
    bool getPowerStatus() const;
    InputSource getSource() const;
    const char* getSourceString();
    DisplayMode getDisplayMode() const;
    int getLampHours() const;
    int getTickInterval();
  
    // Setter
    bool setPower(bool onoff);
    bool setSource(InputSource src);
    bool setSource(const String& srcName);
    bool setDisplayMode(DisplayMode dm);
    bool freeze(bool onoff);
    bool setTickInterval(int ms);
  
  private:
    // Verbindung / Identität
    IPAddress _ip;
    uint8_t _id;
    WiFiClient _client;
  
    // Status
    unsigned long _tickInterval  = 10000;
    unsigned long _lastTick = 0;
    
    bool _powerState = false;
    InputSource _source = InputSource::Unknown;
    DisplayMode _displayMode = DisplayMode::Unknown;
    int _lampHours = 0;
  
    // Helper
    bool sendCommand(const String& command,
                     const String& parameter,
                     String& response);
  
    bool isOkResponse(const String& response);
    bool parseStatusResponse(const String& response);
    String encodeDisplayMode(DisplayMode dm) const;
};
