#pragma once
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include "KinoAPI.h"

void webserverMacroFinished(bool success);

namespace webserver {  
    KinoError begin();      // initialisiert den Webserver
    void loop();
    void handleRoot();
    void handle404();
    void handleCSS();
    void handleJS();

    void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

    void socketMacroFinished(bool success);
    void socketMacroError(int linenr, const char* cmd, const char* msg);
    void listMacros();
    void createNewMacro();
    void handleMacroEdit();
    void handleMacroRename();
    void handleMacroDelete();
    void showMacro(const char* macroName);
    void showLineAddButton(int beforeLineNr);
    //void showMacroLine(String& mLine, int lineIndex);
    void showMacroLine(const char* mLine, int lineIndex);
    //void showDelayInput(const String& curSecs, int lineIndex);
    void showDelayInput(int curSecs, int lineIndex);
    void showMacroCommandSelect(const String& curCmd, int lineIndex);
    void showMacroCommandSelect(const char* curCmd, int lineIndex);
    //void showMacroDeviceSelect(const String& curDevice, int lineIndex);
    void showMacroDeviceSelect(const char* curDevice, int lineIndex);
    void showNewLineTemplate();
    void sendDeviceFuncSelect();
    void sendFuncControls();

    void getMacroValue(const char* dev, const char* key, const char* apikey, char* mVal, size_t mValLen);
    void getMacroValue(const char* dev, const char* key, const char* apikey, KinoVariant& mVal);
    String getSelectedKeyForFuncSelect(const String& dev, const String& key);
    void getSelectedKeyForFuncSelect(const char* dev, const char* key, char* buf, size_t bufLen);
    void showFuncSelect(const String& dev, const String& key, /*const String& value,*/ int lineIndex);
    void showFuncSelect(const char* dev, const char* key, /*const String& value,*/ int lineIndex);
    void showFuncControls(const String& dev, const String& key, int lineIndex);
    void showFuncControls(const char* dev, const char* key, int lineIndex);
    bool isProperty(const char* deviceName, const char* getsetPath);
    void showParamPropertyControl(const char* deviceName, const char* getsetPath);
    void startMacroSelect(const char* deviceName, const char* func, const char* label);
    void endMacroSelect();
    void macroToggleButton(const char* deviceName, const char* func, const char* label, bool value);
    void macroSlider(const char* deviceName, const char* func, const char* label, int value, int minval, int maxval, int steps);
    void macroColorPicker(const char* deviceName, const char* func, const char* label, const char* value);

    bool checkMacroPostParameters();

    void updateMacro();
    bool updateMacroSetLine();
    bool updateMacroDelayLine();
    bool deleteMacroLine();

    void insertMacroLine();
    bool insertMacroSetLine();
    bool insertMacroDelayLine();
    
    void handleDevice(const char* deviceName);
    void handleCmd();

    void buildSimpleControl(const char* deviceName, const KinoPropertyInfo*& pi);
    void buildSelect(const char* deviceName, const KinoPropertyInfo*& pi, bool hasParams);
    void buildComplexList(const char* deviceName, const KinoPropertyInfo*& pi, bool hasParams);
    void showParameters(const char* deviceName, const char* paramPath);

    void pageStart(const char* title)                                                                       ;
    void pageEnd()                                                                                          ;
    void toggleButton(const char* deviceName, const char* func, const char* label, bool value)              ;
    void slider(const char* deviceName, const char* func, const char* label, int value, int minval, int maxval, int steps)     ;
    void selectStart(const char* deviceName, const char* func, const char* label)                           ;
    void optionItem(const char* value, const char* label, bool selected)                                    ;
    void selectEnd()                                                                                        ;
    void button(const char* deviceName, const char* func, const char* label)                                ;
    void infoText(const char* deviceName, const char* func, const char* label, const char* value)           ;
    void groupCardStart(const char* id, const char* label)                                                  ;
    void groupCardEnd()                                                                                     ;
    void colorPicker(const char* deviceName, const char* func, const char* label, const char* value)        ;

    extern const char HTML_CSS[] PROGMEM;
    extern const char HTML_JAVASCRIPT[] PROGMEM;
};
