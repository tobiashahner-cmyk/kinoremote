#include "HueBridge.h"


// ===== Konstruktoren =====

HueBridge::HueBridge(const IPAddress& ip, const String& user)
: _ip(ip), _user(user) {}

HueBridge::HueBridge(const String& ip, const String& user)
: _user(user) {
  _ip.fromString(ip);
}

// ===== Public API =====

#include <ArduinoJson.h> // Stelle sicher, dass die Bibliothek installiert ist

bool HueBridge::begin() {
    if (!readLights()) return false;
    if (!readGroups()) return false;
    if (!readScenes()) return false;
    readSensors();
    return true;
}

bool HueBridge::init() {
    if (!readLights()) return false;
    if (!readGroups()) return false;
    if (!readScenes()) return false;
    readSensors();
    return true;
}

bool HueBridge::tick() {
  if (_tickInterval == 0) return false;
  unsigned long now = millis();
  if (now - _lastTick >= _tickInterval) {
    _lastTick = now;
    return readLights();
  }
  return false;
}

bool HueBridge::setTickInterval(int ms) {
  if (ms == 0) { _tickInterval = 0; return true; }
  if (ms < 0) return false;       // nur für bessere Lesbarkeit hier. negative Werte sind unerlaubt
  if (ms < 2000) return false;    // schneller als alle 2 Sekunden erzeugt zu viel Traffic
  _tickInterval = ms;
  return true;
}

int HueBridge::getTickInterval() {
  return _tickInterval;
}

bool HueBridge::readLights() {
  //Serial.println("Lese Lampen aus der Hue Bridge");
    String path = "/api/" + _user + "/lights";

    if (!httpGET(path)) return false;
    if (!skipHttpHeader()) return false;

    // Anti Memory Leak: Lösche alle bisherigen Lampen-Objekte
    for (auto* l : _lights) delete l;
    _lights.clear(); // Vorherige Lampen entfernen

    String buffer;
    char c;
    bool inLight = false;
    int depth = 0;

    uint8_t id = 0;
    String lightJson; // Hier sammeln wir den JSON-Block einer Lampe

    while (_client.connected() || _client.available()) {
        if (!_client.available()) {
            yield();
            continue;
        }

        c = _client.read();
        buffer += c;

        // Buffer kürzen, damit er nicht zu groß wird
        if (buffer.length() > 128) buffer.remove(0, buffer.length() - 128);

        // --- Start einer Lampe erkennen: "ID":{ ---
        if (!inLight && buffer.endsWith("\":{")) {
            int q2 = buffer.lastIndexOf('"');
            int q1 = buffer.lastIndexOf('"', q2 - 1);
            if (q1 >= 0 && q2 > q1) {
                String idStr = buffer.substring(q1 + 1, q2);
                if (idStr.length() > 0 && isDigit(idStr[0])) {
                    id = idStr.toInt();
                    inLight = true;
                    depth = 1; // Erste { gezählt
                    buffer = "";
                    lightJson = "{"; // Start JSON-Block sammeln

                    //Serial.print("Start Lampe ID ");
                    //Serial.println(id);
                }
            }
            continue;
        }

        // --- Innerhalb Lampe ---
        if (inLight) {
            lightJson += c;
            if (c == '{') depth++;
            if (c == '}') depth--;

            // --- Ende Lampe ---
            if (depth == 0) {
                // lightJson enthält nun das vollständige Lampe-Objekt
                DynamicJsonDocument doc(2048); // Für eine einzelne Lampe reicht 2KB locker
                DeserializationError error = deserializeJson(doc, lightJson);
                if (!error) {
                    String name = doc["name"] | "";
                    bool on = doc["state"]["on"] | false;
                    uint8_t bri = doc["state"]["bri"] | 0;
                    bool hasBri = false;
                    if (doc["state"].containsKey("bri")) {
                      hasBri = true;
                    }
                    bool hasXY = false;
                    float x = 0, y = 0;
                    if (doc["state"].containsKey("xy") && doc["state"]["xy"].size() == 2) {
                        x = doc["state"]["xy"][0].as<float>();
                        y = doc["state"]["xy"][1].as<float>();
                        hasXY = true;
                    }
                    bool hasCT = doc["state"].containsKey("ct");
                    uint16_t ct = hasCT ? doc["state"]["ct"].as<uint16_t>() : 0;

                    _lights.push_back(new HueLight(id, name, on, bri, hasXY, x, y, hasCT, ct, hasBri));

                    //Serial.print("Lampe gefunden namens ");
                    //Serial.println(name);
                } else {
                    //Serial.print("Fehler beim Parsen von Lampe ID ");
                    //Serial.println(id);
                }

                inLight = false;
                buffer = "";
                lightJson = "";
            }
        }
    }

    _client.stop();
    //Serial.print(_lights.size());
    //Serial.println(" Lampen gefunden");

    return !_lights.empty();
}

