#include "SerialCommandDispatcher.h"
#include "YamahaReceiver.h"
#include "OptomaBeamer.h"
#include "WLEDDevice.h"
#include "HyperionDevice.h"
#include "HueBridge.h"
#include "KinoAPI.h"
#include <ESP8266WiFi.h>

// ==== Externe Geräte (existieren im Sketch) ====

// Externe Geräte aus dem Sketch

// ==== Hilfsfunktionen ====

static bool toBool(const String& s) {
  return (s == "1" || s == "true" || s == "on");
}

// Hilfsfunktion, um den Typ eines Parameters zu bestimmen:
bool isValidRgbFormat(const char* input) {
    if (!input) return false;

    // Hilfsvariable, um zu prüfen, ob nach der schließenden Klammer noch Text kommt
    char tail[2]; 
    int r,g,b;
    // sscanf versucht, das Muster "[ r , g , b ]" zu finden.
    // %d überspringt führende Leerzeichen automatisch.
    // %1s am Ende versucht, ein Zeichen nach dem ']' zu lesen.
    int found = sscanf(input, " [ %d , %d , %d ] %1s", &r, &g, &b, tail);

    // Der Rückgabewert muss exakt 3 sein (die drei Zahlen).
    // Wäre die schließende Klammer falsch oder käme danach noch Text, 
    // würde 'found' entweder kleiner 3 sein oder tail wäre gefüllt.
    return (found == 3);
}

int determineType(const char* chk) {
  if (chk == nullptr || chk[0] == '\0') return 0;

  // 1. Check Bool (Vergleich ohne Groß/Kleinschreibung)
  if (strcasecmp(chk, "true") == 0 || strcasecmp(chk, "false") == 0 ) {
    return 1; // 1 = "bool"
  }

  // 2. Check Numerisch (Int oder Float)
  bool hasDecimal = false;
  bool isNumeric = true;
  int i = 0;

  // Optionales Vorzeichen prüfen
  if (chk[0] == '-' || chk[0] == '+') i++;

  // Wenn nach dem Vorzeichen nichts kommt, ist es kein Typ
  if (chk[i] == '\0') isNumeric = false;

  for (; chk[i] != '\0'; i++) {
    if (chk[i] == '.') {
      if (hasDecimal) { // Zweiter Punkt gefunden -> Ungültig für Zahlen
        isNumeric = false;
        break;
      }
      hasDecimal = true;
    } 
    else if (!isdigit(chk[i])) {
      isNumeric = false;
      break;
    }
  }

  if (isNumeric) {
    return hasDecimal ? 3 : 2;  // 2 = int, 3 = float
  }

  // Sring until here. Check for color:
  if (isValidRgbFormat(chk)) return 5; // RGB Color
  
  return 4; // 4 = string
}

static KinoVariant prepareForJson(const String p) {
  int typeNr = determineType(p.c_str());
  KinoVariant val; val.setNone();
  switch (typeNr) {
    case 1  : // bool
      //val = KinoVariant::fromBool(toBool(p));
      val.setBool(toBool(p));
      break;
    case 2  : // int
      //val = KinoVariant::fromInt(p.toInt());
      val.setInt(p.toInt());
      break;
    case 3  : // float
      //val = KinoVariant::fromFloat(p.toFloat());
      val.setFloat(p.toFloat());
      break;
    case 4  : // string
      //val = KinoVariant::fromString(p.c_str());
      val.setString(p.c_str());
      break;
    case 5  : {// rgbcolor
      uint8_t r,g,b;
      int found = sscanf(p.c_str()," [ %d , %d , %d ] ", &r, &g, &b);
      if (found != 3) {
        Serial.println(F("error converting color to RGB, use json encoded command instead"));
        return val;
      }
      //val = KinoVariant::fromColor(r,g,b);
      val.setColor(r,g,b);
      break; }
    default :
      Serial.println(F("could not determine type of value, use json encoded command instead"));
      return val;
      break;
  }
  return val;
}

// ==== Handler-Signaturen ====

typedef bool (*CommandHandler)(String* params, uint8_t paramCount);

// ==== Kommando-Tabelle ====

struct CommandEntry {
  const char* object;
  const char* method;
  uint8_t expectedParams;
  CommandHandler handler;
  const char* help;
};

// ==== Handler-Prototypen ====
bool kino_createMacro(String*p, uint8_t n);
bool kino_addOrUpdateMacro(String* p, uint8_t n);
bool kino_executeMacro(String* p, uint8_t n);
bool kino_testMacro(String* p, uint8_t n);
bool kino_listMacros(String* p, uint8_t n);
bool kino_showMacro(String* p, uint8_t n);
bool kino_addCommandToMacro(String* p, uint8_t n);
bool kino_deleteCommandFromMacro(String* p, uint8_t n);
bool kino_updateCommandInMacro(String* p, uint8_t n);
bool kino_deleteMacro(String* p, uint8_t n);

bool kino_memory(String* p, uint8_t n);
bool kino_help(String* p, uint8_t n);
bool kino_init(String* p, uint8_t n);
bool kino_startTest(String* p, uint8_t n);
bool kino_stopTest(String*p, uint8_t n);
bool kino_disconnect(String* p, uint8_t n);

bool kinoGet(String* p, uint8_t n);
bool kinoSet(String* p, uint8_t n);
bool kino_list(String* p, uint8_t n);
bool kino_showProperties(String* p, uint8_t n);

bool yamaha_info(String*p, uint8_t n);
bool beamer_info(String* p, uint8_t n);
bool canvas_info(String* p, uint8_t n);
bool sound_info(String* p, uint8_t n);
bool hyperion_info(String* p, uint8_t n);

bool hue_LightInfo(String* p, uint8_t n);
bool hue_listLights(String* p, uint8_t n);
bool hue_listGroups(String* p, uint8_t n);
bool hue_listScenes(String* p, uint8_t n);
bool hue_showSensors(String *p, uint8_t n);


// ==== Tabelle ====
static const CommandEntry commandTable[] = {
  {"kino",   "disconnect",0, kino_disconnect,           "trennt die WiFi- Verbindung (Re-Connect Test)"},
  {"kino",   "startTest", 1, kino_startTest,            "startet TickInterval- Test. Parameter gibt das Zeit-Intervall für einen gesamten Gerätezyklus in Millisekunden"},
  {"kino",   "stopTest",  0, kino_stopTest,             "stoppt TickInterval- Test."},
  {"macro",  "run",       1, kino_executeMacro,         "lädt Makro und führt es aus"},
  {"macro",  "test",      1, kino_testMacro,            "lädt Makro und führt Tests aus"},
  {"macro",  "list",      0, kino_listMacros,           "zeigt eine Liste aller gespeicherten Makros"},
  {"macro",  "show",      1, kino_showMacro,            "zeigt den Inhalt des angegebenen Makros"},
  {"macro",  "add",       1, kino_addOrUpdateMacro,     "speichert das gegebene JSON als Makro"},
  {"macro",  "new",       1, kino_createMacro,          "speichert das gegebene JSON als Makro"},
  {"macro",  "create",    1, kino_createMacro,          "speichert das gegebene JSON als Makro"},
  {"macro",  "addLine",   3, kino_addCommandToMacro,    "fügt ein Kommando zu einem Macro hinzu. Param1: Macroname, Param2: Zeilennummer, Param3: action als json"},
  {"macro",  "deleteLine",2, kino_deleteCommandFromMacro,"Löscht eine Zeile aus dem angegebenen Makro. Param1: Makroname, Param2: Zeilennummer"},
  {"macro",  "updateLine",3, kino_updateCommandInMacro, "ersetzt eine Zeile im angegebenen Makro. Param1: Makroname, Param2: Zeilennummer, Param3: neue action als json"},
  {"macro",  "delete",    1, kino_deleteMacro,          "löscht das Makro mit dem gegebenen Namen"},
  {"kino",   "help",      0, kino_help,                 "zeigt diese Hilfe"},
  {"kino",   "mem",      0, kino_memory,                "zeigt Informationen über den RAM an"},
  {"kino",   "init",      0, kino_init,                 "(re-)initialisiert alle Geräte"},
  {"kino",   "get",       2, kinoGet,                   "gibt die Eigenschaft P2 vom Gerät P1 aus"},
  {"kino",   "set",       3, kinoSet,                   "set Eigenschaft P2 von Gerät P1 auf P3 vom Typ P4"},
  {"kino",   "list",      2, kino_list,                 "liest angegebene Liste P2 aus Gerät P1 und gibt die Einträge aus"},
  {"kino",   "showProps", 1, kino_showProperties,       "zeigt die verfügbaren Parameter für ein Gerät an"},
  {"yamaha", "info",      0, yamaha_info,               "Status Yamaha"},
  {"beamer", "info",      0, beamer_info,               "Status Beamer"},
  {"canvas", "info",      0, canvas_info,               "Status Leinwand"},
  {"sound",  "info",      0, sound_info,                "Status Sound-LEDs"},
  {"hyperion","info",     0, hyperion_info,             "Status Hyperion"},
  {"hue",     "LightInfo",1, hue_LightInfo,             "Status der angegebenen Lampe. Param=Lampenname"},
  {"hue",     "listLights",0,hue_listLights,            "listet alle Hue Lampen auf"},
  {"hue",     "listGroups",0,hue_listGroups,            "listet alle Hue Gruppen auf"},
  {"hue",     "listScenes",0,hue_listScenes,            "listet alle Szenen auf"},
  {"hue",     "showSensors",0,hue_showSensors,          "listet die wichtigsten Hue Sensoren auf"},
};

