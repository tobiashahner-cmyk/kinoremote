#include "HueSensor.h"
#include "HueBridge.h"

HueSensor::HueSensor(uint16_t id, const char* name, const char* type)
: _id(id) {
  strlcpy(_name, name, sizeof(_name)); 
  strlcpy(_type, type, sizeof(_type));
  _dirty = false;
}

bool HueSensor::isDirty() { return _dirty;}
void HueSensor::clearDirty() { _dirty = false; }

uint16_t HueSensor::getId() const { return _id; }
const char* HueSensor::getName() const { return _name; }
const char* HueSensor::getType() const { return _type; }

/*
void HueSensor::updateState(const JsonObject& state) {
  for (JsonPair kv : state) {
    if (_dirty) continue;
    if (!_state.containsKey(kv.key())) {
      _dirty = true;
      continue;
    }
    if (_state[kv.key()] != kv.value()) _dirty = true;
  }
  _state.clear();
  for (JsonPair kv : state) {
    _state[kv.key()] = kv.value();
  }
}*/
void HueSensor::updateState(const JsonObject& state) {
  bool changed = false;
  uint32_t newLastUpdated = _lastupdated;
  
  // 1. dirty check
  for (JsonPair kv : state) {
    const char* key = kv.key().c_str();
    
    if (strcmp(key, "lastupdated") == 0) {
      newLastUpdated = isoToTimestamp(kv.value().as<const char*>());
      if (newLastUpdated != _lastupdated) changed = true;
    } else {
      bool found = false;
      float newValue = kv.value().as<float>();
      for (int i = 0; i < 3; i++) {
        if (strlen(_value[i].key) > 0 && strcmp(_value[i].key, key) == 0) {
          if (_value[i].value != newValue) changed = true;
          found = true;
          break;
        }
      }
      if (!found) changed = true;
    }
    if (changed) break;
  }

  if (changed) {
    _dirty = true;
    _lastupdated = newLastUpdated;
    
    // 2. Werte aktualisieren
    // Erst alles zurücksetzen (Keys leeren)
    for (int i = 0; i < 3; i++) _value[i].key[0] = '\0';
    
    int index = 0;
    for (JsonPair kv : state) {
      const char* key = kv.key().c_str();
      if (strcmp(key, "lastupdated") == 0) continue;

      if (index < 3) {
        strncpy(_value[index].key, key, sizeof(_value[index].key) - 1);
        _value[index].key[sizeof(_value[index].key) - 1] = '\0'; // Sicher ist sicher
        _value[index].value = kv.value().as<float>();
        // Typ-Erkennung
        if (kv.value().is<bool>()) {
            _value[index].type = 1; // BOOL
        } else if (kv.value().is<int>() || kv.value().is<long>()) {
            _value[index].type = 2; // INT
        } else {
            _value[index].type = 3; // FLOAT
        }
        index++;
      }
    }
  }
}

/*
bool HueSensor::hasValue(const String& key) const {
    return _state.containsKey(key);
}*/
bool HueSensor::hasValue(const char* key) const {
  if (strcmp(key, "lastupdated") == 0) return true;
  for (int i = 0; i < 3; i++) {
    if (_value[i].key[0] != '\0' && strcmp(_value[i].key, key) == 0) {
      return true;
    }
  }
  return false;
}

/*JsonVariantConst HueSensor::getValue(const String& key) const {
    return _state[key];
}*/
float HueSensor::getValue(const char* key) const {
  // Spezialfall: Zeitstempel
  if (strcmp(key, "lastupdated") == 0) {
    return (float)_lastupdated;
  }

  // Suche in den gespeicherten Werten
  for (int i = 0; i < 3; i++) {
    if (_value[i].key[0] != '\0' && strcmp(_value[i].key, key) == 0) {
      return _value[i].value;
    }
  }

  return 0.0f; // Standardwert, falls Key nicht gefunden
}

HueSensorValue& HueSensor::getRawValue(const char* key) {
  // 1. Spezialfall: lastupdated
  if (strcmp(key, "lastupdated") == 0) {
    strncpy(_tmpValue.key, "lastupdated", sizeof(_tmpValue.key));
    _tmpValue.value = (float)_lastupdated;
    _tmpValue.type = 2; // Als INT behandeln (Unix Timestamp)
    return _tmpValue;
  }

  // 2. Suche in den gespeicherten Werten
  for (int i = 0; i < 3; i++) {
    if (_value[i].key[0] != '\0' && strcmp(_value[i].key, key) == 0) {
      return _value[i];
    }
  }

  // 3. Fallback (Key nicht gefunden)
  _tmpValue.key[0] = '\0';
  _tmpValue.type = 0; // NONE
  return _tmpValue;
}

uint32_t HueSensor::getLastUpdated() {
  return _lastupdated;
}

/*int HueSensor::getStateSize() const {
  return _state.size();
}*/
int HueSensor::getStateSize() const {
  int count = (_lastupdated > 0) ? 1 : 0; // "lastupdated" zählt mit, falls vorhanden

  for (int i = 0; i < 3; i++) {
    if (_value[i].key[0] != '\0') {
      count++;
    }
  }
  return count;
}