HueLight* HueBridge::getLightById(uint8_t id) {
    for (auto* l : _lights) {
        if (l->getId() == id)
            return l;
    }
    return nullptr;
}

HueLight* HueBridge::getLightByName(const String& name) {
  for (auto* l : _lights) {
    if (l->getName() == name) return l;
  }
  return nullptr;
}

const std::vector<HueLight*>& HueBridge::getLights() const {
  return _lights;
}

HueGroup* HueBridge::getGroupByName(const String& name) {
    for (auto* g : _groups) {
        if (g->getName() == name) return g;
    }
    return nullptr;
}

const std::vector<HueGroup*>& HueBridge::getGroups() const {
    return _groups;
}

bool HueBridge::readGroups() {
    //Serial.println("Lese Gruppen aus der Hue Bridge");
    String path = "/api/" + _user + "/groups";

    if (!httpGET(path)) {
      //Serial.println("httpGet fehlgeschlagen");
      return false;
    }
    if (!skipHttpHeader()) {
      //Serial.println("skipHttpHeader fehlgeschlagen");
      return false;
    }
      

    // Alte Gruppen löschen (Anti Memory Leak)
    for (auto* g : _groups) delete g;
    _groups.clear();

    String buffer;
    char c;
    bool inGroup = false;
    int depth = 0;

    uint16_t groupId = 0;
    String groupJson;

    while (_client.connected() || _client.available()) {
        if (!_client.available()) {
            yield();
            continue;
        }

        c = _client.read();
        buffer += c;

        if (buffer.length() > 128)
            buffer.remove(0, buffer.length() - 128);

        // --- Start Gruppe erkennen: "ID":{ ---
        if (!inGroup && buffer.endsWith("\":{")) {
            int q2 = buffer.lastIndexOf('"');
            int q1 = buffer.lastIndexOf('"', q2 - 1);
            if (q1 >= 0 && q2 > q1) {
                String idStr = buffer.substring(q1 + 1, q2);
                if (idStr.length() > 0 && isDigit(idStr[0])) {
                    //Serial.print("Start von Gruppe ");
                    //Serial.println(idStr);
                    groupId = idStr.toInt();
                    inGroup = true;
                    depth = 1;
                    buffer = "";
                    groupJson = "{";
                }
            }
            continue;
        }

        // --- Innerhalb Gruppe ---
        if (inGroup) {
            groupJson += c;
            if (c == '{') depth++;
            if (c == '}') depth--;

            if (depth == 0) {
                DynamicJsonDocument doc(1024);
                DeserializationError err = deserializeJson(doc, groupJson);
                if (!err) {
                    String name = doc["name"] | "";

                    /*std::vector<HueLight*> groupLights;
                    JsonArray lightsArr = doc["lights"].as<JsonArray>();

                    for (JsonVariant v : lightsArr) {
                        uint8_t lightId = String(v.as<const char*>()).toInt();
                        for (auto* l : _lights) {
                            if (l->getId() == lightId) {
                                groupLights.push_back(l);
                                break;
                            }
                        }
                    }

                    _groups.push_back(
                        new HueGroup(groupId, name, groupLights)
                    );*/
                    std::vector<uint8_t> lightIds;

                    JsonArray lightsArr = doc["lights"].as<JsonArray>();
                    for (JsonVariant v : lightsArr) {
                        lightIds.push_back(
                            String(v.as<const char*>()).toInt()
                        );
                    }
                    
                    _groups.push_back(
                        new HueGroup(groupId, name, *this, lightIds)
                    );
                } else {
                  //Serial.println("Fehler beim Parsen des Json: ");
                  //Serial.println(err.f_str());
                }

                inGroup = false;
                buffer = "";
                groupJson = "";
            }
        }
    }

    _client.stop();
    //Serial.print(_groups.size());
    //Serial.println(" Gruppen gefunden");

    return !_groups.empty();
}