static const size_t commandCount =
  sizeof(commandTable) / sizeof(commandTable[0]);

// ==== Hauptfunktion ====

void handleSerialCommands() {
  if (!Serial.available()) return;

  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.isEmpty()) return;
  Serial.println();
  Serial.println(line);

  int dot   = line.indexOf('.');
  int open  = line.indexOf('(', dot);
  int close = line.lastIndexOf(')');

  if (dot < 0 || open < 0 || close < open) {
    Serial.println(F("❌ Syntaxfehler\n"));
    return;
  }

  String object = line.substring(0, dot);
  String method = line.substring(dot + 1, open);
  String paramStr = line.substring(open + 1, close);

  object.trim();
  method.trim();
  paramStr.trim();

  // === Parameter splitten (klammer-sensitiv) ===
  uint8_t maxParamCount = 8;
  String params[maxParamCount];          // Max. 8 Parameter
  uint8_t paramCount = 0;
  
  if (!paramStr.isEmpty()) {
    int braceDepth   = 0;    // {}
    int bracketDepth = 0;    // []
    int parenthesisDepth = 0;// ()
    int start = 0;
  
    for (int i = 0; i < paramStr.length(); ++i) {
      char c = paramStr[i];
  
      if (c == '{') braceDepth++;
      else if (c == '}') braceDepth--;
      else if (c == '[') bracketDepth++;
      else if (c == ']') bracketDepth--;
      else if (c == '(') parenthesisDepth++;
      else if (c == ')') parenthesisDepth--;
  
      // Nur splitten, wenn wir NICHT in Klammern sind
      if (c == ',' && braceDepth == 0 && bracketDepth == 0 && parenthesisDepth == 0) {
        if (paramCount < maxParamCount) {
          params[paramCount++] = paramStr.substring(start, i);
        }
        start = i + 1;
      }
    }
  
    // Letzten Parameter hinzufügen
    if (paramCount < maxParamCount && start < paramStr.length()) {
      params[paramCount++] = paramStr.substring(start);
    }
  }
  
  // Trimmen
  for (uint8_t i = 0; i < paramCount; ++i) {
    params[i].trim();
  }

  // === Dispatch ===

  for (size_t i = 0; i < commandCount; ++i) {
    const CommandEntry& e = commandTable[i];

    if (object == e.object && method == e.method) {

      //if (paramCount != e.expectedParams) {
      if (paramCount < e.expectedParams) { // paramCount darf ruhig grösser sein, das ermöglicht optionale Parameter in der API
        Serial.println(F("❌ Falsche Parameteranzahl: "));
        Serial.print(F("gegeben ")); Serial.print(paramCount); Serial.print(F(", erwartet ")); Serial.println(e.expectedParams);
        return;
      }

      bool ok = e.handler(params, paramCount);
      Serial.println(ok ? F("✅ OK\n") : F("❌ Fehler\n"));
      return;
    }
  }

  Serial.println(F("❌ Unbekanntes Kommando\n Liste der Kommandos mit kino.help()"));
}



// ====================================================
//          HANDLER FUNKTIONEN
// ====================================================

// helper function for showing errors if neccessary.
// returns true, so calling handler can return true, if no error occured
bool showError(KinoError e) {
  if (e == KinoError::OK) return true;
  Serial.println(kinoErrorToString(e));
  return false;
}

// SERIAL ONLY, for Debugging:

bool kino_memory(String* p, uint8_t n) {
  KinoAPI::showMemory();
  return true;
}

bool kino_startTest(String*p, uint8_t n) {
  int totalInterval = p[0].toInt();
  if (totalInterval == 0) return false;
  size_t devCount;
  KinoError e = KinoAPI::getDeviceCount(devCount);
  int waitInterval = totalInterval  / devCount;
  if (waitInterval < 1000) {
    Serial.print(F("Das Intervall ist zu klein. Es sollten mindestens 1000ms Puffer zwischen den einzelnen Geräteabfragen liegen, mit dem gegebenen Intervall sind es aber nur "));
    Serial.print(waitInterval);
    Serial.println(F("ms"));
    return false;
  }
  bool ok = true;
  KinoVariant devName;
  for (int i=0; i < devCount; i++) {
    e = KinoAPI::getDeviceName(i, devName);
    e = KinoAPI::setProperty(devName.c_str(), "tickInterval", KinoVariant::fromInt(totalInterval));
    if (e == KinoError::OK) {
      Serial.print(F("TickInterval for "));
      Serial.print(devName.c_str());
      Serial.print(F(" is now "));
      Serial.println(totalInterval);
    } else {
      Serial.print(F("could not set tickInterval for device "));
      Serial.println(devName.c_str());
      ok = false;
    }
    delay(waitInterval);
  }
  return ok;
}

bool kino_stopTest(String*p, uint8_t n) {
  size_t devCount;
  KinoError e = KinoAPI::getDeviceCount(devCount);
  KinoVariant devName;
  bool ok = true;
  for (int i=0; i < devCount; i++) {
    e = KinoAPI::getDeviceName(i, devName);
    e = KinoAPI::setProperty(devName.c_str(), "tickInterval", KinoVariant::fromInt(0));
    if (e == KinoError::OK) {
      Serial.print(F("TickInterval for "));
      Serial.print(devName.c_str());
      Serial.print(F(" is now 0"));
    } else {
      Serial.print(F("could not set tickInterval for device "));
      Serial.println(devName.c_str());
      ok = false;
    }
  }
  return ok;
}

bool kino_disconnect(String* p, uint8_t n) {
  Serial.println(F("--- MANUELLER STRESSTEST: WiFi wird gekappt ---"));
  WiFi.disconnect(); 
  return true;
}

bool kino_help(String* p, uint8_t n) {
  String cmp = (n == 0) ? "" : p[0];
  Serial.println(F("Folgende Kommandos sind verfügbar:\n-------------------------"));
  for (size_t i = 0; i<commandCount; ++i) {
    const CommandEntry& com = commandTable[i];
    if ((n==0) || (String(com.object) == cmp)) {
      Serial.print(com.object);
      Serial.print(F("."));
      Serial.print(com.method);
      Serial.println(F("()"));
      Serial.print(F("\tParameter: "));
      Serial.println(com.expectedParams);
      Serial.print(F("\t"));
      Serial.println(com.help);
      Serial.println();
    }
  }
  return true;
}

bool yamaha_info(String*p, uint8_t n) {
  KinoVariant v;
  KinoError e = KinoAPI::getProperty("yamaha","power",v);
  bool isOn = v.b;
  Serial.print(F("Power:  ")); Serial.println((isOn) ? "An":"Aus");
  e = KinoAPI::getProperty("yamaha","volume",v);
  Serial.print(F("Volume: ")); Serial.print((v.i)/10); Serial.println("dB");
  e = KinoAPI::getProperty("yamaha","mute",v);
  Serial.print(F("\tMute: ")); Serial.println(v.b ?"An":"Aus");
  Serial.println("Tone:");
  e = KinoAPI::getProperty("yamaha","bass",v);
  Serial.print(F("\tBass     : ")); Serial.println(v.i);
  e = KinoAPI::getProperty("yamaha","treble",v);
  Serial.print(F("\tTreble   : ")); Serial.println(v.i);
  e = KinoAPI::getProperty("yamaha","swtrim",v);
  Serial.print(F("\tSW Trim  : ")); Serial.println(v.i);
  e = KinoAPI::getProperty("yamaha","enhancer",v);
  Serial.print(F("\tEnhancer : ")); Serial.println(v.b ?"An":"Aus");
  e = KinoAPI::getProperty("yamaha","straight",v);
  Serial.print(F("\tStraight : ")); Serial.println(v.b ? F("An, DSP inaktiv") : F("Aus, DSP aktiv"));
  e = KinoAPI::getProperty("yamaha","dsp",v);
  Serial.print(F("\tDSP      : ")); Serial.println(v.s);
  
  KinoVariant src;
  KinoVariant src_custom;
  e = KinoAPI::getProperty("yamaha","input",src);
  Serial.print(F("Source: ")); Serial.print(src.s);
  e = KinoAPI::getProperty("yamaha","inputname",src_custom);
  if (strcmp(src.s,src_custom.s)!=0) {
    Serial.print(F(" (")); Serial.print(src_custom.s); Serial.println(F(" )"));
  } else {
    Serial.println();
  }
  if ((isOn)&&(strcmp(src.s,"NET RADIO")==0)) {
    e = KinoAPI::getProperty("yamaha","station",v);
    Serial.print(F("\tStation: ")); Serial.println(v.s);
    e = KinoAPI::getProperty("yamaha","song",v);
    Serial.print(F("\tSong   : ")); Serial.println(v.s);
    e = KinoAPI::getProperty("yamaha","elapsed",v);
    Serial.print(F("\tElapsed: ")); Serial.println(v.s);
  }
  return true;
}

bool beamer_info(String* p, uint8_t n) {
  KinoVariant v;
  KinoError e;
  /*e = _beamer->get("power",v);*/
  e = KinoAPI::getProperty("beamer","on",v);
  Serial.print(F("Power:  ")); Serial.println(v.b ? "An" : "Aus");
  /*e = _beamer->get("input",v);*/
  e = KinoAPI::getProperty("beamer","input",v);
  Serial.print(F("Source: ")); Serial.println(v.s);
  /*e = _beamer->get("uptime",v);*/
  e = KinoAPI::getProperty("beamer","uptime",v);
  Serial.print(F("Lampe:  ")); Serial.println(v.i);
  Serial.println(F("\n\n"));
  return true;
}