/*JsonObjectConst HueSensor::getState() const {
  return _state.as<JsonObjectConst>();
}*/
const HueSensorValue& HueSensor::getValueAt(int index) const {
  // Spezialfall: Index 0 ist immer lastupdated
  if (index == 0) {
    // Wir "missbrauchen" das tmp-Struct, um eine Referenz bieten zu können
    // Da es const zurückgegeben wird, kann die Bridge es nur lesen.
    strncpy(const_cast<HueSensor*>(this)->_tmpValue.key, "lastupdated", 16);
    const_cast<HueSensor*>(this)->_tmpValue.value = (float)_lastupdated;
    return _tmpValue;
  }

  // Die Werte 1 bis 3 mappen auf unser Array 0 bis 2
  int arrayIdx = index - 1;
  if (arrayIdx >= 0 && arrayIdx < 3) {
    return _value[arrayIdx];
  }

  // Fallback für ungültige Indizes
  const_cast<HueSensor*>(this)->_tmpValue.key[0] = '\0';
  const_cast<HueSensor*>(this)->_tmpValue.value = 0;
  return _tmpValue;
}

bool HueSensor::isWritable() const {
    return (strcmp(_type, "CLIPGenericStatus")==0);
}

/*bool HueSensor::setValue(const char* key, int value) {
    if (!isWritable()) return false;
    _pending[key] = value;
    return true;
}*/
bool HueSensor::setValue(const char* key, int value) {
  if (!isWritable()) return false;

  // 1. Suchen, ob der Key bereits im Pending-Puffer ist
  for (int i = 0; i < 3; i++) {
    if (_pending[i].key[0] != '\0' && strcmp(_pending[i].key, key) == 0) {
      _pending[i].value = (float)value;
      return true;
    }
  }

  // 2. Falls nicht gefunden, im ersten freien Slot speichern
  for (int i = 0; i < 3; i++) {
    if (_pending[i].key[0] == '\0') {
      strncpy(_pending[i].key, key, sizeof(_pending[i].key) - 1);
      _pending[i].key[sizeof(_pending[i].key) - 1] = '\0';
      _pending[i].value = (float)value;
      return true;
    }
  }

  return false; // Puffer voll (sollte bei CLIP-Sensoren nie passieren)
}

/*bool HueSensor::applyChanges(HueBridge* bridge) {

    if (_pending.isNull())
        return true;

    if (!bridge->setSensorState(_id, _pending.as<JsonObject>()))
        return false;
    // lokalen Cache synchronisieren
    for (JsonPair kv : _pending.as<JsonObject>()) {
        _state[kv.key()] = kv.value();
    }

    clearPending();
    _dirty = true;
    return true;
}*/
bool HueSensor::applyChanges(HueBridge* bridge) {
  // 1. Prüfen, ob überhaupt etwas aussteht (erster Key leer?)
  if (_pending[0].key[0] == '\0') return true;

  // 2. JSON-String manuell auf dem Stack bauen
  char jsonBuffer[128]; 
  int pos = snprintf(jsonBuffer, sizeof(jsonBuffer), "{");
  
  bool first = true;
  for (int i = 0; i < 3; i++) {
    if (_pending[i].key[0] != '\0') {
      // Komma setzen, falls es nicht das erste Element ist
      pos += snprintf(jsonBuffer + pos, sizeof(jsonBuffer) - pos, 
                      "%s\"%s\":%.0f", 
                      (first ? "" : ","), _pending[i].key, _pending[i].value);
      first = false;
    }
  }
  pos += snprintf(jsonBuffer + pos, sizeof(jsonBuffer) - pos, "}");

  // 3. An die Bridge senden (Nutzt jetzt den String-Wrapper)
  if (!bridge->setSensorState(_id, jsonBuffer)) {
    return false;
  }

  // 4. Lokalen Cache synchronisieren (Pending -> Value)
  for (int i = 0; i < 3; i++) {
    if (_pending[i].key[0] != '\0') {
      // Im Haupt-Array suchen und Wert aktualisieren
      for (int j = 0; j < 3; j++) {
        if (strcmp(_value[j].key, _pending[i].key) == 0) {
          _value[j].value = _pending[i].value;
          break;
        }
      }
    }
  }

  clearPending();
  _dirty = true;
  return true;
}

/*void HueSensor::clearPending() {
    _pending.clear();
}*/
void HueSensor::clearPending() {
    for (int i = 0; i < 3; i++) _pending[i].key[0] = '\0';
}

// Hilfsfunktion zur Konvertierung
uint32_t HueSensor::isoToTimestamp(const char* isoTime) {
  //Serial.print(F("[HueSensor::isoToTimestamp] isoTime = "));
  //Serial.println(isoTime);
  if (!isoTime || strcmp(isoTime, "none") == 0 || strlen(isoTime) < 10) return 0;
  
  struct tm t = {0}; // <--- WICHTIG: Alles auf 0 setzen
  if (sscanf(isoTime, "%d-%d-%dT%d:%d:%d", 
             &t.tm_year, &t.tm_mon, &t.tm_mday, 
             &t.tm_hour, &t.tm_min, &t.tm_sec) == 6) {
    t.tm_year -= 1900;
    t.tm_mon -= 1;
    t.tm_isdst = -1; // <--- -1 lässt das System entscheiden (Sommerzeit)
    uint32_t ts = (uint32_t)mktime(&t);
    //Serial.print(F("[HueSensor::isoToTimestamp] created timestamp = "));
    //Serial.println(ts);
    return ts;
  }
  return 0;
}
