#include "WLEDDevice.h"
#include "NetworkHelper.h"

// ===== Konstruktoren =====

WLEDDevice::WLEDDevice(const IPAddress& ip)
: _ip(ip) {}

WLEDDevice::WLEDDevice(const String& ip) {
  _ip.fromString(ip);
}

// ===== Public API =====

bool WLEDDevice::begin() {
  if (!readState()) return false;
  return true;
}

bool WLEDDevice::getStatus() {
  if (!readState()) return false;
  return true;
}

bool WLEDDevice::tick() {
  if (_tickInterval == 0) return false;
  
  unsigned long now = millis();
  if (now - _lastTick >= _tickInterval) {
    _lastTick = now;
    readState();
    return true;
  }
  return false;
}

bool WLEDDevice::setTickInterval(int ms) {
  if (ms == 0) { _tickInterval = 0; return true; }
  if (ms < 0) return false;       // nur zur besseren Lesbarkeit hier aufgeführt
  if (ms < 2000) return false;    // unter 2 Sekunden Interval führt zu übermässigem Traffic
  _tickInterval = ms;
  return true;
}

int WLEDDevice::getTickInterval() {
  return _tickInterval;
}

// ===== Getter =====

bool WLEDDevice::getPowerStatus() const {
  return _props["state"]["on"] | false;
}

bool WLEDDevice::inAlarm() const {
  return _alarm;
}

bool WLEDDevice::inPause() const {
  return _pause;
}

uint8_t WLEDDevice::getBrightness() const {
  return _props["state"]["bri"] | 0;
}

bool WLEDDevice::isReceivingLiveData() const {
  return _props["info"]["live"] | false;
}

bool WLEDDevice::isOverridingLiveData() const {
  return _props["state"]["lor"] | true;
}

uint8_t WLEDDevice::getSpeed() const {
  return _props["state"]["seg"][0]["fx"] | 0;
}

uint8_t WLEDDevice::getIntensity() const {
  return _props["state"]["seg"][0]["ix"] | 0;
}

uint16_t WLEDDevice::getEffect() const {
  return _props["state"]["seg"][0]["fx"] | 0;
}

uint8_t WLEDDevice::getPalette() const {
  return _props["state"]["seg"][0]["pal"] | 0;
}

String WLEDDevice::getLiveSource() const {
  return _props["info"]["lm"] | "";
}

WLEDColor WLEDDevice::getColFg() const {
  WLEDColor c = {
    _props["state"]["seg"][0]["col"][0][0] | 0, 
    _props["state"]["seg"][0]["col"][0][1] | 0, 
    _props["state"]["seg"][0]["col"][0][2] | 0 
  };
  return c;
}

WLEDColor WLEDDevice::getColBg() const {
  WLEDColor c = {
    _props["state"]["seg"][0]["col"][1][0] | 0, 
    _props["state"]["seg"][0]["col"][1][1] | 0, 
    _props["state"]["seg"][0]["col"][1][2] | 0 
  };
  return c;
}

WLEDColor WLEDDevice::getColFx() const {
  WLEDColor c = {
    _props["state"]["seg"][0]["col"][2][0] | 0, 
    _props["state"]["seg"][0]["col"][2][1] | 0, 
    _props["state"]["seg"][0]["col"][2][2] | 0 
  };
  return c;
}

// ===== Setter =====

bool WLEDDevice::setPowerStatus(bool onoff) {
  _newProps["state"]["on"] = onoff;
  return true;
}

bool WLEDDevice::setBrightness(uint8_t bri) {
  if (bri < 0)   return false;
  if (bri > 255) return false;

  _newProps["state"]["bri"] = bri;
  return true;
}

bool WLEDDevice::setTransitionTime(int tt) {
  if (tt < 0) return false;
  if (tt < 100) tt = 100;
  _newProps["state"]["tt"] = (int)(tt/100);
  return true;
}

bool WLEDDevice::setEffect(uint16_t effect) {
  _newProps["state"]["seg"][0]["fx"] = effect;
  return true;
}

