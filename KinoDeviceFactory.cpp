#include "KinoDeviceFactory.h"
#include "config.h"
#include <LittleFS.h>

// konkrete Devices
#include "YamahaReceiver.h"
#include "WLEDDevice.h"
#include "OptomaBeamer.h"
#include "HyperionDevice.h"
#include "HueBridge.h"

extern WiFiClient globalWifiClient;

std::vector<KinoDeviceFactory::DeviceEntry> KinoDeviceFactory::_devices;

// ------------------------------------------------------------
// public API
// ------------------------------------------------------------

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
    DeviceEntry e;
    e.name      = d["name"]  | "";
    e.className = d["class"] | "";

    if (e.name.isEmpty() || e.className.isEmpty()) {
      Serial.println(F("DeviceFactory: invalid device entry"));
      continue;
    }

    e.device = createDeviceFromJson(e.className, d);
    if (!e.device) {
      Serial.printf("DeviceFactory: unknown class '%s'\n",
                    e.className.c_str());
      continue;
    }

    KinoError err = e.device->init();
    e.initOk = (err == KinoError::OK);

    if (!e.initOk) {
      Serial.printf("DeviceFactory: init failed for %s\n",
                    e.name.c_str());
    }

    _devices.push_back(e);
  }

  return true;
}

KinoDevice* KinoDeviceFactory::getDeviceByName(const char* name) {
  if (!name) return nullptr;

  for (auto& d : _devices) {
    if (d.name == name) {
      if (!d.initOk && d.device) {
        // Lazy re-init
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

std::vector<String> KinoDeviceFactory::getDeviceNames() {
  std::vector<String> out;
  for (auto& d : _devices) {
    out.push_back(d.name);
  }
  return out;
}

/*String KinoDeviceFactory::getDeviceNameByIndex(int index) {
  if (index >= _devices.size()) return "";
  int i = 0;
  for (auto& d : _devices) {
    if (i==index) return d.name;
    i++;
  }
  return "";
}*/

const String& KinoDeviceFactory::getDeviceNameByIndex(int index) {
  static const String empty = ""; // Statisch, damit die Referenz immer gültig bleibt
  if (index < 0 || index >= _devices.size()) return empty;
  
  int i = 0;
  for (auto& d : _devices) {
    if (i == index) return d.name; // Gibt Referenz auf den existierenden String zurück
    i++;
  }
  return empty;
}

int KinoDeviceFactory::getDeviceCount() {
  return _devices.size();
}

// ------------------------------------------------------------
// JSON helpers
// ------------------------------------------------------------

void printFile() {
  Serial.println("\n\nKontrollausgabe von /devices.json");
  // Datei im Lesemodus ("r") öffnen
  File file = LittleFS.open("/devices.json", "r");
  
  if (!file) {
    Serial.println("Fehler: Datei konnte nicht geöffnet werden.");
    return;
  }

  Serial.println("--- Dateinhalt startet ---");
  
  // Solange Daten verfügbar sind, Byte für Byte auslesen und senden
  while (file.available()) {
    Serial.write(file.read());
  }
  
  Serial.println("\n--- Dateinhalt Ende ---");
  file.close(); // Wichtig: Datei wieder schließen
}


bool KinoDeviceFactory::loadDevicesJson(DynamicJsonDocument& doc) {
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
    Serial.println("could not open /devices.json");
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
    Serial.println("/devices.json existiert");
  } else {
    Serial.println("/devices.json existiert nicht!");
  }
  if (!LittleFS.remove("/devices.json")) {
    Serial.println("Konnte /devices.json nicht löschen!");
  }
  File f = LittleFS.open("/devices.json", "w");
  if (!f) {
    Serial.println("Konnte /devices.json nicht zum Schreiben öffnen!");
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

KinoDevice* KinoDeviceFactory::createDeviceFromJson(const String& className, JsonObject cfg) {
  if (className == "yamahareceiver") {
    return new YamahaReceiver(
      globalWifiClient,
      ipFromJson(cfg["ip"])
    );
  }

  if (className == "wleddevice") {
    return new WLEDDevice(
      globalWifiClient,
      ipFromJson(cfg["ip"])
    );
  }

  if (className == "optomabeamer") {
    return new OptomaBeamer(
      globalWifiClient,
      ipFromJson(cfg["ip"]),
      cfg["id"] | 0
    );
  }

  if (className == "hyperiondevice") {
    return new HyperionDevice(
      globalWifiClient,
      ipFromJson(cfg["ip"])
    );
  }

  if (className == "huebridge") {
    return new HueBridge(
      globalWifiClient,
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
