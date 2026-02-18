#pragma once


#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <functional>
#include <vector>
#include "KinoError.h"
#include "KinoVariant.h"
#include "FileHelper.h"
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
bool startMacro(const char* name, MacroFinishedCallback cb=nullptr);
bool testMacro(const char* name, MacroFinishedCallback cb=nullptr);
void tick();
bool isRunning() const;
bool isPausing() const { return _pausing; }
void getName(char* out, size_t outLen);

// macro management
size_t getMacroCount();
KinoError getMacroNameByIndex(size_t index, KinoVariant& out);
size_t getMacroIndex(const char* macroName);

//std::vector<const char*> getAvailableMacroCommands();
size_t getMacroCommandCount();
bool getMacroCommand(size_t index, char* out, size_t outLen);

bool createMacro(const char* name);
bool addOrUpdateMacro(const char* json); // compatibility shim
bool deleteMacro(const char* macroName);
bool renameMacro(const char* oldName,const char* newName);

// line based API (unchanged externally)
size_t getMacroLineCount(const char* macroName);
bool getMacroLineByIndex(const char* macroName, size_t index, char* out, size_t outLen);
bool addCommand(const char* macroName, size_t index, const char* jsonActionElement); // 1-based
bool deleteCommand(const char* macroName, size_t index); // 1-based
bool updateCommand(const char* macroName, size_t index, const char* jsonActionElement); // 1-based


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
char _currentMacroName[32];

std::vector<MacroError> _errors;
MacroFinishedCallback _onFinished;


// helpers
bool _executeAction(const JsonObject& action, uint16_t index);
void _addError(uint16_t index, const String& cmd, const String& message);
void _clearErrors();


// filesystem helpers
String _macroPath(const String& name) const;
void getMacroPath(const char* macroName, char* out, size_t outLen);
};
