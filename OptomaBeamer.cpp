#include "OptomaBeamer.h"
#include "NetworkHelper.h"

// ===== Konstruktoren =====

OptomaBeamer::OptomaBeamer(const IPAddress& ip, uint8_t beamerId)
: _ip(ip), _id(beamerId) {_dirty = NONE;}

OptomaBeamer::OptomaBeamer(const String& ip, uint8_t beamerId)
: _id(beamerId) {
  _ip.fromString(ip);
  _dirty = NONE;
}

// neue Public API, als Wrapper auf alte Public API
const KinoPropertyInfo OptomaBeamer::_properties[] = {
  { "tickInterval", "Aktualisierung [ms]", Prop_Read | Prop_Write , 0, 20000, 500},
  { "ip",         "IP",             Prop_Read                         },
  { "on",         "Power",          Prop_Read  | Prop_Write           },
  { "uptime",     "Lampenstunden",  Prop_Read                         },
  { "input",      "Eingang",        Prop_Read  | Prop_Write  | Prop_Query }
};

size_t OptomaBeamer::getPropertyCount() const {
  return sizeof(_properties) / sizeof(_properties[0]);
}

const KinoPropertyInfo* OptomaBeamer::getPropertyInfo(size_t index) const {
  if (index >= getPropertyCount()) return nullptr;
  return &_properties[index];
}

KinoError OptomaBeamer::get(const char* prop, KinoVariant& out) {
  if (!prop) return KinoError::PropertyNotSupported;
  if (strcmp(prop,"tickInterval")==0) {
    out.setInt(_tickInterval);
    return KinoError::OK;
  }
  if (strcmp(prop,"ip")==0) {
    char buf[20];
    snprintf(buf, 20, "%d.%d.%d.%d", _ip[0], _ip[1], _ip[2], _ip[3]);
    out.setString(buf);
    return KinoError::OK;
  }
  if ((strcmp(prop,"power")==0)||(strcmp(prop,"on")==0)) {
    out.setBool(_powerState);
    return KinoError::OK;
  }
  if (strcmp(prop,"input")==0) {
    out.setString(OptomaSourceLookup::toString(_source));
    return KinoError::OK;
  }
  if (strcmp(prop,"uptime")==0) {
    out.setInt(_lampHours);
    return KinoError::OK;
  }
  return KinoError::PropertyNotSupported;
}

KinoError OptomaBeamer::set(const char* prop, const KinoVariant& value) {
  if (strcmp(prop,"tickInterval")==0) {
    if (!setTickInterval(value.asInt())) return KinoError::InvalidValue;
    return KinoError::OK;
  }
  if ((strcmp(prop,"power")==0)||(strcmp(prop,"on")==0)) {
    if (!setPower(value.asBool())) return KinoError::InvalidValue;
    return KinoError::OK;
  }
  if (strcmp(prop,"input")==0) {
    if (!setSource(value.c_str())) return KinoError::InvalidValue;
    return KinoError::OK;
  }
  return KinoError::PropertyNotSupported;
}

KinoError OptomaBeamer::queryCount(const char* property, uint16_t& out) {
  if (strcmp(property, "input")==0) {
    out = OptomaSourceLookup::getTableSize();
    return KinoError::OK;
  }
  return KinoError::PropertyNotSupported;
}

KinoError OptomaBeamer::query(const char* property, uint16_t index, KinoVariant &out) {
  if (strcmp(property,"input")==0) {
    char label[20];
    if (!OptomaSourceLookup::labelByIndex(index, label, sizeof(label))) return KinoError::OutOfRange;
    out.setString(label);
    return KinoError::OK;
  }
  return KinoError::PropertyNotSupported;
}

bool OptomaBeamer::getStatusUpdate(const char* devName, JsonObject& root) {
  if (_dirty > 0) {
    root["dev"].set(devName);
    if (_dirty & ON)      root["on"].set(_powerState);
    if (_dirty & SOURCE)  root["input"].set(OptomaSourceLookup::toString(_source));
    if (_dirty & UPTIME)  root["uptime"].set(_lampHours);
    _dirty = NONE;
    return true;
  }
  return false;
}

// ===== Public API =====

bool OptomaBeamer::begin() {
  return getStatus();
}

KinoError OptomaBeamer::init() {
  if(getStatus()) return KinoError::OK;
  return KinoError::DeviceNotReady;
}

bool OptomaBeamer::getStatus() {
  char response[16];
  if (!sendCommand("150", 1, response, sizeof(response))) {
    return false;
  }
  return parseStatusResponse(response);
}

KinoError OptomaBeamer::tick() {
  if (WiFi.status() != WL_CONNECTED) return KinoError::NothingToDo;
  if (_tickInterval == 0) return KinoError::NothingToDo;
  if (_refreshing) return KinoError::NothingToDo;
  int now = millis();
  if (now - _lastTick >= _tickInterval) {
    _lastTick = now;
    _refreshing = true;
    bool ok = getStatus();
    _refreshing = false;
    return (ok ? KinoError::OK : KinoError::DeviceNotReady);
  }
  return KinoError::NothingToDo;
}

bool OptomaBeamer::setPower(bool onoff) {
  if (!getStatus()) return false;
  if (_powerState == onoff) return true;
  char response[3];
  if (!sendCommand("00", onoff ? 1 : 0, response, sizeof(response))) {
    return false;
  }
  _powerState = onoff;
  return true;
}

bool OptomaBeamer::setSource(InputSource src) {
  uint8_t param;
  if (!OptomaSourceLookup::toSetParameter(src, param)) {
    return false;
  }

  char response[3];
  if (!sendCommand("12", param, response, sizeof(response))) {
    return false;
  }

  _source = src;
  return true;
}

