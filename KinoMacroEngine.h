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
  bool addOrUpdateMacro(const String& json);
  bool startMacro(const String& name);
  void tick();
  bool isRunning() const;
  // error handling
  size_t errorCount() const;
  const MacroError& getError(size_t index) const;

private:
  StaticJsonDocument <2048> _macroDoc;
  bool executeAction(const JsonObject& action, uint16_t index);
  void clearErrors();
  std::vector<MacroError> _errors;

  struct {
    bool running = false;
    uint16_t index = 0;
    JsonArray actions;
  } runtime;
};
