#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

struct WLEDColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct stateBackup {
  bool onoff;
  uint16_t fx;
  uint8_t bri;
  uint8_t sx;
  uint8_t ix;
  uint8_t pal;
  WLEDColor fgCol;
  WLEDColor bgCol;
  WLEDColor fxCol;
  uint8_t c1x;
  uint8_t c2x;
  uint8_t c3x;
};



class WLEDDevice {
public:
  // Konstruktoren
  explicit WLEDDevice(const IPAddress& ip);
  explicit WLEDDevice(const String& ip);

  // Lifecycle
  bool begin();
  bool getStatus();
  bool tick();                                      // zum regelmässigen Auslesen des aktuellen Status. Ist true, wenn ausgeführt, sonst false
  bool setTickInterval(int ms);
  int getTickInterval();


  // Getter
  bool getPowerStatus() const;
  uint8_t getBrightness() const;
  bool isReceivingLiveData() const;
  char* isReceivingFrom() const;
  bool isOverridingLiveData() const;
  uint16_t getEffect() const;
  uint8_t getSpeed() const;
  uint8_t getIntensity() const;
  String getLiveSource() const;
  uint8_t getPalette() const;
  bool inAlarm() const;
  bool inPause() const;
  WLEDColor getColFg() const;
  WLEDColor getColBg() const;
  WLEDColor getColFx() const;

  // Setter
  bool setPowerStatus(bool onoff);
  bool setBrightness(uint8_t bri);
  bool setTransitionTime(int tt);
  bool fade(uint8_t newbri, int durationMs);
  bool setEffect(uint16_t effect);
  bool setSpeed(uint8_t speed);
  bool setIntensity(uint8_t intensity);
  bool setFgColor(uint8_t R, uint8_t G, uint8_t B);
  bool setFgColor(WLEDColor col);
  bool setBgColor(uint8_t R, uint8_t G, uint8_t B);
  bool setBgColor(WLEDColor col);
  bool setFxColor(uint8_t R, uint8_t G, uint8_t B);
  bool setFxColor(WLEDColor col);
  bool setCustom(uint8_t c1x, uint8_t c2x, uint8_t c3x);
  bool setPalette(uint8_t pal);
  bool setLive(bool onoff);
  bool setAlarm(bool onoff);
  bool setPause(bool onoff);
  bool backupState();
  bool restoreBackup();
  bool applyChanges();

private:
  IPAddress _ip;
  WiFiClient _client;
  int  _tickInterval  = 10000;
  unsigned long _lastTick = 0;

  // Status
  StaticJsonDocument<1024> _props;
  StaticJsonDocument<1024> _newProps;
  bool readState();
  stateBackup _bkp;
  bool _alarm = false;
  bool _pause = false;

  void initFilter();
  static StaticJsonDocument<512> _jsonFilter; 
};
