#include "SerialCommandDispatcher.h"
#include "YamahaReceiver.h"
#include "OptomaBeamer.h"
#include "WLEDDevice.h"
#include "HyperionDevice.h"
#include "HueBridge.h"
#include "KinoAPI.h"

// ==== Externe Geräte (existieren im Sketch) ====

// Externe Geräte aus dem Sketch
extern YamahaReceiver* _yamaha;
extern WLEDDevice* _canvas;
extern WLEDDevice* _sound;
extern OptomaBeamer* _beamer;
extern HyperionDevice* _hyperion;
extern HueBridge* _hue;


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
  KinoVariant val;
  switch (typeNr) {
    case 1  : // bool
      val = KinoVariant::fromBool(toBool(p));
      break;
    case 2  : // int
      val = KinoVariant::fromInt(p.toInt());
      break;
    case 3  : // float
      val = KinoVariant::fromFloat(p.toFloat());
      break;
    case 4  : // string
      val = KinoVariant::fromString(p.c_str());
      break;
    case 5  : {// rgbcolor
      uint8_t r,g,b;
      int found = sscanf(p.c_str()," [ %d , %d , %d ] ", &r, &g, &b);
      if (found != 3) {
        Serial.println("error converting color to RGB, use json encoded command instead");
        return val;
      }
      val = KinoVariant::fromColor(r,g,b);
      break; }
    default :
      Serial.println("could not determine type of value, use json encoded command instead");
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
bool kino_addOrUpdateMacro(String* p, uint8_t n);
bool kino_executeMacro(String* p, uint8_t n);
bool kino_testMacro(String* p, uint8_t n);
bool kino_listMacros(String* p, uint8_t n);
bool kino_showMacro(String* p, uint8_t n);
bool kino_addCommandToMacro(String* p, uint8_t n);
bool kino_deleteCommandFromMacro(String* p, uint8_t n);
bool kino_updateCommandInMacro(String* p, uint8_t n);
bool kino_deleteMacro(String* p, uint8_t n);

bool kino_help(String* p, uint8_t n);
bool kino_init(String* p, uint8_t n);

bool kinoGet(String* p, uint8_t n);
bool kinoSet(String* p, uint8_t n);
bool kino_list(String*p, uint8_t n);

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
  {"macro",  "run",       1, kino_executeMacro,         "lädt Makro und führt es aus"},
  {"macro",  "test",      1, kino_testMacro,            "lädt Makro und führt Tests aus"},
  {"macro",  "list",      0, kino_listMacros,           "zeigt eine Liste aller gespeicherten Makros"},
  {"macro",  "show",      1, kino_showMacro,            "zeigt den Inhalt des angegebenen Makros"},
  {"macro",  "add",       1, kino_addOrUpdateMacro,     "speichert das gegebene JSON als Makro"},
  {"macro",  "addLine",   3, kino_addCommandToMacro,    "fügt ein Kommando zu einem Macro hinzu. Param1: Macroname, Param2: Zeilennummer, Param3: action als json"},
  {"macro",  "deleteLine",2, kino_deleteCommandFromMacro,"Löscht eine Zeile aus dem angegebenen Makro. Param1: Makroname, Param2: Zeilennummer"},
  {"macro",  "updateLine",3, kino_updateCommandInMacro, "ersetzt eine Zeile im angegebenen Makro. Param1: Makroname, Param2: Zeilennummer, Param3: neue action als json"},
  {"macro",  "delete",    1, kino_deleteMacro,          "löscht das Makro mit dem gegebenen Namen"},
  {"kino",   "help",      0, kino_help,                 "zeigt diese Hilfe"},
  {"kino",   "init",      0, kino_init,                 "(re-)initialisiert alle Geräte"},
  {"kino",   "get",       2, kinoGet,                   "gibt die Eigenschaft P2 vom Gerät P1 aus"},
  {"kino",   "set",       3, kinoSet,                   "set Eigenschaft P2 von Gerät P1 auf P3 vom Typ P4"},
  {"kino",   "list",      2, kino_list,                 "liest angegebene Liste P2 aus Gerät P1 und gibt die Einträge aus"},
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

bool kino_help(String* p, uint8_t n) {
  String cmp = (n == 0) ? "" : p[0];
  Serial.println(F("Folgende Kommandos sind verfügbar:\n-------------------------"));
  for (size_t i = 0; i<commandCount; ++i) {
    const CommandEntry& com = commandTable[i];
    if ((n==0) || (String(com.object) == cmp)) {
      Serial.print(com.object);
      Serial.print(".");
      Serial.print(com.method);
      Serial.println("()");
      Serial.print("\tParameter: ");
      Serial.println(com.expectedParams);
      Serial.print("\t");
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
  Serial.print("Power:  "); Serial.println((isOn) ? "An":"Aus");
  e = KinoAPI::getProperty("yamaha","volume",v);
  Serial.print("Volume: "); Serial.print((v.i)/10); Serial.println("dB");
  e = KinoAPI::getProperty("yamaha","mute",v);
  Serial.print("\tMute: "); Serial.println(v.b ?"An":"Aus");
  Serial.println("Tone:");
  e = KinoAPI::getProperty("yamaha","bass",v);
  Serial.print("\tBass     : "); Serial.println(v.i);
  e = KinoAPI::getProperty("yamaha","treble",v);
  Serial.print("\tTreble   : "); Serial.println(v.i);
  e = KinoAPI::getProperty("yamaha","swtrim",v);
  Serial.print("\tSW Trim  : "); Serial.println(v.i);
  e = KinoAPI::getProperty("yamaha","enhancer",v);
  Serial.print("\tEnhancer : "); Serial.println(v.b ?"An":"Aus");
  e = KinoAPI::getProperty("yamaha","straight",v);
  Serial.print("\tStraight : "); Serial.println(v.b ? F("An, DSP inaktiv") : F("Aus, DSP aktiv"));
  e = KinoAPI::getProperty("yamaha","dsp",v);
  Serial.print("\tDSP      : "); Serial.println(v.s);
  
  KinoVariant src;
  KinoVariant src_custom;
  e = KinoAPI::getProperty("yamaha","input",src);
  Serial.print("Source: "); Serial.print(src.s);
  e = KinoAPI::getProperty("yamaha","inputname",src_custom);
  if (strcmp(src.s,src_custom.s)!=0) {
    Serial.print(" ("); Serial.print(src_custom.s); Serial.println(" )");
  } else {
    Serial.println();
  }
  if ((isOn)&&(strcmp(src.s,"NET RADIO")==0)) {
    e = KinoAPI::getProperty("yamaha","station",v);
    Serial.print("\tStation: "); Serial.println(v.s);
    e = KinoAPI::getProperty("yamaha","song",v);
    Serial.print("\tSong   : "); Serial.println(v.s);
    e = KinoAPI::getProperty("yamaha","elapsed",v);
    Serial.print("\tElapsed: "); Serial.println(v.s);
  }
  return true;
}

bool beamer_info(String* p, uint8_t n) {
  KinoVariant v;
  KinoError e;
  e = _beamer->get("power",v);
  Serial.print("Power:  "); Serial.println(v.b ? "An" : "Aus");
  e = _beamer->get("input",v);
  Serial.print("Source: "); Serial.println(v.s);
  e = _beamer->get("uptime",v);
  Serial.print("Lampe:  "); Serial.println(v.i);
  Serial.println("\n\n");
  return true;
}

bool canvas_info(String* p, uint8_t n) {
  KinoError e;
  KinoVariant v;
  e = KinoAPI::getProperty("canvas","power",v);
  Serial.print("Power:      "); Serial.println(v.b ? "An" : "Aus");
  e = KinoAPI::getProperty("canvas","brightness",v);
  Serial.print("Brightness: "); Serial.println(v.i);
  e = KinoAPI::getProperty("canvas","live",v);
  Serial.print("Live Data:  "); Serial.print(v.b ? "incoming, " : "none, "); 
  e = KinoAPI::getProperty("canvas","override",v);
  Serial.println(v.b ? "ignoriert" : "bearbeitet");
  e = KinoAPI::getProperty("canvas","input",v);
  Serial.print("LD Source:  "); Serial.println(v.s);
  e = KinoAPI::getProperty("canvas","effect",v);
  Serial.print("Effekt:     "); Serial.println(v.i);
  if (v.i != 0) {
    e = KinoAPI::getProperty("canvas","speed",v);
    Serial.print("  Speed:    "); Serial.println(v.i);
    e = KinoAPI::getProperty("canvas","intensity",v);
    Serial.print("  Intensity:"); Serial.println(v.i);
  }
  Serial.println("Farben:");
  e = KinoAPI::getProperty("canvas","palette",v);
  Serial.print("\tPalette: "); Serial.println(v.i);
  e = KinoAPI::getProperty("canvas","FgColor",v);
  Serial.print("\tFg: "); Serial.print(v.color.r); Serial.print(" , "); Serial.print(v.color.g); Serial.print(" , "); Serial.println(v.color.b);
  e = KinoAPI::getProperty("canvas","BgColor",v);
  Serial.print("\tFg: "); Serial.print(v.color.r); Serial.print(" , "); Serial.print(v.color.g); Serial.print(" , "); Serial.println(v.color.b);
  e = KinoAPI::getProperty("canvas","FxColor",v);
  Serial.print("\tFg: "); Serial.print(v.color.r); Serial.print(" , "); Serial.print(v.color.g); Serial.print(" , "); Serial.println(v.color.b);
  return true;
}

bool sound_info(String* p, uint8_t n) {
  KinoVariant v;
  KinoError e;
  e = KinoAPI::getProperty("sound","power",v);
  Serial.print("Power:      "); Serial.println((v.b) ? "An" : "Aus");
  e = KinoAPI::getProperty("sound","brightness",v);
  Serial.print("Brightness: "); Serial.println(v.i);
  e = KinoAPI::getProperty("sound","effect",v);
  Serial.print("Effekt:     "); Serial.println(v.i);
  if (v.i != 0) {
    e = KinoAPI::getProperty("sound","speed",v);
    Serial.print("  Speed:    "); Serial.println(v.i);
    e = KinoAPI::getProperty("sound","intensity",v);
    Serial.print("  Intensity:"); Serial.println(v.i);
  }
  Serial.println("Farben:");
  e = KinoAPI::getProperty("sound","palette",v);
  Serial.print("\tPalette: "); Serial.println(v.i);
  e = KinoAPI::getProperty("sound","color1",v);
  Serial.print("\tFg: "); Serial.print(v.color.r); Serial.print(" , "); Serial.print(v.color.g); Serial.print(" , "); Serial.println(v.color.b);
  e = KinoAPI::getProperty("sound","color2",v);
  Serial.print("\tFg: "); Serial.print(v.color.r); Serial.print(" , "); Serial.print(v.color.g); Serial.print(" , "); Serial.println(v.color.b);
  e = KinoAPI::getProperty("sound","color3",v);
  Serial.print("\tFg: "); Serial.print(v.color.r); Serial.print(" , "); Serial.print(v.color.g); Serial.print(" , "); Serial.println(v.color.b);
  return true;
}

bool hyperion_info(String* p, uint8_t n) {
  KinoVariant v;
  KinoError e;
  Serial.print("Hyperion:\n  Power: ");
  e = KinoAPI::getProperty("hyperion","power",v);
  if (!showError(e)) return false;
  bool power = v.b;
  Serial.println(v.b ? "An":"Aus");
  e = KinoAPI::getProperty("hyperion","live",v);
  if (!showError(e)) return false;
  bool live = v.b;
  Serial.print("LEDs: ");
  Serial.println(live ? "An":"Aus");
  Serial.print("Broadcasting: ");
  Serial.println((power && live) ? "Ja":"Nein");
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
  Serial.print("\tPower: "); Serial.println(v.b ? "An":"Aus");
  snprintf(tmp,32,"lights/%s/brightness",p[0].c_str());
  e = KinoAPI::getProperty("hue",tmp,v);
  if (!showError(e)) return false;
  Serial.print("\tBri  : "); Serial.println(v.i);
  snprintf(tmp,32,"lights/%s/color",p[0].c_str());
  e = KinoAPI::getProperty("hue",tmp,v);
  if (e == KinoError::OutOfRange) {
    Serial.print("\tFarbe: nicht unterstützt\n");
  } else {
    Serial.print("\tFarbe: "); Serial.print(v.color.r); Serial.print(" , "); Serial.print(v.color.g); Serial.print(" , "); Serial.println(v.color.b);
  }
  snprintf(tmp,32,"lights/%s/ct",p[0].c_str());
  e = KinoAPI::getProperty("hue",tmp,v);
  if (e == KinoError::OutOfRange) {
    Serial.print("\tCT   : nicht unterstützt\n");
  } else {
    Serial.print("\tCT   : "); Serial.println(v.i);
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
      Serial.print("occured in getQuery("); Serial.print(queryName); Serial.println(")");
      return false;
    }
    snprintf(keyquery,64,"sensors/%s/%s",sensorName,v.s);
    e = KinoAPI::getProperty("hue",keyquery,kv);
    if (!showError(e)) {
      Serial.print("occured in getQuery("); Serial.print(keyquery); Serial.println(")");
      return false;
    }
    Serial.printf("\t %s : ", v.s);
    Serial.println(kv.toString());
  }
  char isWritable[64];
  KinoVariant isit;
  snprintf(isWritable,64,"sensors/%s/writable",sensorName);
  e = KinoAPI::getProperty("hue",isWritable,isit);
  Serial.print("Dieser Sensor ist ");
  if (showError(e)) Serial.println((isit.b) ? "schreibbar" : "read-only");  // fiese Logik: showError ist true, wenn KEIN Fehler aufgetreten ist!
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
  Serial.println("Standard Hue Sensoren :");
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

// MAKROS

// helper function for showing macro errors
void showMacroErrors() {
  size_t errCount = KinoAPI::getMacroErrorCount();
  Serial.print(KinoAPI::getCurrentMacroName());
  Serial.print(" : ");
  Serial.print(errCount); Serial.println(" Errors:");
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
  std::vector<String> lines;
  bool ok = KinoAPI::getMacroLines(macroName, lines);
  if (!ok) {
    showMacroErrors();
    return false;
  }
  for (auto& l : lines) {
    Serial.print("\t");
    Serial.println(l);
  }
  return true;
}

bool kino_listMacros(String* p, uint8_t n) {
  Serial.println("Gespeicherte Makros:");
  size_t i=0;
  for (auto& macroname : KinoAPI::listMacros()) {
    Serial.print("\t"); Serial.println(macroname);
    i++;
  }
  Serial.print(i); Serial.println(" Makros total");
  return true;
}

bool kino_showMacro(String* p, uint8_t n) {
  Serial.print("actions in Makro "); Serial.println(p[0]);
  return showMacroListing(p[0]);
}



bool kino_addCommandToMacro(String* p, uint8_t n) {
  bool ok = false;
  if (n==3) {
    ok = KinoAPI::addMacroCommand(p[0], p[1].toInt(), p[2]);
  } else if (n == 6) {
    KinoVariant val = prepareForJson(p[5]);
    ok = KinoAPI::addMacroCommand(p[0], p[1].toInt(), p[2], p[3], p[4], val);
  }
  if (!ok) {
    Serial.print("got "); Serial.print(n); Serial.println("parameters");
    showMacroErrors();
    //return false;
  }
  return showMacroListing(p[0]);
}

bool kino_deleteCommandFromMacro(String* p, uint8_t n) {
  Serial.print("\tLösche Zeile ");
  Serial.print(p[1].toInt());
  Serial.print(" aus Makro ");
  Serial.println(p[0]);
  bool ok = KinoAPI::deleteMacroCommand(p[0], p[1].toInt());
  if (!ok) {
    showMacroErrors();
    return false;
  }
  return showMacroListing(p[0]);
}

bool kino_updateCommandInMacro(String* p, uint8_t n) {
  bool ok = false;
  if (n == 3) {
    bool ok = KinoAPI::updateMacroCommand(p[0], p[1].toInt(), p[2]);
  } else if (n == 6) {
    KinoVariant val = prepareForJson(p[5]);
    ok = KinoAPI::updateMacroCommand(p[0], p[1].toInt(), p[2], p[3], p[4], val);
  }
  if (!ok) {
    showMacroErrors();
    //return false;
  }
  return showMacroListing(p[0]);
}

bool kino_addOrUpdateMacro(String* p, uint8_t n) {
  bool ok = KinoAPI::addOrUpdateMacro(p[0]);
  if (!ok) showMacroErrors();
  return ok;
}

bool kino_deleteMacro(String*p, uint8_t n) {
  bool ok = KinoAPI::deleteMacro(p[0]);
  if (!ok) showMacroErrors();
  return ok;
}

void serial_macroFinished(bool success) {
  if (!success) {
    showMacroErrors();
    return;
  }
  Serial.print("Makro \"");
  Serial.print(KinoAPI::getCurrentMacroName());
  Serial.println("\" sauber abgearbeitet\n");
}

bool kino_executeMacro(String* p, uint8_t n) {
  //return KinoAPI::executeMacro(p[0]);
  if (!KinoAPI::executeMacro(p[0], serial_macroFinished)) {
    // Diese Fehler werden direkt beim Starten gefangen:
    showMacroErrors();
  }
  return true;
}

bool kino_testMacro(String* p, uint8_t n) {
  if (!KinoAPI::testMacro(p[0], serial_macroFinished)) {
    // Diese Fehler werden direkt beim Starten gefangen:
    showMacroErrors();
  }
  return true;
}

bool kino_init(String* p, uint8_t n) {
  Serial.println("Initialisiere Geräte:");
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
  Serial.print(p[0]); Serial.print("."); Serial.print(p[1]);
  Serial.print(" = ");
  switch (val.type) {
    case KinoVariant::NONE :
      Serial.println("EMPTY");
      break;
    case KinoVariant::BOOL :
      Serial.print("(bool) "); Serial.println(val.b ? "true" : "false");
      break;
    case KinoVariant::INT : 
      Serial.print("(int) "); Serial.println(val.i);
      break;
    case KinoVariant::FLOAT : 
      Serial.print("(float) "); Serial.println(val.f);
      break;
    case KinoVariant::STRING :
      Serial.print("(string) \""); Serial.print(val.s); Serial.println("\"");
      break;
    case KinoVariant::RGB_COLOR :
      Serial.printf("(RGBColor) %i , %i , %i", val.color.r, val.color.g, val.color.b);
      Serial.println();
      break;
    default :
      Serial.println("unknown KinoVariant type");
      break;
  }
  return showError(e);;
}

bool kino_list(String*p, uint8_t n) {
  const char* devicename = p[0].c_str();
  const char* listname = p[1].c_str();
  uint16_t count;
  KinoVariant v;
  KinoError e;
  e = KinoAPI::getQueryCount(devicename, listname, count);
  if (!showError(e)) return false;
  Serial.printf("%d Einträge gefunden:\n",count);
  for (int i=0; i<count; i++) {
    e = KinoAPI::getQuery(devicename, listname, i,v);
    switch(v.type) {
      case KinoVariant::BOOL :
        Serial.println(v.b ? "(bool) true" : "(bool) false");
        break;
      case KinoVariant::INT :
        Serial.printf("(int) %i\n", v.i);
        break;
      case KinoVariant::FLOAT :
        Serial.printf("(float) %f\n", v.f);
        break;
      case KinoVariant::STRING :
        Serial.printf("(string) \"%s\"\n", v.s);
        break;
      case KinoVariant::RGB_COLOR :
        Serial.printf("(RGB) [%i , %i , %i]\n", v.color.r, v.color.g, v.color.b);
        break;
      default :
        Serial.println("unbekannter Datentyp");
        break;
    }
  }
  return true;
}
