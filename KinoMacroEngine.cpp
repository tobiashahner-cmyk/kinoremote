#include "KinoMacroEngine.h"
#include "KinoAPI.h"

extern StaticJsonDocument<2048> httpJson;

// --------------------------------------------------
// lifecycle
// --------------------------------------------------
bool KinoMacroEngine::begin() {
  _errors.reserve(10);
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

bool KinoMacroEngine::startMacro(const char* mName, MacroFinishedCallback cb, MacroErrorCallback e) {
  if (runtime.running) {
    _addError(0, F("runtime"), F("macroEngine busy"));
    return false;
  }
  char path[48];
  getMacroPath(mName, path, sizeof(path));
  File f = LittleFS.open(path, "r");
  if (!f) {
    _addError(0, F("FS"), F("could not open macro file"));
    return false;
  }
  _clearErrors();
  runtime.file = f;
  runtime.line = 1;
  runtime.running = true;
  runtime.testing = false;
  _onFinished = cb;
  _onError = e;
  strlcpy(_currentMacroName, mName, sizeof(_currentMacroName));
  return true;
}

bool KinoMacroEngine::testMacro(const char* mName, MacroFinishedCallback cb, MacroErrorCallback e) {
  if (!startMacro(mName, cb,e)) return false;
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
    memset(_currentMacroName, 0, sizeof(_currentMacroName));
    return;
  }
  char line[256];
  int bytesRead = runtime.file.readBytesUntil('\n', line, sizeof(line)-1);
  line[bytesRead] = '\0';
  if (bytesRead > 0 && line[bytesRead-1] == '\r') line[bytesRead-1] = '\0';
  if (strlen(line)==0) {
    runtime.line++;
    return;
  }
  
  //_actionDoc.clear();
  httpJson.clear();
  //DeserializationError err = deserializeJson(_actionDoc, line);
  DeserializationError err = deserializeJson(httpJson, line);
  if (err) {
    _addError(runtime.line, "JSON", err.c_str());
    runtime.running = false;
    if (_onFinished) {
      _onFinished(false);
      _onFinished = nullptr;
    }
    return;
  }
  //_executeAction(_actionDoc.as<JsonObject>(), runtime.line);
  _executeAction(httpJson.as<JsonObject>(), runtime.line);
  httpJson.clear();
  runtime.line++;
  yield();
}


bool KinoMacroEngine::isRunning() const {
  return runtime.running;
}

void KinoMacroEngine::getName(char* out, size_t outLen) {
  strlcpy(out, _currentMacroName, outLen);
}

// --------------------------------------------------
// action execution
// --------------------------------------------------

bool KinoMacroEngine::_executeAction(const JsonObject& a, uint16_t index) {
  //if (a["cmd"] == "delay") {
  if (strcmp(a["cmd"], "delay")==0) {
    _pausing = true;
    _pauseDuration = (a["seconds"] | 1) * 1000UL;
    _pauseStart = millis();
    return true;
  }
  ActionResult res = MacroActions::execute(a, runtime.testing);
  if (!res.ok()) {
    char errorBuffer[128];
    const char* cmd = a["cmd"]|"ACTION";
    const char* errStr = MacroActions::translateErrorCode(res.error).c_str();
    snprintf(errorBuffer, sizeof(errorBuffer), "%s: %s", errStr, res.message.c_str());
    /*_addError(index, a["cmd"] | "ACTION",
    MacroActions::translateErrorCode(res.error) + ": " + res.message);*/
    _addError(index, cmd, errStr);
    return false;
  }
  return true;
}


// --------------------------------------------------
// macro manipulation (line based)
// --------------------------------------------------

size_t KinoMacroEngine::getMacroLineCount(const char* macroName) {
  char path[48];
  getMacroPath(macroName, path, sizeof(path));
  return FileHelper::countLines(path);
}

