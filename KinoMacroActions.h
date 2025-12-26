#pragma once
#include <ArduinoJson.h>

class MacroActions {
  

public:
    using ActionHandler = bool (*)(JsonObject action);
    struct ActionEntry {
        const char* cmd;
        ActionHandler handler;
    };

    static bool _unknown(JsonObject action);
    static bool execute(JsonObject action);

private:
    /*struct ActionEntry {
        const char* cmd;
        ActionHandler handler;
    };

    static bool _unknown(JsonObject action);*/
};
