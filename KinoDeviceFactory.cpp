#include "KinoDeviceFactory.h"
#include "config.h"
#include <LittleFS.h>

// konkrete Devices
#include "YamahaReceiver.h"
#include "WLEDDevice.h"
#include "OptomaBeamer.h"
#include "HyperionDevice.h"
#include "HueBridge.h"

extern StaticJsonDocument<2048> httpJson;

std::vector<KinoDeviceFactory::DeviceEntry> KinoDeviceFactory::_devices;

// ------------------------------------------------------------
// public API
// ------------------------------------------------------------
/*
bool KinoDeviceFactory::initDevices() {
  _devices.clear();

  DynamicJsonDocument doc(2048);
  if (!loadDevicesJson(doc)) {
    Serial.println(F("DeviceFactory: failed to load devices.json"));
    return false;
  }

  JsonArray arr = doc["devices"].as<JsonArray>();
  if (!arr) {
    Serial.println(F("DeviceFactory: no devices array"));
    return false;
  }

  for (JsonObject d : arr) {
    if ( (!d.containsKey("name")) || (!d.containsKey("class")) ) {
      Serial.println(F("DeviceFactory: invalid device entry"));
      continue;
    }
    
    DeviceEntry e;
    strlcpy(e.name, d["name"]|"", sizeof(e.name));
    strlcpy(e.className, d["class"]|"", sizeof(e.className));

    

    Serial.print(F("DeviceFactory: creating ")); Serial.print(e.className); Serial.print(F(" ")); Serial.println(e.name);
    e.device = createDeviceFromJson(e.className, d);
    if (!e.device) {
      Serial.printf("DeviceFactory: unknown class '%s'\n",
                    e.className);
      continue;
    }

    KinoError err = e.device->init();
    e.initOk = (err == KinoError::OK);

    if (!e.initOk) {
      Serial.printf("DeviceFactory: init failed for %s\n",
                    e.name);
    }

    _devices.push_back(e);
  }

  return true;
}*/

bool KinoDeviceFactory::initDevices() {
  _devices.clear();

  //DynamicJsonDocument doc(2048);
  if (!loadDevicesJson(httpJson)) {
    Serial.println(F("DeviceFactory: failed to load devices.json"));
    return false;
  }

  JsonArray arr = httpJson["devices"].as<JsonArray>();    // missbrauche globales httpJson
  if (!arr) {
    Serial.println(F("DeviceFactory: no devices array"));
    return false;
  }

  for (JsonObject d : arr) {
    if ( (!d.containsKey("name")) || (!d.containsKey("class")) ) {
      Serial.println(F("DeviceFactory: invalid device entry"));
      continue;
    }
    
    DeviceEntry e;
    strlcpy(e.name, d["name"]|"", sizeof(e.name));
    strlcpy(e.className, d["class"]|"", sizeof(e.className));

    

    Serial.print(F("DeviceFactory: creating ")); Serial.print(e.className); Serial.print(F(" ")); Serial.println(e.name);
    e.device = createDeviceFromJson(e.className, d);
    if (!e.device) {
      Serial.printf("DeviceFactory: unknown class '%s'\n",
                    e.className);
      continue;
    }
    _devices.push_back(e);
  }

  for (DeviceEntry& e : _devices) {
    Serial.print(F("initializing ")); Serial.print(e.name);
    KinoError err = e.device->init();
    e.initOk = (err == KinoError::OK);

    if (!e.initOk) {
      Serial.println(F(" : init failed"));
    } else {
      Serial.println(F(" : OK"));
    }
    // time for cleanup after device init
    yield();
    delay(1000);
    yield();
    httpJson.clear(); // clean up the shared document after use
  }

  return true;
}

KinoDevice* KinoDeviceFactory::getDeviceByName(const char* name) {
  if (!name) return nullptr;

  for (auto& d : _devices) {
    if (strcmp(d.name, name)==0) {
      if (!d.initOk && d.device) {
        // Lazy re-init
        Serial.print(F("re-initializing previously unsuccessful "));
        Serial.println(name);
        d.initOk = (d.device->init() == KinoError::OK);
      }
      return d.device;
    }
  }
  return nullptr;
}

KinoDevice* KinoDeviceFactory::getDeviceByIndex(int index) {
  if (index >= _devices.size()) return nullptr;
  int i = 0;
  for (auto& d : _devices) {
    if (i==index) return d.device;
    i++;
  }
  return nullptr;
}

const bool KinoDeviceFactory::getDeviceNameByIndex(int index, char* devName, size_t devNameLen) {
  if (index < 0 || index >= _devices.size()) {
    if (devNameLen > 0) devName[0] = '\0'; 
    return false; 
  }
  
  int i = 0;
  for (auto& d : _devices) {
    if (i == index) {
      strlcpy(devName, d.name, devNameLen);
      return true;
    }
    i++;
  }
  if (devNameLen > 0) devName[0] = '\0';
  return false;
}

int KinoDeviceFactory::getDeviceCount() {
  return _devices.size();
}

// ------------------------------------------------------------
// JSON helpers
// ------------------------------------------------------------

