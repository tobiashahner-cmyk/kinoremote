#include "HyperionDevice.h"
#include "NetworkHelper.h"

extern StaticJsonDocument<2048> httpJson;

// ===== Konstruktoren =====

HyperionDevice::HyperionDevice(const IPAddress& ip)
: _ip(ip) {_dirty = NONE;}

HyperionDevice::HyperionDevice(const String& ip) {
  _ip.fromString(ip);
  _dirty = NONE;
}

KinoError HyperionDevice::tick() {
  if (WiFi.status() != WL_CONNECTED) return KinoError::NothingToDo;
  bool ok = getStatus();
  return (ok ? KinoError::OK : KinoError::DeviceNotReady);
}

const KinoPropertyInfo HyperionDevice::_properties[] PROGMEM = {
  //{ "tickInterval", "Aktualisierung [ms]", Prop_Read | Prop_Write ,0,20000,500},
  { "ip",         "IP",         Prop_Read                         },
  { "on",         "Power",      Prop_Read  | Prop_Write           },
  { "live",       "Broadcast",  Prop_Read  | Prop_Write           }
};

size_t HyperionDevice::getPropertyCount() const {
  return sizeof(_properties) / sizeof(_properties[0]);
}

const KinoPropertyInfo* HyperionDevice::getPropertyInfo(size_t index) const {
  if (index >= getPropertyCount()) return nullptr;
  //return &_properties[index];
  static KinoPropertyInfo buffer; 
  memcpy_P(&buffer, &_properties[index], sizeof(KinoPropertyInfo));
  
  return &buffer;
}

KinoError HyperionDevice::get(const char* prop, KinoVariant& out) {
  if (!prop) return KinoError::PropertyNotSupported;
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
    root["dev"].set((char*)devName);
    if (_dirty & ON)    root["on"].set(_powerStatus);
    if (_dirty & LIVE)  root["live"].set(_ledDeviceStatus);
    _dirty = NONE;
    return true;
  }
  return false;
}

bool HyperionDevice::begin() {
  return getStatus();
}

KinoError HyperionDevice::init() {
  if(getStatus()) return KinoError::OK;
  return KinoError::DeviceNotReady;
}

bool HyperionDevice::getStatus() {
  EnsureTimeoutBeforeRequest(200);
  WiFiClient wifi;
  HTTPClient http;
  
  // 1. JSON erstellen
  StaticJsonDocument<64> req;
  req["command"] = "serverinfo";
  req["tan"] = 0; // Hyperion mag nur 0 als TAN

  char url[56];
  snprintf(url, sizeof(url), "http://%d.%d.%d.%d:8090/json-rpc", _ip[0], _ip[1], _ip[2], _ip[3]);

  char jsonBuffer[64];
  serializeJson(req, jsonBuffer);
  if (!http.begin(wifi, url)) {
    Serial.println(F("[HyperionDevice::getStatus] could not connect"));
    NetworkHelper::resetWiFiClient(wifi);
    yield();
    delay(1000);
    return false;
  }
  size_t len = strlen(jsonBuffer);
  http.addHeader(F("Content-Type"), F("application/json"));
  int httpCode = http.POST((uint8_t*)jsonBuffer, len);
  bool success = (httpCode == HTTP_CODE_OK || httpCode == 204 || httpCode == 207);
  if (!success) {
    Serial.printf("[HyperionDevice::getStatus] POST failed, error: %s (%d)\n", http.errorToString(httpCode).c_str(), httpCode);
    NetworkHelper::resetClients(wifi, http, false);
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  bool ok = parseComponentsFromStream(*stream);
  if (!ok) Serial.println(F("HyperionDevice::getStatus() : parseComponentsFromStream failed"));
  NetworkHelper::resetClients(wifi, http, true);
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
  if (ok) {
    _ledDeviceStatus = onoff;
    _dirty |= LIVE;
  }
  return ok;
}

bool HyperionDevice::startBroadcast() {
  return setBroadcast(true);
}

bool HyperionDevice::stopBroadcast() {
  return setBroadcast(false);
}

bool HyperionDevice::getPowerStatus() const {
  return _powerStatus;
}

bool HyperionDevice::getLedDeviceStatus() const {
  return _ledDeviceStatus;
}

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
  return httpPOST("json-rpc", request);
}

bool HyperionDevice::httpPOST(const char* path, const JsonDocument& request) {
  WiFiClient wifi;
  HTTPClient http;
  
  char url[64];
  snprintf(url, sizeof(url), "http://%d.%d.%d.%d:8090/%s", _ip[0], _ip[1], _ip[2], _ip[3], path);

  char jsonBuffer[128];
  serializeJson(request, jsonBuffer);
  if (!http.begin(wifi, url)) {
    Serial.println(F("[HyperionDevice::getStatus] could not connect"));
    NetworkHelper::resetWiFiClient(wifi);
    yield();
    delay(1000);
    return false;
  }
  
  size_t len = strlen(jsonBuffer);
  http.addHeader(F("Content-Type"), F("application/json"));
  int httpCode = http.POST((uint8_t*)jsonBuffer, len);
  
  bool success = (httpCode == HTTP_CODE_OK || httpCode == 204 || httpCode == 207);
  if (!success) {
    Serial.printf("[HyperionDevice::httpPOST] POST failed, error: %s (%d)\n", http.errorToString(httpCode).c_str(), httpCode);
    NetworkHelper::resetClients(wifi, http, false);
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  bool ok = parsePostResponse(*stream);
  if (!ok) Serial.println(F("HyperionDevice::httpPOST] parsePostResponse failed"));
  NetworkHelper::resetClients(wifi, http, true);
  return ok;
}

bool HyperionDevice::parseComponentsFromStream(Stream& client) {
  setupFilter();

  // 2. Das Dokument für die gefilterten Daten
  // Da wir nur Namen und Bools für ca. 8-10 Komponenten speichern, reicht 1KB locker
  httpJson.clear();
  
  // 3. Direkt vom Stream lesen
  DeserializationError err = deserializeJson(httpJson, client, DeserializationOption::Filter(_filter));

  if (err) {
    Serial.print(F("Hyperion Parse Error: "));
    Serial.println(err.c_str());
    return false;
  }
  
  // 4. Daten extrahieren (Hyperion liefert info -> components)
  JsonArray components = httpJson["info"]["components"];
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

  httpJson.clear();
  return true;
}

bool HyperionDevice::parsePostResponse(Stream& s) {
  StaticJsonDocument<64> filter;
  httpJson.clear();
  filter["success"] = true;
  filter["tan"] = true;
  DeserializationError e = deserializeJson(httpJson, s, DeserializationOption::Filter(filter));
  return (!e && httpJson["success"]);
}

StaticJsonDocument<64> HyperionDevice::_filter;
bool HyperionDevice::_filterInitialized = false;

void HyperionDevice::setupFilter() {
    if (!_filterInitialized) {
        _filter["info"]["components"] = true;
        _filterInitialized = true;
    }
}
