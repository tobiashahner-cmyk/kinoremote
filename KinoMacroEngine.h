#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

struct MacroRuntime {
  bool running = false;
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
  bool startMacro(const String& name);
  void tick();
  bool isRunning() const;
  bool addOrUpdateMacro(const String& json);
  bool deleteMacro(const String& macroName);
  bool getMacroLines(const String& macroName, std::vector<String>& outLines);
  bool addCommand(const String& macroName, size_t index/*1-basiert*/, const String& jsonActionElement);
  bool deleteCommand(const String& macroName, size_t index);
  bool updateCommand(const String& macroName, size_t index, const String& jsonActionElement);
  
  // error handling
  size_t errorCount() const;
  const MacroError& getError(size_t index) const;
  void _addError(uint8_t index, const String& cmd, const String& message);

private:
  StaticJsonDocument <2048> _macroDoc;
  bool _ready;
  bool _loadMacro(const String& macroName);
  bool _saveMacro();
  bool _executeAction(const JsonObject& action, uint16_t index);
  void _clearErrors();
  std::vector<MacroError> _errors;

  struct {
    bool running = false;
    uint16_t index = 0;
    JsonArray actions;
  } runtime;
};