bool WLEDDevice::setSpeed(uint8_t sx) {
  if (sx < 0)   return false;
  if (sx > 255) return false;
  _newProps["state"]["seg"][0]["sx"] = sx;
  return true;
}

bool WLEDDevice::setIntensity(uint8_t ix) {
  if (ix < 0)   return false;
  if (ix > 255) return false;
  _newProps["state"]["seg"][0]["ix"] = ix;
  return true;
}

bool WLEDDevice::setFgColor(uint8_t R, uint8_t G, uint8_t B) {
  if ((R<0)||(R>255)||(G<0)||(G>255)||(B<0)||(B>255)) return false;
  _newProps["state"]["seg"][0]["col"][0][0] = R;
  _newProps["state"]["seg"][0]["col"][0][1] = G;
  _newProps["state"]["seg"][0]["col"][0][2] = B;
  return true;
}

bool WLEDDevice::setFgColor(WLEDColor c) {
  return setFgColor(c.r, c.g, c.b);
}

bool WLEDDevice::setBgColor(uint8_t R, uint8_t G, uint8_t B) {
  if ((R<0)||(R>255)||(G<0)||(G>255)||(B<0)||(B>255)) return false;
  _newProps["state"]["seg"][0]["col"][1][0] = R;
  _newProps["state"]["seg"][0]["col"][1][1] = G;
  _newProps["state"]["seg"][0]["col"][1][2] = B;
  return true;
}

bool WLEDDevice::setBgColor(WLEDColor c) {
  return setBgColor(c.r, c.g, c.b);
}

bool WLEDDevice::setFxColor(uint8_t R, uint8_t G, uint8_t B) {
  if ((R<0)||(R>255)||(G<0)||(G>255)||(B<0)||(B>255)) return false;
  _newProps["state"]["seg"][0]["col"][2][0] = R;
  _newProps["state"]["seg"][0]["col"][2][1] = G;
  _newProps["state"]["seg"][0]["col"][2][2] = B;
  return true;
}

bool WLEDDevice::setFxColor(WLEDColor c) {
  return setFxColor(c.r, c.g, c.b);
}

bool WLEDDevice::setLive(bool onoff) {
  _newProps["state"]["lor"] = !onoff;
  return true;
}

// Wrapper für besondere Effekte, die oft genutzt werden:

bool WLEDDevice::fade(uint8_t bri, int tt) {
  if (!setBrightness(bri)) return false;
  if (!setTransitionTime(tt)) return false;
  return applyChanges();
}

bool WLEDDevice::setAlarm(bool onoff) {
  if (onoff == _alarm) return false;              // signal back to controller: no change in alarm state
  _alarm = onoff;
  if (onoff && !_pause) return backupState();     // Gehe von "normal" auf "Alarm"
  if (!onoff && _pause) return true;              // schalte "Alarm" aus. Aufrufende Funktion kümmert sich um Reaktivierung des Pausemodus, wir halten hier nur den Backupstate bereit
  if (onoff && _pause)  return true;              // Gehe von "Pause" auf "Alarm". Das Backup von "Pause" bleibt gültig
  if (!onoff && !_pause)return restoreBackup();   // Gehe von "Alarm" auf "normal". Stelle das Backup wieder her
  return true;
}

bool WLEDDevice::setPause(bool onoff) {
  if (_alarm) return false;                       // Von "Alarm" kann man nicht auf "Pause" wechseln!
  _pause = onoff;
  return (onoff ? backupState() : restoreBackup()); // Beim Pause einschalten: Backup erstellen. Beim Pause ausschalten: Backup wiederherstellen
}

bool WLEDDevice::backupState() {
  if (_alarm || _pause) return true;    // nur Backup machen, wenn wir nicht sowieso schon in Alarm oder Pause sind!
  _bkp.onoff = getPowerStatus();
  _bkp.fx = getEffect();
  _bkp.bri = getBrightness();
  _bkp.sx = getSpeed();
  _bkp.ix = getIntensity();
  _bkp.pal = getPalette();
  _bkp.fgCol = getColFg();
  _bkp.bgCol = getColBg();
  _bkp.fxCol = getColFx();
  return true;
}