bool HueBridge::readScenes() {
    String path = "/api/" + _user + "/scenes";

    if (!httpGET(path)) return false;
    if (!skipHttpHeader()) return false;

    // Alte Szenen löschen
    for (auto* s : _scenes) delete s;
    _scenes.clear();

    String buffer;
    char c;
    bool inScene = false;
    int depth = 0;

    String sceneId;
    String sceneJson;

    while (_client.connected() || _client.available()) {
        if (!_client.available()) {
            yield();
            continue;
        }

        c = _client.read();
        buffer += c;

        if (buffer.length() > 128)
            buffer.remove(0, buffer.length() - 128);

        // --- Start Szene erkennen: "ID":{ ---
        if (!inScene && buffer.endsWith("\":{")) {
            int q2 = buffer.lastIndexOf('"');
            int q1 = buffer.lastIndexOf('"', q2 - 1);
            if (q1 >= 0 && q2 > q1) {
                sceneId = buffer.substring(q1 + 1, q2);
                inScene = true;
                depth = 1;
                buffer = "";
                sceneJson = "{";
            }
            continue;
        }

        // --- Innerhalb Szene ---
        if (inScene) {
            sceneJson += c;
            if (c == '{') depth++;
            if (c == '}') depth--;

            if (depth == 0) {
                DynamicJsonDocument doc(2048);
                if (deserializeJson(doc, sceneJson) == DeserializationError::Ok) {

                    String type = doc["type"] | "";
                    if (type == "LightScene") {

                        String name = doc["name"] | "";

                        std::vector<uint8_t> lightIds;
                        for (JsonVariant v : doc["lights"].as<JsonArray>()) {
                            lightIds.push_back(
                                String(v.as<const char*>()).toInt()
                            );
                        }

                        _scenes.push_back(
                            new HueScene(sceneId, name, lightIds)
                        );
                    }
                }

                inScene = false;
                sceneJson = "";
                buffer = "";
            }
        }
    }

    _client.stop();
    return !_scenes.empty();
}

HueScene* HueBridge::getSceneByName(const String& name) {
    for (auto* s : _scenes) {
        if (s->getName() == name)
            return s;
    }
    return nullptr;
}

const std::vector<HueScene*>& HueBridge::getScenes() const {
    return _scenes;
}

bool HueBridge::setScene(const String& sceneName) {
  HueScene* s = getSceneByName(sceneName);
  if (s) return s->setActive(this);
  return false;
}

std::map<uint8_t, bool> HueBridge::getScenePowerStates(const String& sceneId) {
    std::map<uint8_t, bool> result;

    String path = "/api/" + _user + "/scenes/" + sceneId;

    if (!httpGET(path)) return result;
    if (!skipHttpHeader()) return result;

    String buffer;
    char c;
    int depth = 0;
    bool inLightStates = false;
    String json;

    while (_client.connected() || _client.available()) {
        if (!_client.available()) {
            yield();
            continue;
        }

        c = _client.read();

        // lightstates-Block finden
        buffer += c;
        if (buffer.length() > 64)
            buffer.remove(0, buffer.length() - 64);

        if (!inLightStates && buffer.endsWith("\"lightstates\":{")) {
            inLightStates = true;
            depth = 1;
            json = "{";
            continue;
        }

        if (inLightStates) {
            json += c;
            if (c == '{') depth++;
            if (c == '}') depth--;

            if (depth == 0) {
                DynamicJsonDocument doc(2048);
                if (deserializeJson(doc, json) == DeserializationError::Ok) {

                    for (JsonPair kv : doc.as<JsonObject>()) {
                        uint8_t lightId = String(kv.key().c_str()).toInt();
                        bool on = kv.value()["on"] | false;
                        result[lightId] = on;
                    }
                }
                break;
            }
        }
    }

    _client.stop();
    return result;
}