void helper_printColor(KinoVariant& v) {
  Serial.print(v.color.r); Serial.print(F(" , ")); Serial.print(v.color.g); Serial.print(F(" , ")); Serial.println(v.color.b);
}

bool canvas_info(String* p, uint8_t n) {
  KinoError e;
  KinoVariant v;
  e = KinoAPI::getProperty("canvas","power",v);
  Serial.print(F("Power:      ")); Serial.println(v.b ? F("An") : F("Aus"));
  e = KinoAPI::getProperty("canvas","brightness",v);
  Serial.print(F("Brightness: ")); Serial.println(v.i);
  e = KinoAPI::getProperty("canvas","live",v);
  Serial.print(F("Live Data:  ")); Serial.print(v.b ? F("incoming, ") : F("none, ")); 
  e = KinoAPI::getProperty("canvas","override",v);
  Serial.println(v.b ? F("ignoriert") : F("bearbeitet"));
  e = KinoAPI::getProperty("canvas","input",v);
  Serial.print(F("LD Source:  ")); Serial.println(v.s);
  e = KinoAPI::getProperty("canvas","effect",v);
  Serial.print(F("Effekt:     ")); Serial.println(v.i);
  if (v.i != 0) {
    e = KinoAPI::getProperty("canvas","speed",v);
    Serial.print(F("  Speed:    ")); Serial.println(v.i);
    e = KinoAPI::getProperty("canvas","intensity",v);
    Serial.print(F("  Intensity:")); Serial.println(v.i);
  }
  Serial.println(F("Farben:"));
  e = KinoAPI::getProperty("canvas","palette",v);
  Serial.print(F("\tPalette: ")); Serial.println(v.i);
  e = KinoAPI::getProperty("canvas","FgColor",v);
  Serial.print(F("\tFg: ")); /*Serial.print(v.color.r); Serial.print(F(" , ")); Serial.print(v.color.g); Serial.print(F(" , ")); Serial.println(v.color.b);*/
    helper_printColor(v);
  e = KinoAPI::getProperty("canvas","BgColor",v);
  Serial.print(F("\tBg: ")); /*Serial.print(v.color.r); Serial.print(F(" , ")); Serial.print(v.color.g); Serial.print(F(" , ")); Serial.println(v.color.b);*/
    helper_printColor(v);
  e = KinoAPI::getProperty("canvas","FxColor",v);
  Serial.print(F("\tFx: ")); /*Serial.print(v.color.r); Serial.print(F(" , ")); Serial.print(v.color.g); Serial.print(F(" , ")); Serial.println(v.color.b);*/
    helper_printColor(v);
  return true;
}

bool sound_info(String* p, uint8_t n) {
  KinoVariant v;
  KinoError e;
  e = KinoAPI::getProperty("sound","power",v);
  Serial.print(F("Power:      ")); Serial.println((v.b) ? F("An") : F("Aus"));
  e = KinoAPI::getProperty("sound","brightness",v);
  Serial.print(F("Brightness: ")); Serial.println(v.i);
  e = KinoAPI::getProperty("sound","effect",v);
  Serial.print(F("Effekt:     ")); Serial.println(v.i);
  if (v.i != 0) {
    e = KinoAPI::getProperty("sound","speed",v);
    Serial.print(F("  Speed:    ")); Serial.println(v.i);
    e = KinoAPI::getProperty("sound","intensity",v);
    Serial.print(F("  Intensity:")); Serial.println(v.i);
  }
  Serial.println(F("Farben:"));
  e = KinoAPI::getProperty("sound","palette",v);
  Serial.print(F("\tPalette: ")); Serial.println(v.i);
  e = KinoAPI::getProperty("sound","color1",v);
  Serial.print(F("\tFg: ")); /*Serial.print(v.color.r); Serial.print(F(" , ")); Serial.print(v.color.g); Serial.print(F(" , ")); Serial.println(v.color.b);*/
    helper_printColor(v);
  e = KinoAPI::getProperty("sound","color2",v);
  Serial.print(F("\tBg: ")); /*Serial.print(v.color.r); Serial.print(F(" , ")); Serial.print(v.color.g); Serial.print(F(" , ")); Serial.println(v.color.b);*/
    helper_printColor(v);
  e = KinoAPI::getProperty("sound","color3",v);
  Serial.print(F("\tFx: ")); /*Serial.print(v.color.r); Serial.print(F(" , ")); Serial.print(v.color.g); Serial.print(F(" , ")); Serial.println(v.color.b);*/
    helper_printColor(v);
  return true;
}

bool hyperion_info(String* p, uint8_t n) {
  KinoVariant v;
  KinoError e;
  Serial.print(F("Hyperion:\n  Power: "));
  e = KinoAPI::getProperty("hyperion","power",v);
  if (!showError(e)) return false;
  bool power = v.b;
  Serial.println(v.b ? F("An"):F("Aus"));
  e = KinoAPI::getProperty("hyperion","live",v);
  if (!showError(e)) return false;
  bool live = v.b;
  Serial.print(F("LEDs: "));
  Serial.println(live ? F("An"):F("Aus"));
  Serial.print(F("Broadcasting: "));
  Serial.println((power && live) ? F("Ja"):F("Nein"));
  return true;
}

bool hue_listLights(String* p, uint8_t n) {
  uint16_t lightCount;
  KinoVariant v;
  KinoError e = KinoAPI::getQueryCount("hue","lights",lightCount);
  showError(e);
  for (int i=0; i<lightCount; i++) {
    e = KinoAPI::getQuery("hue","lights",i,v);
    if (showError(e)) Serial.println(v.s);
  }
  return true;
}

bool hue_LightInfo(String* p, uint8_t n) {
  KinoError e;
  KinoVariant v;
  char tmp[32];
  snprintf(tmp,32,"lights/%s/power",p[0].c_str());
  Serial.println(p[0]);
  e = KinoAPI::getProperty("hue",tmp,v);
  if (!showError(e)) return false;
  Serial.print(F("\tPower: ")); Serial.println(v.b ? F("An"):F("Aus"));
  snprintf(tmp,32,"lights/%s/brightness",p[0].c_str());
  e = KinoAPI::getProperty("hue",tmp,v);
  if (!showError(e)) return false;
  Serial.print(F("\tBri  : ")); Serial.println(v.i);
  snprintf(tmp,32,"lights/%s/color",p[0].c_str());
  e = KinoAPI::getProperty("hue",tmp,v);
  if (e == KinoError::OutOfRange) {
    Serial.print(F("\tFarbe: nicht unterstützt\n"));
  } else {
    Serial.print(F("\tFarbe: ")); /*Serial.print(v.color.r); Serial.print(" , "); Serial.print(v.color.g); Serial.print(" , "); Serial.println(v.color.b);*/
      helper_printColor(v);
  }
  snprintf(tmp,32,"lights/%s/ct",p[0].c_str());
  e = KinoAPI::getProperty("hue",tmp,v);
  if (e == KinoError::OutOfRange) {
    Serial.print(F("\tCT   : nicht unterstützt\n"));
  } else {
    Serial.print(F("\tCT   : ")); Serial.println(v.i);
  }
  return true;
}

bool hue_listGroups(String*p, uint8_t n) {
  Serial.println("Hue Groups:");
  KinoVariant v;
  KinoError e;
  uint16_t sceneCount;
  //Serial.println("getQueryCount hue groups");
  e = KinoAPI::getQueryCount("hue","groups",sceneCount);
  if (!showError(e)) return false;
  for (int i=0; i<sceneCount; i++) {
    //Serial.print("getQuery hue groups "); Serial.println(i);
    e = KinoAPI::getQuery("hue","groups",i,v);
    if (!showError(e)) return false;
    Serial.print("\t"); Serial.println(v.s);
  }
  return true;
}

bool hue_listScenes(String* p, uint8_t n) {
  Serial.println("Hue Scenes:");
  KinoVariant v;
  KinoError e;
  uint16_t sceneCount;
  e = KinoAPI::getQueryCount("hue","scenes",sceneCount);
  if (!showError(e)) return false;
  for (int i=0; i<sceneCount; i++) {
    e = KinoAPI::getQuery("hue","scenes",i,v);
    if (!showError(e)) return false;
    Serial.print("\t"); Serial.println(v.s);
  }
  return true;
}

// helper function to show a single sensor
bool huehelper_showSensor(const char* sensorName) {
  KinoError e;
  KinoVariant v;
  uint16_t propCount = 0;
  char queryName[64];
  snprintf(queryName, 64, "sensors/%s/states", sensorName);
  e = KinoAPI::getQueryCount("hue",queryName, propCount);
  Serial.printf("\tSensor: %s , %i Werte\n", sensorName, propCount);
  if (!showError(e)) {
    Serial.print("occured in getQueryCount("); Serial.print(queryName); Serial.println(")");
    return false;
  }
  for (int i=0; i<propCount; i++) {
    char keyquery[64];
    KinoVariant kv;
    // Key herausfinden aus query "sensors/{SensorName}/states" und index i
    e = KinoAPI::getQuery("hue",queryName,i,v);
    if (!showError(e)) {
      Serial.print(F("occured in getQuery(")); Serial.print(queryName); Serial.println(F(")"));
      return false;
    }
    
    snprintf(keyquery,64,"sensors/%s/%s",sensorName,v.s);
    e = KinoAPI::getProperty("hue",keyquery,kv);
    if (!showError(e)) {
      Serial.print(F("occured in getQuery(")); Serial.print(keyquery); Serial.println(F(")"));
      return false;
    }
    Serial.printf("\t %s : ", v.s);
    Serial.println(kv.toString());
  }
  char isWritable[64];
  KinoVariant isit;
  snprintf(isWritable,64,"sensors/%s/writable",sensorName);
  e = KinoAPI::getProperty("hue",isWritable,isit);
  Serial.print(F("Dieser Sensor ist "));
  if (showError(e)) Serial.println((isit.b) ? F("schreibbar") : F("read-only"));  // fiese Logik: showError ist true, wenn KEIN Fehler aufgetreten ist!
  Serial.println();
  return true;
}

