#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "KinoMacroActions.h"

using MacroFinishedCallback = std::function<void(bool success)>;

struct MacroRuntime {
  bool running = false;
  bool testing = false;
  uint16_t index = 0;
  JsonArray actions;
};

struct MacroError {
  uint16_t index;
  String cmd;
  String message;

  MacroError(uint16_t i, const String& c, const String& m)
    : index(i), cmd(c), message(m) {}
};

class KinoMacroEngine {
public:
  bool begin();
  bool isReady() const;
  std::vector<String> listMacros();
  bool startMacro(const String& name, MacroFinishedCallback cb=nullptr);
  bool testMacro(const String& name, MacroFinishedCallback cb=nullptr);
  String getName() const;
  void tick();
  bool isRunning() const;
  bool isPausing() const;
  bool addOrUpdateMacro(const String& json);
  bool deleteMacro(const String& macroName);
  bool getMacroLines(const String& macroName, std::vector<String>& outLines);
  bool addCommand(const String& macroName, size_t index, const String& jsonActionElement);    // index ist 1-basiert!
  bool deleteCommand(const String& macroName, size_t index);                                  // index ist 1-basiert!
  bool updateCommand(const String& macroName, size_t index, const String& jsonActionElement); // index ist 1-basiert!

  
  
  // error handling
  size_t errorCount() const;
  const MacroError& getError(size_t index) const;
  void _addError(uint8_t index, const String& cmd, const String& message);

private:
  StaticJsonDocument <2048> _macroDoc;
  bool _ready;
  bool _pausing;
  unsigned long _pauseStart;
  unsigned long _pauseDuration;
  bool _loadMacro(const String& macroName);
  bool _saveMacro();
  bool _executeAction(const JsonObject& action, uint16_t index);
  void _clearErrors();
  
  std::vector<MacroError> _errors;
  MacroFinishedCallback _onFinished;

  struct {
    bool running = false;
    bool testing = false;
    uint16_t index = 0;
    JsonArray actions;
  } runtime;
};
