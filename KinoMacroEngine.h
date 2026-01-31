#pragma once


#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <functional>
#include <vector>
#include "KinoMacroActions.h"


using MacroFinishedCallback = std::function<void(bool success)>;


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


// execution
bool startMacro(const String& name, MacroFinishedCallback cb=nullptr);
bool testMacro(const String& name, MacroFinishedCallback cb=nullptr);
void tick();
bool isRunning() const;
bool isPausing() const { return _pausing; }
String getName() const;

// macro management
std::vector<String> listMacros();
size_t getMacroCount();
String getMacroName(size_t index);
size_t getMacroIndex(const String& macroName);

//std::vector<String> getAvailableMacroCommands();
std::vector<const char*> getAvailableMacroCommands();

bool createMacro(const String& name);
bool addOrUpdateMacro(const String& json); // compatibility shim
bool deleteMacro(const String& macroName);
bool renameMacro(const String& oldName,const String& newName);

// line based API (unchanged externally)
bool getMacroLines(const String& macroName, std::vector<String>& outLines);
bool getMacroLineCount(const String& macroName, size_t& out);
bool getMacroLineByIndex(const String& macroName, size_t index, String& out);
bool addCommand(const String& macroName, size_t index, const String& jsonActionElement); // 1-based
bool deleteCommand(const String& macroName, size_t index); // 1-based
bool updateCommand(const String& macroName, size_t index, const String& jsonActionElement); // 1-based


// error handling
size_t errorCount() const;
const MacroError& getError(size_t index) const;

private:
// runtime
struct {
bool running = false;
bool testing = false;
uint16_t line = 0; // current line number (1-based)
File file;
} runtime;


bool _ready = false;
bool _pausing = false;
unsigned long _pauseStart = 0;
unsigned long _pauseDuration = 0;
String _currentMacroName;

std::vector<MacroError> _errors;
MacroFinishedCallback _onFinished;


// helpers
bool _executeAction(const JsonObject& action, uint16_t index);
void _addError(uint16_t index, const String& cmd, const String& message);
void _clearErrors();


// filesystem helpers
String _macroPath(const String& name) const;
};
