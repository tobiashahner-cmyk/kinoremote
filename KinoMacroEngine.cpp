#include "KinoMacroEngine.h"
#include "KinoAPI.h"


void KinoMacroEngine::clearErrors() {
  _errors.clear();
}

size_t KinoMacroEngine::errorCount() const {
  return _errors.size();
}

const MacroError& KinoMacroEngine::getError(size_t index) const {
  return _errors[index];
}

bool KinoMacroEngine::addOrUpdateMacro(const String& json) {
  _macroDoc.clear();

  DeserializationError err = deserializeJson(_macroDoc, json);
  if (err) {
    Serial.println("Macro JSON invalid");
    return false;
  }

  if (!_macroDoc.containsKey("name") ||
      !_macroDoc.containsKey("actions")) {
    Serial.println("Macro missing name or actions");
    return false;
  }

  return true;
}

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

  Serial.printf("Macro started: %s\n", macroName);
  return true;
}


void KinoMacroEngine::tick() {
  if (!runtime.running) return;

  if (runtime.index >= runtime.actions.size()) {
    runtime.running = false;
    Serial.println("Macro finished");
    return;
  }

  JsonObject action = runtime.actions[runtime.index];
  executeAction(action, runtime.index);
  runtime.index++;

  yield();  // 👈 ABSOLUT PFLICHT
}

bool KinoMacroEngine::isRunning() const {
  return runtime.running;
}


bool KinoMacroEngine::executeAction(const JsonObject& a, uint16_t index) {
  const char* cmd = a["cmd"];

  if (!cmd) {
    _errors.push_back({index, "<none>", "Missing cmd"});
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
    //_errors.push_back({index, cmd, "Unknown command"});
    _errors.emplace_back(index, cmd, "Unknown command");
    return false;
  }

  if (!ok) {
    //_errors.push_back({index, cmd, "Command execution failed"});
    _errors.emplace_back(index, cmd, "Command execution failed");
  }
  yield();
  return ok;
}
