#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "KinoDevice.h"

class HyperionDevice : public KinoDevice {
public:
    const char* deviceType() const override {
        return "hyperion";
    }

  // ===== Konstruktoren =====
  HyperionDevice(const IPAddress& ip);
  HyperionDevice(const String& ip);

  
  KinoError tick();
  bool setTickInterval(int ms);
  int getTickInterval();

  size_t getPropertyCount() const override;
    const KinoPropertyInfo* getPropertyInfo(size_t index) const override;
  KinoError get(const char* property, KinoVariant& out) override;
  KinoError set(const char* property, const KinoVariant& value) override;
  
  KinoError init() override;  // wie begin, nur andere Semantik

  // ===== Public API =====
  bool begin();
  bool getStatus();

  bool isBroadcasting() const;

  bool setBroadcast(bool onoff);
  bool startBroadcast();
  bool stopBroadcast();

  // ===== Getter =====
  bool getPowerStatus() const;
  bool getLedDeviceStatus() const;

private:
  // ===== Eigenschaften =====
  IPAddress _ip;
  //WiFiClient& _client;

  bool _powerStatus = false;
  bool _ledDeviceStatus = false;

  int  _tickInterval  = 0;
  unsigned long _lastTick = 0;
  StaticJsonDocument<1024> _doc;   // zum Parsen der Json HTTP-Antwort
  static StaticJsonDocument<64> _filter; // Filter für deserializeJson
  static bool _filterInitialized;
  void setupFilter();

  // ===== JSON-RPC / HTTP Helper =====
  void EnsureTimeoutBeforeRequest(unsigned long timeout);
  //bool sendJsonRpc(const JsonDocument& request, String& response);
  bool sendJsonRpc(const JsonDocument& request);
  //bool parseServerInfo(const String& json);

  //bool parseComponents(const String& jsonArray);
  //bool readComponentsArray(WiFiClient& client, String& out);

  //bool httpPOST(const char* path, const String& payload, String& response);
  bool httpPOST(const char* path, const JsonDocument& request);
  bool waitForClientData(WiFiClient& client);
  bool parseComponentsFromStream(WiFiClient& client);
  bool readHttpResponse(WiFiClient& client, String& response);

  static const KinoPropertyInfo _properties[];
};
