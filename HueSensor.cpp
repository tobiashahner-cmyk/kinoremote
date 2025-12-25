#include "HueSensor.h"
#include "HueBridge.h"

HueSensor::HueSensor(uint16_t id,
                     const String& name,
                     const String& type)
: _id(id), _name(name), _type(type) {}

uint16_t HueSensor::getId() const { return _id; }
const String& HueSensor::getName() const { return _name; }
const String& HueSensor::getType() const { return _type; }

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

bool HueSensor::isWritable() const {
    return _type == "CLIPGenericStatus";
}

bool HueSensor::setValue(const String& key, int value) {
    if (!isWritable()) return false;
    _pending[key] = value;
    return true;
}

bool HueSensor::applyChanges(HueBridge& bridge) {

    if (_pending.isNull())
        return true;

    if (!bridge.setSensorState(_id, _pending.as<JsonObject>()))
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