bool KinoMacroEngine::getMacroLineByIndex(const char* macroName, size_t index, char* out, size_t outLen) {
  char path[48];
  getMacroPath(macroName, path, sizeof(path));
  return FileHelper::readLineAt(path, index, out, outLen);
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

bool KinoMacroEngine::addCommand(const char* macroName, size_t index, const char* jsonAction) {
  char src[48];
  char tmp[48];
  
  getMacroPath(macroName, src, sizeof(src));
  snprintf(tmp, sizeof(tmp), "%s.tmp", src);
  
  File in = LittleFS.open(src, "r");
  if (!in) return false;
  File out = LittleFS.open(tmp, "w");
  if (!out) {
    in.close();
    return false;
  }
  
  size_t line = 1;
  bool inserted = false;
  char tmpLine[256]; // Ausreichend für ein JSON aus prepareMacroJsonString
  bool hasError = false;
  
  while (in.available()) {
    int bytesRead = in.readBytesUntil('\n', tmpLine, sizeof(tmpLine) - 1);
    tmpLine[bytesRead] = '\0'; // Korrekte Null-Terminierung

    // Hatten bis '\n' gelesen, davor war evtl '\r'
    if (bytesRead > 0 && tmpLine[bytesRead - 1] == '\r') {
      tmpLine[bytesRead - 1] = '\0';
    }

    if (line == index) {
      if (out.println(jsonAction) == 0) hasError = true;
      inserted = true;
    }

    if (out.println(tmpLine) == 0) hasError = true;
    
    line++;
    if (hasError) break;
  }

  // Falls der Index größer war als die Datei lang ist
  if (!inserted && !hasError) {
    if (out.println(jsonAction) == 0) hasError = true;
  }
  
  in.close();
  out.close();

  if (hasError) {
    LittleFS.remove(tmp);
    return false;
  }

  LittleFS.remove(src);
  return LittleFS.rename(tmp, src);
}

bool KinoMacroEngine::deleteCommand(const char* macroName, size_t index) {
  char src[48];
  getMacroPath(macroName, src, sizeof(src));
  char tmp[54];
  snprintf(tmp, sizeof(tmp), "%s.tmp", src);
  
  File in = LittleFS.open(src, "r");
  if (!in) return false;
  File out = LittleFS.open(tmp, "w");
  if (!out) {
    in.close();
    return false;
  }

  char tmpLine[256];
  bool hasError = false;
  size_t line = 1;
  while (in.available()) {
    int bytesRead = in.readBytesUntil('\n', tmpLine, sizeof(tmpLine) - 1);
    tmpLine[bytesRead] = '\0';

    if (bytesRead > 0 && tmpLine[bytesRead - 1] == '\r') {
      tmpLine[bytesRead - 1] = '\0';
    }
    if (line != index) {
      if (out.println(tmpLine)==0) hasError = true;
    }
    if (hasError) break;
    line++;
  }
  
  in.close();
  out.close();
  if (hasError) {
    LittleFS.remove(tmp);
    return false;
  }
  LittleFS.remove(src);
  return LittleFS.rename(tmp, src);
}

bool KinoMacroEngine::updateCommand(const char* macroName, size_t index, const char* jsonAction) {
  char src[48];
  char tmp[54];
  getMacroPath(macroName, src, sizeof(src));
  snprintf(tmp, sizeof(tmp), "%s.tmp", src);

  File in = LittleFS.open(src, "r");
  if (!in) {
    Serial.print(F("KinoMacroEngine::updateCommand : Could not open source file "));
    Serial.println(src);
    return false;
  }
  File out = LittleFS.open(tmp, "w");
  if (!out) {
    in.close();
    Serial.print(F("KinoMacroEngine::updateCommand : Could not open temp file "));
    Serial.println(tmp);
    return false;
  }

  size_t line = 1;
  bool replaced = false;

  char tmpLine[256];
  bool hasError = false;
  while (in.available()) {
    int bytesRead = in.readBytesUntil('\n', tmpLine, sizeof(tmpLine) - 1);
    tmpLine[bytesRead] = '\0';

    if (bytesRead > 0 && tmpLine[bytesRead - 1] == '\r') {
      tmpLine[bytesRead - 1] = '\0';
    }
    if (line == index) {
      if (out.println(jsonAction)==0) hasError = true;
      replaced = true;
    } else {
      if (out.println(tmpLine)==0) hasError = true;
    }
    if (hasError) break;
    line++;
  }

  in.close();
  out.close();

  if (!replaced || hasError) {
    Serial.println("KinoMacroEngine::updateCommand(): Did not replace anything");
    LittleFS.remove(tmp);
    return false;
  }

  LittleFS.remove(src);
  LittleFS.rename(tmp, src);
  return true;
}

bool KinoMacroEngine::createMacro(const char* mName) {
  char path[48];
  getMacroPath(mName, path, sizeof(path));
  if (LittleFS.exists(path)) {
    Serial.println(F("Macro already exists"));
    Serial.println(path);
    return false;
  }
  File f = LittleFS.open(path, "w");
  if (!f) return false;
  f.close();
  return true;
}

bool KinoMacroEngine::addOrUpdateMacro(const char* json) {
  DynamicJsonDocument doc(512);
  if (!deserializeJson(doc, json)) return false;
  if (!doc.containsKey("name") || !doc.containsKey("actions")) return false;

  const char* mName = doc["name"].as<const char*>();
  char path[48];
  getMacroPath(mName, path, sizeof(path));
  File f = LittleFS.open(path, "w");
  if (!f) {
    Serial.print(F("KinoMacroEnging::addOrUpdateMacro : could not open file "));
    Serial.println(path);
    return false;
  }
  for (JsonObject a : doc["actions"].as<JsonArray>()) {
    serializeJson(a, f);
    f.println();
  }
  f.close();
  return true;
}

bool KinoMacroEngine::deleteMacro(const char* macroName) {
  char path[48];
  getMacroPath(macroName, path, sizeof(path));
  if (!LittleFS.exists(path)) {
    Serial.print(F("File does not exist: "));
    Serial.println(path);
    return false;
  }
  return LittleFS.remove(path);
}

bool KinoMacroEngine::renameMacro(const char* oldName, const char* newName) {
  char oldPath[48];
  char newPath[48];
  getMacroPath(oldName, oldPath, sizeof(oldPath));
  getMacroPath(newName, newPath, sizeof(newPath));
  if (strlen(newPath)>31) {
    Serial.print(F("new path is too long for file system (max 32 chars): "));
    Serial.println(newPath);
    return false;
  }
  return LittleFS.rename(oldPath, newPath);
}

size_t KinoMacroEngine::getMacroCount() {
  size_t ct = 0;
  Dir dir = LittleFS.openDir("/macros");
  while (dir.next()) ct++;
  return ct;
}

KinoError KinoMacroEngine::getMacroNameByIndex(size_t index, KinoVariant& out) {
  size_t ct = 0;
  bool found = false;
  char mname[32];
  Dir dir = LittleFS.openDir("/macros");

  while (dir.next()) {
    if (ct == index) {
      // fileName() liefert leider einen String, den wir sofort in unseren Buffer kopieren
      strncpy(mname, dir.fileName().c_str(), sizeof(mname)-1);
      mname[sizeof(mname)-1] = '\0'; // Sicher terminieren
      found = true;
      break;
    }
    ct++;
  }

  if (found) {
    // Entspricht mname.replace(".macro", "");
    // Wir suchen den Punkt der Dateiendung
    char* dot = strstr(mname, ".macro");
    if (dot) {
      *dot = '\0'; // Wir setzen den Null-Terminator einfach auf die Position des Punktes
    }
    out.setString(mname);
    return KinoError::OK;
  }
  out.setNone();
  return KinoError::OutOfRange;
}

size_t KinoMacroEngine::getMacroIndex(const char* mName) {
  char cmpPath[48];
  snprintf(cmpPath, sizeof(cmpPath), "%s.macro", mName);
  size_t index = 0;
  Dir dir = LittleFS.openDir(F("/macros"));
  while(dir.next()) {
    if (strcmp(dir.fileName().c_str(), cmpPath)==0) return index;
    index++;
  }
  return (size_t)-1;
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


void KinoMacroEngine::_addError(uint16_t index, const char* cmd, const char* message) {
  if (_onError) {
    _onError(index, cmd, message);
  }
  if (_errors.size() < 10) {
    _errors.emplace_back(index, cmd, message);
  }
}
void KinoMacroEngine::_addError(uint16_t index, const __FlashStringHelper* cmd, const __FlashStringHelper* msg) {
  if (_onError) {
    char cmdbuf[12];
    char msgbuf[48];
    if (cmd) {
        strncpy_P(cmdbuf, (PGM_P)cmd, sizeof(cmdbuf)-1);
    } else {
        cmdbuf[0] = '\0';
    }
    cmdbuf[sizeof(cmdbuf)-1] = '\0';
    
    if (msg) {
        strncpy_P(msgbuf, (PGM_P)msg, sizeof(msgbuf)-1);
    } else {
        msgbuf[0] = '\0';
    }
    msgbuf[sizeof(msgbuf)-1] = '\0';
    _onError(index, cmdbuf, msgbuf);
  }
  if (_errors.size() < 10) {
    _errors.emplace_back(index, cmd, msg); // Ruft den neuen Konstruktor auf
  }
}


// --------------------------------------------------
// helpers
// --------------------------------------------------
/*
String KinoMacroEngine::_macroPath(const String& name) const {
  return "/macros/" + name + ".macro";
}*/

void KinoMacroEngine::getMacroPath(const char* macroName, char* out, size_t outLen) {
  snprintf(out, outLen, "/macros/%s.macro", macroName);
}