bool WLEDDevice::restoreBackup() {
  setPowerStatus(_bkp.onoff);
  setEffect(_bkp.fx);
  setBrightness(_bkp.bri);
  setSpeed(_bkp.sx);
  setIntensity(_bkp.ix);
  setPalette(_bkp.pal);
  setFgColor(_bkp.fgCol);
  setBgColor(_bkp.bgCol);
  setFxColor(_bkp.fxCol);
  return applyChanges();
}

bool WLEDDevice::setCustom(uint8_t c1x, uint8_t c2x, uint8_t c3x) {
  _newProps["state"]["seg"][0]["c1x"] = c1x;
  _newProps["state"]["seg"][0]["c2x"] = c2x;
  _newProps["state"]["seg"][0]["c3x"] = c3x;
  return true;
}

bool WLEDDevice::setPalette(uint8_t pal) {
  _newProps["state"]["seg"][0]["pal"] = pal;
  return true;
}

// ===== HTTP Helper =====
bool WLEDDevice::readState() {
  NetworkHelper::resetClient(_client);
  if (!_client.connect(_ip, 80)) {
    Serial.print("WLED: GET ");
    Serial.println("/json");
    Serial.println("could not connect");
    return false;
  }
  _client.printf(
    "GET /json HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Connection: close\r\n\r\n",
    _ip.toString().c_str()
  );
  if (!NetworkHelper::skipHeader(_client)) return false;

  initFilter();
  DeserializationError error = deserializeJson(_props, _client, DeserializationOption::Filter(_jsonFilter));

  if (!error) {
    _newProps = _props;   // den aktuellen Status in newProps kopieren. Das spart später aufwendige Prüfungen einzelner Felder
    NetworkHelper::resetClient(_client);
    return true;
  } else {
    Serial.print(F("Parsing fehlgeschlagen: "));
    Serial.println(error.c_str());
  }
  NetworkHelper::resetClient(_client);
  return false;
}

bool WLEDDevice::applyChanges() {
  NetworkHelper::resetClient(_client);
  if (!_client.connect(_ip, 80)) return false;
  
  _client.printf(
    "POST /json/state HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: %d\r\n"
    "Connection: close\r\n\r\n",
    _ip.toString().c_str(),
    measureJson(_newProps["state"])
  );

  serializeJson(_newProps["state"], _client);

  String resp = _client.readStringUntil('\n');
  bool success = resp.startsWith("HTTP/1.1 200"); 
  NetworkHelper::resetClient(_client);

  if (success) {
    _props.clear();
    _props = _newProps;
  } else {
    _newProps.clear();
    _newProps = _props;
  }
  return success;
}
  
// Json Filter für die Properties und pending
StaticJsonDocument<512> WLEDDevice::_jsonFilter;

void WLEDDevice::initFilter() {
  _jsonFilter.clear();
  _jsonFilter["state"]["on"] = true;
  _jsonFilter["state"]["bri"] = true;
  _jsonFilter["state"]["lor"] = true;
  _jsonFilter["state"]["tt"] = true;
  _jsonFilter["state"]["seg"][0]["tt"] = true;
  _jsonFilter["state"]["seg"][0]["fx"] = true;
  _jsonFilter["state"]["seg"][0]["sx"] = true;
  _jsonFilter["state"]["seg"][0]["ix"] = true;
  _jsonFilter["state"]["seg"][0]["pal"] = true;
  _jsonFilter["state"]["seg"][0]["col"] = true;
  _jsonFilter["state"]["seg"][0]["c1x"] = true;
  _jsonFilter["state"]["seg"][0]["c2x"] = true;
  _jsonFilter["state"]["seg"][0]["c3x"] = true;
  _jsonFilter["info"]["live"] = true;
  _jsonFilter["info"]["lm"] = true;
}
