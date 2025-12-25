#include "OptomaSourceLookup.h"

const OptomaSourceLookup::Entry OptomaSourceLookup::_table[] PROGMEM = {
  {InputSource::None,            0,  0,  "None"},
  {InputSource::DVI,             1,  2,  "DVI"},
  {InputSource::VGA1,            2,  5,  "VGA1"},
  {InputSource::VGA2,            3,  6,  "VGA2"},
  {InputSource::SVideo,          4,  9,  "S-Video"},
  {InputSource::Video,           5, 10,  "Video"},
  {InputSource::BNC,             6,  4,  "BNC"},
  {InputSource::HDMI1,           7,  1,  "HDMI1"},
  {InputSource::HDMI2,           8, 15,  "HDMI2"},
  {InputSource::Wireless,        9, 11,  "Wireless"},
  {InputSource::Component,      10, 14,  "Component"},
  {InputSource::FlashDrive,     11, 17,  "Flash Drive"},
  {InputSource::NetworkDisplay, 12, 18,  "Network Display"},
  {InputSource::USB,            13, 19,  "USB"},
  {InputSource::HDMI3,           14, 16,  "HDMI3"},
  {InputSource::DisplayPort,    15, 20,  "Display Port"},
  {InputSource::HDBaseT,         16, 21,  "HDBaseT"}
};

const uint8_t OptomaSourceLookup::_tableSize =
  sizeof(_table) / sizeof(_table[0]);

OptomaSourceLookup::InputSource
OptomaSourceLookup::fromReadCode(uint8_t code) {
  for (uint8_t i = 0; i < _tableSize; ++i) {
    Entry e;
    memcpy_P(&e, &_table[i], sizeof(Entry));
    if (e.readCode == code) {
      return e.src;
    }
  }
  return InputSource::Unknown;
}

bool OptomaSourceLookup::toSetParameter(InputSource src, String& outParam) {
  for (uint8_t i = 0; i < _tableSize; ++i) {
    Entry e;
    memcpy_P(&e, &_table[i], sizeof(Entry));
    if (e.src == src) {
      outParam = String(e.setParam);
      return true;
    }
  }
  return false;
}

const char* OptomaSourceLookup::toString(InputSource src) {
  static char buffer[24];

  for (uint8_t i = 0; i < _tableSize; ++i) {
    Entry e;
    memcpy_P(&e, &_table[i], sizeof(Entry));
    if (e.src == src) {
      strncpy_P(buffer, e.name, sizeof(buffer));
      buffer[sizeof(buffer) - 1] = '\0';
      return buffer;
    }
  }
  return "Unknown";
}

OptomaSourceLookup::InputSource
OptomaSourceLookup::fromString(const String& name) {
  String s = name;
  s.trim();
  s.toUpperCase();

  for (uint8_t i = 0; i < _tableSize; ++i) {
    Entry e;
    memcpy_P(&e, &_table[i], sizeof(Entry));

    char buf[24];
    strncpy_P(buf, e.name, sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';

    String tableName(buf);
    tableName.toUpperCase();

    if (s == tableName) {
      return e.src;
    }
  }
  return InputSource::Unknown;
}
