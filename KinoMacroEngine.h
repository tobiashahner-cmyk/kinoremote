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
  bool isReady();
  std::vector<String> listMacros();
  bool addOrUpdateMacro(const String& json);
  bool deleteMacro(const String& macroName);
  bool startMacro(const String& name);
  void tick();
  bool isReady() const;
  bool isRunning() const;
  
  // error handling
  size_t errorCount() const;
  const MacroError& getError(size_t index) const;
  void clearErrors();
  void _addError(uint8_t index, const String& cmd, const String& message);

private:
  StaticJsonDocument <2048> _macroDoc;
  bool _ready;
  bool _executeAction(const JsonObject& action, uint16_t index);
  void _clearErrors();
  std::vector<MacroError> _errors;

  struct {
    bool running = false;
    uint16_t index = 0;
    JsonArray actions;
  } runtime;
};
