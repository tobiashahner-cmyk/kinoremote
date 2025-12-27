#include "HyperionDevice.h"

// ===== Konstruktoren =====

HyperionDevice::HyperionDevice(const IPAddress& ip)
: _ip(ip) {}

HyperionDevice::HyperionDevice(const String& ip) {
  _ip.fromString(ip);
}

bool HyperionDevice::tick() {
  if (_tickInterval == 0) return false;
  
  unsigned long now = millis();
  if (now - _lastTick >= _tickInterval) {
    _lastTick = now;
    getStatus();
    return true;
  }
  return false;
}

bool HyperionDevice::setTickInterval(int ms) {
  if (ms == 0) { _tickInterval = 0; return true; }
  if (ms < 0) return false;       // nur zur besseren Lesbarkeit hier aufgeführt
  if (ms < 2000) return false;    // unter 2 Sekunden Interval führt zu übermässigem Traffic
  _tickInterval = ms;
  return true;
}

int HyperionDevice::getTickInterval() {
  return _tickInterval;
}

// ===== Public API =====

bool HyperionDevice::begin() {
  return getStatus();
}

bool HyperionDevice::init() {
  return getStatus();
}

bool HyperionDevice::getStatus() {
  String payload;
  StaticJsonDocument<64> req;
  req["command"] = "serverinfo";
  req["tan"] = 0;
  serializeJson(req, payload);

  if (!_client.connect(_ip, 8090)) return false;

  _client.printf(
    "POST /json-rpc HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: %d\r\n"
    "Connection: close\r\n\r\n",
    _ip.toString().c_str(),
    payload.length()
  );
  _client.print(payload);

  String componentsJson;
  if (!readComponentsArray(componentsJson)) {
    Serial.println("readComponentsArray fehlgeschlagen");
    return false;
  }
  return parseComponents(componentsJson);
}


bool HyperionDevice::isBroadcasting() const {
  return _powerStatus && _ledDeviceStatus;
}

bool HyperionDevice::setBroadcast(bool onoff) {
  StaticJsonDocument<192> req;
  req["command"]  = "componentstate";
  req["tan"]      = 1;

  JsonObject params = req.createNestedObject("componentstate");
  params["component"] = "LEDDEVICE";
  params["state"]     = onoff;

  String response;
  if (!sendJsonRpc(req, response)) return false;

  _ledDeviceStatus = onoff;
  return true;
}

bool HyperionDevice::startBroadcast() {
  return setBroadcast(true);
}

bool HyperionDevice::stopBroadcast() {
  return setBroadcast(false);
}

// ===== Getter =====

bool HyperionDevice::getPowerStatus() const {
  return _powerStatus;
}

bool HyperionDevice::getLedDeviceStatus() const {
  return _ledDeviceStatus;
}

// ===== JSON-RPC Helper =====

bool HyperionDevice::sendJsonRpc(const JsonDocument& request, String& response) {
  String payload;
  serializeJson(request, payload);
  Serial.println(payload);
  return httpPOST("/json-rpc", payload, response);
}

// ===== HTTP Helper =====

bool HyperionDevice::httpPOST(const char* path,
                              const String& payload,
                              String& response) {
  if (!_client.connect(_ip, 8090)) {
    return false;
  }

  _client.printf(
    "POST %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: %d\r\n"
    "Connection: close\r\n\r\n",
    path,
    _ip.toString().c_str(),
    payload.length()
  );

  _client.print(payload);

  if (!waitForClientData()) return false;
  if (!readHttpResponse(response)) return false;
  return response.startsWith("HTTP/1.1 200");
}

bool HyperionDevice::waitForClientData() {
  unsigned long start = millis();
  while (_client.connected() && !_client.available()) {
    if (millis() - start > 2000) {
      _client.stop();
      return false;
    }
    yield();
  }
  return _client.available() > 0;
}

bool HyperionDevice::readHttpResponse(String& response) {
  const unsigned long overallTimeout = 3000;
  const unsigned long idleTimeout    = 500;
  unsigned long startTime = millis();
  unsigned long lastData  = millis();

  while (true) {
    while (_client.available()) {
      response += char(_client.read());
      lastData = millis();
    }

    if (!_client.connected()) break;
    if (millis() - lastData > idleTimeout) break;

    if (millis() - startTime > overallTimeout) {
      _client.stop();
      return false;
    }

    yield();
  }
  Serial.print(response);
  _client.stop();
  return response.length() > 0;
}

bool HyperionDevice::readComponentsArray(String& out) {
  const unsigned long overallTimeout = 3000;
  unsigned long startTime = millis();

  bool headerDone = false;
  bool capturing  = false;
  int  bracketDepth = 0;

  String window;   // Sliding window zum Erkennen von "components":[

  while (_client.connected() || _client.available()) {
    if (millis() - startTime > overallTimeout) {
      Serial.println("Overall Timeout!");
      _client.stop();
      return false;
    }

    if (!_client.available()) {
      yield();
      continue;
    }

    char c = _client.read();

    // 1) HTTP-Header überspringen
    if (!headerDone) {
      window += c;
      if (window.endsWith("\r\n\r\n")) {
        headerDone = true;
        window = "";
      }
      continue;
    }

    // 2) Nach "components":[ suchen
    if (!capturing) {
      window += c;
      if ((window.endsWith("\"components\": ["))||(window.endsWith("\"components\":["))) {
        capturing = true;
        bracketDepth = 1;
        out = "[";          // wir bauen ein eigenes, sauberes Array
      }
      if (window.length() > 32) window.remove(0, 1);
      continue;
    }

    // 3) Array-Inhalt sammeln
    if (c == '[') bracketDepth++;
    if (c == ']') bracketDepth--;

    out += c;

    // 4) Array vollständig
    if (bracketDepth == 0) {
      _client.stop();
      return true;
    }
  }

  _client.stop();
  return false;
}

bool HyperionDevice::parseComponents(const String& jsonArray) {
  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, jsonArray)) return false;

  for (JsonObject comp : doc.as<JsonArray>()) {
    const char* name = comp["name"];
    bool enabled = comp["enabled"] | false;

    if (!strcmp(name, "ALL"))       _powerStatus = enabled;
    if (!strcmp(name, "LEDDEVICE")) _ledDeviceStatus = enabled;
  }

  return true;
}



// ===== JSON Parsing =====

bool HyperionDevice::parseServerInfo(const String& json) {
  int jsonStart = json.indexOf("\r\n\r\n");
  if (jsonStart < 0) return false;

  String payload = json.substring(jsonStart + 4);
  Serial.println(payload);

  StaticJsonDocument<512> filter;
  JsonArray comps = filter["result"]["components"].to<JsonArray>();
  JsonObject c1 = comps.createNestedObject();
  c1["name"] = true;
  c1["enabled"] = true;

  StaticJsonDocument<768> doc;
  DeserializationError err =
    deserializeJson(doc, payload, DeserializationOption::Filter(filter));

  if (err) return false;

  for (JsonObject comp : doc["result"]["components"].as<JsonArray>()) {
    const char* name = comp["name"];
    bool enabled = comp["enabled"] | false;

    if (!strcmp(name, "ALL"))       _powerStatus     = enabled;
    if (!strcmp(name, "LEDDEVICE")) _ledDeviceStatus = enabled;
  }

  return true;
}
