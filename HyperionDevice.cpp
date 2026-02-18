#include "HyperionDevice.h"
#include "NetworkHelper.h"

// ===== Konstruktoren =====

HyperionDevice::HyperionDevice(const IPAddress& ip)
: _ip(ip) {_dirty = NONE;}

HyperionDevice::HyperionDevice(const String& ip) {
  _ip.fromString(ip);
  _dirty = NONE;
}

KinoError HyperionDevice::tick() {
  if (WiFi.status() != WL_CONNECTED) return KinoError::NothingToDo;
  if (_tickInterval == 0) return KinoError::NothingToDo;
  if (_refreshing) return KinoError::NothingToDo;
  
  unsigned long now = millis();
  if (now - _lastTick >= _tickInterval) {
    _lastTick = now;
    _refreshing = true;
    bool ok = getStatus();
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
    out.setInt(_tickInterval);
    return KinoError::OK;
  }
  if (strcmp(prop,"ip")==0) {
    char buf[20];
    snprintf(buf, sizeof(buf), "%d.%d.%d.%d", _ip[0], _ip[1], _ip[2], _ip[3]);
    out.setString(buf);
    return KinoError::OK;
  }
  if ((strcmp(prop,"power")==0)||(strcmp(prop,"on")==0)) {
    out.setBool(_powerStatus);
    return KinoError::OK;
  }
  if (strcmp(prop,"live")==0) {
    out.setBool(_ledDeviceStatus && _powerStatus);
    return KinoError::OK;
  }
  return KinoError::PropertyNotSupported;
}

KinoError HyperionDevice::set(const char* prop, const KinoVariant& value) {
  if (strcmp(prop,"tickInterval")==0) {
    if (!setTickInterval(value.asInt())) return KinoError::InvalidValue;
    return KinoError::OK;
  }
  if ((strcmp(prop,"power")==0)||(strcmp(prop,"on")==0)) {
    return KinoError::OK;
  }
  if (strcmp(prop,"live")==0) {
    if (!setBroadcast(value.asBool())) return KinoError::InternalError;
    return KinoError::OK;
  }
  return KinoError::PropertyNotSupported;
}

bool HyperionDevice::getStatusUpdate(const char* devName, JsonObject& root) {
  if (_dirty > 0) {
    root["dev"].set(devName);
    if (_dirty & ON)    root["on"].set(_powerStatus);
    if (_dirty & LIVE)  root["live"].set(_ledDeviceStatus);
    _dirty = NONE;
    return true;
  }
  return false;
}

// ===== Public API =====

bool HyperionDevice::begin() {
  return getStatus();
}

KinoError HyperionDevice::init() {
  if(getStatus()) return KinoError::OK;
  return KinoError::DeviceNotReady;
}

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
    NetworkHelper::resetClient(client);
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
     NetworkHelper::resetClient(client);
     return false;
  }

  // Hier kommt jetzt dein Stream-Parsing (wie bei Hue gelernt)
  bool ok = parseComponentsFromStream(client);
  if (!ok) Serial.println(F("HyperionDevice::getStatus() : parseComponentsFromStream failed"));
  NetworkHelper::resetClient(client);
  return ok;
}



bool HyperionDevice::isBroadcasting() const {
  return _powerStatus && _ledDeviceStatus;
}

bool HyperionDevice::setBroadcast(bool onoff) {
  StaticJsonDocument<192> req;
  req["command"]  = "componentstate";
  req["tan"]      = 0;

  JsonObject params = req.createNestedObject("componentstate");
  params["component"] = "LEDDEVICE";
  params["state"]     = onoff;

  bool ok = sendJsonRpc(req);
  if (ok) _ledDeviceStatus = onoff;
  return ok;
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

bool HyperionDevice::sendJsonRpc(const JsonDocument& request) {
  return httpPOST("/json-rpc", request);
}


// ===== HTTP Helper =====

/* httpPOST V2 */
bool HyperionDevice::httpPOST(const char* path, const JsonDocument& request) {
  WiFiClient client;
  if (!client.connect(_ip, 8090)) {
    Serial.println(F("HyperionDevice::httpPOST(): could not connect"));
    NetworkHelper::resetClient(client);
    return false;
  }

  size_t len = measureJson(request);
  char ipbuf[20];
  snprintf(ipbuf, 20, "%d.%d.%d.%d", _ip[0], _ip[1], _ip[2], _ip[3]);

  // Header senden
  client.printf(
    "POST %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: %d\r\n"
    "Connection: close\r\n\r\n",
    path,
    ipbuf,
    len
  );

  // Payload direkt streamen
  serializeJson(request, client);

  // Auf Antwort warten
  if (!waitForClientData(client)) {
    NetworkHelper::resetClient(client);
    return false;
  }

  // Header überspringen
  if (!NetworkHelper::skipHeader(client)) {
    NetworkHelper::resetClient(client);
    return false;
  }

  // Jetzt direkt parsen (wir nutzen die Funktion von vorhin)
  // nach POST bekommt man keine vollständige Json- Antwort über den Status zurück!
  // Trotzdem versuchen wir es, damit wenigstens der client sauber zurückgelassen wird.
  parseComponentsFromStream(client); 
  NetworkHelper::resetClient(client);
  return true;
}

/* waitForClientData  V1  */
bool HyperionDevice::waitForClientData(WiFiClient& client) {
  unsigned long start = millis();
  while (client.connected() && !client.available()) {
    if (millis() - start > 2000) {
      NetworkHelper::resetClient(client);
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
  setupFilter();

  // 2. Das Dokument für die gefilterten Daten
  // Da wir nur Namen und Bools für ca. 8-10 Komponenten speichern, reicht 1KB locker
  _doc.clear();
  
  // 3. Direkt vom Stream lesen
  DeserializationError err = deserializeJson(_doc, client, DeserializationOption::Filter(_filter));

  if (err) {
    Serial.print(F("Hyperion Parse Error: "));
    Serial.println(err.c_str());
    return false;
  }
  
  // 4. Daten extrahieren (Hyperion liefert info -> components)
  JsonArray components = _doc["info"]["components"];
  if (components.isNull()) {
    return false;
  }

  for (JsonObject comp : components) {
    const char* name = comp["name"] | "";
    bool enabled = comp["enabled"] | false;

    if (strcmp(name, "ALL") == 0) {
      if (_powerStatus != enabled) _dirty |= ON;
      _powerStatus = enabled;
    }
    else if (strcmp(name, "LEDDEVICE") == 0) {
      if (_ledDeviceStatus != enabled) _dirty |= LIVE;
      _ledDeviceStatus = enabled;
    }
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
