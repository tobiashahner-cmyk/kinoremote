#pragma once

#include <Arduino.h>

class OptomaSourceLookup {
public:
  enum class InputSource : uint8_t {
    None,
    DVI,
    VGA1,
    VGA2,
    SVideo,
    Video,
    BNC,
    HDMI1,
    HDMI2,
    Wireless,
    Component,
    FlashDrive,
    NetworkDisplay,
    USB,
    HDMI3,
    DisplayPort,
    HDBaseT,
    Unknown
  };

  static InputSource fromReadCode(uint8_t code);
  static bool toSetParameter(InputSource src, String& outParam);
  static const char* toString(InputSource src);
  static InputSource fromString(const String& name);

private:
  struct Entry {
    InputSource src;
    uint8_t readCode;
    uint8_t setParam;
    const char* name;
  };

  static const Entry _table[];
  static const uint8_t _tableSize;
};