void printFile() {
  Serial.println(F("\n\nKontrollausgabe von /devices.json"));
  // Datei im Lesemodus ("r") öffnen
  File file = LittleFS.open("/devices.json", "r");
  
  if (!file) {
    Serial.println(F("Fehler: Datei konnte nicht geöffnet werden."));
    return;
  }

  Serial.println(F("--- Dateinhalt startet ---"));
  
  // Solange Daten verfügbar sind, Byte für Byte auslesen und senden
  while (file.available()) {
    Serial.write(file.read());
  }
  
  Serial.println(F("\n--- Dateinhalt Ende ---"));
  file.close(); // Wichtig: Datei wieder schließen
}

bool KinoDeviceFactory::loadDevicesJson(JsonDocument& doc) {
  if (!LittleFS.begin()) {
    Serial.println(F("LittleFS mount failed"));
    return false;
  }

  if (!LittleFS.exists("/devices.json")) {
    Serial.println(F("devices.json missing, creating default"));
    return createDefaultDevicesFile();
  }

  File f = LittleFS.open("/devices.json", "r");
  if (!f) {
    Serial.println(F("could not open /devices.json"));
    return false;
  }

  DeserializationError err = deserializeJson(doc, f);
  f.close();

  if (err) {
    Serial.println(err.c_str());
    printFile();
    return false;
  }
  return true;
}

bool KinoDeviceFactory::createDefaultDevicesFile() {
  if (!LittleFS.begin()) {
    Serial.println(F("LittleFS mount failed"));
    return false;
  }
  if (LittleFS.exists("/devices.json")) {
    Serial.println(F("/devices.json existiert"));
  } else {
    Serial.println(F("/devices.json existiert nicht!"));
  }
  if (!LittleFS.remove("/devices.json")) {
    Serial.println(F("Konnte /devices.json nicht löschen!"));
  }
  File f = LittleFS.open("/devices.json", "w");
  if (!f) {
    Serial.println(F("Konnte /devices.json nicht zum Schreiben öffnen!"));
    return false;
  }
  char buffer[256];

  f.println(F("{"));
  f.println(F("  \"devices\": ["));
  snprintf_P(buffer,256,DEFAULT_AVR,YAMAHA_IP);
  f.print(buffer); f.println(F(","));
  snprintf_P(buffer,256,DEFAULT_BEAMER,BEAMER_IP,BEAMER_ID);
  f.print(buffer); f.println(F(","));
  snprintf_P(buffer,256,DEFAULT_CANVAS,CANVAS_IP);
  f.print(buffer); f.println(F(","));
  snprintf_P(buffer,256,DEFAULT_SOUND,SOUND_IP);
  f.print(buffer); f.println(F(","));
  snprintf_P(buffer,256,DEFAULT_HYPERION,HYPERION_IP);
  f.print(buffer); f.println(F(","));
  snprintf_P(buffer,256,DEFAULT_HUEBRIDGE,HUE_BRIDGE_IP,HUE_TOKEN);
  f.println(buffer);
  f.println(F("  ]"));
  f.println(F("}"));
  f.close();
  return true;
}

// ------------------------------------------------------------
// Device creation
// ------------------------------------------------------------

static IPAddress ipFromJson(JsonVariant v) {
  IPAddress ip;
  ip.fromString(v.as<const char*>());
  return ip;
}

KinoDevice* KinoDeviceFactory::createDeviceFromJson(const char* className, JsonObject cfg) {
  if (strcmp(className, "yamahareceiver")==0) {
    return new YamahaReceiver(
      ipFromJson(cfg["ip"])
    );
  }

  if (strcmp(className, "wleddevice")==0) {
    return new WLEDDevice(
      ipFromJson(cfg["ip"])
    );
  }

  if (strcmp(className, "optomabeamer")==0) {
    return new OptomaBeamer(
      ipFromJson(cfg["ip"]),
      cfg["id"] | 0
    );
  }

  if (strcmp(className, "hyperiondevice")==0) {
    return new HyperionDevice(
      ipFromJson(cfg["ip"])
    );
  }

  if (strcmp(className, "huebridge")==0) {
    return new HueBridge(
      ipFromJson(cfg["ip"]),
      cfg["token"] | ""
    );
  }

  return nullptr;
}

const char KinoDeviceFactory::DEFAULT_AVR[] PROGMEM       = "    { \"name\": \"yamaha\", \"class\": \"yamahareceiver\", \"ip\": \"%s\"}";
const char KinoDeviceFactory::DEFAULT_BEAMER[] PROGMEM    = "    { \"name\": \"beamer\", \"class\": \"optomabeamer\", \"ip\": \"%s\", \"id\":%i }";
const char KinoDeviceFactory::DEFAULT_CANVAS[] PROGMEM    = "    { \"name\": \"canvas\", \"class\": \"wleddevice\", \"ip\": \"%s\" }";
const char KinoDeviceFactory::DEFAULT_SOUND[] PROGMEM     = "    { \"name\": \"sound\", \"class\": \"wleddevice\", \"ip\": \"%s\" }";
const char KinoDeviceFactory::DEFAULT_HYPERION[] PROGMEM  = "    { \"name\": \"hyperion\", \"class\": \"hyperiondevice\", \"ip\": \"%s\" }";
const char KinoDeviceFactory::DEFAULT_HUEBRIDGE[] PROGMEM = "    { \"name\": \"hue\", \"class\": \"huebridge\", \"ip\": \"%s\", \"token\": \"%s\" }";