bool OptomaBeamer::setSource(const char* srcName) {
  InputSource src = OptomaSourceLookup::fromString(srcName);
  if (src == InputSource::Unknown) {
    return false;
  }
  return setSource(src);
}

bool OptomaBeamer::setDisplayMode(DisplayMode dm) {
  uint8_t param = encodeDisplayMode(dm);

  char response[3];
  if (!sendCommand("20", param, response, sizeof(response))) {
    return false;
  }

  _displayMode = dm;
  return true;
}

bool OptomaBeamer::freeze(bool onoff) {
  char response[3];
  return sendCommand("04", onoff ? 1 : 0, response, sizeof(response));
}

bool OptomaBeamer::setTickInterval(int ms) {
  if (ms == 0) { _tickInterval = 0; return true; }
  if (ms < 0) return false;       // nur für bessere Lesbarkeit hier. negative Werte sind unerlaubt
  if (ms < 2000) return false;    // schneller als alle 2 Sekunden erzeugt zu viel Traffic
  _tickInterval = ms;
  return true;
}

// ===== Getter =====

bool OptomaBeamer::getPowerStatus() const {
  return _powerState;
}

OptomaBeamer::InputSource OptomaBeamer::getSource() const {
  return _source;
}

const char* OptomaBeamer::getSourceString() {
  return OptomaSourceLookup::toString(_source);
}

OptomaBeamer::DisplayMode OptomaBeamer::getDisplayMode() const {
  return _displayMode;
}

int OptomaBeamer::getLampHours() const {
  return _lampHours;
}

int OptomaBeamer::getTickInterval() {
  return _tickInterval;
}

// ===== Helper =====

void OptomaBeamer::EnsureTimeoutBeforeRequest(unsigned long timeout) {
  static unsigned long LastRequest = 0;
  unsigned long now = millis();
  while (now - LastRequest < timeout) {
    delay(10);
    now = millis();
  }
  return;
}

bool OptomaBeamer::sendCommand(const char* command, const int parameter, char* response, size_t responseLen) {
  EnsureTimeoutBeforeRequest(200); // Deine bestehende Logik
  
  WiFiClient client;
  if (!client.connect(_ip, 23)) {
    NetworkHelper::resetClient(client);
    return false;
  }

  char cmd[32];
  if (parameter >= 0) {
    snprintf(cmd, sizeof(cmd), "~%02u%s %d", _id, command, parameter);
  } else {
    snprintf(cmd, sizeof(cmd), "~%02u%s", _id, command);
  }

  client.print(cmd);
  client.print("\r");

  // --- Antwort in response lesen (max responseLen Zeichen) ---
  unsigned long start = millis();
  size_t index = 0;
  bool foundDelimiter = false;

  // Warten auf Daten mit Timeout (2000ms)
  while (millis() - start < 2000) {
    while (client.available()) {
      char c = client.read();
      
      if (c == '\r') {
        foundDelimiter = true;
        break;
      }
      
      // Zeichen im Buffer speichern, sofern noch Platz ist (1 Byte für \0 lassen)
      if (index < responseLen - 1) {
        response[index++] = c;
      }
    }
    if (foundDelimiter) break;
    yield(); // ESP8266 Background-Tasks füttern
  }

  // Null-Terminator immer setzen!
  response[index] = '\0';

  NetworkHelper::resetClient(client);
  
  if (!foundDelimiter && index == 0) return false; // Timeout ohne Daten
  
  return isOkResponse(response);
}

bool OptomaBeamer::isOkResponse(const char* response) {
  return (strncasecmp(response,"OK",2)==0);
}

// ===== Parsing =====
/*
bool OptomaBeamer::parseStatusResponse(const String& response) {
  if (!isOkResponse(response) || response.length() < 13) {
    return false;
  }

  _powerState = (response.charAt(2) == '1');
  _lampHours  = response.substring(3, 7).toInt();

  uint8_t readCode = response.substring(7, 9).toInt();
  _source = OptomaSourceLookup::fromReadCode(readCode);

  return true;
}
*/

bool OptomaBeamer::parseStatusResponse(const char* response) {
  // 1. Plausibilitäts-Check (Länge und OK-Status)
  if (!isOkResponse(response) || strlen(response) < 13) {
    return false;
  }

  // 2. Power State (Direkter Zugriff auf den Index)
  _powerState = (response[2] == '1');

  // 3. Lampenstunden (response.substring(3, 7).toInt())
  _lampHours = parseFixedInt(response, 3, 4);

  // 4. Source Code (response.substring(7, 9).toInt())
  uint8_t readCode = (uint8_t)parseFixedInt(response, 7, 2);
  _source = OptomaSourceLookup::fromReadCode(readCode);

  return true;
}

/**
 * Liest aus str die length Zeichen ab start und übersetzt sie nach int
 */
int OptomaBeamer::parseFixedInt(const char* str, size_t start, size_t length) {
    char temp[8]; 
    strncpy(temp, str + start, length);
    temp[length] = '\0';
    
    // strtol(String, EndPointer, Basis)
    return (int)strtol(temp, nullptr, 10); 
}

// ===== Encoding =====

uint8_t OptomaBeamer::encodeDisplayMode(DisplayMode dm) const {
  switch (dm) {
    case DisplayMode::Presentation: return -1;
    case DisplayMode::Bright:       return 2;
    case DisplayMode::Movie:        return 3;
    case DisplayMode::sRGB:         return 4;
    case DisplayMode::User:         return 5;
    case DisplayMode::Blackboard:   return 7;
    case DisplayMode::DICOM_SIM:    return 13;
    default:                        return -1;
  }
}
