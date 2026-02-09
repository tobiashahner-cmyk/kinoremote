#include "KinoMacroEngine.h"
#include "KinoAPI.h"


// --------------------------------------------------
// lifecycle
// --------------------------------------------------
bool KinoMacroEngine::begin() {
  _errors.clear();
  _ready = false;
  if (!LittleFS.begin()) {
    _addError(0, "FS", "LittleFS init failed");
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


// --------------------------------------------------
// execution
// --------------------------------------------------

bool KinoMacroEngine::startMacro(const String& name, MacroFinishedCallback cb) {
  if (runtime.running) {
    _addError(0, "runtime", "macro already running");
    return false;
  }
  File f = LittleFS.open(_macroPath(name), "r");
  if (!f) {
    _addError(0, "FS", "could not open macro");
    return false;
  }
  _clearErrors();
  runtime.file = f;
  runtime.line = 1;
  runtime.running = true;
  runtime.testing = false;
  _onFinished = cb;
  _currentMacroName = name;
  return true;
}


bool KinoMacroEngine::testMacro(const String& name, MacroFinishedCallback cb) {
  if (!startMacro(name, cb)) return false;
  runtime.testing = true;
  return true;
}

void KinoMacroEngine::tick() {
  if (!runtime.running) return;
  
  if (_pausing) {
    if (millis() - _pauseStart < _pauseDuration) return;
    _pausing = false;
  }
  
  if (!runtime.file.available()) {
    runtime.file.close();
    runtime.running = false;
    if (_onFinished) {
      _onFinished(_errors.empty());
      _onFinished = nullptr;
    }
    _currentMacroName = "";
    return;
  }
  String line = runtime.file.readStringUntil('\n');
  line.trim();
  if (line.isEmpty()) {
    runtime.line++;
    return;
  }
  DynamicJsonDocument actionDoc(1024);
  DeserializationError err = deserializeJson(actionDoc, line);
  if (err) {
    _addError(runtime.line, "JSON", err.c_str());
    runtime.running = false;
    if (_onFinished) {
      _onFinished(false);
      _onFinished = nullptr;
    }
    return;
  }
  _executeAction(actionDoc.as<JsonObject>(), runtime.line);
  
  runtime.line++;
  yield();
}


bool KinoMacroEngine::isRunning() const {
  return runtime.running;
}

String KinoMacroEngine::getName() const {
  return _currentMacroName;
}

// --------------------------------------------------
// action execution
// --------------------------------------------------

bool KinoMacroEngine::_executeAction(const JsonObject& a, uint16_t index) {
  if (a["cmd"] == "delay") {
    _pausing = true;
    _pauseDuration = (a["seconds"] | 1) * 1000UL;
    _pauseStart = millis();
    return true;
  }
  ActionResult res = MacroActions::execute(a, runtime.testing);
  if (!res.ok()) {
    _addError(index, a["cmd"] | "ACTION",
    MacroActions::translateErrorCode(res.error) + ": " + res.message);
    return false;
  }
  return true;
}


// --------------------------------------------------
// macro manipulation (line based)
// --------------------------------------------------

bool KinoMacroEngine::getMacroLines(const String& macroName, std::vector<String>& outLines) {
  _clearErrors();
  outLines.clear();

  File f = LittleFS.open(_macroPath(macroName), "r");
  if (!f) {
    _addError(0, "FS", "could not open macro");
    return false;
  }
  size_t line = 1;
  while (f.available()) {
    String l = f.readStringUntil('\n');
    l.trim();
    if (!l.isEmpty()) {
      outLines.push_back(String(line) + ": " + l);
    }
    line++;
  }
  f.close();
  return true;
 }

bool KinoMacroEngine::getMacroLineCount(const String& macroName, size_t& out) {
  File f = LittleFS.open(_macroPath(macroName), "r");
  if (!f) {out = 0; return false;}
  size_t linecount = 0;
  while (f.available()) {
    String l = f.readStringUntil('\n');
    linecount++;
  }
  f.close();
  out = linecount;
  return true;
}

bool KinoMacroEngine::getMacroLineByIndex(const String& macroName, size_t index, String& out) {
  File f = LittleFS.open(_macroPath(macroName), "r");
  if (!f) { Serial.println("no file handle"); out = ""; return false; }
  size_t linecount = 0;
  while (f.available()) {
    String l = f.readStringUntil('\n');
    if (linecount == index) {
      out = l;
      f.close();
      return true;
    }
    linecount++;
  }
  f.close();
  out = "";
  return false;
}

/*
std::vector<String> KinoMacroEngine::getAvailableMacroCommands() {
  std::vector<String> commands;
  commands.push_back("set");
  commands.push_back("delay");
  return commands;
}*/

std::vector<const char*> KinoMacroEngine::getAvailableMacroCommands() {
  // Wir geben nur Pointer auf statische Strings zurück. 
  // Diese liegen im Flash und belegen keinen Heap/Stack für Kopien.
  return { "set", "delay" }; 
}

size_t KinoMacroEngine::getMacroCommandCount() {
  return 2;
}

bool KinoMacroEngine::getMacroCommand(size_t index, char* out, size_t outLen) {
  if (index > 1) {
    out[0] = '\0';
    return false;
  }
  if (index == 0) strncpy(out, "set", outLen);
  if (index == 1) strncpy(out, "delay", outLen);
  out[outLen-1] = '\0';
  return true;
}

bool KinoMacroEngine::addCommand(const String& macroName, size_t index, const String& jsonAction) {
  String src = _macroPath(macroName);
  String tmp = src + ".tmp";
  
  File in = LittleFS.open(src, "r");
  if (!in) return false;
  File out = LittleFS.open(tmp, "w");
  if (!out) return false;
  
  size_t line = 1;
  bool inserted = false;
  
  while (in.available()) {
    String l = in.readStringUntil('\n');
    if (line == index) {
      out.println(jsonAction);
      inserted = true;
    }
    out.println(l);
    line++;
  }
  if (!inserted) out.println(jsonAction);
  
  in.close();
  out.close();
  LittleFS.remove(src);
  LittleFS.rename(tmp, src);
  return true;
}

bool KinoMacroEngine::deleteCommand(const String& macroName, size_t index) {
  String src = _macroPath(macroName);
  String tmp = src + ".tmp";
  
  File in = LittleFS.open(src, "r");
  File out = LittleFS.open(tmp, "w");
  if (!in || !out) return false;
  
  size_t line = 1;
  while (in.available()) {
    String l = in.readStringUntil('\n');
    if (line != index) out.println(l);
    line++;
  }
  
  in.close();
  out.close();
  LittleFS.remove(src);
  LittleFS.rename(tmp, src);
  return true;
}

bool KinoMacroEngine::updateCommand(const String& macroName,
                                    size_t index,
                                    const String& jsonAction) {
  String src = _macroPath(macroName);
  String tmp = src + ".tmp";

  File in = LittleFS.open(src, "r");
  File out = LittleFS.open(tmp, "w");
  if (!in || !out) {
    Serial.println("KinoMacroEngine::updateCommand(): Could not open file");
    Serial.print("Source file: "); Serial.println(src);
    Serial.print("Tmp    file: "); Serial.println(tmp);
    if (!in) Serial.println("Error lies in Source file");
    if (!out)Serial.println("Error lies in Tmp file");
    return false;
  }

  size_t line = 1;
  bool replaced = false;

  while (in.available()) {
    String l = in.readStringUntil('\n');
    if (line == index) {
      out.println(jsonAction);
      replaced = true;
    } else {
      out.println(l);
    }
    line++;
  }

  in.close();
  out.close();

  if (!replaced) {
    Serial.println("KinoMacroEngine::updateCommand(): Did not replace anything");
    LittleFS.remove(tmp);
    return false;
  }

  LittleFS.remove(src);
  LittleFS.rename(tmp, src);
  return true;
}

bool KinoMacroEngine::createMacro(const String& name) {
  String path = _macroPath(name);
  if (LittleFS.exists(path)) return false;
  File f = LittleFS.open(path, "w");
  if (!f) return false;
  f.close();
  return true;
}

bool KinoMacroEngine::addOrUpdateMacro(const String& json) {
  DynamicJsonDocument doc(512);
  if (deserializeJson(doc, json)) return false;
  if (!doc.containsKey("name") || !doc.containsKey("actions")) return false;
  
  String name = doc["name"].as<String>();
  File f = LittleFS.open(_macroPath(name), "w");
  if (!f) return false;
  
  for (JsonObject a : doc["actions"].as<JsonArray>()) {
    serializeJson(a, f);
    f.println();
  }
  f.close();
  return true;
}

bool KinoMacroEngine::deleteMacro(const String& macroName) {
  String path = _macroPath(macroName);
  if (!LittleFS.exists(path)) return false;
  return LittleFS.remove(path);
}

bool KinoMacroEngine::renameMacro(const String& oldName,const String& newName) {
  String onm = _macroPath(oldName);
  String nnm = _macroPath(newName);
  if (nnm.length() > 31) return false;
  return LittleFS.rename(onm, nnm);
}

std::vector<String> KinoMacroEngine::listMacros() {
  std::vector<String> names;
  Dir dir = LittleFS.openDir("/macros");
  while (dir.next()) names.push_back(dir.fileName());
  return names;
}

size_t KinoMacroEngine::getMacroCount() {
  size_t ct = 0;
  Dir dir = LittleFS.openDir("/macros");
  while (dir.next()) ct++;
  return ct;
}

String KinoMacroEngine::getMacroName(size_t index) {
  size_t ct = 0;
  String mname = "";
  Dir dir = LittleFS.openDir("/macros");
  while (dir.next()) {
    if (ct == index) {
      mname = dir.fileName();
      break;
    }
    ct++;
  }
  mname.replace(".macro","");
  return mname;
}

size_t KinoMacroEngine::getMacroIndex(const String& macroName) {
  String cmp = macroName+String(".macro");
  size_t index = -1;
  Dir dir = LittleFS.openDir("/macros");
  while(dir.next()) {
    if (cmp == dir.fileName()) return (index+1);
    index++;
  }
  return index;
}



// --------------------------------------------------
// error handling
// --------------------------------------------------

void KinoMacroEngine::_clearErrors() {
  _errors.clear();
}


size_t KinoMacroEngine::errorCount() const {
  return _errors.size();
}


const MacroError& KinoMacroEngine::getError(size_t index) const {
  return _errors[index];
}


void KinoMacroEngine::_addError(uint16_t index, const String& cmd, const String& message) {
  _errors.emplace_back(index, cmd, message);
}


// --------------------------------------------------
// helpers
// --------------------------------------------------

String KinoMacroEngine::_macroPath(const String& name) const {
  return "/macros/" + name + ".macro";
}
