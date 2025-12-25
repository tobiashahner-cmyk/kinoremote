#include "KinoMacroEngine.h"
#include "KinoAPI.h"
#include <LittleFS.h>

// Lifecycle
bool KinoMacroEngine::begin() {
  _errors.clear();
  _ready = false;
  if (!LittleFS.begin()) {
      _addError(0,"", "LittleFS init failed");
      return false;
  }

  if (!LittleFS.exists("/macros")) {
      LittleFS.mkdir("/macros");
  }
  _ready = true;
  return true;
}

bool KinoMacroEngine::isReady() {
  return _ready;
}


// Error handling

void KinoMacroEngine::clearErrors() {
  _errors.clear();
}

size_t KinoMacroEngine::errorCount() const {
  return _errors.size();
}

void KinoMacroEngine::_addError(uint8_t index, const String& cmd, const String& message) {
  _errors.push_back({index, cmd, message});
}

const MacroError& KinoMacroEngine::getError(size_t index) const {
  return _errors[index];
}



// Macro Logic
std::vector<String> KinoMacroEngine::listMacros() {
  std::vector<String> names;
  Dir dir = LittleFS.openDir("/macros");
  while (dir.next()) {
      names.push_back(dir.fileName());
  }
  return names;
}

bool KinoMacroEngine::addOrUpdateMacro(const String& json) {
  Serial.println("KinoMacroEngine::addOrUpdateMacro(const String& json)");
  _macroDoc.clear();
  DeserializationError err = deserializeJson(_macroDoc, json);
  if (err) {
    Serial.println(json);
    _addError(0,"JSON", err.c_str());
    return false;
  }

  if (!_macroDoc.containsKey("name")) {
    _addError(0,"JSON", "missing name");
    return false;
  }
  if (!_macroDoc.containsKey("actions")) {
    _addError(0,"JSON", "missing actions");
  }

  String name = _macroDoc["name"].as<String>();
  String path = "/macros/" + name + ".json";

  File f = LittleFS.open(path, "w");
  if (!f) {
      _addError(0,"FS: write failed", path);
      return false;
  }

  f.print(json);
  f.close();

  return true;
}
/*
bool KinoMacroEngine::addOrUpdateMacro(const String& json) {
  _macroDoc.clear();

  DeserializationError err = deserializeJson(_macroDoc, json);
  if (err) {
    addError(0,"","invalid macro Json");
    return false;
  }

  if (!_macroDoc.containsKey("name") ||
      !_macroDoc.containsKey("actions")) {
    addError(0,"","Macro missing name or actions");
    return false;
  }

  return true;
}*/

bool KinoMacroEngine::deleteMacro(const String& macroName) {
  String path = "/macros/" + macroName + ".json";
  if (!LittleFS.exists(path)) {
    _addError(0,"FS: not found", path);
    return false;
  }
  if (!LittleFS.remove(path)) {
    _addError(0,"FS: could not remove", path);
    return false;
  }
  return true;
}

bool KinoMacroEngine::startMacro(const String& name) {
  if (runtime.running) {
    _addError(0,"runtime", "macro already running");
    return false;
  }

  String path = "/macros/" + name + ".json";
  if (!LittleFS.exists(path)) {
    _addError(0,"FS: not found", path);
    return false;
  }

  File f = LittleFS.open(path, "r");
  if (!f) {
    _addError(0,"FS: read failed", path);
    return false;
  }

  _macroDoc.clear();

  DeserializationError err = deserializeJson(_macroDoc, f);
  f.close();

  if (err) {
    _addError(0,"JSON", err.c_str());
    return false;
  }

  runtime.actions = _macroDoc["actions"].as<JsonArray>();
  runtime.index = 0;
  runtime.running = true;

  return true;
}
/*
bool KinoMacroEngine::startMacro(const String& name) {
  clearErrors();

  if (runtime.running) {
    Serial.println("Macro already running");
    return false;
  }

  if (_macroDoc.isNull()) {
    Serial.println("No macro loaded");
    return false;
  }

  const char* macroName = _macroDoc["name"];
  if (!macroName || name != macroName) {
    Serial.println("Macro not found");
    return false;
  }

  runtime.actions = _macroDoc["actions"].as<JsonArray>();
  runtime.index = 0;
  runtime.running = true;

  //Serial.printf("Macro started: %s\n", macroName);
  return true;
}*/


void KinoMacroEngine::tick() {
  if (!runtime.running) return;

  if (runtime.index >= runtime.actions.size()) {
    runtime.running = false;
    //Serial.println("Macro finished");
    return;
  }

  JsonObject action = runtime.actions[runtime.index];
  _executeAction(action, runtime.index);
  runtime.index++;

  yield();  // 👈 ABSOLUT PFLICHT
}

bool KinoMacroEngine::isRunning() const {
  return runtime.running;
}


bool KinoMacroEngine::_executeAction(const JsonObject& a, uint16_t index) {
  const char* cmd = a["cmd"];

  if (!cmd) {
    _addError(index, "<none>", "Missing cmd");
    return false;
  }

  bool ok = false;
  bool commit = a["commit"] | false;
  
  yield();
  if (strcmp(cmd, "yamaha_power") == 0) {
    ok = KinoAPI::yamaha_setPower(a["on"] | false);
  }
  else if (strcmp(cmd, "yamaha_input") == 0) {
    ok = KinoAPI::yamaha_setInput(a["input"].as<String>());
  }
  else if (strcmp(cmd, "yamaha_volume_percent") ==0) {
    ok = KinoAPI::yamaha_setVolumePercent(a["percent"] | 0);
  }
  else if (strcmp(cmd, "beamer_power") == 0) {
    ok = KinoAPI::beamer_setPower(a["on"] | false);
  }
  else if (strcmp(cmd, "canvas_effect") == 0) {
    ok = KinoAPI::canvas_setEffect((a["effect"] | 0),commit);
  }
  else if (strcmp(cmd, "canvas_musicmode") == 0) {
    ok = KinoAPI::canvas_setMusicEffect();
  }
  else if (strcmp(cmd, "canvas_brightness") == 0) {
    ok = KinoAPI::canvas_setBrightness((a["bri"] | 0),commit);
  }
  else if (strcmp(cmd, "sound_musicmode") == 0) {
    ok = KinoAPI::sound_setMusicEffect();
  }
  else if (strcmp(cmd, "huegroup_brightness") == 0) {
    ok = KinoAPI::hueGroup_setBri((a["group"]|""),(a["bri"]|0),commit);
  }
  else {
    _addError(index, cmd, "Unknown command");
    return false;
  }

  if (!ok) {
    _addError(index, cmd, "Command execution failed");
  }
  yield();
  return ok;
}
