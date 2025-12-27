#pragma once
#include <ArduinoJson.h>

enum class ActionError : uint8_t {
    OK = 0,

    // Struktur / JSON
    MissingCmd,
    MissingParam,
    InvalidParamType,
    ParamOutOfRange,

    // Dispatch / Logik
    UnknownCommand,
    ExecutionFailed,

    // System
    DeviceNotReady,
    Timeout,

    // Fallback
    InternalError
};

struct ActionResult {
    ActionError error = ActionError::OK;
    String message;   // optional, menschenlesbar

    bool ok() const {
        return error == ActionError::OK;
    }
};



class MacroActions {
  

public:
    using ActionHandler = ActionResult (*)(JsonObject action);
    struct ActionEntry {
        const char* cmd;
        ActionHandler handler;
    };

    static bool _unknown(JsonObject action);
    static ActionResult execute(const JsonObject& action);

private:

};