SceneLightStates HueBridge::getSceneLightStates(const String& sceneId) {
    SceneLightStates result;

    String path = "/api/" + _user + "/scenes/" + sceneId;
    if (!httpGET(path)) return result;
    if (!skipHttpHeader()) return result;

    String buffer;
    char c;
    bool inLightStates = false;
    int depth = 0;
    String json;

    while (_client.connected() || _client.available()) {
        if (!_client.available()) {
            yield();
            continue;
        }

        c = _client.read();

        // lightstates-Block finden
        buffer += c;
        if (buffer.length() > 64)
            buffer.remove(0, buffer.length() - 64);

        if (!inLightStates && buffer.endsWith("\"lightstates\":{")) {
            inLightStates = true;
            depth = 1;
            json = "{";
            continue;
        }

        if (inLightStates) {
            json += c;

            if (c == '{') depth++;
            if (c == '}') depth--;

            if (depth == 0) {
                DynamicJsonDocument doc(2048);
                if (deserializeJson(doc, json) == DeserializationError::Ok) {

                    for (JsonPair kv : doc.as<JsonObject>()) {
                        uint8_t lightId =
                            String(kv.key().c_str()).toInt();

                        SceneLightState state;
                        JsonObject obj = kv.value().as<JsonObject>();

                        if (obj.containsKey("on")) {
                            state.hasOn = true;
                            state.on = obj["on"];
                        }
                        if (obj.containsKey("bri")) {
                            state.hasBri = true;
                            state.bri = obj["bri"];
                        }
                        if (obj.containsKey("ct")) {
                            state.hasCT = true;
                            state.ct = obj["ct"];
                        }

                        result[lightId] = state;
                    }
                }
                break;
            }
        }
    }

    _client.stop();
    return result;
}

bool HueBridge::readSensors() {

    String path = "/api/" + _user + "/sensors";

    if (!httpGET(path)) return false;
    if (!skipHttpHeader()) return false;

    // --- alte Sensoren löschen ---
    for (auto* s : _sensors) delete s;
    _sensors.clear();

    String buffer;
    char c;
    bool inSensor = false;
    int depth = 0;

    uint16_t sensorId = 0;
    String sensorJson;

    while (_client.connected() || _client.available()) {

        if (!_client.available()) {
            yield();
            continue;
        }

        c = _client.read();
        buffer += c;

        // Buffer begrenzen
        if (buffer.length() > 128)
            buffer.remove(0, buffer.length() - 128);

        // --- Start Sensor erkennen: "ID":{ ---
        if (!inSensor && buffer.endsWith("\":{")) {

            int q2 = buffer.lastIndexOf('"');
            int q1 = buffer.lastIndexOf('"', q2 - 1);

            if (q1 >= 0 && q2 > q1) {
                String idStr = buffer.substring(q1 + 1, q2);
                if (idStr.length() > 0 && isDigit(idStr[0])) {

                    sensorId = idStr.toInt();
                    inSensor = true;
                    depth = 1;
                    buffer = "";
                    sensorJson = "{";
                }
            }
            continue;
        }

        // --- innerhalb Sensor ---
        if (inSensor) {
            sensorJson += c;

            if (c == '{') depth++;
            if (c == '}') depth--;

            // --- Ende Sensor ---
            if (depth == 0) {

                DynamicJsonDocument doc(2048);
                DeserializationError err =
                    deserializeJson(doc, sensorJson);

                if (!err) {

                    String name = doc["name"] | "";
                    String type = doc["type"] | "";

                    HueSensor* s =
                        new HueSensor(sensorId, name, type);

                    if (doc.containsKey("state")) {
                        s->updateState(
                            doc["state"].as<JsonObject>()
                        );
                    }

                    _sensors.push_back(s);
                }

                inSensor = false;
                sensorJson = "";
                buffer = "";
            }
        }
    }

    _client.stop();
    return !_sensors.empty();
}


