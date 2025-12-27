#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

class HyperionDevice {
public:
  // ===== Konstruktoren =====
  HyperionDevice(const IPAddress& ip);
  HyperionDevice(const String& ip);

  
  bool tick();
  bool setTickInterval(int ms);
  int getTickInterval();

  // ===== Public API =====
  bool begin();
  bool init();  // wie begin, nur andere Semantik
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
  WiFiClient _client;

  bool _powerStatus = false;
  bool _ledDeviceStatus = false;

  int  _tickInterval  = 10000;
  unsigned long _lastTick = 0;

  // ===== JSON-RPC / HTTP Helper =====
  bool sendJsonRpc(const JsonDocument& request, String& response);
  bool parseServerInfo(const String& json);

  bool parseComponents(const String& jsonArray);
  bool readComponentsArray(String& out);

  bool httpPOST(const char* path, const String& payload, String& response);
  bool waitForClientData();
  bool readHttpResponse(String& response);
};
