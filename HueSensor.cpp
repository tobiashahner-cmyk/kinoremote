#include "HueSensor.h"
#include "HueBridge.h"

HueSensor::HueSensor(uint16_t id, const char* name, const char* type)
: _id(id) {
  strlcpy(_name, name, sizeof(_name)); 
  strlcpy(_type, type, sizeof(_type));
}

uint16_t HueSensor::getId() const { return _id; }
const char* HueSensor::getName() const { return _name; }
const char* HueSensor::getType() const { return _type; }

void HueSensor::updateState(const JsonObject& state) {
    _state.clear();
    for (JsonPair kv : state) {
        _state[kv.key()] = kv.value();
    }
}

bool HueSensor::hasValue(const String& key) const {
    return _state.containsKey(key);
}

JsonVariantConst HueSensor::getValue(const String& key) const {
    return _state[key];
}

int HueSensor::getStateSize() const {
  return _state.size();
}

JsonObjectConst HueSensor::getState() const {
  return _state.as<JsonObjectConst>();
}

bool HueSensor::isWritable() const {
    //return _type == "CLIPGenericStatus";
    return (strcmp(_type, "CLIPGenericStatus")==0);
}

bool HueSensor::setValue(const String& key, int value) {
    if (!isWritable()) return false;
    _pending[key] = value;
    return true;
}

bool HueSensor::applyChanges(HueBridge* bridge) {

    if (_pending.isNull())
        return true;

    if (!bridge->setSensorState(_id, _pending.as<JsonObject>()))
        return false;

    // lokalen Cache synchronisieren
    for (JsonPair kv : _pending.as<JsonObject>()) {
        _state[kv.key()] = kv.value();
    }

    clearPending();
    return true;
}

void HueSensor::clearPending() {
    _pending.clear();
}