bool hue_showSensors(String* p, uint8_t n) {
  KinoError e;
  KinoVariant v;
  char sensorName[32];
  if (n > 0) {
    bool ok = true;
    for (int i=0; i<n; i++) {
      if (!huehelper_showSensor(p[i].c_str())) {
        ok = false;
      }
    }
    return ok;
  }
  // kein spezieller Sensor angefordert: Zeige eine Auswahl
  bool ok = true;
  Serial.println(F("Standard Hue Sensoren :"));
  snprintf(sensorName, 32, "Presence Clip Theke");
  if (!huehelper_showSensor(sensorName)) ok = false;
  snprintf(sensorName, 32, "Temp Sensor Theke");
  if (!huehelper_showSensor(sensorName)) ok = false;
  snprintf(sensorName, 32, "Daylight");
  if (!huehelper_showSensor(sensorName)) ok = false;
  snprintf(sensorName, 32, "Licht Sensor Theke");
  if (!huehelper_showSensor(sensorName)) ok = false;
  return ok;
}


void kino_showTicker(const char* deviceName, bool isOk) {
  if (!isOk) {
    Serial.print(F("Ticker "));
    Serial.print(deviceName);
    Serial.print(F(" failed!"));
    KinoAPI::showMemory();
  }
}


bool kino_showProperties(String* p, uint8_t n) {
  const char* deviceName = p[0].c_str();
  const KinoPropertyInfo* prop = nullptr;
  size_t propCount;
  KinoVariant v;
  
  Serial.println(deviceName);
  
  KinoError e = KinoAPI::getPropertyCount(deviceName, propCount);
  Serial.print(propCount); Serial.println(F(" unterstützte Properties: "));
  if (!showError(e)) return false;

  for (int pc=0; pc < propCount; pc++) {
    e = KinoAPI::getPropertyInfo(deviceName, pc, prop);
    if (!showError(e)) return false;
    bool hasLabel = KinoAPI::hasLabel(prop);
    bool hasValue = KinoAPI::hasValue(prop);
    bool hasQuery = KinoAPI::hasQuery(prop);
    bool hasParam = KinoAPI::hasParam(prop);
    bool isWritable = KinoAPI::isWritable(prop);
    
    Serial.print(F("\t")); Serial.println(prop->label);
    if (hasValue) {
      e = KinoAPI::getProperty(deviceName, prop->key, v);
      if (!showError(e)) return false;
      Serial.print(F("\t\tValue: "));
      Serial.print(v.toString());
    }
    Serial.print(F("\t\tSchreibbar: "));
    if (isWritable) {
      Serial.print(F("JA, per KinoAPI::set(")); Serial.print(deviceName); Serial.print(F(", ")); Serial.print(prop->key); Serial.println(F(", [newVal])"));
    } else {
      Serial.println(F("NEIN"));
    }
    Serial.print(F("\t\tOptionen : "));
    if (hasQuery) {
      uint16_t optionCount;
      e = KinoAPI::getQueryCount(deviceName, prop->key, optionCount);
      if (!showError(e)) return false;
      Serial.print("JA, "); Serial.print(optionCount); Serial.println(" Stück:");
      if (optionCount > 20) {
        Serial.print(F("(mehr als 20 Optionen. Volle Liste per kino.list("));
        Serial.print(deviceName);
        Serial.print(F(","));
        Serial.print(prop->key);
        Serial.println(F(")"));
        optionCount = 20;
      }
      constexpr size_t pathLen = 128; 
      char path[pathLen];
      constexpr size_t optIdLen = 32;
      char optId[optIdLen];
      for (int optIndex=0; optIndex < optionCount; optIndex++) {
        // Option zeigen
        e = KinoAPI::getQuery(deviceName, prop->key, optIndex, v);
        if (!showError(e)) return false;
        //String optId = v.toString();
        strncpy(optId, v.c_str(), optIdLen);
        optId[optIdLen-1] = '\0';
        Serial.print(F("\t\t\t")); Serial.print(optId);
        if (hasLabel) {
          // Label zeigen
          /*String path = prop->key;
          path += "/";
          path += optId;
          path += "/label";*/
          snprintf(path, pathLen, "%s/%s/label", prop->key, optId);
          path[pathLen-1] = '\0';
          //e = KinoAPI::getProperty(deviceName, path.c_str(),v);
          e = KinoAPI::getProperty(deviceName, path,v);
          if (!showError(e)) return false;
          Serial.print(F(" => "));
          //Serial.print(v.toString());
          Serial.print(v.c_str());
        } else {
          //Serial.print();
        }
        if (hasParam) {
          /*String basePath = prop->key;
          basePath += "/";
          basePath += optId;
          basePath += "/param";*/
          snprintf(path, pathLen, "%s/%s/param", prop->key, optId);
          uint16_t paramCount;
          //e = KinoAPI::getQueryCount(deviceName, basePath.c_str(), paramCount);
          e = KinoAPI::getQueryCount(deviceName, path, paramCount);
          if (!showError(e)) return false;
          Serial.print(F("   "));
          Serial.print(paramCount);
          Serial.print(F(" Params: ["));
          for (int paramIndex=0; paramIndex < paramCount; paramIndex++) {
            // Parameter Label anzeigen
            //String path = basePath + "/" + String(paramIndex) + "/label";
            //e = KinoAPI::getProperty(deviceName, path.c_str(), v);
            snprintf(path, pathLen, "%s/%s/param/%d/label", prop->key, optId, paramIndex);
            path[pathLen-1] = '\0';
            e = KinoAPI::getProperty(deviceName, path, v);
            if (!showError(e)) return false;
            Serial.print(v.toString());
            // Parameter Value anzeigen
            //path = basePath + "/" + String(paramIndex);
            //e = KinoAPI::getProperty(deviceName, path.c_str(), v);
            snprintf(path, pathLen, "%s/%s/param/%d", prop->key, optId, paramIndex);
            path[pathLen-1] = '\0';
            e = KinoAPI::getProperty(deviceName, path, v);
            if (!showError(e)) return false;
            //String getsetPath = v.toString();
            char getsetpath[64];
            strncpy(getsetpath, v.c_str(), 64);
            getsetpath[63] = '\0';
            //e = KinoAPI::getProperty(deviceName, getsetPath.c_str(), v);
            e = KinoAPI::getProperty(deviceName, getsetpath, v);
            if (!showError(e)) return false;
            Serial.print(F("=")); Serial.print(v.toString());
            if (paramIndex+1 < paramCount) Serial.print(F(", "));
          }
          Serial.println(F("]"));
        } else {
          Serial.println(F("   Params: keine"));
        }
      }
    } else {
      Serial.println(F("NEIN"));
    }
  }
  return true;
}

/*
bool kino_showProperties(String* p, uint8_t n) {
    const char* deviceName = p[0].c_str();
    const KinoPropertyInfo* prop = nullptr;
    size_t propCount;
    KinoVariant v;

    KinoError e = KinoAPI::getPropertyCount(deviceName, propCount);
    if (!showError(e)) return false;

    // Spaltenbreiten
    constexpr int W_LABEL   = 16;
    constexpr int W_APIKEY  = 12;
    constexpr int W_TYPE    = 10;
    constexpr int W_VALUE   = 50;
    constexpr int W_WRITE   = 6;
    constexpr int W_OPTIONS = 26;

    constexpr int TABLE_WIDTH =
        W_LABEL + 1 +
        W_APIKEY + 1 +
        W_TYPE + 1 +
        W_VALUE + 1 +
        W_WRITE + 1 +
        W_OPTIONS;

    auto printSeparator = []() {
        for (int i = 0; i < TABLE_WIDTH; i++) Serial.print('-');
        Serial.println();
    };

    // Header
    Serial.println();
    Serial.printf("%-*s %-*s %-*s %-*s %-*s %-*s\n",
                  W_LABEL, "Label",
                  W_APIKEY, "API-Key",
                  W_TYPE, "Type",
                  W_VALUE, "Value",
                  W_WRITE, "Write",
                  W_OPTIONS, "Options");
    printSeparator();

    for (size_t i = 0; i < propCount; i++) {
        e = KinoAPI::getPropertyInfo(deviceName, i, prop);
        if (!showError(e)) return false;

        // ---- Type ----
        String typeStr;
        switch (prop->type) {
            case KinoPropertyType::Bool:   typeStr = "BOOL";   break;
            case KinoPropertyType::Int:    typeStr = "INT";    break;
            case KinoPropertyType::String: typeStr = "STRING"; break;
            case KinoPropertyType::Color:  typeStr = "COLOR";  break;
            default:                       typeStr = "UNKNOWN"; break;
        }

        // ---- Value ----
        String val;
        String valCustom;

        if (prop->flags & KinoPropertyFlags::Prop_Read) {
            if (showError(KinoAPI::getProperty(deviceName, prop->key, v))) {
                val = v.toString();
            }
            if (val.isEmpty()) val = "(leer)";

            if (prop->key_custom) {
                KinoVariant vCustom;
                if (showError(KinoAPI::getProperty(deviceName, prop->key_custom, vCustom))) {
                    valCustom = vCustom.toString();
                }
            }
        } else {
            val = "(nicht lesbar)";
        }

        // ---- Min / Max (eigene Zeile) ----
        String range;
        if (prop->minValue.has_value() || prop->maxValue.has_value()) {
            range = "[";
            if (prop->minValue.has_value()) {
                range += " " + String(prop->minValue.value());
            }
            if (prop->maxValue.has_value()) {
                if (prop->minValue.has_value()) range += " ";
                range += " .. " + String(prop->maxValue.value());
            }
            range += "]";
        }

        // ---- Writable ----
        String writable = (prop->flags & KinoPropertyFlags::Prop_Write) ? "Yes" : "No";

        // ---- Options (untereinander) ----
        std::vector<String> options;
        if (prop->flags & KinoPropertyFlags::Prop_Query) {
            uint16_t qCount;
            if (showError(KinoAPI::getQueryCount(deviceName, prop->key, qCount))) {
                int qCountLimit = qCount;
                if (qCountLimit > 20) qCountLimit = 20;
                for (uint16_t q = 0; q < qCountLimit; q++) {
                    KinoVariant qVal, qCustom;
                    if (showError(KinoAPI::getQuery(deviceName, prop->key, q, qVal))) {
                        String opt = qVal.toString();
                        if (prop->key_custom &&
                            showError(KinoAPI::getQuery(deviceName, prop->key_custom, q, qCustom))) {
                            opt += " (" + qCustom.toString() + ")";
                        }
                        // limit nr of options to 30, to avoid memory overflow
                        if (options.size() < 20) options.push_back(opt);
                    }
                }
                if (qCount > qCountLimit) {
                  String opt = "(";
                  opt += String(qCount);
                  opt += " total, see kino.list( ";
                  opt += deviceName;
                  opt += ", ";
                  opt += prop->key;
                  opt += ")";
                  options.push_back(opt);
                }
            }
        }

        // ---- Helper: String umbrechen ----
        auto wrap = [](const String& s, int width) {
            std::vector<String> out;
            for (int i = 0; i < s.length(); i += width) {
                out.push_back(s.substring(i, i + width));
            }
            if (out.empty()) out.push_back("");
            return out;
        };

        auto labelLines   = wrap(prop->label, W_LABEL);
        auto apiKeyLines  = wrap(prop->key, W_APIKEY);
        auto typeLines    = wrap(typeStr, W_TYPE);
        auto valueLines   = wrap(val, W_VALUE);
        auto writeLines   = wrap(writable, W_WRITE);

        std::vector<String> optionLines;
        for (auto& o : options) {
            auto wrapped = wrap(o, W_OPTIONS);
            optionLines.insert(optionLines.end(), wrapped.begin(), wrapped.end());
        }

        size_t maxLines = std::max({
            labelLines.size(),
            apiKeyLines.size(),
            typeLines.size(),
            valueLines.size(),
            writeLines.size(),
            optionLines.size()
        });

        // ---- Hauptzeilen ----
        for (size_t l = 0; l < maxLines; l++) {
            Serial.printf("%-*s %-*s %-*s %-*s %-*s %-*s\n",
                W_LABEL,   l < labelLines.size()  ? labelLines[l].c_str()  : "",
                W_APIKEY,  l < apiKeyLines.size() ? apiKeyLines[l].c_str() : "",
                W_TYPE,    l < typeLines.size()   ? typeLines[l].c_str()   : "",
                W_VALUE,   l < valueLines.size()  ? valueLines[l].c_str()  : "",
                W_WRITE,   l < writeLines.size()  ? writeLines[l].c_str()  : "",
                W_OPTIONS, l < optionLines.size() ? optionLines[l].c_str() : ""
            );
        }

        // ---- Custom-Value-Zeile ----
        if (!valCustom.isEmpty()) {
            Serial.printf("%-*s %-*s %-*s %-*s %-*s %-*s\n",
                W_LABEL,   "",
                W_APIKEY,  prop->key_custom,
                W_TYPE,    "",
                W_VALUE,   valCustom.c_str(),
                W_WRITE,   "",
                W_OPTIONS, ""
            );
        }

        // ---- Min/Max-Zeile ----
        if (!range.isEmpty()) {
            Serial.printf("%-*s %-*s %-*s %-*s %-*s %-*s\n",
                W_LABEL,   "",
                W_APIKEY,  "",
                W_TYPE,    "",
                W_VALUE,   range.c_str(),
                W_WRITE,   "",
                W_OPTIONS, ""
            );
        }

        printSeparator();
    }

    return true;
}
*/
/*
bool kino_showProperties(String* p, uint8_t n) {
    const char* deviceName = p[0].c_str();
    const KinoPropertyInfo* prop = nullptr;
    size_t propCount;
    KinoVariant v;

    KinoError e = KinoAPI::getPropertyCount(deviceName, propCount);
    if (!showError(e)) return false;

    // Spaltenbreiten
    constexpr int W_LABEL   = 16;
    constexpr int W_APIKEY  = 12;
    constexpr int W_TYPE    = 10;
    constexpr int W_VALUE   = 50;
    constexpr int W_WRITE   = 6;
    constexpr int W_OPTIONS = 26;

    // Header
    Serial.printf("\n%-*s %-*s %-*s %-*s %-*s %-*s\n",
                  W_LABEL, "Label",
                  W_APIKEY, "API-Key",
                  W_TYPE, "Type",
                  W_VALUE, "Value",
                  W_WRITE, "Write",
                  W_OPTIONS, "Options");
    Serial.println(F("------------------------------------------------------------------------------------------------"));

    for (int i = 0; i < propCount; i++) {
        e = KinoAPI::getPropertyInfo(deviceName, i, prop);
        if (!showError(e)) return false;

        // Typ mit optional Min/Max
        String typ;
        switch (prop->type) {
            case KinoPropertyType::Bool: typ = "BOOL"; break;
            case KinoPropertyType::Int:
                typ = "INT";
                if (prop->minValue.has_value() && prop->maxValue.has_value()) {
                    typ += String(" [") + prop->minValue.value() + ".." + prop->maxValue.value() + "]";
                }
                break;
            case KinoPropertyType::String: typ = "STRING"; break;
            default: typ = "UNKNOWN"; break;
        }

        // Wert
        String val;
        String valCustom; // <-- HIER deklarieren, Scope für die ganze Schleife
        if (prop->flags & KinoPropertyFlags::Prop_Read) {
            if (showError(KinoAPI::getProperty(deviceName, prop->key, v))) val = v.toString();
            if (val.isEmpty()) val = "(leer)";

            // Custom-Wert extra Zeile vorbereiten
            //String valCustom;
            if (prop->key_custom) {
                KinoVariant v_custom;
                if (showError(KinoAPI::getProperty(deviceName, prop->key_custom, v_custom))) {
                    valCustom = v_custom.toString();
                }
            }
        } else {
            val = "(nicht lesbar)";
        }

        String writable = (prop->flags & KinoPropertyFlags::Prop_Write) ? "Yes" : "No";

        // Optionen: untereinander
        std::vector<String> opts;
        if (prop->flags & KinoPropertyFlags::Prop_Query) {
            uint16_t qCount;
            if (showError(KinoAPI::getQueryCount(deviceName, prop->key, qCount))) {
                for (int qc = 0; qc < qCount; qc++) {
                    KinoVariant q, q_custom;
                    if (showError(KinoAPI::getQuery(deviceName, prop->key, qc, q))) {
                        String opt = q.toString();
                        if (prop->key_custom && showError(KinoAPI::getQuery(deviceName, prop->key_custom, qc, q_custom))) {
                            opt += " (" + q_custom.toString() + ")";
                        }
                        opts.push_back(opt);
                    }
                }
            }
        }

        // Hilfsfunktion: langen Text auf mehrere Zeilen splitten
        auto splitString = [](const String& s, int width) -> std::vector<String> {
            std::vector<String> lines;
            int start = 0;
            while (start < s.length()) {
                lines.push_back(s.substring(start, start + width));
                start += width;
            }
            if (lines.empty()) lines.push_back("");
            return lines;
        };

        // Alle Spalten vorbereiten: Hauptzeile
        std::vector<String> labelLines = splitString(prop->label, W_LABEL);
        std::vector<String> apiKeyLines = splitString(prop->key, W_APIKEY);
        std::vector<String> typeLines = splitString(typ, W_TYPE);
        std::vector<String> valueLines = splitString(val, W_VALUE);
        std::vector<String> writeLines = splitString(writable, W_WRITE);
        std::vector<String> optionLines; // Options noch untereinander
        for (auto &o : opts) {
            std::vector<String> wrapped = splitString(o, W_OPTIONS);
            optionLines.insert(optionLines.end(), wrapped.begin(), wrapped.end());
        }

        // Maximale Zeilenanzahl bestimmen
        size_t maxLines = labelLines.size();
        maxLines = std::max(maxLines, apiKeyLines.size());
        maxLines = std::max(maxLines, typeLines.size());
        maxLines = std::max(maxLines, valueLines.size());
        maxLines = std::max(maxLines, writeLines.size());
        maxLines = std::max(maxLines, optionLines.size());

        // Zeilen ausgeben
        for (size_t line = 0; line < maxLines; line++) {
            Serial.printf("%-*s %-*s %-*s %-*s %-*s %-*s\n",
                          W_LABEL, line < labelLines.size() ? labelLines[line].c_str() : "",
                          W_APIKEY, line < apiKeyLines.size() ? apiKeyLines[line].c_str() : "",
                          W_TYPE, line < typeLines.size() ? typeLines[line].c_str() : "",
                          W_VALUE, line < valueLines.size() ? valueLines[line].c_str() : "",
                          W_WRITE, line < writeLines.size() ? writeLines[line].c_str() : "",
                          W_OPTIONS, line < optionLines.size() ? optionLines[line].c_str() : "");
        }

        // Falls key_custom vorhanden: eigene Zeile
        if (!valCustom.isEmpty()) {
            Serial.printf("%-*s %-*s %-*s %-*s %-*s %-*s\n",
                          W_LABEL, "",
                          W_APIKEY, prop->key_custom,
                          W_TYPE, "",
                          W_VALUE, valCustom.c_str(),
                          W_WRITE, "",
                          W_OPTIONS, "");
        }

        Serial.println(F("------------------------------------------------------------------------------------------------"));
    }

    return true;
}

*/



/*
bool kino_showProperties(String* p, uint8_t n) {
  // angeblich bessere tabellarische Ansicht
    const char* deviceName = p[0].c_str();
    const KinoPropertyInfo* prop = nullptr;
    size_t propCount;
    KinoVariant v;

    KinoError e = KinoAPI::getPropertyCount(deviceName, propCount);
    if (!showError(e)) return false;

    // Spaltenbreiten (kann man nach Bedarf anpassen)
    constexpr int W_LABEL = 15;
    constexpr int W_TYPE  = 12;
    constexpr int W_VALUE = 20;
    constexpr int W_WRIT  = 8;
    constexpr int W_OPT   = 40;

    // Header
    Serial.printf("\n%-*s %-*s %-*s %-*s %-*s\n",
                  W_LABEL, "Label",
                  W_TYPE,  "Type",
                  W_VALUE, "Value",
                  W_WRIT,  "Write",
                  W_OPT,   "Options");
    Serial.println(F("-------------------------------------------------------------------------------"));

    for (int i = 0; i < propCount; i++) {
        e = KinoAPI::getPropertyInfo(deviceName, i, prop);
        if (!showError(e)) return false;

        // Typ mit optionalen Min/Max
        String typ;
        switch (prop->type) {
            case KinoPropertyType::Bool: typ = "BOOL"; break;
            case KinoPropertyType::Int:
                typ = "INT";
                if (prop->minValue.has_value() && prop->maxValue.has_value()) {
                    typ += String(" [") + prop->minValue.value() + ".." + prop->maxValue.value() + "]";
                }
                break;
            case KinoPropertyType::String: typ = "STRING"; break;
            default: typ = "UNKNOWN"; break;
        }

        // Wert
        String val = "(leer)";
        if (prop->flags & KinoPropertyFlags::Prop_Read) {
            if (showError(KinoAPI::getProperty(deviceName, prop->key, v))) val = v.toString();
            if (val.isEmpty()) val = "(leer)";

            if (prop->key_custom) {
                KinoVariant v_custom;
                if (showError(KinoAPI::getProperty(deviceName, prop->key_custom, v_custom))) {
                    val += " (" + v_custom.toString() + ")";
                }
            }
        }

        String writable = (prop->flags & KinoPropertyFlags::Prop_Write) ? "Yes" : "No";

        // Optionen
        String opts;
        if (prop->flags & KinoPropertyFlags::Prop_Query) {
            uint16_t qCount;
            if (showError(KinoAPI::getQueryCount(deviceName, prop->key, qCount))) {
                for (int qc = 0; qc < qCount; qc++) {
                    KinoVariant q, q_custom;
                    if (showError(KinoAPI::getQuery(deviceName, prop->key, qc, q))) {
                        String opt = q.toString();
                        if (prop->key_custom && showError(KinoAPI::getQuery(deviceName, prop->key_custom, qc, q_custom))) {
                            opt += " (" + q_custom.toString() + ")";
                        }
                        if (!opts.isEmpty()) opts += ", ";
                        opts += opt;
                    }
                }
            }
        }

        // Zeile ausgeben
        Serial.printf("%-*s %-*s %-*s %-*s %-*s\n",
                      W_LABEL, prop->label,
                      W_TYPE,  typ.c_str(),
                      W_VALUE, val.c_str(),
                      W_WRIT,  writable.c_str(),
                      W_OPT,   opts.c_str());
    }

    return true;
}
*/

/*
bool kino_showProperties(String* p, uint8_t n) {
  // Kompaktere Ansicht
    const char* deviceName = p[0].c_str();
    const KinoPropertyInfo* prop = nullptr;
    size_t propCount;
    KinoVariant v;

    KinoError e = KinoAPI::getPropertyCount(deviceName, propCount);
    if (!showError(e)) return false;

    Serial.println("\nLabel\tType\tValue\tWritable\tOptions");

    for (int i = 0; i < propCount; i++) {
        e = KinoAPI::getPropertyInfo(deviceName, i, prop);
        if (!showError(e)) return false;

        String typ;
        switch (prop->type) {
            case KinoPropertyType::Bool: typ = "BOOL"; break;
            case KinoPropertyType::Int: 
                typ = "INT"; 
                if (prop->minValue.has_value() && prop->maxValue.has_value()) {
                    typ += String(" [") + prop->minValue.value() + ".." + prop->maxValue.value() + "]";
                }
                break;
            case KinoPropertyType::String: typ = "STRING"; break;
            default: typ = "UNKNOWN"; break;
        }

        String val = "(leer)";
        if (prop->flags & KinoPropertyFlags::Prop_Read) {
            e = KinoAPI::getProperty(deviceName, prop->key, v);
            if (showError(e)) val = v.toString();
            if (val.isEmpty()) val = "(leer)";

            if (prop->key_custom) {
                KinoVariant v_custom;
                if (showError(KinoAPI::getProperty(deviceName, prop->key_custom, v_custom))) {
                    val += " (" + v_custom.toString() + ")";
                }
            }
        }

        String writable = (prop->flags & KinoPropertyFlags::Prop_Write) ? "Yes" : "No";

        // Optionen
        String opts;
        if (prop->flags & KinoPropertyFlags::Prop_Query) {
            uint16_t qCount;
            if (showError(KinoAPI::getQueryCount(deviceName, prop->key, qCount))) {
                for (int qc = 0; qc < qCount; qc++) {
                    KinoVariant q, q_custom;
                    if (showError(KinoAPI::getQuery(deviceName, prop->key, qc, q))) {
                        String opt = q.toString();
                        if (prop->key_custom && showError(KinoAPI::getQuery(deviceName, prop->key_custom, qc, q_custom))) {
                            opt += "(" + q_custom.toString() + ")";
                        }
                        if (!opts.isEmpty()) opts += ", ";
                        opts += opt;
                    }
                }
            }
        }

        Serial.printf("%s\t%s\t%s\t%s\t%s\n",
                      prop->label, typ.c_str(), val.c_str(), writable.c_str(), opts.c_str());
    }

    return true;
}

*/

/*
bool kino_showProperties(String* p, uint8_t n) {
    const char* deviceName = p[0].c_str();
    const KinoPropertyInfo* prop = nullptr;
    size_t propCount;
    KinoVariant v;
    
    KinoError e = KinoAPI::getPropertyCount(deviceName, propCount);
    if (!showError(e)) return false;

    for (int i = 0; i < propCount; i++) {
        e = KinoAPI::getPropertyInfo(deviceName, i, prop);
        if (!showError(e)) return false;

        // --- Überschrift ---
        Serial.print("\n\t"); Serial.print(prop->label);
        Serial.print(" => API-Key: "); Serial.print(prop->key);

        // --- Typ + optional min/max ---
        switch (prop->type) {
            case KinoPropertyType::Bool: 
                Serial.print(", Type: BOOL"); 
                break;
            case KinoPropertyType::Int:
                Serial.print(", Type: INTEGER");
                if (prop->minValue.has_value() && prop->maxValue.has_value()) {
                    Serial.printf(", Bereich: [%d..%d]", prop->minValue.value(), prop->maxValue.value());
                }
                break;
            case KinoPropertyType::String:
                Serial.print(", Type: STRING");
                break;
            default:
                Serial.print(", Type: unbekannt");
                continue;
        }

        Serial.println();

        // --- Aktueller Wert ---
        if (prop->flags & KinoPropertyFlags::Prop_Read) {
            e = KinoAPI::getProperty(deviceName, prop->key, v);
            String val = showError(e) ? v.toString() : "(Fehler beim Lesen)";
            if (val.isEmpty()) val = "(leer)";

            // Optional key_custom
            if (prop->key_custom) {
                KinoVariant v_custom;
                KinoError f = KinoAPI::getProperty(deviceName, prop->key_custom, v_custom);
                if (showError(f)) {
                    val += " (" + v_custom.toString() + ")";
                }
            }

            Serial.print("\taktueller Wert: "); Serial.println(val);
        }

        // --- Flags ---
        if (prop->flags & KinoPropertyFlags::Prop_Write) Serial.println("\tSchreibbar");
        if (prop->flags & KinoPropertyFlags::Prop_Internal) Serial.println("\tNur für interne Zwecke");
        
        // --- Query-Optionen ---
        if (prop->flags & KinoPropertyFlags::Prop_Query) {
            Serial.println("\tOptionen:");
            if (prop->key_custom) {
                Serial.print("\t(Anzeigewerte in Klammern via API-Key: ");
                Serial.print(prop->key_custom);
                Serial.println(")");
            }

            uint16_t qCount;
            e = KinoAPI::getQueryCount(deviceName, prop->key, qCount);
            if (!showError(e)) {
                Serial.println("\tFEHLER: Konnte Anzahl Optionen nicht auslesen");
                continue;
            }

            for (int qc = 0; qc < qCount; qc++) {
                KinoVariant q, q_custom;
                if (showError(KinoAPI::getQuery(deviceName, prop->key, qc, q))) {
                    String opt = q.toString();
                    if (prop->key_custom && showError(KinoAPI::getQuery(deviceName, prop->key_custom, qc, q_custom))) {
                        opt += " (" + q_custom.toString() + ")";
                    }
                    Serial.print("\t\t"); Serial.println(opt);
                }
            }
        }
    }

    return true;
}
*/


/*
bool kino_showProperties(String* p, uint8_t n) {
  const char* deviceName = p[0].c_str();
  const KinoPropertyInfo* prop = nullptr;;
  size_t propCount;
  KinoVariant v;
  KinoError e = KinoAPI::getPropertyCount(deviceName, propCount);
  if (!showError(e)) return false;  // showError(e) is true, if e == KinoError::OK
  for (int i=0; i<propCount; i++) {
    e = KinoAPI::getPropertyInfo(deviceName, i, prop);
    if (!showError(e)) return false;
    Serial.print("\n\t"); Serial.print(prop->label);
    Serial.print(" => API-Key: "); Serial.print(prop->key);
    switch (prop->type) {
      case KinoPropertyType::Bool: {
        Serial.print(", Type: BOOL, ");
        break;
      }
      case KinoPropertyType::Int: {
        Serial.print(", Type: INTEGER, ");
        break;
      }
      case KinoPropertyType::String: {
        Serial.print(", Type: STRING, ");
        break;
      }
      default: {
        Serial.println("Type: unbekannt");
        continue; // next for i
        break;
      }
    }
    Serial.print("\n\t");
    if (prop->flags & KinoPropertyFlags::Prop_Read) {
      // property ist lesbar
      e = KinoAPI::getProperty(deviceName, prop->key, v);
      if (!showError(e)) { Serial.println("aktueller Wert aufgrund eines Fehlers nicht bekannt"); continue; }
      Serial.print("aktueller Wert: ");
      Serial.print(v.toString());
      if (prop->key_custom) {
        e = KinoAPI::getProperty(deviceName, prop->key_custom, v);
        if (showError(e)) { // Wenn e == KinoError::OK
          Serial.print(" (");
          Serial.print(v.toString());
          Serial.print("), gelesen aus API-Key: ");
          Serial.print(prop->key_custom);
        }
      }
    }
    if (prop->flags & KinoPropertyFlags::Prop_Write) {
      Serial.println(", schreibbar");
      if (prop->minValue.has_value()) {
        Serial.print("\t\tMindestens "); Serial.println(prop->minValue.value());
      }
      if (prop->maxValue.has_value()) {
        Serial.print("\t\tMaximal    "); Serial.println(prop->maxValue.value());
      }
    }
    Serial.println();
    if (prop->flags & KinoPropertyFlags::Prop_Internal) {
      Serial.println("\t\tDer Wert ist nur für interne Zwecke und nicht für UI bestimmt");
    }
    if (prop->flags & KinoPropertyFlags::Prop_Query) {
      Serial.println("\t\tBeim Schreiben stehen folgende Optionen zur Auswahl:");
      if (prop->key_custom) {
        Serial.print("\t\t(Anzeigewerte in Klammern können ausgelesen werden per API-Key: ");
        Serial.print(prop->key_custom);
        Serial.println(" )");
      }
      uint16_t qCount;
      KinoVariant q;
      KinoVariant q_custom;
      e = KinoAPI::getQueryCount(deviceName, prop->key, qCount);
      if (!showError(e)) {
        Serial.println("\t\tFEHLER: Konnte die Anzahl der Optionen nicht auslesen");
        continue; // next i
      }
      for (int qc=0; qc<qCount; qc++) {
        e = KinoAPI::getQuery(deviceName, prop->key, qc, q);
        if (showError(e)) { // Diesmal: wenn e == KinoError::OK
          Serial.print("\t\t");
          Serial.print(q.toString());
          if (prop->key_custom) {
            KinoError f = KinoAPI::getQuery(deviceName, prop->key_custom, qc, q_custom);
            if (showError(f)) { // Auch hier: nur, wenn f == KinoError::OK
              Serial.print(" (");
              Serial.print(q_custom.toString());
              Serial.print(")");
            }
          } else {
            //Serial.print(" , für Anzeige: genauso");
          }
          Serial.println();
        }
      }
    }
  }
  return true;
}*/


// MAKROS

// helper function for showing macro errors
void showMacroErrors() {
  size_t errCount = KinoAPI::getMacroErrorCount();
  //Serial.print(KinoAPI::getCurrentMacroName());
  char mName[32];
  KinoAPI::getCurrentMacroName(mName, sizeof(mName));
  Serial.print(mName);
  Serial.print(F(" : "));
  Serial.print(errCount); Serial.println(F(" Errors:"));
  for (size_t i=0; i<errCount; i++) {
    auto& e = KinoAPI::getMacroError(i);
    Serial.printf(
      " #%d cmd=%s msg=%s\n",
      e.index,
      e.cmd.c_str(),
      e.message.c_str()
    );
  }
}

// helper function for showing actions inside a macro
bool showMacroListing(const String& macroName) {
  size_t mlcount = KinoAPI::getMacroLineCount(macroName.c_str());
  if (mlcount == 0) {
    Serial.println(F("Der Makro hat keinen Inhalt"));
    return true;
  }
  char l[256];
  for (size_t i=0; i<mlcount; i++) {
    KinoAPI::getMacroLineByIndex(macroName.c_str(), i, l, sizeof(l));
    Serial.println(l);
  }
  return true;
}

bool kino_listMacros(String* p, uint8_t n) {
  Serial.println(F("Gespeicherte Makros:"));
  size_t mc;
  if (KinoAPI::getMacroCount(mc) != KinoError::OK) {
    Serial.println(F("Konnte Makros nicht lesen"));
    return false;
  }
  Serial.print(mc); Serial.println(F("Makros insgesamt:"));
  KinoVariant mName;
  for (size_t i = 0; i < mc; i++) {
    if (KinoAPI::getMacroNameByIndex(i, mName) != KinoError::OK) {
      Serial.print(F("\tFehler beim Auslesen von Makro Nr ")); Serial.println(i);
      return false;
    }
    Serial.print(F("\t")); Serial.println(mName.c_str());
  }
  return true;
}

bool kino_showMacro(String* p, uint8_t n) {
  Serial.print(F("actions in Makro ")); Serial.println(p[0]);
  return showMacroListing(p[0]);
}



bool kino_addCommandToMacro(String* p, uint8_t n) {
  bool ok = false;
  if (n==3) {
    ok = KinoAPI::addMacroCommand(p[0].c_str(), p[1].toInt(), p[2].c_str());
  } else if (n == 6) {
    KinoVariant val = prepareForJson(p[5]);
    ok = KinoAPI::addMacroCommand(p[0].c_str(), p[1].toInt(), p[2].c_str(), p[3].c_str(), p[4].c_str(), val);
  }
  if (!ok) {
    Serial.print(F("got ")); Serial.print(n); Serial.println(F("parameters"));
    showMacroErrors();
    //return false;
  }
  return showMacroListing(p[0]);
}

bool kino_deleteCommandFromMacro(String* p, uint8_t n) {
  Serial.print(F("\tLösche Zeile "));
  Serial.print(p[1].toInt());
  Serial.print(F(" aus Makro "));
  Serial.println(p[0]);
  bool ok = KinoAPI::deleteMacroCommand(p[0].c_str(), p[1].toInt());
  if (!ok) {
    showMacroErrors();
    return false;
  }
  return showMacroListing(p[0]);
}

bool kino_updateCommandInMacro(String* p, uint8_t n) {
  bool ok = false;
  if (n == 3) {
    bool ok = KinoAPI::updateMacroCommand(p[0].c_str(), p[1].toInt(), p[2].c_str());
  } else if (n == 6) {
    KinoVariant val = prepareForJson(p[5].c_str());
    ok = KinoAPI::updateMacroCommand(p[0].c_str(), p[1].toInt(), p[2].c_str(), p[3].c_str(), p[4].c_str(), val);
  }
  if (!ok) {
    showMacroErrors();
    //return false;
  }
  return showMacroListing(p[0]);
}

bool kino_createMacro(String*p, uint8_t n) {
  if (!KinoAPI::createMacro(p[0].c_str())) {
    Serial.println(F("could not create macro file"));
    showMacroErrors();
    return false;
  }
  Serial.println(F("Macro file created successfully"));
  bool ok = true;
  if (n==2) {
    String args[3];
    args[0] = p[0];      // macro name
    args[1] = "1";       // index
    args[2] = p[1];      // json command
    ok = kino_addCommandToMacro(args,3);
    if (ok) {
      Serial.println(F("first command successfully inserted"));
    } else {
      Serial.println(F("an error occured while inserting the first command"));
      showMacroErrors();
    }
  }
  return showMacroListing(p[0].c_str());
}


bool kino_addOrUpdateMacro(String* p, uint8_t n) {
  bool ok = KinoAPI::addOrUpdateMacro(p[0].c_str());
  if (!ok) showMacroErrors();
  return ok;
}

bool kino_deleteMacro(String*p, uint8_t n) {
  bool ok = KinoAPI::deleteMacro(p[0].c_str());
  if (!ok) showMacroErrors();
  return ok;
}

void serial_macroFinished(bool success) {
  if (!success) {
    showMacroErrors();
    return;
  }
  Serial.print(F("Makro \""));
  char mName[32];
  KinoAPI::getCurrentMacroName(mName, sizeof(mName));
  Serial.print(mName);
  Serial.println(F("\" sauber abgearbeitet\n"));
}

bool kino_executeMacro(String* p, uint8_t n) {
  //return KinoAPI::executeMacro(p[0]);
  if (!KinoAPI::executeMacro(p[0].c_str(), serial_macroFinished)) {
    // Diese Fehler werden direkt beim Starten gefangen:
    showMacroErrors();
  }
  return true;
}

bool kino_testMacro(String* p, uint8_t n) {
  if (!KinoAPI::testMacro(p[0].c_str(), serial_macroFinished)) {
    // Diese Fehler werden direkt beim Starten gefangen:
    showMacroErrors();
  }
  return true;
}

bool kino_init(String* p, uint8_t n) {
  Serial.println(F("Initialisiere Geräte:"));
  bool ok = true;
  if (n==0) {
    for (auto& devName : KinoAPI::getDeviceNames()) {
      KinoError err = KinoAPI::initDevice(devName.c_str());
      if (err == KinoError::OK) {
        KinoVariant v;
        KinoAPI::getDeviceType(devName.c_str(),v);
        Serial.printf("\t%s wurde initialisiert als %s \n",devName.c_str(),v.s);
      } else {
        Serial.printf("\t%s konnte nicht initialisiert werden:\n",devName.c_str());
        showError(err);
        ok = false;
      }
    }
    return ok;
  }
  // mindestens ein Gerät wurde namentlich genannt
  for (int i=0; i<n; i++) {
    KinoError err = KinoAPI::initDevice(p[i].c_str());
    if (err == KinoError::OK) {
      KinoVariant v;
      KinoAPI::getDeviceType(p[i].c_str(),v);
      Serial.printf("\t%s wurde initialisiert als %s \n",p[i].c_str(),v.s);
    } else {
      Serial.printf("\t%s konnte nicht initialisiert werden:\n",p[i].c_str());
      showError(err);
      ok = false;
    }
  }
  return ok;
}



bool kinoSet(String *p, uint8_t n) {
  // p[0] = device name
  // p[1] = property
  // p[2] = value
  // p[3] = value type
  
  KinoVariant val;
  int varType = determineType(p[2].c_str());  // 1 = bool, 2 = int, 3 = float, 4 = string, 5 = RGBColor
  if (varType == 1)   val = KinoVariant::fromBool(toBool(p[2]));
  if (varType == 2)   val = KinoVariant::fromInt(p[2].toInt());
  if (varType == 3)   val = KinoVariant::fromFloat(p[2].toFloat());
  if (varType == 4)   val = KinoVariant::fromString(p[2].c_str());
  if (varType == 5) {
    int r,g,b;
    int found = sscanf(p[2].c_str(), " [ %d , %d , %d ] ", &r, &g, &b);
    val = KinoVariant::fromColor(r,g,b);
  }
  if (val.type == KinoVariant::NONE) {  // val wurde nicht gesetzt
    Serial.println("Unknown value type");
    return false;
  }
  KinoError e = KinoAPI::setProperty(p[0].c_str(), p[1].c_str(), val);
  showError(e);
  e = KinoAPI::commit(p[0].c_str());
  return showError(e);
}

bool kinoGet(String* p, uint8_t n) {
  const char* deviceName = p[0].c_str();
  const char* prop = p[1].c_str();
  KinoVariant val;
  KinoError e = KinoAPI::getProperty(deviceName, prop, val);
  if (e != KinoError::OK) return showError(e);
  Serial.print(p[0]); Serial.print(F(".")); Serial.print(p[1]);
  Serial.print(" = ");
  switch (val.type) {
    case KinoVariant::NONE :
      Serial.println("EMPTY");
      break;
    case KinoVariant::BOOL :
      Serial.print("(bool) "); Serial.println(val.b ? F("true") : F("false"));
      break;
    case KinoVariant::INT : 
      Serial.print("(int) "); Serial.println(val.i);
      break;
    case KinoVariant::FLOAT : 
      Serial.print("(float) "); Serial.println(val.f);
      break;
    case KinoVariant::STRING :
      Serial.print("(string) \""); Serial.print(val.s); Serial.println(F("\""));
      break;
    case KinoVariant::RGB_COLOR :
      Serial.printf("(RGBColor) %i , %i , %i", val.color.r, val.color.g, val.color.b);
      Serial.println();
      break;
    default :
      Serial.println(F("unknown KinoVariant type"));
      break;
  }
  return showError(e);;
}

bool kino_list(String*p, uint8_t n) {
  const char* devicename = p[0].c_str();
  const char* listname = p[1].c_str();
  uint16_t count;
  KinoVariant value;
  KinoVariant label;
  KinoError e;
  char displayList[64];
  
  // Hier Properties abfragen, ob es einen "verwandten" query für die Anzeigenamen gibt
  size_t propCount = 0;
  KinoAPI::getPropertyCount(devicename,propCount);
  const KinoPropertyInfo* prop = nullptr;
  //const char* displayList = nullptr;
  bool hasLabel = false;
  
  for (int i=0; i<propCount; i++) {
    KinoAPI::getPropertyInfo(devicename, i, prop);
    if (strcmp(prop->key,listname)==0) {
      //displayList = prop->key_custom;
      //if (prop->flags & KinoProperty::Prop_hasLabel) hasLabel = true;
      if (KinoAPI::hasLabel(prop)) hasLabel = true;
    }
  }
  e = KinoAPI::getQueryCount(devicename, listname, count);
  if (!showError(e)) return false;
  Serial.printf("%d Einträge gefunden:\n",count);
  for (int i=0; i<count; i++) {
    e = KinoAPI::getQuery(devicename, listname, i,value);
    switch(value.type) {
      case KinoVariant::BOOL :
        Serial.print(value.b ? F("(bool) true") : F("(bool) false"));
        break;
      case KinoVariant::INT :
        Serial.printf("(int) %i", value.i);
        break;
      case KinoVariant::FLOAT :
        Serial.printf("(float) %f", value.f);
        break;
      case KinoVariant::STRING :
        Serial.print(F("(string) \""));
        Serial.print(value.s);
        Serial.print(F("\""));
        break;
      case KinoVariant::RGB_COLOR :
        Serial.printf("(RGB) [%i , %i , %i]", value.color.r, value.color.g, value.color.b);
        break;
      default :
        Serial.println(F("unbekannter Datentyp"));
        break;
    }
    if (hasLabel) {
      //String displayList = listname;
      //displayList += "/" + String(v.toString()) + "/label";
      snprintf(displayList, sizeof(displayList), "%s/%s/label", listname, value.c_str());
      displayList[sizeof(displayList)-1] = '\0';
      //e = KinoAPI::getProperty(devicename, displayList.c_str(),v);
      e = KinoAPI::getProperty(devicename, displayList, label);
      if (showError(e)) { // wenn e == KinoError::OK
        Serial.print(" => ");
        Serial.println(label.c_str());
      } else {
        Serial.println();
      }
    } else {
      Serial.println();
    }
  }
  return true;
}
/*
bool kino_list(String*p, uint8_t n) {
  const char* devicename = p[0].c_str();
  const char* listname = p[1].c_str();
  uint16_t count;
  KinoVariant v;
  KinoError e;
  // Hier Properties abfragen, ob es einen "verwandten" query für die Anzeigenamen gibt
  size_t propCount = 0;
  KinoAPI::getPropertyCount(devicename,propCount);
  const KinoPropertyInfo* prop = nullptr;
  const char* displayList = nullptr;
  
  for (int i=0; i<propCount; i++) {
    KinoAPI::getPropertyInfo(devicename, i, prop);
    if (strcmp(prop->key,listname)==0) {
      displayList = prop->key_custom;
    }
  }
  e = KinoAPI::getQueryCount(devicename, listname, count);
  if (!showError(e)) return false;
  Serial.printf("%d Einträge gefunden:\n",count);
  for (int i=0; i<count; i++) {
    e = KinoAPI::getQuery(devicename, listname, i,v);
    switch(v.type) {
      case KinoVariant::BOOL :
        Serial.print(v.b ? "(bool) true" : "(bool) false");
        break;
      case KinoVariant::INT :
        Serial.printf("(int) %i", v.i);
        break;
      case KinoVariant::FLOAT :
        Serial.printf("(float) %f", v.f);
        break;
      case KinoVariant::STRING :
        //Serial.printf("(string) \"%s\"", v.s);
        Serial.print("(string) \"");
        Serial.print(v.s);
        Serial.print("\"");
        break;
      case KinoVariant::RGB_COLOR :
        Serial.printf("(RGB) [%i , %i , %i]", v.color.r, v.color.g, v.color.b);
        break;
      default :
        Serial.println("unbekannter Datentyp");
        break;
    }
    if (displayList) {
      e = KinoAPI::getQuery(devicename, displayList, i,v);
      if (showError(e)) { // wenn e == KinoError::OK
        Serial.print(" => ");
        Serial.println(v.toString());
      } else {
        Serial.println();
      }
    }
  }
  return true;
}
*/