HueSensor* HueBridge::getSensorByName(const String& name) {
    for (auto* s : _sensors)
        if (s->getName() == name)
            return s;
    return nullptr;
}


bool HueBridge::setSensorState(uint16_t id,
                               const JsonObject& state) {

    String path =
        "/api/" + _user + "/sensors/" +
        String(id) + "/state";

    String payload;
    serializeJson(state, payload);

    return sendState(path, payload);
}



// ===== HTTP =====

bool HueBridge::httpGET(const String& path) {
  if (!_client.connect(_ip, 80)) return false;

  _client.printf(
    "GET %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Connection: close\r\n\r\n",
    path.c_str(),
    _ip.toString().c_str()
  );

  return waitForData();
}

bool HueBridge::sendLightState(uint8_t lightId, const String& jsonPayload) {
  String path = "/api/" + _user + "/lights/" + String(lightId) + "/state";
  return sendState(path, jsonPayload);
}

bool HueBridge::sendGroupState(uint16_t groupId, const String& jsonPayload) {
  String path = "/api/" + _user + "/groups/" + String(groupId) + "/action";
  return sendState(path, jsonPayload);
}

bool HueBridge::saveScene(const String& sceneId, const String& jsonPayload) {
    String path = "/api/" + _user + "/scenes/" + sceneId;
    return sendState(path, jsonPayload);
}


bool HueBridge::sendState(const String& path, const String& jsonPayload) {
    if (!_client.connect(_ip, 80)) return false;

    _client.printf(
        "PUT %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n"
        "%s",
        path.c_str(),
        _ip.toString().c_str(),
        jsonPayload.length(),
        jsonPayload.c_str()
    );

    return waitForData();
}

bool HueBridge::waitForData(uint32_t timeout) {
  uint32_t start = millis();
  while (_client.connected() && !_client.available()) {
    if (millis() - start > timeout) {
      _client.stop();
      return false;
    }
    yield();
  }
  return true;
}

bool HueBridge::skipHttpHeader() {
  unsigned long start = millis();
  String line;

  while (!_client.available()) {
    if (millis() - start > 2000) {
      return false;
    }
    yield();
  }

  while (_client.connected() || _client.available()) {
    if (!_client.available()) {
      if (millis() - start > 2000) {
        return false;
      }
      yield();
      continue;
    }

    line = _client.readStringUntil('\n');

    if (line.length() == 0 || line == "\r") {
      return true;
    }
  }
  return false;
}

bool HueBridge::setPower(bool onoff) {
  if (onoff) {  // einschalten
    if (anyOn()) return true;   // no action needed, light is already on
    HueSensor* sensor = getSensorByName("Daylight");
    // Tageslicht?
    bool day = (sensor && sensor->hasValue("daylight")) ? sensor->getValue("daylight").as<bool>() : false;
    if (day) return setScene("Standard");  // Tagsüber: verhalte Dich wie der Lichtschalter
    return setScene("Nachtlicht");         // Nachts: setze Nachtlicht, um einen Schock zu verhindern ;-)
  }
  // still here: ausschalten
  if (!sendGroupState(0,"{\"on\":false}")) return false;
  for (auto& l : _lights) {
    l->forceOn(false);
  }
  return true;
}

bool HueBridge::anyOn() {
  for (auto& l : _lights) {
    if (l->isOn()) return true;
  }
  return false;
}


/*
 * ALTE VERSION

bool HueBridge::skipHttpHeader() {
  unsigned long start = millis();
  String line;

  // Erst warten, bis überhaupt Daten kommen
  while (!_client.available()) {
    if (millis() - start > 2000) return false;
    yield();
  }

  while (_client.connected()) {
    if (!_client.available()) {
      if (millis() - start > 2000) return false;
      yield();
      continue;
    }

    line = _client.readStringUntil('\n');
    Serial.println(line);
    if (line == "\r" || line == "") {
        return true;
    }
  }
  return false;
}
*/
