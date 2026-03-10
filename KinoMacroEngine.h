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
using MacroErrorCallback = std::function<void(int linenr, const char* cmd, const char* errMsg)>;


struct MacroError {
  uint16_t index;
  char cmd[12];
  char message[48]; // Etwas mehr Puffer für Fehlermeldungen schadet meist nicht

  MacroError(uint16_t i, const char* c, const char* m) : index(i) {
    // Sicher kopieren mit Längenbegrenzung
    strncpy(cmd, c ? c : "", sizeof(cmd) - 1);
    cmd[sizeof(cmd) - 1] = '\0';
    
    strncpy(message, m ? m : "", sizeof(message) - 1);
    message[sizeof(message) - 1] = '\0';
  }
  MacroError(uint16_t i, const __FlashStringHelper* c, const __FlashStringHelper* m) : index(i) {
    // Für cmd
    if (c) {
        strncpy_P(cmd, (PGM_P)c, sizeof(cmd) - 1);
    } else {
        cmd[0] = '\0';
    }
    cmd[sizeof(cmd) - 1] = '\0';
    
    // Für message
    if (m) {
        strncpy_P(message, (PGM_P)m, sizeof(message) - 1);
    } else {
        message[0] = '\0';
    }
    message[sizeof(message) - 1] = '\0';
  }
};

class KinoMacroEngine {
public:
bool begin();
bool isReady() const;


// execution
bool startMacro(const char* name, MacroFinishedCallback cb=nullptr, MacroErrorCallback e=nullptr);
bool testMacro(const char* name, MacroFinishedCallback cb=nullptr, MacroErrorCallback e=nullptr);
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
MacroErrorCallback _onError;

//StaticJsonDocument<1024> _actionDoc;

// helpers
bool _executeAction(const JsonObject& action, uint16_t index);
void _addError(uint16_t index, const char* cmd, const char* message);
void _addError(uint16_t index, const __FlashStringHelper* cmd, const __FlashStringHelper* msg);
void _clearErrors();


// filesystem helpers
//String _macroPath(const String& name) const;
void getMacroPath(const char* macroName, char* out, size_t outLen);
};
