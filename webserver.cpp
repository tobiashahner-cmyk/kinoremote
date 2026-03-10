#include "webserver.h"
#include <ArduinoJson.h>

void webserverMacroFinished(bool success) {
  return webserver::socketMacroFinished(success);
}

namespace webserver {
  ESP8266WebServer _server(80);
  WebSocketsServer _socket(81);
  
  StaticJsonDocument<1024> _settings;         // Container für Werte von Properties und Parametern, die innerhalb eines Makros gesetzt werden
  StaticJsonDocument<256>  _lineSettings;     // Container für Werte von Properties und Parametern, die innerhalb einer Makrozeile gesetzt werden
  StaticJsonDocument<256>  _lineKeys;         // Merkliste für Funktions-Selects, die für eine Makrozeile schon angezeigt wurden
  StaticJsonDocument<512> _parseContainer;    // Puffer für diverse kleine Jsons (macroLine, Websocket Push)

  
  const int _paramPathLen = 128;
  char _paramPath[128];
  const int _pathHelperLen = 128;
  char _pathHelper[128];
  char responseBuffer[512];

  size_t WSConnected = 0;

KinoError begin() {
  _server.on("/",handleRoot);
  _server.on("/cmd",handleCmd);
  _server.on("/macroEdit",handleMacroEdit);
  _server.on("/macroRename",handleMacroRename);
  _server.on("/macroDelete",handleMacroDelete);
  _server.on("/updateMacro",updateMacro);
  _server.on("/devFuncSelect",sendDeviceFuncSelect);
  _server.on("/devFuncControls",sendFuncControls);
  _server.on("/insertMacroLine",insertMacroLine);
  _server.on("/newMacro",createNewMacro);
  _server.on("/style.css", handleCSS);
  _server.on("/script.js", handleJS);
  _server.onNotFound(handle404);
  _server.begin();
  _socket.begin();
  _socket.onEvent(webSocketEvent);
  _socket.enableHeartbeat(15000, 3000, 4);
  return KinoError::OK;
}

void handleCSS() {
  // Cache für 31 Tage (2678400 Sekunden) aktivieren
  _server.sendHeader("Cache-Control", "public, max-age=2678400");
  _server.send_P(200, "text/css", HTML_CSS);
}

void handleJS() {
  // Cache für 31 Tage (2678400 Sekunden) aktivieren
  _server.sendHeader("Cache-Control", "public, max-age=2678400");
  _server.send_P(200, "text/js", HTML_JAVASCRIPT);
}

void loop() {
  static unsigned long lastupdate = millis();
  static size_t lastWSConnected = WSConnected;
  _socket.loop();
  _server.handleClient();
  _parseContainer.clear();
  if (WSConnected == 0) return;
  if (KinoAPI::getJsonUpdates(_parseContainer) == KinoError::OK) {
    serializeJson(_parseContainer, responseBuffer);
    delay(10);
    _socket.broadcastTXT(responseBuffer);
  } else {  // keine updates zu senden, schicke WSConnected
    if ((millis() - lastupdate > 5000)&&(WSConnected != lastWSConnected)) {
      _parseContainer["dev"].set((char*)"system");
      _parseContainer["online"].set(WSConnected);
      _parseContainer["timestamp"].set(millis());
      serializeJson(_parseContainer, responseBuffer);
      delay(10);
      _socket.broadcastTXT(responseBuffer);
      lastupdate = millis();
      lastWSConnected = WSConnected;
    }
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            if (WSConnected > 0) WSConnected--;
            break;
        case WStype_CONNECTED: {
            IPAddress ip = _socket.remoteIP(num);
            Serial.printf("[%u] Connected from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
            WSConnected++;
            break;
        }
        case WStype_TEXT:
            // Hier könnten Befehle vom Browser ankommen
            Serial.printf("[%u] get Text: %s\n", num, payload);
            break;
    }
}

void handle404() {
  size_t devCount = 0;
  KinoError e = KinoAPI::getDeviceCount(devCount);
  KinoVariant dev;
  for (int i=0; i<devCount; i++) {
    KinoAPI::getDeviceName(i, dev);
    if (_server.uri().substring(1) == dev.c_str()) {
      handleDevice(dev.c_str());
      return;
    }
  }
  _server.send(404,"text/html","Seite nicht gefunden");
}

void handleCmd() {
  Serial.println(F("CMD empfangen:"));
  for (int i=0; i<_server.args(); i++) {
    Serial.print(_server.argName(i));
    Serial.print(F(" : "));
    Serial.println(_server.arg(i));
  }
  bool ok = true;
  if (!_server.hasArg("dev")) ok = false;
  if (!_server.hasArg("f")) ok = false;
  if (!_server.hasArg("v")) ok = false;
  // first, check if "dev" is "macro"
  if ((ok)&&(strcmp(_server.arg("dev").c_str(),"macro")==0)) {
    if (_server.arg("f").length() < 4) { _server.send(200, "text/plain", "Fehler"); return; }
    //bool err = KinoAPI::executeMacro(_server.arg("f").c_str()+4,webserverMacroFinished); // überspringe die ersten 4 Zeichen, also "run/"
    bool err = KinoAPI::executeMacro(_server.arg("f").c_str()+4,webserver::socketMacroFinished,webserver::socketMacroError); // überspringe die ersten 4 Zeichen, also "run/"
    Serial.println((err)?F("Makro gestartet"):F("Fehler beim Starten des Makros"));
    _server.send(200, "text/plain", (err)?"OK":"Fehler");
    return;
  }
  if(ok) {
    KinoVariant value = KinoVariant::fromString(_server.arg("v").c_str());
    Serial.print(F("setting ")); Serial.print(_server.arg("dev")); Serial.print(F(".")); Serial.print(_server.arg("f")); Serial.print(F(" to ")); Serial.println(value.c_str());
    KinoError err = KinoAPI::setProperty(_server.arg("dev").c_str(), _server.arg("f").c_str(), value);
    Serial.println(kinoErrorToString(err));
    _server.send(200, "text/plain", kinoErrorToString(err));
    err = KinoAPI::commit(_server.arg("dev").c_str());
  } else {
    _server.send(200, "text/plain", "missing parameter");
  }
}

void socketMacroFinished(bool success) {
  char mName[32];
  KinoAPI::getCurrentMacroName(mName, sizeof(mName));
  if (!success) {
    snprintf(responseBuffer, sizeof(responseBuffer), "{\"dev\":\"macro\",\"cmd\":\"error\",\"val\":{\"text\":\"Es gab Fehler in Makro %s\"}}", mName);
  } else {
    snprintf(responseBuffer, sizeof(responseBuffer), "{\"dev\":\"macro\",\"cmd\":\"success\",\"val\":{\"text\":\"Makro %s fertig\"}}", mName);
  }
  _socket.broadcastTXT(responseBuffer);
}

void socketMacroError(int linenr, const char* cmd, const char* msg) {
  char mName[32];
  KinoAPI::getCurrentMacroName(mName, sizeof(mName));
  snprintf(responseBuffer, sizeof(responseBuffer), "{\"dev\":\"macro\",\"cmd\":\"error\",\"val\":{\"macro\":\"%s\",\"line\":%d,\"cmd\":\"%s\",\"msg\":\"%s\"}}", mName, linenr, cmd, msg);
  _socket.broadcastTXT(responseBuffer);
}

void handleRoot() {
  KinoError err;
  pageStart("Kino");

  listMacros();
  
  size_t devCount;
  err = KinoAPI::getDeviceCount(devCount);
  char cardName[32];
  snprintf(cardName, sizeof(cardName), "%d Ger&auml;te", devCount);
  cardName[31] = '\0';
  groupCardStart("devices",cardName);
  KinoVariant devName;
  for (int i=0; i<devCount; i++) {
    err = KinoAPI::getDeviceName(i, devName);
    _server.sendContent(F("<div class='card'>"));
    _server.sendContent(F("<a class='btn' href='/"));
    _server.sendContent(devName.c_str());
    _server.sendContent(F("'><h2>"));
    _server.sendContent(devName.c_str());
    _server.sendContent(F("</h2></a>"));
    _server.sendContent(F("</div>"));
  }
  groupCardEnd();
  pageEnd();
}

void handleMacroRename() {
  bool POSTOK = true;
  int macroIndex = _server.arg("macronr").toInt();
  KinoVariant mName;
  KinoError err = KinoAPI::getMacroNameByIndex(macroIndex, mName);
  if (err != KinoError::OK) POSTOK = false;
  char* rawS = mName.s;
  char* fileEnding = strstr(rawS, ".macro");
  if (fileEnding) *fileEnding = '\0';
  if (strlen(rawS)==0) POSTOK = false;
  
  if (!_server.hasArg("newname") || strlen(_server.arg("newname").c_str())==0) POSTOK = false; 
  bool ok = false;
  if (POSTOK) {
    ok = KinoAPI::renameMacro(mName.c_str(), _server.arg("newname").c_str());
  }
  if (ok) {
    size_t newMacroIndex;
    err = KinoAPI::getMacroIndexByName(_server.arg("newname").c_str(), newMacroIndex);
    char indexbuf[12];
    snprintf(indexbuf, sizeof(indexbuf), "%d", newMacroIndex);
    pageStart("Erfolg");
    _server.sendContent(F("<div class='card'><h2>Makro wurde umbenannt</h2>"));
    _server.sendContent(F("<p><a href='/macroEdit?m="));
    _server.sendContent(indexbuf);
    _server.sendContent(F("'>zur&uuml;ck zum Makro</a></p></div>"));
  } else {
    pageStart("Fehler!");
    if (POSTOK) {
      _server.sendContent(F("<div class='card'><h2>Filesystem meldet Fehler</h2>"));
      _server.sendContent(F("<p>Checken Sie den Dateinamen und versuchen Sie es erneut.<br>(Vielleicht zu lang?)</p>"));
      _server.sendContent(F("<p><a href='/macroEdit?m="));
      _server.sendContent(_server.arg("macronr").c_str());
      _server.sendContent(F("#ma'>zur&uuml;ck zum Makro</a></p></div>"));
    } else {
      _server.sendContent(F("<div class='card'><h2>Probleme mit den Parametern</h2>"));
      if (strlen(rawS)==0)  _server.sendContent(F("<p>Makro wurde nicht gefunden.</p>"));
      if (strlen(_server.arg("newname").c_str())==0) _server.sendContent(F("<p>Der Name darf nicht leer sein.</p>"));
      _server.sendContent(F("<p><a href='/macroEdit?m="));
      _server.sendContent(_server.arg("macronr").c_str());
      _server.sendContent(F("#ma'>zur&uuml;ck zum Makro</a></p>"));
    }
  }
  _server.sendContent(F("<p><a href='/'>zur&uuml;ck zur Startseite</a></p></div>"));
  pageEnd();
}

void handleMacroDelete() {
  bool POSTOK = true;
  int macroIndex = _server.arg("macronr").toInt();
  KinoVariant mName;
  KinoError err = KinoAPI::getMacroNameByIndex(macroIndex, mName);
  if (err != KinoError::OK) POSTOK = false;
  char* macroName = mName.s;
  char* fileEnding = strstr(macroName, ".macro");
  if (fileEnding) *fileEnding = '\0';
  if (strlen(macroName)==0) POSTOK = false;
  
  if (POSTOK && !_server.hasArg("confirm")) {
    pageStart("Wirklich?");
    _server.sendContent(F("<div class='card'><p>Soll der Makro "));
    _server.sendContent(macroName);
    _server.sendContent(F(" wirklich gel&ouml;scht werden?</p></div>"));
    _server.sendContent(F("<div class='card'><form action='' method='POST'><input type='hidden' name='macronr' value='"));
    _server.sendContent(_server.arg("macronr").c_str());
    _server.sendContent(F("'><button class='btn' name='confirm' value='1'>L&ouml;schen</button><form><div>"));
    _server.sendContent(F("<div class='card'><p><a class='btn' href='/macroEdit?m="));
    _server.sendContent(_server.arg("macronr").c_str());
    _server.sendContent(F("'>zur&uuml;ck zum Makro</a></p><p><a class='btn' href='/'>zur&uuml;ck zur Startseite</a></p></div>"));
    pageEnd();
    return;
  }
  if (POSTOK && _server.hasArg("confirm")) {
    bool ok = KinoAPI::deleteMacro(macroName);
    if (ok) {
      pageStart("Erfolg");
      _server.sendContent(F("<div class='card'><p>Makro "));
      _server.sendContent(macroName);
      _server.sendContent(F(" wurde erfolgreich gel&ouml;scht.</p><p><a class='btn' href='/'>zur&uuml;ck zur Startseite</a></p></div>"));
      pageEnd();
      return;
    } else {
      pageStart("Fehler");
      _server.sendContent(F("<div class='card'><h2>Es ist ein Fehler aufgetreten</h2>"));
      _server.sendContent(F("<p>Der Makro "));
      _server.sendContent(macroName);
      _server.sendContent(F(" konnte aufgrund eines unbekannten Fehlers nicht gelöscht werden.</p>"));
      _server.sendContent(F("<a class='btn' href='/macroEdit?m="));
      _server.sendContent(_server.arg("macronr").c_str());
      _server.sendContent(F("#ma'>zur&uuml;ck zum Makro</a></p><p><a class='btn' href='/'>zur&uuml;ck zur Startseite</a></p></div>"));
      pageEnd();
      return;
    }
  }
  if (!POSTOK) {
    pageStart("Fehler");
    _server.sendContent(F("<div class='card'><h2>Parameter passen nicht</h2><p>Es gibt ein Problem mit den Formularwerten. Konnte Nichts machen...</p>"));
    _server.sendContent(F("<p><a class='btn' href='/'>zur&uuml;ck zur Startseite</a></p></div>"));
    pageEnd();
    return;
  }
}

void handleMacroEdit() {
  if (!_server.hasArg("m")) {
    pageStart("Makros");
    listMacros();
    pageEnd();
    return;
  }
  KinoVariant macroName;
  KinoError err = KinoAPI::getMacroNameByIndex(_server.arg("m").toInt(), macroName);
  pageStart(macroName.c_str());
  showMacro(macroName.c_str());
  groupCardStart("ma","Makro Aktionen");
  _server.sendContent(F("<div class='card'><h2>Umbenennen</h2><form action='/macroRename'><input type='hidden' name='macronr' value='"));
  _server.sendContent(_server.arg("m").c_str());
  _server.sendContent(F("'><input type='text' name='newname' placeholder='newName' value='"));
  _server.sendContent(macroName.c_str());
  _server.sendContent(F("'><button>Umbenennen</button></form></div>"));
  _server.sendContent(F("<div class='card'><h2>L&ouml;schen</h2><form action='/macroDelete' method='POST'><button name='macronr' value='"));
  _server.sendContent(_server.arg("m").c_str());
  _server.sendContent(F("'>Makro l&ouml;schen</button></form></div>"));
  groupCardEnd();
  pageEnd();
}

void createNewMacro() {
  bool POSTOK = true;
  if ( (!_server.hasArg("newmacroname")) || (strlen(_server.arg("newmacroname").c_str())==0) ) POSTOK = false;

  bool ok = POSTOK;
  size_t macroIndex;
  if(POSTOK) {
    if (!KinoAPI::createMacro(_server.arg("newmacroname").c_str())) ok = false;
    KinoError e = KinoAPI::getMacroIndexByName(_server.arg("newmacroname").c_str(), macroIndex);
    if (e != KinoError::OK) ok = false;
  }

  if (ok) {
    char macroIndexBuffer[12];
    snprintf(macroIndexBuffer, sizeof(macroIndexBuffer), "%d", macroIndex);
    pageStart("Erfolg");
    _server.sendContent(F("<div class='card'><h2>Neues Makro</h2>"));
    _server.sendContent(F("<p>Makro mit dem Namen "));
    _server.sendContent(_server.arg("newmacroname").c_str());
    _server.sendContent(F(" wurde angelegt.</p>"));
    _server.sendContent(F("<p><a href='/macroEdit?m="));
    _server.sendContent(macroIndexBuffer);
    _server.sendContent(F("'>Makro bearbeiten</a></p>"));
  } else {
    pageStart("Fehler");
    _server.sendContent(F("<p>Makro mit dem Namen "));
    _server.sendContent(_server.arg("newmacroname").c_str());
    _server.sendContent(F(" konnte nicht angelegt werden.</p>"));
  }
  _server.sendContent(F("<p><a href='/'>zur&uuml;ck zur Startseite</a></p>"));
  _server.sendContent(F("</div>"));

  pageEnd();
}

void listMacros() {
  size_t macroCount;
  KinoError err =  KinoAPI::getMacroCount(macroCount);
  char cardName[20];
  snprintf(cardName,20,"%d Makros",macroCount);
  groupCardStart("macros",cardName);
  char dev[] = "macro";
  KinoVariant mName;
  for (int i=0; i<macroCount; i++) {
    err = KinoAPI::getMacroNameByIndex(i, mName);
    if (err != KinoError::OK) continue;
    char* realMacroName = mName.s;
    char* fileEnding = strstr(realMacroName, ".macro");
    if (fileEnding) *fileEnding = '\0';
    char id[20];
    snprintf(id,20,"macroname_%d", i);
    groupCardStart(id, realMacroName);
    char func[40];
    snprintf(func,40, "run/%s", realMacroName);
    func[39] = '\0';
    button(dev,func, "Execute");
    _server.sendContent(F("<div class='card'><a href='macroEdit?m="));
    char _i[8];
    itoa(i, _i, 10);
    _server.sendContent(_i);
    _server.sendContent(F("'>Edit</a></div>"));
    groupCardEnd();
  }
  groupCardStart("nm","Neues Makro");
  _server.sendContent(F("<div class='card'><form action='/newMacro'>"));
  _server.sendContent(F("<input type='text' name='newmacroname' placeholder='newMacro' value=''>"));
  _server.sendContent(F("<button class='btn'>Makro anlegen</button></form></div>"));
  groupCardEnd();
  groupCardEnd();
}

bool abortCheckPostParameters(const __FlashStringHelper* reason) {
  pageStart("Fehler beim Aktualisieren");
  _server.sendContent(reason);
  _server.sendContent(F("<p>Der Makro wurde nicht aktualisiert</p>"));
  return false;
}

bool checkMacroPostParameters() {
  int macroIndex = _server.arg("macronr").toInt();
  KinoVariant mName;
  KinoError err = KinoAPI::getMacroNameByIndex(macroIndex, mName);
  if (err != KinoError::OK) return abortCheckPostParameters(F("Makro wurde nicht gefunden"));
  char* macroName = mName.s;
  char* fileEnding = strstr(macroName, ".macro");
  if (fileEnding) *fileEnding = '\0';
  if (strlen(macroName)==0) return abortCheckPostParameters(F("Makroname kann nicht leer sein"));
  
  if ( (!_server.hasArg("cmd")) || (strlen(_server.arg("cmd").c_str())==0) ) {
    return abortCheckPostParameters(F("cmd missing"));
  }

  int lineIndex = -1;
  if (_server.hasArg("linenr")) lineIndex = _server.arg("linenr").toInt();
  if (lineIndex == -1) {
    return abortCheckPostParameters(F("line nr missing"));
  }

  if (strcmp(_server.arg("cmd").c_str(),"set")==0) {
    if ( (!_server.hasArg("dev")) || (strlen(_server.arg("dev").c_str())==0) ) {
      return abortCheckPostParameters(F("set selected, but dev missing"));
    }
  }

  int delaySeconds = -1;
  if (strcmp(_server.arg("cmd").c_str(), "delay")==0) {
    if (_server.hasArg("seconds")) delaySeconds = _server.arg("seconds").toInt();
    if (delaySeconds == -1) {
      return abortCheckPostParameters(F("delay selected, but seconds missing"));
    }
  }

  return true;
}

void updateMacro() {
  int macroIndex = _server.arg("macronr").toInt();

  bool POSTOK = checkMacroPostParameters();
  
  bool ok = false;
  if (  (POSTOK)&& (_server.hasArg("delete"))&& (strcmp(_server.arg("delete").c_str(), "yesplease")==0) ) { 
    ok = deleteMacroLine(); 
    POSTOK = false; //damit Nichts weiter gemacht wird
  }
  
  if (  (POSTOK)&&(strcmp(_server.arg("cmd").c_str(),"set")==0) ) {
    ok = updateMacroSetLine();
  }
  
  if (  (POSTOK)&&(strcmp(_server.arg("cmd").c_str(),"delay")==0) ) {
    ok = updateMacroDelayLine();
  }

  
  if (ok) {
    pageStart("Erfolg");
    _server.sendContent(F("<p>Der Makro wurde aktualisiert</p>"));
  } else {
    pageStart("Fehler");
    _server.sendContent(F("<p>Der Makro konnte nicht aktualisiert werden</p>"));
  }
  
  _server.sendContent(F("<p><a href='/macroEdit?m="));
  _server.sendContent(_server.arg("macronr").c_str());
  _server.sendContent(F("#line_"));
  _server.sendContent(_server.arg("linenr").c_str());
  _server.sendContent(F("'>zur&uuml;ck zum Listing</a></p>"));
  pageEnd();
}

bool deleteMacroLine() {
  KinoVariant mName;
  KinoError err = KinoAPI::getMacroNameByIndex(_server.arg("macronr").toInt(), mName);
  if (err != KinoError::OK) return false;
  char* macroName = mName.s;
  char* fileEnding = strstr(macroName, ".macro");
  if (fileEnding) *fileEnding = '\0';
  int linenr = _server.arg("linenr").toInt();
  bool ok = KinoAPI::deleteMacroCommand(macroName, linenr);
  return ok;
}

bool updateMacroSetLine() {
  KinoVariant mName;
  KinoError err = KinoAPI::getMacroNameByIndex(_server.arg("macronr").toInt(), mName);
  if (err != KinoError::OK) return false;
  char* macroName = mName.s;
  if (strlen(macroName)==0) return false;
  char* fileEnding = strstr(macroName, ".macro");
  if (fileEnding) *fileEnding = '\0';
  if (strlen(macroName)==0) return false;
  
  _parseContainer.clear();
  int linenr = _server.arg("linenr").toInt();
  _parseContainer["cmd"].set(_server.arg("cmd").c_str());
  _parseContainer["dev"].set(_server.arg("dev").c_str());
  if ( (_server.hasArg("commit")) && (strcmp(_server.arg("commit").c_str(), "on")==0) ) _parseContainer["commit"].set(true);
  int n = _server.args();

  char key[64];
  char value[64];

  for (int i = 0; i < n; i++) {
    strlcpy(key, _server.argName(i).c_str(), sizeof(key));
    strlcpy(value, _server.arg(i).c_str(), sizeof(value));

    char incChk[5];
    strlcpy(incChk, key, sizeof(incChk));
    if ( (strcmp(incChk,"inc_")==0) && (strcmp(value,"on")==0) ) {  // Wenn die inc_<key> checkbox checked ist
      char* newKey = &key[4]; // überspringe "inc_"
      char chkVal[64];
      snprintf(chkVal, sizeof(chkVal), "val_%s", newKey);           // ersetze "inc_" durch "val_". Unter diesem Namen wurde der Wert gepostet
      KinoVariant newVal = KinoVariant::fromString("off");          // Fallback für checkboxen, die wegen "off" nicht mitgesendet wurden
      if (_server.hasArg(chkVal)) newVal.setString(_server.arg(chkVal).c_str());
      KinoVariant curVal;                                           // hole den aktuellen Wert der Eigenschaft <key>, damit wir den Typen wissen
      KinoError err = KinoAPI::getProperty(_server.arg("dev").c_str(), newKey, curVal);
      switch(curVal.type) {
        case KinoVariant::BOOL: {
          _parseContainer["val"][newKey].set(newVal.asBool());
          break;
        }
        case KinoVariant::INT: {
          _parseContainer["val"][newKey].set(newVal.asInt());
          break;
        }
        case KinoVariant::FLOAT: {
          _parseContainer["val"][newKey].set(newVal.asFloat());
          break;
        }
        default: {  // also STRING und RGB_COLOR
          _parseContainer["val"][newKey].set((char*)newVal.c_str());  // Cast auf char*, um eine Kopie zu erzwingen
          break;
        }
      }
    }
  }

  char out[256];    // das sollte reichen für den Json-String
  serializeJson(_parseContainer, out);
  Serial.println(out);
  return KinoAPI::updateMacroCommand(macroName, linenr, out);
}

bool updateMacroDelayLine() {
  KinoVariant mName;
  KinoError err = KinoAPI::getMacroNameByIndex(_server.arg("macronr").toInt(), mName);
  if (err != KinoError::OK) return false;
  char* macroName = mName.s;
  if (strlen(macroName)==0) return false;
  char* fileEnding = strstr(macroName, ".macro");
  if (fileEnding) *fileEnding = '\0';
  
  _parseContainer.clear();
  
  int linenr = _server.arg("linenr").toInt();
  _parseContainer["cmd"].set(_server.arg("cmd").c_str());
  _parseContainer["seconds"].set(_server.arg("seconds").toInt());
  
  char out[256];
  serializeJson(_parseContainer, out);
  return KinoAPI::updateMacroCommand(macroName, linenr, out);
}

void insertMacroLine() {
  bool POSTOK = checkMacroPostParameters();
  int macroIndex = _server.arg("macronr").toInt();
  KinoVariant mName;
  KinoError err = KinoAPI::getMacroNameByIndex(macroIndex, mName);
  
  if (err != KinoError::OK) POSTOK = false;
  char* macroName = nullptr;
  if (POSTOK) {
    macroName = mName.s;
    char* fileEnding = strstr(macroName, ".macro");
    if (fileEnding) *fileEnding = '\0';
  }
  
  
  bool ok = false;
  if ((POSTOK)&&(strcmp(_server.arg("cmd").c_str(),"set")   ==0)) ok = insertMacroSetLine();
  if ((POSTOK)&&(strcmp(_server.arg("cmd").c_str(),"delay") ==0)) ok = insertMacroDelayLine();

  
  if (ok) {
    pageStart("Erfolg");
    _server.sendContent(F("<p>Der Makro wurde aktualisiert</p>"));
  } else {
    pageStart("Fehler");
    _server.sendContent(F("<p>Der Makro konnte nicht aktualisiert werden, KinoAPI::updateMacroCommand gab false zur&uuml;ck</p>"));
  }
  
  _server.sendContent(F("<p><a href='/macroEdit?m="));
  _server.sendContent(_server.arg("macronr").c_str());
  _server.sendContent(F("#line_"));
  _server.sendContent(_server.arg("linenr").c_str());
  _server.sendContent(F("'>zur&uuml;ck zum Listing</a></p>"));
  pageEnd();
}

bool insertMacroSetLine() {
  KinoVariant mName;
  KinoError err = KinoAPI::getMacroNameByIndex(_server.arg("macronr").toInt(), mName);
  if(err != KinoError::OK) return false;
  char* macroName = mName.s;
  if (strlen(macroName)==0) return false;
  char* fileEnding = strstr(macroName, ".macro");
  if (fileEnding) *fileEnding = '\0';
  
  _parseContainer.clear();
  int linenr = _server.arg("linenr").toInt();
  _parseContainer["cmd"].set(_server.arg("cmd").c_str());
  _parseContainer["dev"].set(_server.arg("dev").c_str());
  
  if ((_server.hasArg("commit")) && (strcmp(_server.arg("commit").c_str(),"on")==0)) _parseContainer["commit"].set(true);
  int n = _server.args();
  char key[64];
  char value[64];
  for (int i = 0; i < n; i++) {
    strlcpy(key,  _server.argName(i).c_str(), sizeof(key));
    strlcpy(value,_server.arg(i).c_str(),     sizeof(value));

    char incChk[5];
    strlcpy(incChk, key, sizeof(incChk));
    if ( (strcmp(incChk,"inc_")==0) && (strcmp(value,"on")==0) ) {
      char* newKey = key+4; // schneidet "inc_" vorne ab
      char chkVal[64];
      snprintf(chkVal, sizeof(chkVal), "val_%s", newKey);
      KinoVariant newVal = KinoVariant::fromString("off");  // Fallback für checkboxen, die wegen "off" nicht mitgesendet wurden
      if (_server.hasArg(chkVal)) newVal.setString(_server.arg(chkVal).c_str());
      KinoVariant curVal;
      KinoError err = KinoAPI::getProperty(_server.arg("dev").c_str(), newKey, curVal);
      switch(curVal.type) {
        case KinoVariant::BOOL: {
          _parseContainer["val"][newKey].set(newVal.asBool());
          break;
        }
        case KinoVariant::INT: {
          _parseContainer["val"][newKey].set(newVal.asInt());
          break;
        }
        case KinoVariant::FLOAT: {
          _parseContainer["val"][newKey].set(newVal.asFloat());
          break;
        }
        default: {  // STRING oder RGB_COLOR
          _parseContainer["val"][newKey].set((char*)newVal.c_str());
          break;
        }
      }
    }
  }

  char out[256];
  serializeJson(_parseContainer, out);
  return KinoAPI::addMacroCommand(macroName, linenr, out);
}

bool insertMacroDelayLine() {
  KinoVariant mName;
  KinoError err = KinoAPI::getMacroNameByIndex(_server.arg("macronr").toInt(), mName);
  if (err != KinoError::OK) return false;
  char* macroName = mName.s;
  if (strlen(macroName)==0) return false;
  char* fileEnding = strstr(macroName, ".macro");
  if (fileEnding) *fileEnding = '\0';
  
  _parseContainer.clear();
  int linenr = _server.arg("linenr").toInt();
  _parseContainer["cmd"].set(_server.arg("cmd").c_str());
  _parseContainer["seconds"].set(_server.arg("seconds").toInt());
  char out[256];
  serializeJson(_parseContainer, out);
  return KinoAPI::addMacroCommand(macroName, linenr, out);
}

void showMacro(const char* macroName) {
  _settings.clear();
  char tmpLine[256];
  char mName[32]; 
  strncpy(mName, macroName, sizeof(mName) - 1);
  mName[sizeof(mName) - 1] = '\0';
  char* fileEnding = strstr(mName, ".macro");
  if (fileEnding) *fileEnding = '\0'; 
  
  size_t lineCount = KinoAPI::getMacroLineCount(mName);

  char indexBuffer[5];
  KinoError err;
  for (int i=0; i < lineCount; i++) {
    _lineSettings.clear();
    itoa((i+1), indexBuffer, 10);
    
    err = KinoAPI::getMacroLineByIndex(mName, i, tmpLine, sizeof(tmpLine));
    if (err != KinoError::OK) {
      Serial.println(kinoErrorToString(err));
      break;
    }
    showLineAddButton(i);
    _server.sendContent(F("<div class='card' id='line_"));
    _server.sendContent(indexBuffer);
    _server.sendContent(F("'><form method='POST' action='/updateMacro'>"));
    _server.sendContent(F("<h2><input type='hidden' name='linenr' value='"));
    _server.sendContent(indexBuffer);
    _server.sendContent(F("'>"));
    _server.sendContent(F("<input type='hidden' name='macronr' value='"));
    _server.sendContent(_server.arg("m"));
    _server.sendContent(F("'>"));
    _server.sendContent(F("Zeile "));
    _server.sendContent(indexBuffer);
    _server.sendContent(F("<button class='deleteLine' name='delete' value='yesplease'>L&ouml;schen</button>"));
    _server.sendContent(F("</h2>"));
    showMacroLine(tmpLine, i);
    _server.sendContent(F("<br><input type='submit' value='Update'>"));
    _server.sendContent(F("</form></div>"));
  }
  showLineAddButton(lineCount);
  // Leere Makrozeile für den letzten Insert-Button
  _server.sendContent(F("<div id='line_"));
  itoa((lineCount+1),indexBuffer, 10);
  _server.sendContent(indexBuffer);
  _server.sendContent(F("'></div>"));
  // HTML Templates für das Javascript
  showNewLineTemplate();
  _server.sendContent(F("<template id='delay'>"));
  showDelayInput(1, 0);
  _server.sendContent(F("</template><template id='set'>"));
  showMacroDeviceSelect("",0);
  _server.sendContent(F("</template>"));
  _settings.clear();
}

void showLineAddButton(int beforeLineNr) {
  char buf[10];
  itoa((beforeLineNr+1), buf, 10);// +1, weil menschliche Nummerierung ab 1 benutzt wird!
  _server.sendContent(F("<div class='macroEditButtons'><button class='macroEditButton' onclick='addLineBefore("));
  _server.sendContent(buf);
  _server.sendContent(F(")'> neue Zeile einf&uuml;gen </button></div>"));
}

void showNewLineTemplate() {
  _server.sendContent(F("<template id='newLine'>"));
  _server.sendContent(F("<div class='card'><form method='POST' action='/insertMacroLine'>"));
  _server.sendContent(F("<h2><input type='hidden' class='linenr' name='linenr' value=''>"));
  _server.sendContent(F("<input type='hidden' name='macronr' value='"));
  _server.sendContent(_server.arg("m"));
  _server.sendContent(F("'>Neue Zeile"));
  _server.sendContent(F("<button class='remove' type='button'>L&ouml;schen</button>"));
  _server.sendContent(F("</h2>"));
  showMacroCommandSelect("",0);
  _server.sendContent(F("<div class='params'></div><br>"));
  _server.sendContent(F("<input type='submit' value='Insert'>"));
  _server.sendContent(F("</div></template>"));
}

//void webserver::showMacroLine(String& mLine, int lineIndex) {
void showMacroLine(const char* mLine, int lineIndex) {
  _lineSettings.clear();      // Container für die Werte von Properties und Parametern, die in dieser Makrozeile gesetzt werden
  _lineKeys.clear();          // Merkliste für schon angezeigte Func-Selects
  _parseContainer.clear();    // Container zum Parsen von mLine
  DeserializationError err = deserializeJson(_parseContainer, mLine);
  if (err) {
    _server.sendContent(F("Fehler in Command: "));
    _server.sendContent(err.c_str());
    return;
  }
  // build command select
  const char* cmd = _parseContainer["cmd"] | "";
  showMacroCommandSelect(cmd,lineIndex);
  if (strcmp(cmd,"delay")==0) {
    int seconds = _parseContainer["seconds"] | 1;
    showDelayInput(seconds, lineIndex);
    _server.sendContent(F("<div class='params'></div>"));
    return;
  }
  const char* dev = _parseContainer["dev"] | "";
  const bool commit = _parseContainer["commit"]|false;
  showMacroDeviceSelect(dev,lineIndex);
  JsonObject value = _parseContainer["val"].as<JsonObject>();
  _server.sendContent(F("<div class='params'>"));
  // store all settings made in this line for showing the value controls
  for (JsonPair jp : value) {
    const char* key = jp.key().c_str();
    _settings[dev][key] = jp.value();      // container for all macro settings
    _lineSettings[dev][key] = jp.value();  // container for this macro line´s settings
  }
  for (JsonPair jp : value) {
    const char* key = jp.key().c_str();
   
    // zeige ein select mit allen auswählbaren Einstellungen für das device
    char selectedFunc[64];
    getSelectedKeyForFuncSelect(dev, key, selectedFunc, sizeof(selectedFunc));
    if (! _lineKeys.containsKey(selectedFunc)) {
      showFuncSelect(dev, key, lineIndex);
      // zeige controls für alle Parameter der ausgewählten Einstellung
      _server.sendContent(F("<div class='funccontrols'>"));
      if (strcmp(selectedFunc, "Nuescht")!=0) showFuncControls(dev, selectedFunc, lineIndex);
      _server.sendContent(F("</div>"));
      _lineKeys[selectedFunc] = true;     // markiere diese Einstellung als erledigt für diese Zeile
    }
  }
  // some devices need a commit checkbox
  bool nc = false;
  KinoError e = KinoAPI::needsCommit(dev,nc);
  if (nc) {
    _server.sendContent(F("<br><input type='checkbox' name='commit'"));
    if (commit) _server.sendContent(F(" checked"));
    _server.sendContent(F("> Commit"));
  }
  _server.sendContent(F("</div>"));
}

// Helper function to get the value for a macro control
void getMacroValue(const char* dev, const char* key, const char* apikey, char* mVal, size_t mValLen) {
  // 1. Check in den aktuellen Zeilen-Einstellungen
  const char* firstJson = _lineSettings[dev][key] | "";
  if (firstJson[0] != '\0') { // Schneller als strlen
    strncpy(mVal, firstJson, mValLen - 1);
    mVal[mValLen - 1] = '\0';
    return;
  }
  // 2. Check in den allgemeinen Makro-Einstellungen
  const char* secndJson = _settings[dev][key] | "";
  if (secndJson[0] != '\0') {
    strncpy(mVal, secndJson, mValLen - 1);
    mVal[mValLen - 1] = '\0';
    return;
  }
  // 3. Fallback: Live-Wert von der API
  KinoVariant mv;
  if (KinoAPI::getProperty(dev, apikey, mv) == KinoError::OK) {
    strncpy(mVal, mv.c_str(), mValLen - 1);
    mVal[mValLen - 1] = '\0';
    return;
  }
  // 4. Wenn gar nichts gefunden wurde: Leerer String
  mVal[0] = '\0';
}

// Helper function to get the value for a macro control
void getMacroValue(const char* dev, const char* key, const char* apikey, KinoVariant& mVal) {
  // 1. Hole den Variant einmal ab
  JsonVariant v = _lineSettings[dev][key];
  if (!v.isNull()) { 
    //mVal = KinoVariant::fromJsonVariant(v);
    mVal.setFromJsonVariant(v);
    return;
  }
  // 2. Check in den allgemeinen Makro-Einstellungen
  v = _settings[dev][key];
  if (!v.isNull()) {
    //mVal = KinoVariant::fromJsonVariant(v);
    mVal.setFromJsonVariant(v);
    return;
  }
  // 3. Fallback: Live-Wert von der API
  if (KinoAPI::getProperty(dev, apikey, mVal) == KinoError::OK) {
    // mVal wurde direkt von getProperty befüllt!
    return;
  }
  // 4. Default
  //mVal = KinoVariant(); // Setzt den Typ auf NONE
  mVal.setNone();
}

void showDelayInput(int curSecs, int lineIndex) {
  char buf[10];
  itoa(curSecs, buf, 10);
  _server.sendContent(F("<div class='cmdparam'>"));
  _server.sendContent(F("<input type='number' name='seconds' value="));
  _server.sendContent(buf);
  _server.sendContent(F("> Sekunden"));
  _server.sendContent(F("</div>"));
}

void showMacroCommandSelect(const char* curCmd, int lineIndex) {
  _server.sendContent(F("<select name='cmd' class='cmdselect'>"));
  int cmdCount = KinoAPI::getMacroCommandCount();
  
  _server.sendContent(F("<option value=''>Kommando</option>"));
  char cmd[20];
  for (int i=0; i<cmdCount; i++) {
    KinoAPI::getMacroCommand(i, cmd, sizeof(cmd));
    _server.sendContent(F("<option value='"));
    _server.sendContent(cmd);
    _server.sendContent("'");
    if (strcmp(cmd, curCmd)==0) _server.sendContent(F(" selected='selected'"));
    _server.sendContent(F(">"));
    _server.sendContent(cmd);
    _server.sendContent(F("</option>"));
  }
  _server.sendContent("</select>");
}

void sendDeviceFuncSelect() {
  _server.setContentLength(CONTENT_LENGTH_UNKNOWN); // Stream-Modus
  _server.send(200, "text/html", "");
  showFuncSelect(_server.arg("dev").c_str(),"",0);
  _server.sendContent(F("<div class='funccontrols'></div>"));
  _server.sendContent("");
}

void sendFuncControls() {
  const char* dev = _server.arg("dev").c_str();
  const char* func = _server.arg("func").c_str();
  _server.setContentLength(CONTENT_LENGTH_UNKNOWN); // Stream-Modus
  _server.send(200, "text/html", "");
  showFuncControls(dev, func, 0);
  bool nc = false;
  bool commit = false;
  KinoError e = KinoAPI::needsCommit(dev, nc);
  if (nc) {
    _server.sendContent(F("<br><input type='checkbox' name='commit'"));
    if (commit) _server.sendContent(F(" checked"));
    _server.sendContent(F("> Commit"));
  }
  _server.sendContent("");
    
}

void showMacroDeviceSelect(const char* curDevice, int lineIndex) {
  size_t deviceCount = 0;
  KinoError e = KinoAPI::getDeviceCount(deviceCount);
  _server.sendContent(F("<select name='dev' class='cmdparam devselect'>"));
  _server.sendContent(F("<option value=''>Choose Device...</option>"));
  KinoVariant dev;
  for (int i=0; i < deviceCount; i++) {
    e = KinoAPI::getDeviceName(i, dev);
    if (e != KinoError::OK) break;
    _server.sendContent(F("<option value='"));
    _server.sendContent(dev.c_str());
    _server.sendContent("'");
    if (strcmp(dev.c_str(),curDevice)==0) _server.sendContent(F(" selected='selected'"));
    _server.sendContent(F(">"));
    _server.sendContent(dev.c_str());
    _server.sendContent(F("</option>"));
  }
  _server.sendContent(F("</select>"));
}

void getSelectedKeyForFuncSelect(const char* dev, const char* key, char* buf, size_t bufLen) {
  size_t propCount = 0;
  KinoError err = KinoAPI::getPropertyCount(dev, propCount);
  for (int pc=0; pc<propCount; pc++) {
    const KinoPropertyInfo* prop = nullptr;
    err = KinoAPI::getPropertyInfo(dev, pc, prop);
    bool hasLabel     = KinoAPI::hasValue(prop);
    bool isWritable   = KinoAPI::isWritable(prop);
    bool hasValue     = KinoAPI::hasValue(prop);
    bool hasQuery     = KinoAPI::hasQuery(prop);
    bool hasParams    = KinoAPI::hasParam(prop);
    bool isSelected   = (strcmp(key, prop->key)==0);
    if (isSelected) { strncpy(buf,prop->key,bufLen); buf[bufLen-1]='\0'; return; }

    if (isWritable && !hasParams) { // für schreibbare properties ohne Parameter
      if (isSelected) { strncpy(buf, prop->key, bufLen); buf[bufLen-1]='\0'; return; }
    }
    if (isWritable && hasParams) {
      // Dies ist ein schreibbarer Wert, der aber zusätzliche Parameter hat. Checke, ob einer der Parameter hier gefragt ist, und setzte dementsprechend
      // das "selected"
      // Bedenke dabei, dass die Parameter abhängig vom gesetzten Wert der Property sein können.
      char setValue[32];
      getMacroValue(dev, prop->key, prop->key, setValue, sizeof(setValue));
      snprintf(_paramPath, _paramPathLen, "%s/%s/param", prop->key, setValue);

      uint16_t nrOfParams = 0;
      err = KinoAPI::getQueryCount(dev, _paramPath, nrOfParams);

      KinoVariant tmp;
      for (int pc=0; pc < nrOfParams; pc++) {
        snprintf(_pathHelper, _pathHelperLen, "%s/%d", _paramPath, pc);
        err = KinoAPI::getProperty(dev, _pathHelper, tmp);
        if (!isSelected && (strcmp(tmp.c_str(), key)==0)) {
          strncpy(buf, prop->key, bufLen);
          buf[bufLen-1] = '\0';
          return;
        }
      }
    }
    if (!isWritable && hasValue) continue;  // In diesem Fall ist die Eigenschaft eine reine Info-Eigenschaft
    if (!isWritable && !hasValue && hasQuery && hasParams) {            
      // Es gibt eine Auflistung von Unter-Eigenschaften, die evtl setzbare Parameter haben
      // Checke jedes Element auf seine Parameter. Wenn es setzbare Parameter besitzt, zeige den Pfad des Elements im select an
      uint16_t optionCount = 0;
      err = KinoAPI::getQueryCount(dev, prop->key, optionCount);
      KinoVariant optionKey;
      KinoVariant optionLabel;
      for (int oc=0; oc < optionCount; oc++) {
        err = KinoAPI::getQuery(dev, prop->key, oc, optionKey);
        if ((!isSelected) && (strcmp(optionKey.c_str(), key)==0)) {
          strncpy(buf, optionKey.c_str(), bufLen);
          buf[bufLen-1] = '\0';
          return;
        }
        optionLabel = optionKey;
        if (hasLabel) {
          snprintf(_pathHelper, _pathHelperLen, "%s/%s/label", prop->key, optionKey.c_str());
          _pathHelper[_pathHelperLen-1] = '\0';
          err = KinoAPI::getProperty(dev, _pathHelper, optionLabel);
        }
        // key und label der potentiellen option sind jetzt bekannt. Jetzt checke, ob sie setzbare Parameter besitzt. Wenn ja, gib den Pfad zu dieser Option
        // im select aus
        snprintf(_pathHelper, _pathHelperLen, "%s/%s/param", prop->key, optionKey.c_str());
        _pathHelper[_pathHelperLen-1] = '\0';
        uint16_t paramCount = 0;
        err = KinoAPI::getQueryCount(dev, _pathHelper, paramCount);
        bool hasWritableParams = false;
        KinoVariant tmp;
        int getsetPathLen = 64; char getsetPath[getsetPathLen];
        
        for (int pc=0; pc < paramCount; pc++) {
          // Gehe alle verfügbaren Parameter für diese Option durch. Wir interessieren uns nur dafür, ob mindestens eine davon writable ist
          snprintf(_pathHelper, _pathHelperLen, "%s/%s/param/%d/access", prop->key, optionKey.c_str(), pc);
          _pathHelper[_pathHelperLen-1] = '\0';
          err = KinoAPI::getProperty(dev, _pathHelper, tmp);
          if (tmp.asInt() < 2) continue;
          // Der Parameter ist writable. Jetzt checke noch schnell, ob er vielleicht gerade selected ist
          hasWritableParams = true;
          snprintf(_pathHelper, _pathHelperLen, "%s/%s/param/%d", prop->key, optionKey.c_str(), pc);
          _pathHelper[_pathHelperLen-1] = '\0';
          err = KinoAPI::getProperty(dev, _pathHelper, tmp);
          if ((!isSelected) && (strcmp(tmp.c_str(), key)==0)) {
            snprintf(_pathHelper, _pathHelperLen, "%s/%s", prop->key, optionKey.c_str());
            _pathHelper[_pathHelperLen-1] = '\0';
            strncpy(buf, _pathHelper, bufLen);
            buf[bufLen-1] = '\0';
            return;
          }
        }
      }
    }
    if (!isWritable && !hasValue && !hasQuery && hasParams) {
      // Eine Gruppierung von direkten Parametern. Checke, ob mindestens eine davon schreibbar ist. Wenn ja, zeige die Option an
      // Wenn einer dieser Parameter gerade abgehandelt wird, setze auch selected
      bool hasWritableParams = false;
      snprintf(_paramPath, _paramPathLen, "%s/param", prop->key);
      _paramPath[_paramPathLen-1] = '\0';
      uint16_t nrOfParams = 0;
      err = KinoAPI::getQueryCount(dev, _paramPath, nrOfParams);
      
      KinoVariant tmp;
      int getsetPathLen = 64; char getsetPath[getsetPathLen];
      
      for (int i=0; i < nrOfParams; i++) {
        snprintf(_pathHelper, _pathHelperLen, "%s/%d", _paramPath, i);
        _pathHelper[_pathHelperLen-1] = '\0';
        err = KinoAPI::getProperty(dev, _pathHelper, tmp);
        strncpy(getsetPath, tmp.c_str(), getsetPathLen);
        getsetPath[getsetPathLen-1] = '\0';
        snprintf(_pathHelper, _pathHelperLen, "%s/%d/access", _paramPath, i);
        _pathHelper[_pathHelperLen-1] = '\0';
        err = KinoAPI::getProperty(dev, _pathHelper, tmp);
        if (tmp.asInt() < 2) continue;  // Dieser Parameter ist nicht writable, also im Moment uninteressant
        hasWritableParams = true;
        if (!isSelected && (strcmp(getsetPath, key)==0)) {
          strncpy(buf, prop->key, bufLen);
          buf[bufLen-1] = '\0';
          return;
        }
      }
    }
  }
  strncpy(buf, "Nuescht", bufLen - 1);
  buf[bufLen - 1] = '\0';
  return;
}

// Helperfunktion, die ein Select mit allen verfügbaren schreibbaren Pfaden zum angegebenen device zusammenstellt und den gegebenen key vor-auswählt
void showFuncSelect(const char* dev, const char* key, int lineIndex) {
  // hole alle verfügbaren Properties für das device
  size_t propCount = 0;
  KinoError err = KinoAPI::getPropertyCount(dev,propCount);
  _server.sendContent(F("<select name='func' class='funcselect'>"));
  _server.sendContent(F("<option value="">Choose function</option>"));
  const KinoPropertyInfo* prop = nullptr;
  for (int pc=0; pc<propCount; pc++) {
    //const KinoPropertyInfo* prop = nullptr;
    err = KinoAPI::getPropertyInfo(dev, pc, prop);
    bool hasLabel     = KinoAPI::hasValue(prop);
    bool isWritable   = KinoAPI::isWritable(prop);
    bool hasValue     = KinoAPI::hasValue(prop);
    bool hasQuery     = KinoAPI::hasQuery(prop);
    bool hasParams    = KinoAPI::hasParam(prop);
    bool isSelected   = (strcmp(key, prop->key)==0);
    if (isWritable && !hasParams) { // für schreibbare properties ohne Parameter
      _server.sendContent(F("<option value='"));
      _server.sendContent(prop->key);
      _server.sendContent(F("'"));
      if (isSelected) _server.sendContent(F(" selected='selected'"));
      _server.sendContent(F(">"));
      _server.sendContent(prop->label);
      _server.sendContent(F("</option>"));
      continue;
    }
    if (isWritable && hasParams) {
      // Dies ist ein schreibbarer Wert, der aber zusätzliche Parameter hat. Checke, ob einer der Parameter hier gefragt ist, und setzte dementsprechend
      // das "selected"
      // Bedenke dabei, dass die Parameter abhängig vom gesetzten Wert der Property sein können.
      //Serial.print(prop->key); Serial.println(" is writable and has Params. Checking them for selectedness");
      KinoVariant value;
      char setValue[32];
      getMacroValue(dev, prop->key, prop->key, setValue, 32);
      snprintf(_paramPath, _paramPathLen, "%s/%s/param", prop->key, setValue);
      _paramPath[_paramPathLen-1] = '\0';
      
      uint16_t nrOfParams = 0;
      err = KinoAPI::getQueryCount(dev, _paramPath, nrOfParams);

      KinoVariant tmp;
      int getsetPathLen = 64; char getsetPath[getsetPathLen];
      
      for (int pc=0; pc < nrOfParams; pc++) {
        snprintf(_pathHelper, _pathHelperLen, "%s/%d", _paramPath, pc);
        _pathHelper[_pathHelperLen-1] = '\0';
        err = KinoAPI::getProperty(dev, _pathHelper, tmp);
        if (!isSelected && (strcmp(tmp.c_str(), key)==0)) {
          isSelected = true;
        }
      }
      _server.sendContent(F("<option value='"));
      _server.sendContent(prop->key);
      _server.sendContent("'");
      if (isSelected) _server.sendContent(F(" selected='selected'"));
      _server.sendContent(">");
      _server.sendContent(prop->label);
      _server.sendContent(F("</option>"));
      isSelected = false;
      continue;
    }
    if (!isWritable && hasValue) continue;  // In diesem Fall ist die Eigenschaft eine reine Info-Eigenschaft
    if (!isWritable && !hasValue && hasQuery && hasParams) {            
      // Es gibt eine Auflistung von Unter-Eigenschaften, die evtl setzbare Parameter haben
      // Checke jedes Element auf seine Parameter. Wenn es setzbare Parameter besitzt, zeige den Pfad des Elements im select an
      uint16_t optionCount = 0;
      err = KinoAPI::getQueryCount(dev, prop->key, optionCount);

      KinoVariant optionKey;
      KinoVariant optionLabel;
      KinoVariant tmp;
      for (int oc=0; oc < optionCount; oc++) {
        err = KinoAPI::getQuery(dev, prop->key, oc, optionKey);
        if ((!isSelected) && (strcmp(optionKey.c_str(), key)==0)) {
          isSelected = true;
        }
        optionLabel = optionKey;
        if (hasLabel) {
          snprintf(_pathHelper, _pathHelperLen, "%s/%s/label", prop->key, optionKey.c_str());
          _pathHelper[_pathHelperLen-1] = '\0';
          err = KinoAPI::getProperty(dev, _pathHelper, optionLabel);
        }
        // key und label der potentiellen option sind jetzt bekannt. Jetzt checke, ob sie setzbare Parameter besitzt. Wenn ja, gib den Pfad zu dieser Option
        // im select aus
        snprintf(_paramPath, _paramPathLen, "%s/%s/param", prop->key, optionKey.c_str());
        _paramPath[_paramPathLen-1] = '\0';
        uint16_t paramCount = 0;
        err = KinoAPI::getQueryCount(dev, _paramPath, paramCount);
        bool hasWritableParams = false;
        for (int pc=0; pc < paramCount; pc++) {
          // Gehe alle verfügbaren Parameter für diese Option durch. Wir interessieren uns nur dafür, ob mindestens eine davon writable ist
          snprintf(_pathHelper, _pathHelperLen, "%s/%d/access", _paramPath, pc);
          _pathHelper[_pathHelperLen-1] = '\0';
          err = KinoAPI::getProperty(dev, _pathHelper, tmp);
          if (tmp.asInt() < 2) continue;
          // Der Parameter ist writable. Jetzt checke noch schnell, ob er vielleicht gerade selected ist
          hasWritableParams = true;
          KinoVariant paramGetsetPath;
          snprintf(_pathHelper, _pathHelperLen, "%s/%d", _paramPath, pc);
          _pathHelper[_pathHelperLen-1] = '\0';
          err = KinoAPI::getProperty(dev, _pathHelper, tmp);
          if ((!isSelected) && (strcmp(tmp.c_str(), key)==0)) {
            isSelected = true;
          }
        }
        // Jetzt: gibt es wenigstens einen schreibbaren Parameter für diese option? Dann zeige sie an und checke die nächste option
        if (hasWritableParams) {
          _server.sendContent(F("<option value='"));
          _server.sendContent(prop->key);
          _server.sendContent(F("/"));
          _server.sendContent(optionKey.c_str());
          _server.sendContent("'");
          if (isSelected) _server.sendContent(F(" selected='selected'"));
          _server.sendContent(F(">"));
          _server.sendContent(prop->label);
          _server.sendContent(F(" / ")); 
          _server.sendContent(optionLabel.c_str());
          _server.sendContent(F("</option>"));
          isSelected = false;
        }
      }
    }
    if (!isWritable && !hasValue && !hasQuery && hasParams) {
      // Eine Gruppierung von direkten Parametern. Checke, ob mindestens eine davon schreibbar ist. Wenn ja, zeige die Option an
      // Wenn einer dieser Parameter gerade abgehandelt wird, setze auch selected
      bool hasWritableParams = false;
      snprintf(_paramPath, _paramPathLen, "%s/param", prop->key);
      _paramPath[_paramPathLen-1] = '\0';
      
      uint16_t nrOfParams = 0;
      err = KinoAPI::getQueryCount(dev, _paramPath, nrOfParams);

      KinoVariant tmp;
      int paramGetsetpathLen = 64; char paramGetsetpath[paramGetsetpathLen];
      
      for (int i=0; i < nrOfParams; i++) {
        //String paramSubPath = paramPath + String("/") + String(i);
        snprintf(_pathHelper, _pathHelperLen, "%s/%d", _paramPath, i);
        _pathHelper[_pathHelperLen-1] = '\0';
        err = KinoAPI::getProperty(dev, _pathHelper, tmp);
        strncpy(paramGetsetpath, tmp.c_str(), paramGetsetpathLen);
        paramGetsetpath[paramGetsetpathLen-1] = '\0';
        
        snprintf(_pathHelper, _pathHelperLen, "%s/%d/access", _paramPath, i);
        _pathHelper[_pathHelperLen-1] = '\0';
        err = KinoAPI::getProperty(dev, _pathHelper, tmp);
        if (tmp.asInt() < 2) continue;  // Dieser Parameter ist nicht writable, also im Moment uninteressant
        hasWritableParams = true;
        if (!isSelected && (strcmp(paramGetsetpath, key)==0)) isSelected = true;
      }
      if (hasWritableParams) {
        _server.sendContent(F("<option value='"));
        _server.sendContent(prop->key);
        _server.sendContent(F("'"));
        if (isSelected) _server.sendContent(F(" selected='selected'"));
        _server.sendContent(F(">"));
        _server.sendContent(prop->label);
        _server.sendContent(F("</option>"));
        isSelected = false;
      }
    }
  }
  _server.sendContent(F("</select>"));
}

// Helperfunktion, um festzustellen, ob ein Parameter, identifiziert durch getsetPath, eine reguläre Property von device ist
bool isProperty(const char* deviceName, const char* getsetPath) {
  const KinoPropertyInfo* prop = nullptr;
  KinoError err = KinoAPI::getPropertyInfoByName(deviceName, getsetPath, prop);
  return (err == KinoError::OK);
}

// Helperfunktion, um ein Control für eine Property innerhalb der Parameterliste darzustellen
void showParamPropertyControl(const char* deviceName, const char* getsetPath) {
  const KinoPropertyInfo* prop = nullptr;
  KinoError err = KinoAPI::getPropertyInfoByName(deviceName, getsetPath, prop);
  bool isWritable = KinoAPI::isWritable(prop);
  if (!isWritable) return;
  bool hasValue = KinoAPI::hasValue(prop);
  KinoVariant value;
  if (hasValue) KinoAPI::getProperty(deviceName, getsetPath, value);    // sollte eigentlich immer der Fall sein, ansonsten wäre das ein Fall für eine Rekursion...
  
  KinoVariant mValue;
  getMacroValue(deviceName, getsetPath, getsetPath, mValue);
  
  bool hasQuery = KinoAPI::hasQuery(prop);
  bool hasLabel = KinoAPI::hasLabel(prop);
  if (hasQuery) {
    startMacroSelect(deviceName, getsetPath, prop->label);
    uint16_t optionCount = 0;
    err = KinoAPI::getQueryCount(deviceName, getsetPath, optionCount);
    
    KinoVariant optionValue;
    KinoVariant optionLabel;
    int pathLen = 128; char path[pathLen];
    for (int i=0; i < optionCount; i++) {
      err = KinoAPI::getQuery(deviceName, getsetPath, i, optionValue);
      optionLabel = optionValue;
      snprintf(path, pathLen, "%s/%d/label", getsetPath, i);
      path[pathLen-1] = '\0';
      if (hasLabel) err = KinoAPI::getProperty(deviceName, path, optionLabel);
      bool isSelected = (strcmp(mValue.c_str(), optionValue.c_str())==0);
      _server.sendContent(F("<option value='"));
      _server.sendContent(optionValue.c_str());
      _server.sendContent(F("'"));
      if (isSelected) _server.sendContent(F(" selected='selected'"));
      _server.sendContent(F(">"));
      _server.sendContent(optionLabel.c_str());
      _server.sendContent(F("</option>"));
    }
    endMacroSelect();
  } else {
    switch (value.type) {
      case KinoVariant::BOOL: {
        macroToggleButton(deviceName, getsetPath, prop->label, mValue.asBool());
        break;
      }
      case KinoVariant::INT: {
        int minval = prop->minValue.value_or(0);
        int maxval = prop->maxValue.value_or(100);
        int valuestep = prop->valueStp.value_or(1);
        macroSlider(deviceName, getsetPath, prop->label, mValue.asInt(), minval, maxval, valuestep);
        break;
      }
      case KinoVariant::RGB_COLOR: {
        macroColorPicker(deviceName, getsetPath, prop->label, mValue.c_str());
        break;
      }
      default: {
        infoText(deviceName, getsetPath, prop->label, "unknown");
        break;
      }
    }
  }
}

void showFuncControls(const char* dev, const char* key, int lineIndex) {
  // key ist entweder eine Property oder der Pfad einer Property zu einer Option
  // Gehe alle Properties zu dem Device durch und checke, womit wir es genau zu tun haben
  bool isPath = (strchr(key, '/') != nullptr);

  if (!isPath) {
    // einfacher Fall: Es handelt sich um eine Property
    const KinoPropertyInfo* pi = nullptr;
    KinoError err = KinoAPI::getPropertyInfoByName(dev, key, pi);
    if (!pi) {
      //Serial.print("FEHLER!!! Konnte PropertyInfo "); Serial.print(key); Serial.println(" nicht lesen!");
      return;
    }
    bool hasValue = KinoAPI::hasValue(pi);
    bool hasQuery = KinoAPI::hasQuery(pi);
    bool hasParam = KinoAPI::hasParam(pi);
    if (hasValue && hasQuery) {
      // baue ein Select zusammen mit allen Optionen. Falls ein Wert in _settings dazu existiert, wähle ihn aus
      // Falls in _settings KEIN Wert existiert, wähle den aktuellen Wert aus
      int valueLen = 32;
      char value[valueLen];
      getMacroValue(dev, key, pi->key, value, valueLen);
      
      startMacroSelect(dev, pi->key, pi->label);
      
      uint16_t nrOfOptions = 0;
      err = KinoAPI::getQueryCount(dev, pi->key, nrOfOptions);

      KinoVariant tmp;
      int optionValueLen = 32; char optionValue[optionValueLen];
      int optionLabelLen = 64; char optionLabel[optionLabelLen];
      for (int oc=0; oc < nrOfOptions; oc++) {
        err = KinoAPI::getQuery(dev, pi->key, oc, tmp);
        strncpy(optionValue, tmp.c_str(), optionValueLen);
        optionValue[optionValueLen-1] = '\0';
        snprintf(_pathHelper, _pathHelperLen, "%s/%s/label", pi->key, optionValue);
        err = KinoAPI::getProperty(dev, _pathHelper, tmp);
        
        if (tmp.type == KinoVariant::NONE) {
          strncpy(optionLabel, optionValue, optionLabelLen); 
        } else {
          strncpy(optionLabel, tmp.c_str(), optionLabelLen);
        }
        optionLabel[optionLabelLen-1] = '\0';
        
        bool selected = (strcmp(optionValue, value)==0);
        
        _server.sendContent(F("<option value='"));
        _server.sendContent(optionValue);
        _server.sendContent(F("'"));
        if (selected) _server.sendContent(F(" selected='selected'"));
        _server.sendContent(">");
        _server.sendContent(optionLabel);
        _server.sendContent(F("</option>"));
      }
      endMacroSelect();
      if (hasParam) {
        uint16_t paramCount = 0;
        snprintf(_pathHelper, _pathHelperLen, "%s/%s/param", pi->key, value);
        _pathHelper[_pathHelperLen-1] = '\0';
        err = KinoAPI::getQueryCount(dev, _pathHelper, paramCount);

        KinoVariant tmp;
        int getsetPathLen = 64; char getsetPath[getsetPathLen];
        int labelLen = 64; char label[labelLen];
        KinoVariant paramCurrentValue;
        KinoVariant pv;
        for (int i = 0; i < paramCount; i++) {
          snprintf(_pathHelper, _pathHelperLen, "%s/%s/param/%d", pi->key, value, i);
          _pathHelper[_pathHelperLen-1] = '\0';
          err = KinoAPI::getProperty(dev, _pathHelper, tmp);
          strncpy(getsetPath, tmp.c_str(), getsetPathLen);
          getsetPath[getsetPathLen-1] = '\0';
          if (isProperty(dev, getsetPath)) {
            showParamPropertyControl(dev, getsetPath);
            continue;
          }
          err = KinoAPI::getProperty(dev, getsetPath, paramCurrentValue); // brauchen den type, und den Value als Fallback
          snprintf(_pathHelper, _pathHelperLen, "%s/%s/param/%d/access", pi->key, value, i);
          _pathHelper[_pathHelperLen-1] = '\0';
          err = KinoAPI::getProperty(dev, _pathHelper, tmp);
          if (tmp.asInt() < 2) continue;  // uns interessieren hier im Moment nur schreibbare Parameter
          
          getMacroValue(dev, getsetPath, getsetPath, pv);
          
          snprintf(_pathHelper, _pathHelperLen, "%s/%s/param/%d/label", pi->key, value, i);
          _pathHelper[_pathHelperLen-1] = '\0';
          err = KinoAPI::getProperty(dev, _pathHelper, tmp);
          strncpy(label, tmp.c_str(), labelLen);
          label[labelLen-1] = '\0';
          switch (paramCurrentValue.type) {
            case KinoVariant::BOOL: {
              macroToggleButton(dev, getsetPath, label, pv.asBool());
              break;
            }
            case KinoVariant::INT: {
              int minval, maxval, valuestep;
              snprintf(_pathHelper, _pathHelperLen, "%s/%s/param/%d/minvalue", pi->key, value, i);
              _pathHelper[_pathHelperLen-1] = '\0';
              err = KinoAPI::getProperty(dev, _pathHelper, tmp);
              minval = tmp.asInt();
              snprintf(_pathHelper, _pathHelperLen, "%s/%s/param/%d/maxvalue", pi->key, value, i);
              _pathHelper[_pathHelperLen-1] = '\0';
              err = KinoAPI::getProperty(dev, _pathHelper, tmp);
              maxval = tmp.asInt();
              snprintf(_pathHelper, _pathHelperLen, "%s/%s/param/%d/valuestep", pi->key, value, i);
              _pathHelper[_pathHelperLen-1] = '\0';
              err = KinoAPI::getProperty(dev, _pathHelper, tmp);
              valuestep = tmp.asInt();
              macroSlider(dev, getsetPath, label, pv.asInt(), minval, maxval, valuestep);
              break;
            }
            case KinoVariant::RGB_COLOR: {
              macroColorPicker(dev, getsetPath, label, pv.c_str());
              break;
            }
            default: {
              infoText(dev, getsetPath, label, "unknown");
              break;
            }
          } // one parameter done
        } // all parameters done
      } // end of if (hasParam)
    } // end of if (hasValue && hasQuery)
    if (hasValue && !hasQuery) {
      KinoVariant propValue;
      KinoError err = KinoAPI::getProperty(dev, key, propValue);

      KinoVariant cv;
      getMacroValue(dev, key, key, cv);
      switch (propValue.type) {
        case KinoVariant::BOOL: {
          macroToggleButton(dev, pi->key, pi->label, cv.asBool());
          break;
        }
        case KinoVariant::INT: {
          int minvalue = pi->minValue.value_or(0);
          int maxvalue = pi->maxValue.value_or(255);
          int valuestp = pi->valueStp.value_or(1);
          macroSlider(dev, pi->key, pi->label, cv.asInt(), minvalue, maxvalue, valuestp);
          break;
        }
        case KinoVariant::RGB_COLOR: {
          macroColorPicker(dev, pi->key, pi->label, cv.c_str());
          break;
        }
        default: {
          infoText(dev, pi->key, pi->label, "unknown");
          break;
        }
      }
      if (hasParam) {
        uint16_t paramCount = 0;
        snprintf(_pathHelper, _pathHelperLen, "%s/%s/param", pi->key, cv.c_str());
        _pathHelper[_pathHelperLen-1] = '\0';
        err = KinoAPI::getQueryCount(dev, _pathHelper, paramCount);
        
        KinoVariant tmp;
        int getsetPathLen = 64; char getsetPath[getsetPathLen];
        int labelLen = 64; char label[labelLen];
        KinoVariant paramCurrentValue;
        KinoVariant pv;
        
        for (int i = 0; i < paramCount; i++) {
          snprintf(_pathHelper, _pathHelperLen, "%s/%s/param/%d", pi->key, cv.c_str(), i);
          _pathHelper[_pathHelperLen-1] = '\0';
          err = KinoAPI::getProperty(dev, _pathHelper, tmp);
          strncpy(getsetPath, tmp.c_str(), getsetPathLen);
          getsetPath[getsetPathLen-1] = '\0';
          err = KinoAPI::getProperty(dev, getsetPath, paramCurrentValue); // brauchen den type, und den Value als Fallback
          
          snprintf(_pathHelper, _pathHelperLen, "%s/%s/param/%d/access", pi->key, cv.c_str(), i);
          _pathHelper[_pathHelperLen-1] = '\0';
          err = KinoAPI::getProperty(dev, _pathHelper, tmp);
          if (tmp.asInt() < 2) continue;  // uns interessieren hier im Moment nur schreibbare Parameter
          
          getMacroValue(dev, getsetPath, getsetPath, pv);
          
          snprintf(_pathHelper, _pathHelperLen, "%s/%s/param/%d/label", pi->key, cv.c_str(), i);
          _pathHelper[_pathHelperLen-1] = '\0';
          err = KinoAPI::getProperty(dev, _pathHelper, tmp);
          strncpy(label, tmp.c_str(), labelLen);
          label[labelLen-1] = '\0';
          switch (paramCurrentValue.type) {
            case KinoVariant::BOOL: {
              macroToggleButton(dev, getsetPath, label, pv.asBool());
              break;
            }
            case KinoVariant::INT: {
              int minval, maxval, valuestep;
              snprintf(_pathHelper, _pathHelperLen, "%s/%s/param/%d/minvalue", pi->key, cv.c_str(), i);
              _pathHelper[_pathHelperLen-1] = '\0';
              err = KinoAPI::getProperty(dev, _pathHelper, tmp);
              minval = tmp.asInt();
              snprintf(_pathHelper, _pathHelperLen, "%s/%s/param/%d/maxvalue", pi->key, cv.c_str(), i);
              _pathHelper[_pathHelperLen-1] = '\0';
              err = KinoAPI::getProperty(dev, _pathHelper, tmp);
              maxval = tmp.asInt();
              snprintf(_pathHelper, _pathHelperLen, "%s/%s/param/%d/valuestep", pi->key, cv.c_str(), i);
              _pathHelper[_pathHelperLen-1] = '\0';
              err = KinoAPI::getProperty(dev, _pathHelper, tmp);
              valuestep = tmp.asInt();
              macroSlider(dev, getsetPath, label, pv.asInt(), minval, maxval, valuestep);
              break;
            }
            case KinoVariant::RGB_COLOR: {
              macroColorPicker(dev, getsetPath, label, pv.c_str());
              break;
            }
            default: {
              infoText(dev, getsetPath, label, "unknown");
              break;
            }
          } // one parameter done
        } // all parameters done
      } // end of if(hasParam)
    } // end of if(hasValue && !hasQuery)
    if (!hasValue && hasParam) {
      uint16_t paramCount = 0;
      snprintf(_pathHelper, _pathHelperLen, "%s/param", pi->key);
      _pathHelper[_pathHelperLen-1] = '\0';
      err = KinoAPI::getQueryCount(dev, _pathHelper, paramCount);
      
      KinoVariant tmp;    // zum Auslesen benötigter Werte aus der API
      int getsetPathLen = 128; char getsetPath[getsetPathLen];  // Puffer für verschiedene getsetPath
      KinoVariant paramCurrentValue;  // der jeweilige aktuelle Wert des Parameters aus der API
      KinoVariant pv;     // der Anzeigewert für den Parameter
      int labelLen = 64; char label[labelLen];  // Puffer für das Label des Parameters
      for (int i = 0; i < paramCount; i++) {
        snprintf(_pathHelper, _pathHelperLen, "%s/param/%d", pi->key, i);
        _pathHelper[_pathHelperLen-1] = '\0';
        err = KinoAPI::getProperty(dev, _pathHelper, tmp);
        strncpy(getsetPath, tmp.c_str(), getsetPathLen);
        getsetPath[getsetPathLen-1] = '\0';
        //Serial.print("found parameter "); Serial.println(getsetPath.toString());
        if (isProperty(dev, getsetPath)) {
          showParamPropertyControl(dev, getsetPath);
          continue;
        }
        err = KinoAPI::getProperty(dev, getsetPath, paramCurrentValue); // brauchen den type
        snprintf(_pathHelper, _pathHelperLen, "%s/param/%d/access", pi->key, i);
        _pathHelper[_pathHelperLen-1] = '\0';
        err = KinoAPI::getProperty(dev, _pathHelper, tmp);
        if (tmp.asInt() < 2) continue;  // uns interessieren hier im Moment nur schreibbare Parameter
        getMacroValue(dev, getsetPath, getsetPath, pv);
        snprintf(_pathHelper, _pathHelperLen, "%s/param/%d/label", pi->key, i);
        _pathHelper[_pathHelperLen-1] = '\0';
        err = KinoAPI::getProperty(dev, _pathHelper, tmp);
        strncpy(label, tmp.c_str(), labelLen);
        label[labelLen-1] = '\0';
        switch (paramCurrentValue.type) {
          case KinoVariant::BOOL: {
            macroToggleButton(dev, getsetPath, label, pv.asBool());
            break;
          }
          case KinoVariant::INT: {
            int minval, maxval, valuestep;
            snprintf(_pathHelper, _pathHelperLen, "%s/param/%d/minvalue", pi->key, i);
            _pathHelper[_pathHelperLen-1] = '\0';
            err = KinoAPI::getProperty(dev, _pathHelper, tmp);
            minval = tmp.asInt();
            snprintf(_pathHelper, _pathHelperLen, "%s/param/%d/maxvalue", pi->key, i);
            _pathHelper[_pathHelperLen-1] = '\0';
            err = KinoAPI::getProperty(dev, _pathHelper, tmp);
            maxval = tmp.asInt();
            snprintf(_pathHelper, _pathHelperLen, "%s/param/%d/valuestep", pi->key, i);
            _pathHelper[_pathHelperLen-1] = '\0';
            err = KinoAPI::getProperty(dev, _pathHelper, tmp);
            valuestep = tmp.asInt();
            macroSlider(dev, getsetPath, label, pv.asInt(), minval, maxval, valuestep);
            break;
          }
          case KinoVariant::RGB_COLOR: {
            macroColorPicker(dev, getsetPath, label, pv.c_str());
            break;
          }
          default: {
            infoText(dev, getsetPath, label, "unknown");
            break;
          }
        } // one parameter done
      } // all parameters done
    } // end of if (!hasValue && hasParam)
  }
  if (isPath) {
    // Das kann nur sein, wenn die ursprüngliche Property keinen Wert hat (wenn es sich also um eine reine Ansammlung von Parametern handelt)
    uint16_t paramCount = 0;
    snprintf(_pathHelper, _pathHelperLen, "%s/param", key);
    _pathHelper[_pathHelperLen-1] = '\0';
    KinoError err = KinoAPI::getQueryCount(dev, _pathHelper, paramCount);

    KinoVariant tmp;
    int getsetPathLen = 64; char getsetPath[getsetPathLen];
    int labelLen = 64; char label[labelLen];
    KinoVariant pCurrentValue;
    KinoVariant pMacroValue;
    for(int i=0; i < paramCount; i++) {
      snprintf(_pathHelper, _pathHelperLen, "%s/param/%d", key, i);
      _pathHelper[_pathHelperLen-1] = '\0';
      err = KinoAPI::getProperty(dev, _pathHelper, tmp);
      strncpy(getsetPath, tmp.c_str(), getsetPathLen);
      getsetPath[getsetPathLen-1] = '\0';
      
      snprintf(_pathHelper, _pathHelperLen, "%s/param/%d/access", key, i);
      _pathHelper[_pathHelperLen-1] = '\0';
      err = KinoAPI::getProperty(dev, _pathHelper, tmp);
      if (tmp.asInt() < 2) continue;

      snprintf(_pathHelper, _pathHelperLen, "%s/param/%d/label", key, i);
      _pathHelper[_pathHelperLen-1] = '\0';
      err = KinoAPI::getProperty(dev, _pathHelper, tmp);
      if (tmp.type == KinoVariant::NONE) {
        strncpy(label, getsetPath, labelLen);
      } else {
        strncpy(label, tmp.c_str(), labelLen);
      }
      label[labelLen-1] = '\0';

      err = KinoAPI::getProperty(dev, getsetPath, pCurrentValue);
      getMacroValue(dev, getsetPath, getsetPath, pMacroValue);
      
      switch (pCurrentValue.type) {
        case KinoVariant::BOOL: {
          macroToggleButton(dev, getsetPath, label, pMacroValue.asBool());
          break;
        }
        case KinoVariant::INT: {
          int minvalue, maxvalue, valuestep;
          snprintf(_pathHelper, _pathHelperLen, "%s/param/%d/minvalue", key, i);
          _pathHelper[_pathHelperLen-1] = '\0';
          err = KinoAPI::getProperty(dev, _pathHelper, tmp);
          minvalue = tmp.asInt();
          snprintf(_pathHelper, _pathHelperLen, "%s/param/%d/maxvalue", key, i);
          _pathHelper[_pathHelperLen-1] = '\0';
          err = KinoAPI::getProperty(dev, _pathHelper, tmp);
          maxvalue = tmp.asInt();
          snprintf(_pathHelper, _pathHelperLen, "%s/param/%d/valuestep", key, i);
          _pathHelper[_pathHelperLen-1] = '\0';
          err = KinoAPI::getProperty(dev, _pathHelper, tmp);
          valuestep = tmp.asInt();
          macroSlider(dev, getsetPath, label, pMacroValue.asInt(), minvalue, maxvalue, valuestep);
          break;
        }
        case KinoVariant::RGB_COLOR: {
          macroColorPicker(dev, getsetPath, label, pMacroValue.c_str());
          break;
        }
        default: {
          infoText(dev, getsetPath, label, "unknown");
          break;
        }
      } // end of one parameter
    } // end of all parameters
  } // end of if(isPath)

}

void handleDevice(const char* deviceName) {  
  pageStart(deviceName);
  
  size_t propCount;
  KinoError err = KinoAPI::getPropertyCount(deviceName, propCount);
  const KinoPropertyInfo* pi = nullptr;
  for (int i=0; i<propCount; i++) {
    err = KinoAPI::getPropertyInfo(deviceName,i,pi);
    bool hasValue     = KinoAPI::hasValue(pi);
    bool hasLabel     = KinoAPI::hasLabel(pi);
    bool isWritable   = KinoAPI::isWritable(pi);
    bool hasQuery     = KinoAPI::hasQuery(pi);
    bool isInternal   = KinoAPI::isInternal(pi);
    bool isStatus     = KinoAPI::isStatus(pi);
    bool hasParams    = KinoAPI::hasParam(pi);
    bool isOptional   = KinoAPI::isOptional(pi);

    if (isOptional && (!KinoAPI::isAvailable(deviceName, pi->key))) continue;

    KinoVariant propValue;
    /*if (hasValue) {
      err = KinoAPI::getProperty(deviceName, pi->key, propValue);
    }*/
    if ((hasValue)&&(isWritable)&&(!hasQuery)) {  // einfacher Wert mit Schreibrechten
      buildSimpleControl(deviceName, pi);
    } else if ((hasValue)&&(isWritable)&&(hasQuery)) { // select mit aktuellem Wert
      buildSelect(deviceName, pi, hasParams);
    } else if ((!hasValue) && (hasQuery)) { // komplexer Aufbau: Eine Liste von Options mit evtl jeweils eigenen Parametern
      buildComplexList(deviceName, pi, hasParams);
    } else if ((hasValue)&&(!isWritable)&&(!hasQuery)) {  // einfache read-only info-Werte
      if (isInternal) { continue; }
      if (!isStatus) { continue; }
      KinoVariant value;
      err = KinoAPI::getProperty(deviceName, pi->key, value);
      infoText(deviceName, pi->key, pi->label, value.c_str());
    } else if ((!hasValue) && (hasParams)) {  // die Property ist eine Zusammenfassung von Parametern
      groupCardStart(pi->key, pi->label);
      snprintf(_pathHelper, _pathHelperLen, "%s/param", pi->key);
      showParameters(deviceName, _pathHelper);
      groupCardEnd();
    }
    
    
    else {
      _server.sendContent(F("<div class='card'><h2>"));
      _server.sendContent(pi->label);
      _server.sendContent(F("</h2>"));
      _server.sendContent(F("</div>"));
    }

  }

  pageEnd();
}

// builds a simple control for the given KinoPropertyInfo, depending on its value type
void buildSimpleControl(const char* deviceName, const KinoPropertyInfo*& pi) {
  KinoVariant propValue;
  KinoError err = KinoAPI::getProperty(deviceName, pi->key, propValue);
  switch (propValue.type) {
        case KinoVariant::BOOL: {
          toggleButton(deviceName, pi->key, pi->label, propValue.asBool());
          break;
        case KinoVariant::INT: {
          int minvalue = pi->minValue.value_or(0);
          int maxvalue = pi->maxValue.value_or(255);
          int valuestp = pi->valueStp.value_or(1);
          slider(deviceName, pi->key, pi->label, propValue.asInt(), minvalue, maxvalue, valuestp);
          break;
        }
        case KinoVariant::RGB_COLOR: {
          colorPicker(deviceName, pi->key, pi->label, propValue.c_str());
          break;
        }
        default:
          //infoText(deviceName, pi->key, pi->label, "unknown");
          break;
        }
      }
}

void buildSelect(const char* deviceName, const KinoPropertyInfo*& pi, bool hasParams) {
  if (hasParams) {
    groupCardStart(pi->key, pi->label);
    selectStart(deviceName, pi->key, pi->label);
  } else {
    selectStart(deviceName, pi->key, pi->label);
  }
  KinoVariant value;
  KinoVariant label;
  KinoError err = KinoAPI::getProperty(deviceName, pi->key, value);
  
  const char* curValue = value.c_str();
  
  uint16_t optionCount;
  err = KinoAPI::getQueryCount(deviceName, pi->key, optionCount);
  char valShort[11]; strncpy(valShort, curValue, 10); valShort[10] = '\0';
  char optShort[11];
  for (int opt=0; opt<optionCount; opt++) {
    yield();
    KinoAPI::getQuery(deviceName, pi->key, opt, value);
    strncpy(optShort, value.c_str(), 10);
    optShort[10] = '\0';
    bool selected = (strcmp(optShort, valShort)==0);
    snprintf(_pathHelper, _pathHelperLen, "%s/%s/label", pi->key, value.c_str());
    _pathHelper[_pathHelperLen-1] = '\0';
    err = KinoAPI::getProperty(deviceName, _pathHelper, label);
      if (err != KinoError::OK) label = value;
    optionItem(value.c_str(), label.c_str(), selected);
  }
  selectEnd();
  if (hasParams) {
    snprintf(_pathHelper, _pathHelperLen, "%s/%s/param", pi->key, curValue);
    _pathHelper[_pathHelperLen-1] = '\0';
    showParameters(deviceName, _pathHelper);
    groupCardEnd();
  }
}

void buildComplexList(const char* deviceName, const KinoPropertyInfo*& pi, bool hasParams) {
  groupCardStart(pi->key, pi->label);
      uint16_t nrOfOptions;
      KinoVariant option;
      KinoVariant optLabel;
      
      KinoError err = KinoAPI::getQueryCount(deviceName,pi->key, nrOfOptions);
      for (int opt=0; opt < nrOfOptions; opt++) {
        err = KinoAPI::getQuery(deviceName, pi->key, opt, option);
        
        snprintf(_pathHelper, _pathHelperLen, "%s/%s/label", pi->key, option.c_str());
        _pathHelper[_pathHelperLen-1] = '\0';
        
        err = KinoAPI::getProperty(deviceName, _pathHelper, optLabel);
        if (err != KinoError::OK) optLabel = option;
        char groupId[32]; snprintf(groupId,32,"%s_%i",pi->label,opt);
        groupCardStart(groupId, optLabel.toString().c_str());
        if (hasParams) {  // hier die Parameter zur jeweiligen Option
          snprintf(_pathHelper, _pathHelperLen, "%s/%s/param", pi->key, option.c_str());
          _pathHelper[_pathHelperLen-1] = '\0';
          showParameters(deviceName, _pathHelper);
        }
        groupCardEnd();
      }
      groupCardEnd();
}

void showParameters(const char* deviceName, const char* paramPath) {
  uint16_t paramCount;
  KinoError err = KinoAPI::getQueryCount(deviceName, paramPath, paramCount);
  int pathLen = 128; char path[pathLen];

  KinoVariant getsetpath;
  KinoVariant label;
  KinoVariant value;
  KinoVariant tmp;
  int access = 0;
  int minvalue, maxvalue, valuestep;
  for (int i=0; i<paramCount; i++) {
    yield();
    snprintf(path, pathLen, "%s/%d", paramPath, i);
    path[pathLen-1] = '\0';
    err = KinoAPI::getProperty(deviceName, path, getsetpath);

    snprintf(path, pathLen, "%s/%d/label", paramPath, i);
    path[pathLen-1] = '\0';
    err = KinoAPI::getProperty(deviceName, path, label);

    snprintf(path, pathLen, "%s/%d/access", paramPath, i);
    path[pathLen-1] = '\0';
    err = KinoAPI::getProperty(deviceName, path, tmp);
    access = tmp.asInt();

    snprintf(path, pathLen, "%s/%d/minvalue", paramPath, i);
    path[pathLen-1] = '\0';
    err = KinoAPI::getProperty(deviceName, path, tmp);
    minvalue = tmp.asInt();

    snprintf(path, pathLen, "%s/%d/maxvalue", paramPath, i);
    path[pathLen-1] = '\0';
    err = KinoAPI::getProperty(deviceName, path, tmp);
    maxvalue = tmp.asInt();

    snprintf(path, pathLen, "%s/%d/valuestep", paramPath, i);
    path[pathLen-1] = '\0';
    err = KinoAPI::getProperty(deviceName, path, tmp);
    valuestep = tmp.asInt();
    
    if ((access == 1) || (access == 3)) {
      err = KinoAPI::getProperty(deviceName, getsetpath.c_str(), value);
    }
    if (access == 1) {
      infoText(deviceName, getsetpath.c_str(), label.c_str(), value.c_str());
    }
    if (access == 2) {
      button(deviceName, getsetpath.c_str(), label.c_str());
    }
    if (access == 3) {
      switch (value.type) {
        case KinoVariant::BOOL: {
          toggleButton(deviceName, getsetpath.c_str(), label.c_str(), value.asBool());
          break;
        }
        case KinoVariant::INT: {
          slider(deviceName, getsetpath.c_str(), label.c_str(), value.asInt(), minvalue, maxvalue, valuestep);
          break;
        }
        case KinoVariant::RGB_COLOR: {
          colorPicker(deviceName, getsetpath.c_str(), label.c_str(), value.c_str());
          break;
        }
        default: {
          infoText(deviceName, getsetpath.c_str(), label.c_str(), value.c_str());
          break;
        }
      }
    }
  }
  yield();
}

void pageStart(const char* title) {
  Serial.print("start serving page: ");
  Serial.println(title);
    _server.sendHeader("Connection", "close");
    _server.setContentLength(CONTENT_LENGTH_UNKNOWN); // Stream-Modus
    _server.send(200, "text/html", "<!DOCTYPE html>");
    _server.sendContent(F("<html><head><meta name='viewport' content='width=device-width, initial-scale=1'>"));
    _server.sendContent(F("<link rel='stylesheet' type='text/css' href='/style.css'>"));
    _server.sendContent(F("<script src='/script.js'></script>"));
        
    _server.sendContent(F("</head><body>"));
    // 1. Hole den URI-Pfad als einfachen C-String
    const char* uri = _server.uri().c_str();
    
    // 2. Prüfen, ob es eine Unterseite ist (Länge > 1)
    // uri[0] ist immer '/', wenn uri[1] nicht '\0' ist, sind wir auf einer Unterseite.
    bool isSubPage = (uri[0] == '/' && uri[1] != '\0');
    
    // Title card
    _server.sendContent(F("<div class='card'><h1>"));
    
    if (isSubPage) {
      // 3. Den letzten Slash finden
      const char* lastSlashPtr = strrchr(uri, '/');
  
      _server.sendContent(F("<a href='"));
      
      // 4. Den Pfad bis zum letzten Slash senden
      if (lastSlashPtr == uri) {
        // Wenn der letzte Slash ganz am Anfang steht, ist das Ziel das Root-Verzeichnis "/"
        _server.sendContent(F("/"));
      } else {
        // Wir senden manuell nur den Teil vor dem letzten Slash
        size_t lengthToCopy = lastSlashPtr - uri;
        
        // Wir nutzen eine kleine Hilfsvariable für den Teil-String ohne Kopie!
        _server.client().write((const uint8_t*)uri, lengthToCopy);
      }
      
      _server.sendContent(F("'>&lt;</a>  "));
    }
    _server.sendContent(title);
    _server.sendContent(F("</h1></div>"));
}

void pageEnd() {
  Serial.println(F("ready sending page"));
    _server.sendContent(F("<footer><small><output data-dev=\"system\" data-path=\"online\">"));
    char ctr[5];
    itoa(WSConnected, ctr, 10);
    _server.sendContent(ctr);
    _server.sendContent(F("<output></small> Users online</footer></body></html>"));
    _server.sendContent("");
}

void groupCardStart(const char* id, const char* label) {
    _server.sendContent(F("<div class='card'>"));
    _server.sendContent(F("<h2 class='accordion-header' onclick=\"toggleCard('"));
    _server.sendContent(id);
    _server.sendContent(F("')\">"));
    _server.sendContent(label);
    _server.sendContent(F(" &dArr;</h2>"));
    _server.sendContent(F("<div class='accordion-content' id='"));
    _server.sendContent(id);
    _server.sendContent(F("'>"));
}

void groupCardEnd() {
    _server.sendContent(F("</div></div>"));
}

// =====================================================================================================
// Standard Formular- Elemente
// =====================================================================================================

void toggleButton(const char* deviceName, const char* func, const char* label, bool value) {
    _server.sendContent(F("<div class='card'><h2>"));
    _server.sendContent(label);
    _server.sendContent(F("</h2>"));

    _server.sendContent(F("<label class='switch'>"));
    _server.sendContent(F("<input type='checkbox' data-dev='"));
    _server.sendContent(deviceName);
    _server.sendContent(F("' data-path='"));
    _server.sendContent(func);
    _server.sendContent(F("' "));
    if (value) _server.sendContent(F("checked "));
    
    _server.sendContent(F("onchange=\"sendCmd('"));
    _server.sendContent(deviceName);
    _server.sendContent(F("','"));
    _server.sendContent(func);
    _server.sendContent(F("',this.checked?1:0)\">"));

    _server.sendContent(F("<span class='slider'></span></label></div>"));
}

void slider(const char* deviceName, const char* func, const char* label, int value, int minval, int maxval, int steps) {
    _server.sendContent(F("<div class='card'><h2>"));
    _server.sendContent(label);
    _server.sendContent(F("</h2>"));
        
    _server.sendContent(F("<input type='range' id='"));
    _server.sendContent(func);
    _server.sendContent(F("' data-dev='"));
    _server.sendContent(deviceName);
    _server.sendContent(F("' data-path='"));
    _server.sendContent(func);
    _server.sendContent(F("' "));

    char buf[64];
    snprintf(buf, sizeof(buf),
             "min='%d' max='%d' step='%d' value='%d' ",
             minval, maxval, steps, value);
    _server.sendContent(buf);

    _server.sendContent(F("oninput=\"sendCmd('"));
    _server.sendContent(deviceName);
    _server.sendContent(F("','"));
    _server.sendContent(func);
    _server.sendContent(F("',this.value)\">"));
    
    _server.sendContent(F("<button type='button' data-controls='"));
    _server.sendContent(func);
    _server.sendContent(F("' data-action='down'>-</button>"));
    _server.sendContent(F("<output for='"));
    _server.sendContent(func);
    _server.sendContent(F("' data-dev='"));
    _server.sendContent(deviceName);
    _server.sendContent(F("' data-path='"));
    _server.sendContent(func);
    _server.sendContent(F("'>"));
    _server.sendContent(String(value));
    _server.sendContent(F("</output>"));
    _server.sendContent(F("<button type='button' data-controls='"));
    _server.sendContent(func);
    _server.sendContent(F("' data-action='up'>+</button></div>"));
}

void selectStart(const char* deviceName, const char* func, const char* label) {
    _server.sendContent(F("<div class='card'><h2>"));
    _server.sendContent(label);
    _server.sendContent(F("</h2><select "));
    _server.sendContent(F("data-dev='"));
    _server.sendContent(deviceName);
    _server.sendContent(F("' data-path='"));
    _server.sendContent(func);
    _server.sendContent(F("' onchange=\"sendCmd('"));
    _server.sendContent(deviceName);
    _server.sendContent(F("','"));
    _server.sendContent(func);
    _server.sendContent(F("',this.value)\">"));
}

void optionItem(const char* value, const char* label, bool selected) {
    _server.sendContent(F("<option value='"));
    if (strlen(value)>0) {
      _server.sendContent(value);
    } else {
      _server.sendContent(F(" "));
    }
    _server.sendContent(F("'"));
    if (selected) _server.sendContent(F(" selected"));
    _server.sendContent(F(">"));
    _server.sendContent(label);
    _server.sendContent(F("</option>"));
}

void selectEnd() {
    _server.sendContent(F("</select></div>"));
}

void button(const char* deviceName, const char* func, const char* label) {
  _server.sendContent(F("<div class='card'>"));
  _server.sendContent(F("<button class='btn' onclick=\"sendCmd('"));
  _server.sendContent(deviceName);
  _server.sendContent(F("','"));
  _server.sendContent(func);
  _server.sendContent(F("',1)\">"));
  _server.sendContent(label);
  _server.sendContent(F("</button></div>"));
}

void infoText(const char* deviceName, const char* func, const char* label, const char* value) {
  _server.sendContent(F("<div class='card'><h2>"));
  _server.sendContent(label);
  _server.sendContent(F("</h2><output data-dev='"));
  _server.sendContent(deviceName);
  _server.sendContent(F("' data-path='"));
  _server.sendContent(func);
  _server.sendContent(F("'>"));
  if (strlen(value)>0) {
    _server.sendContent(value);
  } else {
    _server.sendContent(F("&nbsp;"));
  }
  _server.sendContent(F("</output></div>"));
}

void colorPicker(const char* deviceName, const char* func, const char* label, const char* value){
    _server.sendContent(F("<div class='card'><h2>"));
    _server.sendContent(label);
    _server.sendContent(F("</h2>"));

    _server.sendContent(F("<input type='color' data-dev='"));
    _server.sendContent(deviceName);
    _server.sendContent(F("' data-path='"));
    _server.sendContent(func);
    _server.sendContent(F("' value='"));
    _server.sendContent(value);
    _server.sendContent(F("' onchange=\"sendCmd('"));

    _server.sendContent(deviceName);
    _server.sendContent(F("','"));
    _server.sendContent(func);
    _server.sendContent(F("',this.value)\">"));

    _server.sendContent(F("</div>"));
}

void startMacroSelect(const char* deviceName, const char* func, const char* label) {
  _server.sendContent(F("<input type='checkbox' name='inc_"));
  _server.sendContent(func);
  _server.sendContent(F("'"));
  bool isIncluded = false;
  if ((_lineSettings.containsKey(deviceName)) && (_lineSettings[deviceName].containsKey(func))) isIncluded = true;
  if (isIncluded) _server.sendContent(F(" checked"));
  _server.sendContent(F(">"));

  _server.sendContent(F("<label class='toggle-inline'><span class='toggle-text'>"));
  _server.sendContent(label);
  _server.sendContent(F("</span><select name='val_"));
  _server.sendContent(func);
  _server.sendContent(F("'>"));
  
}

void endMacroSelect() {
  _server.sendContent(F("</select></label><br>"));
}

void macroToggleButton(const char* deviceName, const char* func, const char* label, bool value) {
  _server.sendContent(F("<input type='checkbox' name='inc_"));
  _server.sendContent(func);
  _server.sendContent(F("'"));
  /*bool isIncluded = false;
  if ((_lineSettings.containsKey(deviceName)) && (_lineSettings[deviceName].containsKey(func))) isIncluded = true;
  if (isIncluded) _server.sendContent(F(" checked"));*/
  if ((_lineSettings.containsKey(deviceName)) && (_lineSettings[deviceName].containsKey(func))) {
    _server.sendContent(F(" checked"));
  }
  _server.sendContent(F(">"));
  
  _server.sendContent(F("<label class='toggle-inline'><span class='toggle-text'>"));
  _server.sendContent(label);
  _server.sendContent(F("</span><span class='switch'><input type='checkbox' name='val_"));
  _server.sendContent(func);
  _server.sendContent(F("'"));
  if (value) _server.sendContent(F("checked "));
  _server.sendContent(F(">"));
  _server.sendContent(F("<span class='slider'></span></span></label><br>"));
}

void macroSlider(const char* deviceName, const char* func, const char* label, int value, int minval, int maxval, int steps) {
  static int sliderCounter = 0;
  if (sliderCounter > 999) sliderCounter = 0;
  char Ch_sliderCounter[5];
  snprintf(Ch_sliderCounter,4,"%i",sliderCounter);
  Ch_sliderCounter[4] = '\0';
  _server.sendContent(F("<input type='checkbox' name='inc_"));
  _server.sendContent(func);
  _server.sendContent(F("'"));
  /*bool isIncluded = false;
  if ((_lineSettings.containsKey(deviceName)) && (_lineSettings[deviceName].containsKey(func))) isIncluded = true;
  if (isIncluded) _server.sendContent(F(" checked"));*/
  if ((_lineSettings.containsKey(deviceName)) && (_lineSettings[deviceName].containsKey(func))) {
    _server.sendContent(F(" checked"));
  }
  _server.sendContent(F(">"));

  _server.sendContent(F("<label class='toggle-inline'><span class='toggle-text'>"));
  _server.sendContent(label);
  _server.sendContent(F("</span>"));

  _server.sendContent(F("<input type='range' name='val_"));
  _server.sendContent(func);
  _server.sendContent(F("' "));
  char buf[64];
  snprintf(buf, sizeof(buf),
           "min='%d' max='%d' step='%d' value='%d' ",
           minval, maxval, steps, value);
  _server.sendContent(buf);
  _server.sendContent(F(" id='val_"));
  _server.sendContent(func);
  _server.sendContent(Ch_sliderCounter);
  _server.sendContent(F("'></label>"));

  _server.sendContent(F("<button type='button' data-controls='val_"));
  _server.sendContent(func);
  _server.sendContent(Ch_sliderCounter);
  _server.sendContent(F("' data-action='down'>-</button>"));
  _server.sendContent(F("<output for='val_"));
  _server.sendContent(func);
  _server.sendContent(Ch_sliderCounter);
  _server.sendContent(F("'>"));
  _server.sendContent(String(value));
  _server.sendContent(F("</output>"));
  _server.sendContent(F("<button type='button' data-controls='val_"));
  _server.sendContent(func);
  _server.sendContent(Ch_sliderCounter);
  _server.sendContent(F("' data-action='up'>+</button><br>"));
  
  sliderCounter++;
}

void macroColorPicker(const char* deviceName, const char* func, const char* label, const char* value) {
  _server.sendContent(F("<input type='checkbox' name='inc_"));
  _server.sendContent(func);
  _server.sendContent(F("'"));
  /*bool isIncluded = false;
  if ((_lineSettings.containsKey(deviceName)) && (_lineSettings[deviceName].containsKey(func))) isIncluded = true;
  if (isIncluded) _server.sendContent(F(" checked"));*/
  if ((_lineSettings.containsKey(deviceName)) && (_lineSettings[deviceName].containsKey(func))) {
    _server.sendContent(F(" checked"));
  }
  
  _server.sendContent(F(">"));
  
  _server.sendContent(F("<label class='toggle-inline'><span class='toggle-text'>"));
  _server.sendContent(label);
  _server.sendContent(F("</span><input type='color' name='val_"));
  _server.sendContent(func);
  _server.sendContent(F("' value='"));
  _server.sendContent(value);
  _server.sendContent(F("'></label><br>"));
}

// =====================================================================================================
// die grossen HTML- Chunks: CSS und Javascript
// =====================================================================================================


const char HTML_CSS[] PROGMEM = R"raw(
/*<style>*/
  body{font-family:Arial;background:#121212;color:#eee;margin:0;padding:10px;}
  a{color:#ccc;}a:active,a:visited{color:#ccc;}
  .card{background:#1e1e1e;padding:15px;margin-bottom:15px;border-radius:10px;
  box-shadow:0 0 10px #0006;}
  h1{margin-top:0; text-align: center;}
  h2{margin-top:0;}
  h2 button.remove, h2 button.deleteLine { float: right; margin-right: 2em; }
  h3{font-size:1em;}
  .params {margin:0px;margin-left:3em; padding:0px;}
  .macroEditButtons {margin:10px;padding:0px;}
  .macroEditButton {background:#1e1e1e; color:#eee; border:1px solid #333; border-raius:6px;}
  .toggle-inline{display:inline-flex; align-items:center; gap:8px; cursor:pointer;}
  .toggle-inline .switch{flex-shrink:0;}
  .toggle-inline input[type=range] {width: 200px;}
  .toggle-inline select {width: 200px;}
  input[type=number] { background:#1e1e1e;color:#eee;border:1px solid #333;border-radius:6px; width:4em;}
  .switch{position:relative;display:inline-block;width:50px;height:24px;}
  .switch input{display:none;}
  .slider{position:absolute;cursor:pointer;background-color:#666;border-radius:24px;
  top:0;left:0;right:0;bottom:0;transition:.3s;}
  .slider:before{position:absolute;content:'';height:20px;width:20px;left:2px;bottom:2px;
  background-color:white;border-radius:50%;transition:.3s;}
  input:checked + .slider{background-color:#03a9f4;}
  input:checked + .slider:before{transform:translateX(26px);}
  /*.btn{background:#333;color:#fff;padding:10px 15px;border-radius:6px;border:none;margin:5px 0;*/
  .btn{background:#2979ff;color:#fff;padding:12px;border:none;border-radius:6px;
        font-size:16px;width:100%;cursor:pointer;transition:0.2s;}
   .btn:hover{background:#5393ff;}
   .btn:active{background:#1c54b2;}
  width:100%;font-size:16px;}
  .btn:hover{background:#444;}
  input[type=range]{width:200px;margin:10px 0;}
  select{width:100%;padding:10px;background:#1e1e1e;color:#eee;border-radius:6px;border:1px solid #333;
  margin:8px 0;}
  .accordion-header { cursor:pointer; padding:0; }
  .accordion-content { display:none; margin-top:10px; }
  .accordion-content .card { background: #242424; }
  .accordion-content.active, .accordion-content.active .card {background: #303030; }
  a.btn, a.btn:hover, a.btn:active{ background: transparent; width: 200px; text-align: center; text-decoration: none; }
/*</style>*/
)raw";

const char HTML_JAVASCRIPT[] PROGMEM = R"raw(
/*<script>*/
const timers = {};
function sendCmd(device, func, value){
 const timername = device+func;
 clearTimeout(timers[timername]);
 value = encodeURIComponent(value);

 timers[timername] = setTimeout(function(){reallySendCmd(device,func,value)}, 300);
}
function reallySendCmd(device,func,value) {
  console.log('/cmd?dev='+device+'&f='+func+'&v='+value);
  fetch('/cmd?dev='+device+'&f='+func+'&v='+value)
    .then(r=>r.text()).then(t=>console.log(t));
}
function toggleCard(id){
  const el=document.getElementById(id);
  if(!el) return;
  el.style.display = (el.style.display==='block') ? 'none' : 'block';
  if (el.style.display==='block') {el.classList.add('active');} else { el.classList.remove('active'); }
}
function addLineBefore(linenr){
 console.log('adding line before nr '+linenr);
 const template = document.getElementById('newLine');
 const target = document.getElementById('line_'+linenr);
 const clone = template.content.cloneNode(true);
 clone.querySelector('.linenr').value = linenr;
 clone.querySelector('.cmdselect').addEventListener('change',loadCmdParam);
 clone.querySelector('.remove').addEventListener('click',removeAddedLine);
 target.before(clone);
}
function removeAddedLine(event) {
  event.preventDefault();
  const container = event.target.closest('.card');
  if (container) container.remove();
}
function loadCmdParam(event){
 const template = document.getElementById(event.target.value);
 const clone = template.content.cloneNode(true);
 const devsel = clone.querySelector('.devselect');
 if (devsel) devsel.addEventListener('change',loadDeviceFuncSelect);
 const existingParam = event.target.nextElementSibling;
 if (existingParam && existingParam.classList.contains('cmdparam')) {
 existingParam.remove();
 }
 event.target.after(clone);
 const form = event.target.closest('form'); const paramElement = form.querySelector('.params');
 if(paramElement) paramElement.innerHTML = '';
}
function loadDeviceFuncSelect() {
 const deviceName = event.target.value;
 const form = event.target.closest('form');
 fetch('/devFuncSelect?dev=' + deviceName)
  .then(r => r.text())
  .then(t => {
   form.querySelector('.params').innerHTML = t;
   const funcSelect = form.querySelector('.funcselect');
   if (funcSelect) {
    funcSelect.addEventListener('change', loadFuncControls);
   }
  });
}
function loadFuncControls(){
 const form = event.target.closest('form');
 const dev = form.querySelector('.devselect').value;
 const func = event.target.value;
 fetch('/devFuncControls?dev='+dev+'&func='+func)
   .then(r=>r.text()).then(t=>form.querySelector('.funccontrols').innerHTML = t)
}
document.addEventListener('DOMContentLoaded', () => {
  const selects = document.querySelectorAll('.cmdselect');
  selects.forEach(element => {
    element.addEventListener('change', loadCmdParam);
  });
  const devsels = document.querySelectorAll('.devselect');
  devsels.forEach(element => {
    element.addEventListener('change', loadDeviceFuncSelect);
  });
  const sliders = document.querySelectorAll('input[type=range]');
  sliders.forEach(slider => {
    const output = document.querySelector(`output[for="${slider.id}"]`);      
    slider.addEventListener('input', () => {
        if (output) output.value = slider.value;
    });
  });
  // 2. Alle +/- Buttons
  document.querySelectorAll('button[data-controls]').forEach(btn => {
    btn.addEventListener('click', () => {
      const targetId = btn.getAttribute('data-controls');
      const action = btn.getAttribute('data-action');
      const slider = document.getElementById(targetId);
      if (slider) {
        if (action === 'up') slider.stepUp();
        if (action === 'down') slider.stepDown();
        // Triggert das 'input' Event, damit der Output oben aktualisiert wird
        slider.dispatchEvent(new Event('input'));
      }
    });
  });
  console.log(`${selects.length} cmdHandler erfolgreich registriert.`);
  console.log(`${devsels.length} devHandler erfolgreich registriert.`);
});
let socket;
function decodeHtml(html) {
    const doc = new DOMParser().parseFromString(html, "text/html");
    return doc.documentElement.textContent;
}
  function handleUpdate(data) {
    const dev = data.dev;   // z.B. "hue"
    const basePath = data.path || ""; // z.B. "sensors/Licht Sensor Theke"

    // Wir iterieren über alle Keys im JSON (außer Metadaten)
    Object.keys(data).forEach(key => {
        if (key === 'dev' || key === 'path') return;

        let value = data[key];
        if (typeof value === 'string') {
            value.replace("&amp;","&");
            value = decodeHtml(value);
        }
        // Pfad zusammenbauen: Wenn basePath existiert, hängen wir den Key an
        const fullPath = basePath ? `${basePath}/${key}` : key;
        // Selektor finden: Element mit passendem data-dev UND data-path
        const selector = `[data-dev="${dev}"][data-path="${fullPath}"]`;
        const elements = document.querySelectorAll(selector);

        elements.forEach(el => {
            // Typspezifische Aktualisierung
            if (el.tagName === 'OUTPUT') {
                // Bei Output schreiben wir den Wert als Text
                el.textContent = value;
            } 
            else if (el.tagName === 'SELECT') {
                // 1. Versuch: Exakter Match (Standard)
                el.value = value;
            
                // 2. Versuch: Falls kein exakter Match gefunden wurde (value ist leer oder unpassend)
                if (el.selectedIndex === -1 || el.value !== value) {
                    // Wir suchen die Option, deren Text am besten im empfangenen String vorkommt
                    const options = Array.from(el.options);
                    const bestMatch = options.find(opt => 
                        value.includes(opt.value) || value.includes(opt.text)
                    );
            
                    if (bestMatch) {
                        el.value = bestMatch.value;
                    }
                }
            }
            else if (el.tagName === 'INPUT') {
                switch (el.type) {
                    case 'checkbox':
                        el.checked = (value === true || value === 1 || value === "true");
                        break;
                    case 'range':
                    case 'color':
                    case 'number':
                    case 'text':
                        // WICHTIG: Nur aktualisieren, wenn der User nicht gerade schiebt
                        if (document.activeElement !== el) {
                            el.value = value;
                            // Trigger für eventuelle Anzeige-Outputs (z.B. Range-Zahlenwerte)
                            //el.dispatchEvent(new Event('input'));
                        }
                        break;
                }
            }
        });
    });
}
  
  function initWebSocket() {
      // Verbindet sich automatisch mit der IP des ESP8266 auf Port 81
      const gateway = `ws://${window.location.hostname}:81/`;
      socket = new WebSocket(gateway);
  
      socket.onopen = function(e) {
          console.log("WebSocket verbunden");
      };
  
      socket.onclose = function(e) {
          console.log("WebSocket getrennt - versuche Reconnect in 2s...");
          setTimeout(initWebSocket, 2000); // Automatischer Reconnect
      };
  
      socket.onerror = function(e) {
          console.error("WebSocket Fehler:", e);
      };
  
      socket.onmessage = function(event) {
          console.log("Daten empfangen:", event.data);
          const data = JSON.parse(event.data);
          handleUpdate(data); // Hier kommt deine Update-Logik rein
      };
  }
  
  // Startet die Verbindung, sobald die Seite geladen ist
  document.addEventListener('DOMContentLoaded', initWebSocket);
 

/*</script>*/
)raw";

}
