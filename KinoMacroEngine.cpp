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

bool KinoMacroEngine::isReady() const {
  return _ready;
}

bool KinoMacroEngine::startMacro(const String& name, MacroFinishedCallback cb/*=nullptr*/) {
  if (runtime.running) {
    _addError(0,"runtime", "macro already running");
    return false;
  }
  // always (re-)load the macro, to fill the runtime with correct data
  if (!_loadMacro(name)) return false;
  _onFinished = cb;
  _errors.clear();
  runtime.actions = _macroDoc["actions"].as<JsonArray>();
  runtime.index = 0;
  runtime.running = true;

  return true;
}

String KinoMacroEngine::getName() const {
  return (_macroDoc["name"] | "");
}

void KinoMacroEngine::tick() {
  if (!runtime.running) return;

  if (runtime.index >= runtime.actions.size()) {
    runtime.running = false;
    bool success = _errors.empty();
    if (_onFinished) {
      _onFinished(success);
      _onFinished = nullptr; // 🔥 wichtig!
    }
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

  ActionResult res = MacroActions::execute(a);

  if (!res.ok()) {
    _addError(
      index,
      "ACTION",
      String((int)res.error) + ": " + res.message
    );
    return false;
  }
  
  return true;
}


// Error handling

void KinoMacroEngine::_clearErrors() {
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
bool KinoMacroEngine::addOrUpdateMacro(const String& json) {
  _macroDoc.clear();
  DeserializationError err = deserializeJson(_macroDoc, json);
  if (err) {
    _addError(0,"JSON", err.c_str());
    _addError(0,"",json);
    return false;
  }

  if (!_macroDoc.containsKey("name")) {
    _addError(0,"JSON", "missing name");
    return false;
  }
  if (!_macroDoc.containsKey("actions")) {
    _addError(0,"JSON", "missing actions");
  }

  return _saveMacro();
}

// Makro- Manipulationen:
bool KinoMacroEngine::getMacroLines(const String& macroName, std::vector<String>& outLines) {
  _errors.clear();
  outLines.clear();
  if (runtime.running) {
    _addError(0,"","Macro running, could not load");
    return false;
  }
  outLines.clear();
  if (!_loadMacro(macroName)) {
    return false;
  }
  JsonArray actions = _macroDoc["actions"].as<JsonArray>();
  size_t lineNr = 1;
  for (JsonObject action : actions) {
    String line;
    serializeJson(action, line);
    outLines.push_back(String(lineNr++) + ": " + line);
  }
  return true;
}

bool KinoMacroEngine::addCommand(const String& macroName, size_t index, const String& jsonActionElement) {
    _clearErrors();
    if (!_loadMacro(macroName)) {
      _addError(0,"FS","could not open macro file");
      return false;
    }
    JsonArray actions = _macroDoc["actions"];

    // ---------- Action-Element parsen ----------
    DynamicJsonDocument actionDoc(512);
    DeserializationError err = deserializeJson(actionDoc, jsonActionElement);
    if (err) {
        _addError(0, "JSON", String("action: ") + err.c_str());
        return false;
    }

    if (!actionDoc.is<JsonObject>()) {
        _addError(0, "JSON", "action is not object");
        return false;
    }

    // ---------- Index normalisieren ----------
    size_t insertIndex;
    if (index == 0) {
        insertIndex = 0;                // defensiv
    } else if (index > actions.size()) {
        insertIndex = actions.size();   // anhängen
    } else {
        insertIndex = index - 1;        // 1-basiert → 0-basiert
    }

    // ---------- Einfügen ----------
    JsonArray newActions = _macroDoc.createNestedArray("_tmp");

    for (size_t i = 0; i < actions.size(); ++i) {
        if (i == insertIndex) {
            newActions.add(actionDoc.as<JsonObject>());
        }
        newActions.add(actions[i]);
    }

    if (insertIndex >= actions.size()) {
        newActions.add(actionDoc.as<JsonObject>());
    }

    _macroDoc["actions"] = newActions;
    _macroDoc.remove("_tmp");

    return _saveMacro();
}

bool KinoMacroEngine::deleteCommand(const String& macroName, size_t index) {
  if (index < 1) {
    _addError(0, "CMD", "index must be >= 1");
    return false;
  }
  if (!_loadMacro(macroName)) {
    _addError(0,"FS","could not open macro file");
    return false;
  }
  
  JsonArray actions = _macroDoc["actions"].as<JsonArray>();
  if (actions.isNull()) {
    _addError(0, "JSON", "actions is not an array");
    return false;
  }

  if (index > actions.size()) {
    _addError(0, "CMD", "index out of range");
    return false;
  }

  // 1-basiert → 0-basiert
  actions.remove(index - 1);

  return _saveMacro();
}

bool KinoMacroEngine::updateCommand(const String& macroName, size_t index, const String& jsonActionElement) {
  if (index < 1) {
    _addError(0, "CMD", "index must be >= 1");
    return false;
  }

  // Neues Action-Element parsen
  DynamicJsonDocument actionDoc(512);
  DeserializationError actionErr = deserializeJson(actionDoc, jsonActionElement);
  if (actionErr) {
    _addError(0, "JSON", actionErr.c_str());
    return false;
  }
  if (!actionDoc.is<JsonObject>()) {
    _addError(0, "JSON", "action must be an object");
    return false;
  }
  
  if (!_loadMacro(macroName)) {
    _addError(0,"FS","could not open macro file");
    return false;
  }
  JsonArray actions = _macroDoc["actions"].as<JsonArray>();
  if (actions.isNull()) {
    _addError(0, "JSON", "actions is not an array");
    return false;
  }
  if (index > actions.size()) {
    _addError(0, "CMD", "index out of range");
    return false;
  }

  // Ersetzen (1-basiert → 0-basiert)
  actions[index - 1].set(actionDoc.as<JsonObject>());

  return _saveMacro();
}

// File system helpers:
std::vector<String> KinoMacroEngine::listMacros() {
  std::vector<String> names;
  Dir dir = LittleFS.openDir("/macros");
  while (dir.next()) {
      names.push_back(dir.fileName());
  }
  return names;
}

bool KinoMacroEngine::_loadMacro(const String& macroName) {
  String path;
  path.reserve(32);
  path += "/macros/" + macroName + ".json";
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
  return true;
}

bool KinoMacroEngine::_saveMacro() {
  String name = _macroDoc["name"].as<String>();
  String path;
  path.reserve(32);
  path += "/macros/" + name + ".json";

  File f = LittleFS.open(path, "w");
  if (!f) {
      _addError(0,"FS: write failed", path);
      return false;
  }

  size_t bytesWritten = serializeJson(_macroDoc, f);
  f.close();

  return (bytesWritten > 0);
}

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

// Lifecycle:
