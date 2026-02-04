#include "HyperionDevice.h"
#include "NetworkHelper.h"

// ===== Konstruktoren =====

HyperionDevice::HyperionDevice(const IPAddress& ip)
: _ip(ip) {}

HyperionDevice::HyperionDevice(const String& ip) {
  _ip.fromString(ip);
}

KinoError HyperionDevice::tick() {
  if (WiFi.status() != WL_CONNECTED) return KinoError::NothingToDo;
  if (_tickInterval == 0) return KinoError::NothingToDo;
  if (_refreshing) return KinoError::NothingToDo;
  
  unsigned long now = millis();
  if (now - _lastTick >= _tickInterval) {
    _lastTick = now;
    _refreshing = true;
    //showMemory();
    bool ok = getStatus();
    //showMemory();
    _refreshing = false;
    return (ok ? KinoError::OK : KinoError::DeviceNotReady);
  }
  return KinoError::NothingToDo;
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

// ===== neue Public API, als Wrapper auf alte Public API ===
const KinoPropertyInfo HyperionDevice::_properties[] = {
  { "tickInterval", "Aktualisierung [ms]", Prop_Read | Prop_Write ,0,20000,500},
  { "ip",         "IP",         Prop_Read                         },
  { "on",         "Power",      Prop_Read  | Prop_Write           },
  { "live",       "Broadcast",  Prop_Read  | Prop_Write           }
};

size_t HyperionDevice::getPropertyCount() const {
  return sizeof(_properties) / sizeof(_properties[0]);
}

const KinoPropertyInfo* HyperionDevice::getPropertyInfo(size_t index) const {
  if (index >= getPropertyCount()) return nullptr;
  return &_properties[index];
}

KinoError HyperionDevice::get(const char* prop, KinoVariant& out) {
  if (!prop) return KinoError::PropertyNotSupported;
  if (strcmp(prop,"tickInterval")==0) {
    //out = KinoVariant::fromInt(_tickInterval);
    out.setInt(_tickInterval);
    return KinoError::OK;
  }
  if (strcmp(prop,"ip")==0) {
    //out = KinoVariant::fromString(_ip.toString().c_str());
    char buf[20];
    snprintf(buf, sizeof(buf), "%d.%d.%d.%d", _ip[0], _ip[1], _ip[2], _ip[3]);
    out.setString(buf);
    return KinoError::OK;
  }
  if ((strcmp(prop,"power")==0)||(strcmp(prop,"on")==0)) {
    //out = KinoVariant::fromBool(_powerStatus);
    out.setBool(_powerStatus);
    return KinoError::OK;
  }
  if (strcmp(prop,"live")==0) {
    //out = KinoVariant::fromBool(_ledDeviceStatus && _powerStatus);
    out.setBool(_ledDeviceStatus && _powerStatus);
    return KinoError::OK;
  }
  return KinoError::PropertyNotSupported;
}

KinoError HyperionDevice::set(const char* prop, const KinoVariant& value) {
  if (strcmp(prop,"tickInterval")==0) {
    //if(value.type != KinoVariant::INT) return KinoError::InvalidType;
    if (!setTickInterval(value.asInt())) return KinoError::InvalidValue;
    return KinoError::OK;
  }
  if ((strcmp(prop,"power")==0)||(strcmp(prop,"on")==0)) {
    //if(value.type != KinoVariant::BOOL) return KinoError::InvalidType;
    return KinoError::OK;
  }
  if (strcmp(prop,"live")==0) {
    //if(value.type != KinoVariant::BOOL) return KinoError::InvalidType;
    if (!setBroadcast(value.asBool())) return KinoError::InternalError;
    return KinoError::OK;
  }
  return KinoError::PropertyNotSupported;
}


// ===== Public API =====

bool HyperionDevice::begin() {
  return getStatus();
}

KinoError HyperionDevice::init() {
  if(getStatus()) return KinoError::OK;
  return KinoError::DeviceNotReady;
}

/* getStatus() V1
bool HyperionDevice::getStatus() {
  WiFiClient client;
  char payload[32];
  StaticJsonDocument<64> req;
  req["command"] = "serverinfo";
  req["tan"] = 0;
  serializeJson(req, payload);

  if (!client.connect(_ip, 8090)) {
    client.stop();
    return false;
  }

  client.printf(
    "POST /json-rpc HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: %d\r\n"
    "Connection: close\r\n\r\n",
    _ip.toString().c_str(),
    //payload.length()
    strlen(payload)
  );
  client.print(payload);

  String componentsJson;
  if (!readComponentsArray(client, componentsJson)) {
    Serial.println(F("readComponentsArray fehlgeschlagen"));
    client.stop();
    return false;
  }
  client.stop();
  return parseComponents(componentsJson);
}*/

/* getStatus() V2 Strings entfernt, helper function parseComponentsFromStream() eingeführt  */
bool HyperionDevice::getStatus() {
  EnsureTimeoutBeforeRequest(200);
  WiFiClient client;
  
  // 1. JSON erstellen
  StaticJsonDocument<64> req;
  req["command"] = "serverinfo";
  req["tan"] = 0; // Hyperion mag manchmal keine 0 als TAN

  if (!client.connect(_ip, 8090)) {
    Serial.println(F("HyperionDevice::getStatus() : could not connect"));
    client.stop();
    return false;
  }

  // 2. Header senden ohne printf/String-Objekte
  client.print(F("POST /json-rpc HTTP/1.1\r\n"));
  client.print(F("Host: ")); client.println(_ip);
  client.print(F("Content-Type: application/json\r\n"));
  client.print(F("Content-Length: ")); client.println(measureJson(req));
  client.print(F("Connection: close\r\n\r\n"));

  // 3. Payload direkt streamen (kein char-Puffer nötig!)
  serializeJson(req, client);

  // 4. Antwort verarbeiten (Stream-Parsing statt String-Buffer)
  // Wir übergeben den Client direkt an die nächste Stufe
  if (!NetworkHelper::skipHeader(client)) {
     client.stop();
     return false;
  }

  // Hier kommt jetzt dein Stream-Parsing (wie bei Hue gelernt)
  bool ok = parseComponentsFromStream(client);
  if (!ok) Serial.println(F("HyperionDevice::getStatus() : parseComponentsFromStream failed"));
  client.stop();
  return ok;
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

  //String response;
  //if (!sendJsonRpc(req, response)) return false;
  if (!sendJsonRpc(req)) return false;

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
void HyperionDevice::EnsureTimeoutBeforeRequest(unsigned long timeout) {
  static unsigned long LastRequest = 0;
  unsigned long now = millis();
  while (now - LastRequest < timeout) {
    delay(10);
    now = millis();
  }
  return;
}




/* sendJsonRpc()  V1
bool HyperionDevice::sendJsonRpc(const JsonDocument& request, String& response) {
  String payload;
  serializeJson(request, payload);
  //Serial.println(payload);
  return httpPOST("/json-rpc", payload, response);
}
*/

bool HyperionDevice::sendJsonRpc(const JsonDocument& request) {
  return httpPOST("/json-rpc", request);
}


// ===== HTTP Helper =====
/* httpPOST V1
bool HyperionDevice::httpPOST(const char* path, const String& payload, String& response) {
  WiFiClient client;
  EnsureTimeoutBeforeRequest(200);
  if (!client.connect(_ip, 8090)) {
    client.stop();
    return false;
  }

  client.printf(
    "POST %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: %d\r\n"
    "Connection: close\r\n\r\n",
    path,
    _ip.toString().c_str(),
    payload.length()
  );

  client.print(payload);

  waitForClientData(client);
  readHttpResponse(client, response);
  //Serial.println(response);
  return response.startsWith("HTTP/1.1 200");
}
*/

/* httpPOST V2 */
bool HyperionDevice::httpPOST(const char* path, const JsonDocument& request) {
  WiFiClient client;
  if (!client.connect(_ip, 8090)) {
    Serial.println(F("HyperionDevice::httpPOST(): could not connect"));
    client.stop();
    return false;
  }

  size_t len = measureJson(request);

  // Header senden
  client.print(F("POST ")); client.print(path); client.println(F(" HTTP/1.1"));
  client.print(F("Host: ")); client.println(_ip);
  client.print(F("Content-Type: application/json\r\n"));
  client.print(F("Content-Length: ")); client.println(len);
  client.println(F("Connection: close\r\n")); // Wichtig: Leerzeile folgt durch println

  // Payload direkt streamen
  serializeJson(request, client);

  // Auf Antwort warten
  if (!waitForClientData(client)) return false;

  // Header überspringen
  if (!NetworkHelper::skipHeader(client)) return false;

  // Jetzt direkt parsen (wir nutzen die Funktion von vorhin)
  return parseComponentsFromStream(client);
}

/* waitForClientData  V1  */
bool HyperionDevice::waitForClientData(WiFiClient& client) {
  unsigned long start = millis();
  while (client.connected() && !client.available()) {
    if (millis() - start > 2000) {
      client.stop();
      Serial.println(F("HyperionDevice::waitForClientData(): reached timeout"));
      return false;
    }
    yield();
  }
  bool ok = (client.available() > 0);
  if (!ok) Serial.println(F("HyperionDevice::waitForClientData(): no data available"));
  return ok;
}

bool HyperionDevice::parseComponentsFromStream(WiFiClient& client) {
  // 1. Filter erstellen: Wir wollen nur "info" -> "components"
  // Und darin nur "name" und "enabled"
  // 2026-02-02 : ersetzt durch static StaticJsonDocument<128> _filter
  //StaticJsonDocument<128> filter;
  setupFilter();
  
  //filter["info"]["components"]["*"]["name"] = true;
  //filter["info"]["components"]["*"]["enabled"] = true;
  //filter["info"]["components"] = true;

  // 2. Das Dokument für die gefilterten Daten
  // Da wir nur Namen und Bools für ca. 8-10 Komponenten speichern, reicht 1KB locker
  // neu 2026-02-02: doc ist jetzt ein privater Member
  //DynamicJsonDocument doc(1024);
  _doc.clear();
  
  // 3. Direkt vom Stream lesen
  DeserializationError err = deserializeJson(_doc, client, DeserializationOption::Filter(_filter));

  if (err) {
    Serial.print(F("Hyperion Parse Error: "));
    Serial.println(err.c_str());
    return false;
  }
  
  //serializeJson(doc, Serial);

  // 4. Daten extrahieren (Hyperion liefert info -> components)
  JsonArray components = _doc["info"]["components"];
  if (components.isNull()) {
    //Serial.println(F("HyperionDevice::parseComponentsFromStream() : components is Null"));
    return false;
  }

  for (JsonObject comp : components) {
    const char* name = comp["name"] | "";
    bool enabled = comp["enabled"] | false;

    if (strcmp(name, "ALL") == 0)       _powerStatus = enabled;
    else if (strcmp(name, "LEDDEVICE") == 0) _ledDeviceStatus = enabled;
    // Hier könntest du weitere Komponenten wie "SMOOTHING" oder "BLACKBORDER" prüfen
  }

  
  _doc.clear();
  return true;
}

StaticJsonDocument<64> HyperionDevice::_filter;
bool HyperionDevice::_filterInitialized = false;

void HyperionDevice::setupFilter() {
    if (!_filterInitialized) {
        _filter["info"]["components"] = true;
        _filterInitialized = true;
    }
}

/*
bool HyperionDevice::readHttpResponse(WiFiClient& client, String& response) {
  const unsigned long overallTimeout = 3000;
  const unsigned long idleTimeout    = 500;
  unsigned long startTime = millis();
  unsigned long lastData  = millis();

  while (true) {
    while (client.available()) {
      response += char(client.read());
      lastData = millis();
    }

    if (!client.connected()) break;
    if (millis() - lastData > idleTimeout) break;

    if (millis() - startTime > overallTimeout) {
      client.stop();
      return false;
    }

    yield();
  }
  //Serial.print(response);
  client.stop();
  return response.length() > 0;
}
*/
/*
bool HyperionDevice::readComponentsArray(WiFiClient& client, String& out) {
  const unsigned long overallTimeout = 3000;
  unsigned long startTime = millis();

  bool headerDone = false;
  bool capturing  = false;
  int  bracketDepth = 0;

  String window;   // Sliding window zum Erkennen von "components":[

  while (client.connected() || client.available()) {
    if (millis() - startTime > overallTimeout) {
      Serial.println("Overall Timeout!");
      client.stop();
      return false;
    }

    if (!client.available()) {
      yield();
      continue;
    }

    char c = client.read();

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
      client.stop();
      return true;
    }
  }

  client.stop();
  return false;
}*/
/*
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
*/


// ===== JSON Parsing =====
/*
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
}*/
