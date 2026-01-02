#include "OptomaBeamer.h"
#include "NetworkHelper.h"

// ===== Konstruktoren =====

OptomaBeamer::OptomaBeamer(const IPAddress& ip, uint8_t beamerId)
: _ip(ip), _id(beamerId) {}

OptomaBeamer::OptomaBeamer(const String& ip, uint8_t beamerId)
: _id(beamerId) {
  _ip.fromString(ip);
}

// neue Public API, als Wrapper auf alte Public API
KinoError OptomaBeamer::get(const char* prop, KinoVariant& out) {
  if (!prop) return KinoError::PropertyNotSupported;
  if (strcmp(prop,"tickInterval")==0) {
    out = KinoVariant::fromInt(_tickInterval);
    return KinoError::OK;
  }
  if ((strcmp(prop,"power")==0)||(strcmp(prop,"on")==0)) {
    out = KinoVariant::fromBool(_powerState);
    return KinoError::OK;
  }
  if (strcmp(prop,"input")==0) {
    out = KinoVariant::fromString(OptomaSourceLookup::toString(_source));
    return KinoError::OK;
  }
  if (strcmp(prop,"uptime")==0) {
    out = KinoVariant::fromInt(_lampHours);
    return KinoError::OK;
  }
  return KinoError::PropertyNotSupported;
}

KinoError OptomaBeamer::set(const char* prop, const KinoVariant& value) {
  if (strcmp(prop,"tickInterval")==0) {
    if(value.type != KinoVariant::INT) return KinoError::InvalidType;
    if (!setTickInterval(value.i)) return KinoError::InvalidValue;
    return KinoError::OK;
  }
  if ((strcmp(prop,"power")==0)||(strcmp(prop,"on")==0)) {
    if(value.type != KinoVariant::BOOL) return KinoError::InvalidType;
    if (!setPower(value.b)) return KinoError::InternalError;
    return KinoError::OK;
  }
  if (strcmp(prop,"input")==0) {
    if(value.type != KinoVariant::STRING) return KinoError::InvalidType;
    if (!setSource(String(value.s))) return KinoError::InvalidValue;
    return KinoError::OK;
  }
  return KinoError::PropertyNotSupported;
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
  String response;
  if (!sendCommand("150", "1", response)) {
    return false;
  }
  return parseStatusResponse(response);
}



bool OptomaBeamer::tick() {
  if (_tickInterval == 0) return false;
  int now = millis();
  if (now - _lastTick >= _tickInterval) {
    _lastTick = now;
    return getStatus();
  }
  return false;
}

bool OptomaBeamer::setPower(bool onoff) {
  if (!init()) return false;
  if (_powerState == onoff) return true;
  String response;
  if (!sendCommand("00", onoff ? "1" : "0", response)) {
    return false;
  }
  _powerState = onoff;
  return true;
}

bool OptomaBeamer::setSource(InputSource src) {
  String param;
  if (!OptomaSourceLookup::toSetParameter(src, param)) {
    return false;
  }

  String response;
  if (!sendCommand("12", param, response)) {
    return false;
  }

  _source = src;
  return true;
}

bool OptomaBeamer::setSource(const String& srcName) {
  InputSource src = OptomaSourceLookup::fromString(srcName);
  if (src == InputSource::Unknown) {
    return false;
  }
  return setSource(src);
}

bool OptomaBeamer::setDisplayMode(DisplayMode dm) {
  String param = encodeDisplayMode(dm);

  String response;
  if (!sendCommand("20", param, response)) {
    return false;
  }

  _displayMode = dm;
  return true;
}

bool OptomaBeamer::freeze(bool onoff) {
  String response;
  return sendCommand("04", onoff ? "1" : "0", response);
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

bool OptomaBeamer::sendCommand(const String& command,
                              const String& parameter,
                              String& response) {
  NetworkHelper::resetClient(_client);
  EnsureTimeoutBeforeRequest(200);
  
  if (!_client.connect(_ip, 23)) {
    return false;
  }

  char cmd[32];
  if (parameter.length() > 0) {
    snprintf(cmd, sizeof(cmd),
             "~%02u%s %s",
             _id,
             command.c_str(),
             parameter.c_str());
  } else {
    snprintf(cmd, sizeof(cmd),
             "~%02u%s",
             _id,
             command.c_str());
  }

  _client.print(cmd);
  _client.print("\r");

  unsigned long start = millis();
  while (!_client.available()) {
    if (millis() - start > 2000) {
      _client.stop();
      return false;
    }
  }

  response = _client.readStringUntil('\r');
  _client.stop();
  return isOkResponse(response);
}

bool OptomaBeamer::isOkResponse(const String& response) {
  return response.startsWith("Ok");
}

// ===== Parsing =====

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

// ===== Encoding =====

String OptomaBeamer::encodeDisplayMode(DisplayMode dm) const {
  switch (dm) {
    case DisplayMode::Presentation: return "";
    case DisplayMode::Bright:       return "2";
    case DisplayMode::Movie:        return "3";
    case DisplayMode::sRGB:         return "4";
    case DisplayMode::User:         return "5";
    case DisplayMode::Blackboard:   return "7";
    case DisplayMode::DICOM_SIM:    return "13";
    default:                        return "";
  }
}
