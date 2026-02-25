#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "KinoDevice.h"
#include "OptomaSourceLookup.h"

class OptomaBeamer : public KinoDevice {
  public:
    enum dirtyBit : uint16_t {
      NONE    = 0,
      ON      = 1 << 0,
      SOURCE  = 1 << 1,
      UPTIME  = 1 << 2
    };
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

    size_t getPropertyCount() const override;
    const KinoPropertyInfo* getPropertyInfo(size_t index) const override;
    KinoError get(const char* property, KinoVariant& out) override;
    KinoError set(const char* property, const KinoVariant& value) override;
    KinoError queryCount(const char* property, uint16_t& out) override;
    KinoError query(const char* property, uint16_t index, KinoVariant &out) override;
    KinoError init() override;    // wie begin, nur andere Semantik
    bool getStatusUpdate(const char* devName, JsonObject& root) override;
  
    // Lifecycle
    bool begin();
    bool getStatus();
    KinoError tick();
  
    // Getter
    bool getPowerStatus() const;
    InputSource getSource() const;
    const char* getSourceString();
    DisplayMode getDisplayMode() const;
    int getLampHours() const;
    //int getTickInterval();
  
    // Setter
    bool setPower(bool onoff);
    bool setSource(InputSource src);
    bool setSource(const char* srcName);
    bool setDisplayMode(DisplayMode dm);
    bool freeze(bool onoff);
    //bool setTickInterval(int ms);
  
  private:
    // Verbindung / Identität
    IPAddress _ip;
    uint8_t _id;
  
    // Status refresh
    //unsigned long _tickInterval  = 0;
    //unsigned long _lastTick = 0;
    
    bool _powerState = false;
    InputSource _source = InputSource::Unknown;
    DisplayMode _displayMode = DisplayMode::Unknown;
    int _lampHours = 0;
    uint16_t _dirty;
  
    // Helper
    void EnsureTimeoutBeforeRequest(unsigned long timeout);
    bool sendCommand(const char* command, const int parameter, char* response, size_t responseLen);
    bool isOkResponse(const char* response);
    bool parseStatusResponse(const char* response);
    int parseFixedInt(const char* str, size_t start, size_t length);
    uint8_t encodeDisplayMode(DisplayMode dm) const;
    static const KinoPropertyInfo _properties[];
};
