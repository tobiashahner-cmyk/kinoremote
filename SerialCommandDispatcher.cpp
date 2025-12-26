#include "SerialCommandDispatcher.h"
#include "YamahaReceiver.h"
#include "OptomaBeamer.h"
#include "WLEDDevice.h"
#include "HyperionDevice.h"
#include "HueBridge.h"
#include "KinoAPI.h"

// ==== Externe Geräte (existieren im Sketch) ====

extern YamahaReceiver yamaha;
extern OptomaBeamer beamer;
extern WLEDDevice canvas;
extern WLEDDevice sound;
extern HyperionDevice hyperion;
extern HueBridge hue;

// ==== Hilfsfunktionen ====

static bool toBool(const String& s) {
  return (s == "1" || s == "true" || s == "on");
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
bool kino_listMacros(String* p, uint8_t n);
bool kino_showMacro(String* p, uint8_t n);
bool kino_addCommandToMacro(String* p, uint8_t n);
bool kino_deleteCommandFromMacro(String* p, uint8_t n);
bool kino_updateCommandInMacro(String* p, uint8_t n);
bool kino_deleteMacro(String* p, uint8_t n);
bool kino_help(String* p, uint8_t n);
bool kino_RadioMode(String*p, uint8_t n);
bool kino_BlurayMode(String* p, uint8_t n);
bool kino_StreamingMode(String* p, uint8_t n);
bool kino_GamingMode(String* p, uint8_t n);
bool yamaha_setTicker(String* p, uint8_t n);
bool yamaha_info(String*p, uint8_t n);
bool yamaha_setPower(String* p, uint8_t n);
bool yamaha_setVolume(String* p, uint8_t n);
bool yamaha_setMute(String* p, uint8_t n);
bool yamaha_listSources(String *p, uint8_t n);
bool yamaha_setSource(String* p, uint8_t n);
bool yamaha_listDsps(String* p, uint8_t n);
bool yamaha_setDsp(String* p, uint8_t n);
bool yamaha_setAlexa(String* p, uint8_t n);
bool yamaha_setPercent(String* p, uint8_t n);
bool yamaha_setStraight(String* p, uint8_t n);
bool yamaha_setEnhancer(String* p, uint8_t n);
bool yamaha_listStations(String* p, uint8_t n);
bool yamaha_setStation(String* p, uint8_t n);
bool beamer_setTicker(String* p, uint8_t n);
bool beamer_info(String* p, uint8_t n);
bool beamer_setPower(String* p, uint8_t n);
bool canvas_setTicker(String*p, uint8_t n);
bool canvas_info(String* p, uint8_t n);
bool canvas_setPower(String* p, uint8_t n);
bool canvas_setBrightness(String*p, uint8_t n);
bool canvas_setEffect(String* p, uint8_t n);
bool canvas_setSpeed(String* p, uint8_t n);
bool canvas_setIntensity(String* p, uint8_t n);
bool canvas_setLive(String* p, uint8_t n);
bool canvas_setSolid(String* p, uint8_t n);
bool canvas_setColor(String* p, uint8_t n);
bool canvas_setPalette(String* p, uint8_t n);
bool canvas_MusicMode(String* p, uint8_t n);
bool canvas_Lightning(String* p, uint8_t n);
bool canvas_Alarm(String* p, uint8_t n);
bool sound_setTicker(String*p, uint8_t n);
bool sound_info(String* p, uint8_t n);
bool sound_setPower(String* p, uint8_t n);
bool sound_setBrightness(String* p, uint8_t n);
bool sound_setEffect(String* p, uint8_t n);
bool sound_setSoundMode(String* p, uint8_t n);
bool sound_setLightningMode(String* p, uint8_t n);
bool wled_setSoundMode(String* p, uint8_t n);
bool wled_setLightningMode(String* p, uint8_t n);
bool hyperion_info(String* p, uint8_t n);
bool hyperion_setTicker(String* p, uint8_t n);
bool hyperion_setBroadcast(String* p, uint8_t n);
bool hue_LightInfo(String* p, uint8_t n);
bool hue_listLights(String* p, uint8_t n);
bool hue_init(String* p, uint8_t n); 
bool hue_setTicker(String* p, uint8_t n);
bool hue_setPower(String* p, uint8_t n);
bool hue_setBri(String* p, uint8_t n);
bool hue_setGroup(String* p, uint8_t n);
bool hue_setScene(String* p, uint8_t n);
bool hue_listScenes(String* p, uint8_t n);
bool hue_showSensors(String *p, uint8_t n);


// ==== Tabelle ====
static const CommandEntry commandTable[] = {
  {"macro",  "run",       1, kino_executeMacro,         "lädt Makro und führt es aus"},
  {"macro",  "list",      0, kino_listMacros,           "zeigt eine Liste aller gespeicherten Makros"},
  {"macro",  "show",      1, kino_showMacro,            "zeigt den Inhalt des angegebenen Makros"},
  {"macro",  "add",       1, kino_addOrUpdateMacro,     "speichert das gegebene JSON als Makro"},
  {"macro",  "addLine",   3, kino_addCommandToMacro,    "fügt ein Kommando zu einem Macro hinzu. Param1: Macroname, Param2: Zeilennummer, Param3: action als json"},
  {"macro",  "deleteLine",2, kino_deleteCommandFromMacro,"Löscht eine Zeile aus dem angegebenen Makro. Param1: Makroname, Param2: Zeilennummer"},
  {"macro",  "updateLine",3, kino_updateCommandInMacro, "ersetzt eine Zeile im angegebenen Makro. Param1: Makroname, Param2: Zeilennummer, Param3: neue action als json"},
  {"macro",  "delete",    1, kino_deleteMacro,          "löscht das Makro mit dem gegebenen Namen"},
  {"kino",   "help",      0, kino_help,                 "zeigt diese Hilfe"},
  {"kino",   "radioModus",0, kino_RadioMode,            "startet Radio Modus im Kino"},
  {"kino",   "blurayModus",0, kino_BlurayMode,          "startet Bluray Modus im Kino"},
  {"kino",   "streamingModus", 0, kino_StreamingMode,   "startet Streaming Modus im Kino"},
  {"kino",   "gamingModus",0, kino_GamingMode,          "startet Gaming Modus im Kino"},
  {"yamaha", "info",      0, yamaha_info,               "Status Yamaha"},
  {"yamaha", "setTicker", 1, yamaha_setTicker,          "setzt Info-Ticker, Param in Millisekunden"},
  {"yamaha", "setPower",  1, yamaha_setPower,           "schaltet Yamaha ein oder aus"},
  {"yamaha", "setVolume", 1, yamaha_setVolume,          "setzt Lautstärke, Param -800 (leise) bis -200 (laut)"},
  {"yamaha", "setMute",   1, yamaha_setMute,            "schaltet Mute ein oder aus"},
  {"yamaha", "listSources",0,yamaha_listSources,        "Listet alle Quellen auf"},
  {"yamaha", "setSource", 1, yamaha_setSource,          "setzt Quelle"},
  {"yamaha", "listDsps",  0, yamaha_listDsps,           "Listet alle verfügbaren DSPs auf"},
  {"yamaha", "setDsp",    1, yamaha_setDsp,             "setzt DSP"},
  {"yamaha", "setAlexa",  1, yamaha_setAlexa,           "setzt Lautstärke, Param 0-255"},
  {"yamaha", "setPercent",1, yamaha_setPercent,         "setzt Lautstärke in %, Param 0-100"},
  {"yamaha", "setStraight",1,yamaha_setStraight,        "schaltet Straight ein oder aus"},
  {"yamaha", "setEnhancer",1,yamaha_setEnhancer,        "schaltet Enhancer ein oder aus"},
  {"yamaha", "listStations",0,yamaha_listStations,      "listet die NET RADIO Favoriten auf"},
  {"yamaha", "setStation",1, yamaha_setStation,         "schaltet NET RADIO auf angegebenen Sender"},
  {"beamer", "info",      0, beamer_info,               "Status Beamer"},
  {"beamer", "setTicker", 1, beamer_setTicker,          "setzt Info-Ticker, Param in Millisekunden"},
  {"beamer", "setPower",  1, beamer_setPower,           "schaltet Beamer ein oder aus"},
  {"canvas", "info",      0, canvas_info,               "Status Leinwand"},
  {"canvas", "setTicker", 1, canvas_setTicker,          "setzt Info-Ticker, Param in Millisekunden"},
  {"canvas", "setPower",  1, canvas_setPower,           "schaltet Leinwand LEDs ein oder aus"},
  {"canvas", "setBrightness", 1, canvas_setBrightness,  "setzt Helligkeit, Param 0-255"},
  {"canvas", "setEffect", 1, canvas_setEffect,          "setzt Effekt, Param ist Nr des Effekts"},
  {"canvas", "setSolid",  4, canvas_setSolid,           "setzt feste Farbe: R, G, B, Helligkeit"},
  {"canvas", "setColor",  4, canvas_setColor,           "setzt Effektfarbe: Index (1-3), R, G, B"},
  {"canvas", "setPalette",1, canvas_setPalette,         "setzt Effektpalette Param"},
  {"canvas", "setSpeed",  1, canvas_setSpeed,           "setzt Effekt Speed, Param 0-255"},
  {"canvas", "setIntensity",1, canvas_setIntensity,     "setzt Effekt Intensity, Param 0-255"},
  {"canvas", "setLive",   1, canvas_setLive,            "schaltet Live-Modus ein oder aus"},
  {"canvas", "MusicMode", 0, canvas_MusicMode,          "schaltet MusicMode Effekt ein"},
  {"canvas", "Lightning", 0, canvas_Lightning,          "schaltet Gewitter-Effekt ein"},
  {"canvas", "Alarm",     1, canvas_Alarm,              "schaltet Alarm-Effekt ein oder aus"},
  {"sound",  "info",      0, sound_info,                "Status Sound-LEDs"},
  {"sound",  "setTicker", 1, sound_setTicker,           "setzt Info-Ticker, Param in Millisekunden"},
  {"sound",  "setPower",  1, sound_setPower,            "schaltet Sound-LEDs ein oder aus"},
  {"sound",  "setBrightness",1, sound_setBrightness,    "setzt Helligkeit, Param 0-255"},
  {"sound",  "setEffect", 1, sound_setEffect,           "setzt Effekt, Param ist Nr des Effekts"},
  {"sound",  "MusicMode", 0, sound_setSoundMode,        "schaltet SoundReactive Effekt ein"},
  {"sound",  "Lightning", 0, sound_setLightningMode,    "schaltet Gewitter-Effekt ein"},
  {"wled",  "MusicMode", 0,  wled_setSoundMode,         "schaltet Musik Effekt auf allen WLEDs ein"},
  {"wled",  "Lightning", 0,  wled_setLightningMode,     "schaltet Gewitter-Effekt auf allen WLEDs ein"},
  {"hyperion","info",     0, hyperion_info,             "Status Hyperion"},
  {"hyperion","setTicker",1, hyperion_setTicker,        "setzt Info-Ticker, Param in Millisekunden"},
  {"hyperion","setBroadcast",1,hyperion_setBroadcast,   "schaltet den LiveModus in Hyperion ein oder aus"},
  {"hue",     "init",     0, hue_init,                  "liest den Status aller Lampen neu ein"},
  {"hue",     "setTicker",1, hue_setTicker,             "setzt Info-Ticker, Param in Millisekunden"},
  {"hue",     "LightInfo",1, hue_LightInfo,             "Status der angegebenen Lampe. Param=Lampenname"},
  {"hue",     "listLights",0,hue_listLights,            "listet alle Hue Lampen auf"},
  {"hue",     "setPower", 2, hue_setPower,              "Schaltet Lampe ein oder aus. Param1=Name, Param2=bool onoff"},
  {"hue",     "setBri",   2, hue_setBri,                "Setzt Brightness für Param1 auf Param2"},
  {"hue",     "setGroup", 3, hue_setGroup,              "Schaltet Lampengruppe Param1 ein oder aus (Param2) und auf Helligkeit Param3"},
  {"hue",     "setScene", 1, hue_setScene,              "Aktiviert Szene Param1"},
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
  uint8_t maxParamCount = 5;
  String params[maxParamCount];          // Max. 5 Parameter
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
  if (!yamaha.getStatus()) {
    Serial.print("Yamaha Status konnte nicht ausgelesen werden!");
    return false;
  }
  Serial.print("Power:  "); Serial.println((yamaha.getPowerStatus()) ? "An":"Aus");
  Serial.print("Volume: "); Serial.print(yamaha.getVolume()/10); Serial.println("dB");
  Serial.print("\tMute: "); Serial.println(yamaha.getMute()?"An":"Aus");
  Serial.println("Tone:");
  Serial.print("\tBass     : "); Serial.println(yamaha.getBass());
  Serial.print("\tTreble   : "); Serial.println(yamaha.getTreble());
  Serial.print("\tSW Trim  : "); Serial.println(yamaha.getSubTrim());
  Serial.print("\tEnhancer : "); Serial.println(yamaha.getEnhancer()?"An":"Aus");
  Serial.print("\tStraight : "); Serial.println(yamaha.getStraight() ? F("An, DSP inaktiv") : F("Aus, DSP aktiv"));
  Serial.print("\tDSP      : "); Serial.println(yamaha.getSoundProgram());
  
  InputSource src = yamaha.getInputSource();
  Serial.print("Source: "); Serial.print(src.internal);
  if (String(src.internal) != src.custom) {
    Serial.print(" ("); Serial.print(src.custom); Serial.println(" )");
  } else {
    Serial.println();
  }
  if (String(src.internal) == F("NET RADIO")) {
    NetRadioTrackInfo nri = yamaha.readCurrentlyPlayingNetRadio();
    Serial.print("\tStation: "); Serial.println(nri.station);
    Serial.print("\tSong   : "); Serial.println(nri.song);
    Serial.print("\tElapsed: "); Serial.println(nri.elapsed);
  }
  
  return true;
}

bool yamaha_listSources(String *p, uint8_t n) {
  String currentSource = yamaha.getSource();
  for (const auto& src : yamaha.readInputSources()) {
    if (src.skip) {
      Serial.print("... ");
    } else {
      //if (src.internal == currentSource) {
      if (String(src.internal) == currentSource) {
        Serial.print("[X] ");
      } else {
        Serial.print("[ ] ");
      }
    }
    Serial.print(src.internal);
    Serial.print(" ");
    Serial.println(src.custom);
  }
  return true;
}

bool yamaha_listDsps(String* p, uint8_t n) {
  Serial.println("\n\nLese alle DSPs aus");
  std::vector<String> dsps = yamaha.readDspNames();
  for (String d : dsps) {
    Serial.print(d);
    if (d == yamaha.getSoundProgram()) {
      Serial.println(" !!");
    } else {
      Serial.println();
    }
  }
  return true;
}

bool yamaha_listStations(String* p, uint8_t n) {
  std::vector<String> stations = yamaha.readNetRadioFavorites();
  for (auto& s : stations) {
    Serial.println(s);
  }
  return true;
}

bool beamer_info(String* p, uint8_t n) {
  if (!beamer.getStatus()) {
    Serial.print("Beamer Status konnte nicht gelesen werden!");
    return false;
  }
  Serial.print("Power:  "); Serial.println((beamer.getPowerStatus()) ? "An" : "Aus");
  Serial.print("Source: "); Serial.println(beamer.getSourceString());
  Serial.print("Lampe:  "); Serial.println(beamer.getLampHours());
  Serial.println("\n\n");
  return true;
}

bool canvas_info(String* p, uint8_t n) {
  if (p[0] == "true") {
    Serial.print("Lese Status neu aus : ");
    Serial.println(canvas.getStatus() ? "OK" : "FEHLER");
    Serial.println();
  }
  Serial.print("Power:      "); Serial.println((canvas.getPowerStatus()) ? "An" : "Aus");
  Serial.print("Brightness: "); Serial.println(canvas.getBrightness());
  Serial.print("Live Data:  "); Serial.print((canvas.isReceivingLiveData()) ? "incoming, " : "none, "); Serial.println((canvas.isOverridingLiveData()) ? "ignoriert" : "bearbeitet");
  Serial.print("LD Source:  "); Serial.println(canvas.getLiveSource());
  Serial.print("Effekt:     "); Serial.println(canvas.getEffect());
  if (canvas.getEffect() != 0) {
    Serial.print("  Speed:    "); Serial.println(canvas.getSpeed());
    Serial.print("  Intensity:"); Serial.println(canvas.getIntensity());
  }
  Serial.println("Farben:");
  Serial.print("\tPalette: "); Serial.println(canvas.getPalette());
  WLEDColor c = canvas.getColFg();
  Serial.print("\tFg: "); Serial.print(c.r); Serial.print(" , "); Serial.print(c.g); Serial.print(" , "); Serial.println(c.b);
  c = canvas.getColFg();
  Serial.print("\tFg: "); Serial.print(c.r); Serial.print(" , "); Serial.print(c.g); Serial.print(" , "); Serial.println(c.b);
  c = canvas.getColFg();
  Serial.print("\tFg: "); Serial.print(c.r); Serial.print(" , "); Serial.print(c.g); Serial.print(" , "); Serial.println(c.b);
  return true;
}

bool sound_info(String* p, uint8_t n) {
  if (p[0] == "true") {
    Serial.print("Lese Status neu aus : ");
    Serial.println(sound.getStatus() ? "OK" : "FEHLER");
    Serial.println();
  }
  Serial.print("Power:      "); Serial.println((sound.getPowerStatus()) ? "An" : "Aus");
  Serial.print("Brightness: "); Serial.println(sound.getBrightness());
  Serial.print("Effekt:     "); Serial.println(sound.getEffect());
  if (sound.getEffect() != 0) {
    Serial.print("  Speed:    "); Serial.println(sound.getSpeed());
    Serial.print("  Intensity:"); Serial.println(sound.getIntensity());
  }
  Serial.println("Farben:");
  Serial.print("\tPalette: "); Serial.println(sound.getPalette());
  WLEDColor c = sound.getColFg();
  Serial.print("\tFg: "); Serial.print(c.r); Serial.print(" , "); Serial.print(c.g); Serial.print(" , "); Serial.println(c.b);
  c = sound.getColBg();
  Serial.print("\tFg: "); Serial.print(c.r); Serial.print(" , "); Serial.print(c.g); Serial.print(" , "); Serial.println(c.b);
  c = sound.getColFx();
  Serial.print("\tFg: "); Serial.print(c.r); Serial.print(" , "); Serial.print(c.g); Serial.print(" , "); Serial.println(c.b);
  return true;
}

bool hyperion_info(String* p, uint8_t n) {
  if (!hyperion.getStatus()) {
    Serial.print("Hyperion Status konnte nicht gelesen werden!");
    return false;
  }
  Serial.print("Hyperion:\n  Power: ");
  Serial.println(hyperion.getPowerStatus() ? "An":"Aus");
  Serial.print("LEDs: ");
  Serial.println(hyperion.getLedDeviceStatus() ? "An":"Aus");
  Serial.print("Broadcasting: ");
  Serial.println(hyperion.isBroadcasting() ? "Ja":"Nein");
  return true;
}

bool hue_listLights(String* p, uint8_t n) {
  for (auto& l : hue.getLights()) {
    Serial.println(l->getName());
  }
  return true;
}

bool hue_LightInfo(String* p, uint8_t n) {
  HueLight* light = hue.getLightByName(p[0]);
  if (!light) {
    Serial.println("Lampe mit diesem Namen wurde nicht gefunden.");
    return false;
  }
  Serial.println(light->getName());
  Serial.print("ID     : "); Serial.println(light->getId());
  Serial.print("\tPower: "); Serial.println(light->isOn() ? "An":"Aus");
  Serial.print("\tBri  : "); Serial.println(light->getBrightness());
  if (light->hasXYColor()) {
    Serial.print("\tXY   : "); Serial.print(light->getX()); Serial.print(" "); Serial.println(light->getY());
  }
  if (light->hasCTColor()) {
    Serial.print("\tCT   : "); Serial.println(light->getCT());
  }
  return true;
}

bool hue_listScenes(String* p, uint8_t n) {
  for (auto& s : hue.getScenes()) {
    Serial.println(s->getName());
  }
  return true;
}

bool hue_showSensors(String* p, uint8_t n) {
  if (!hue.readSensors()) {
    Serial.println("Sensoren konnten nicht aktualisiert werden");
    return false;
  }
  HueSensor* sensor = hue.getSensorByName("Presence Clip Theke");
  if (sensor) {
    Serial.print(sensor->getName()); Serial.print(" : ");
    if (sensor->hasValue("status")) {
      Serial.println(sensor->getValue("status").as<const char*>());
    } else {
      Serial.println("Status nicht gefunden");
    }
  } else {
    Serial.println("Presence Clip Theke nicht gefunden!");
  }
  sensor = hue.getSensorByName("Temp Sensor Theke");
  if (sensor) {
    Serial.print(sensor->getName()); Serial.print(" : ");
    int temp = 0;
    if (sensor->hasValue("temperature")) temp = sensor->getValue("temperature").as<int>();
    float celsius = temp / 100.0f;
    Serial.print(celsius); Serial.println("°C");
  } else {
    Serial.println("Temp Sensor Theke nicht gefunden!");
  }
  sensor = hue.getSensorByName("Daylight");
  if (sensor) {
    Serial.print(sensor->getName()); Serial.print(" : ");
    if (sensor->hasValue("daylight")) {
      Serial.println((sensor->getValue("daylight").as<bool>()) ? "true" : "false");
    } else {
      Serial.println("daylight nicht gefunden");
    }
  } else {
    Serial.println("Daylight nicht gefunden!");
  }
  sensor = hue.getSensorByName("Licht Sensor Theke");
  if (sensor) {
    Serial.print(sensor->getName()); Serial.println(" : ");
    Serial.print("\t\tlightlevel: ");
    if (sensor->hasValue("lightlevel")) {
      Serial.println(sensor->getValue("lightlevel").as<int>());
    } else {
      Serial.println("nicht gefunden");
    }
    Serial.print("\t\tdark: ");
    if (sensor->hasValue("dark")) {
      Serial.println((sensor->getValue("dark").as<bool>()) ? "true" : "false");
    } else {
      Serial.println("nicht gefunden");
    }
    Serial.print("\t\tdaylight: ");
    if (sensor->hasValue("daylight")) {
      Serial.println((sensor->getValue("daylight").as<bool>()) ? "true" : "false");
    } else {
      Serial.println("nicht gefunden");
    }
  } else {
    Serial.println("Licht Sensor Theke nicht gefunden!");
  }
  return true;
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
  Serial.print("\tHänge neue Action\n\t");
  Serial.print(p[2]);
  Serial.print("\n\tin Zeile ");
  Serial.print(p[1]);
  Serial.print(" von Makro ");
  Serial.print(p[0]);
  Serial.print(" ein");
  bool ok = KinoAPI::addMacroCommand(p[0], p[1].toInt(), p[2]);
  if (!ok) {
    showMacroErrors();
    return false;
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
  Serial.print("\tErsetze Zeile ");
  Serial.print(p[1].toInt());
  Serial.print(" in Makro ");
  Serial.print(p[0]);
  Serial.print(" durch\n\t");
  Serial.println(p[2]);
  bool ok = KinoAPI::updateMacroCommand(p[0], p[1].toInt(), p[2]);
  if (!ok) {
    showMacroErrors();
    return false;
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
  Serial.print("Makro ");
  Serial.print(KinoAPI::getCurrentMacroName());
  Serial.println("sauber abgearbeitet\n");
}

bool kino_executeMacro(String* p, uint8_t n) {
  //return KinoAPI::executeMacro(p[0]);
  if (!KinoAPI::executeMacro(p[0], serial_macroFinished)) {
    // Diese Fehler werden direkt beim Starten gefangen:
    showMacroErrors();
  }
  return true;
}

bool kino_setPower(String* p, uint8_t n) {
  return KinoAPI::Power(toBool(p[0]));
}

bool kino_RadioMode(String*p, uint8_t n) {
  return KinoAPI::radioMode();
}

bool kino_BlurayMode(String* p, uint8_t n) {
  return KinoAPI::blurayMode();
}

bool kino_StreamingMode(String* p, uint8_t n) {
  return KinoAPI::streamingMode();
}

bool kino_GamingMode(String* p, uint8_t n) {
  return KinoAPI::GamingMode();
}

bool yamaha_setTicker(String* p, uint8_t n) {
  return KinoAPI::yamaha_setTicker(p[0].toInt());
}

bool yamaha_setPower(String* p, uint8_t n) {
  return KinoAPI::yamaha_setPower(toBool(p[0]));
}

bool yamaha_setVolume(String* p, uint8_t n) {
  return KinoAPI::yamaha_setVolume(p[0].toInt());
}

bool yamaha_setMute(String* p, uint8_t n) {
  return KinoAPI::yamaha_setMute(toBool(p[0]));
}

bool yamaha_setSource(String* p, uint8_t n) {
  return KinoAPI::yamaha_setInput(p[0]);
}

bool yamaha_setDsp(String* p, uint8_t n){
  return KinoAPI::yamaha_setDsp(p[0]);
}

bool yamaha_setAlexa(String* p, uint8_t n) {
  return KinoAPI::yamaha_setVolumePercent(p[0].toInt());
}

bool yamaha_setPercent(String* p, uint8_t n) {
  return KinoAPI::yamaha_setVolumePercent(p[0].toInt());
}

bool yamaha_setStraight(String* p, uint8_t n) {
  return KinoAPI::yamaha_setStraight(toBool(p[0]));
}

bool yamaha_setEnhancer(String* p, uint8_t n) {
  return KinoAPI::yamaha_setEnhancer(toBool(p[0]));
}

bool yamaha_setStation(String* p, uint8_t n) {
  return KinoAPI::yamaha_setStation(p[0]);
}

bool beamer_setTicker(String* p, uint8_t n) {
  return KinoAPI::beamer_setTicker(p[0].toInt());
}

bool beamer_setPower(String* p, uint8_t n) {
  return KinoAPI::beamer_setPower(toBool(p[0]));
}

bool canvas_setTicker(String*p, uint8_t n) {
  return KinoAPI::canvas_setTicker(p[0].toInt());
}

bool canvas_setPower(String* p, uint8_t n) {
  return KinoAPI::canvas_setPower(toBool(p[0]));
}

bool canvas_setBrightness(String*p, uint8_t n) {
  return KinoAPI::canvas_setBrightness(p[0].toInt());
}

bool canvas_setEffect(String* p, uint8_t n) {
  return KinoAPI::canvas_setEffect(p[0].toInt());
}

bool canvas_setSpeed(String* p, uint8_t n) {
  return KinoAPI::canvas_setSpeed(p[0].toInt());
}

bool canvas_setIntensity(String* p, uint8_t n) {
  return KinoAPI::canvas_setIntensity(p[0].toInt());
}

bool canvas_setLive(String* p, uint8_t n) {
  return KinoAPI::canvas_setLive(toBool(p[0]));
}

bool canvas_setSolid(String* p, uint8_t n) {
  uint8_t r = p[0].toInt();
  uint8_t g = p[1].toInt();
  uint8_t b = p[2].toInt();
  uint8_t bri = p[3].toInt();
  return KinoAPI::canvas_setSolid(r, g, b, bri);
}

bool canvas_setColor(String* p, uint8_t n) {
  uint8_t i = p[0].toInt();
  uint8_t r = p[1].toInt();
  uint8_t g = p[2].toInt();
  uint8_t b = p[3].toInt();
  switch (i) {
    case 1  :
      if (!KinoAPI::canvas_setFgColor(r,g,b)) return false;
      break;
    case 2  :
      if (!KinoAPI::canvas_setBgColor(r,g,b)) return false;
      break;
    case 3  : 
      if (!KinoAPI::canvas_setFxColor(r,g,b)) return false;
      break;
    default :
      Serial.println("Der Index muss 1 (Vordergrund), 2 (Hintergrund) oder 3 (Effekt) sein");
      return false;
  }
  return KinoAPI::canvas_commit();
}

bool canvas_setPalette(String* p, uint8_t n) {
  return KinoAPI::canvas_setPalette(p[0].toInt());
}

bool canvas_MusicMode(String* p, uint8_t n) {
  return KinoAPI::canvas_setMusicEffect();
}

bool canvas_Lightning(String* p, uint8_t n) {
  return KinoAPI::canvas_setLightningEffect();
}

bool canvas_Alarm(String* p, uint8_t n) {
  bool onoff = toBool(p[0]);
  return (onoff) ? KinoAPI::canvas_setAlarm() : KinoAPI::canvas_stopAlarm();
}

bool sound_setTicker(String*p, uint8_t n) {
  return KinoAPI::sound_setTicker(p[0].toInt());
}

bool sound_setEffect(String* p, uint8_t n) {
  if (!KinoAPI::sound_setEffect(p[0].toInt())) return false;
  return KinoAPI::sound_commit();
}

bool sound_setPower(String* p, uint8_t n) {
  if (!KinoAPI::sound_setPower(toBool(p[0]))) return false;
  return KinoAPI::sound_commit();
}

bool sound_setBrightness(String*p, uint8_t n) {
  if (!KinoAPI::sound_setBrightness(p[0].toInt())) return false;
  return KinoAPI::sound_commit();
}

bool sound_setSoundMode(String* p, uint8_t n) {
  return KinoAPI::sound_setMusicEffect();
}

bool sound_setLightningMode(String* p, uint8_t n) {
  return KinoAPI::sound_setLightningEffect();
}

bool wled_setSoundMode(String* p, uint8_t n) {
  return KinoAPI::wled_setMusicEffect();
}

bool wled_setLightningMode(String* p, uint8_t n) {
  return KinoAPI::wled_setLightningEffect();
}

bool hyperion_setTicker(String* p, uint8_t n) {
  return KinoAPI::hyperion_setTicker(p[0].toInt());
}

bool hyperion_setBroadcast(String* p, uint8_t n) {
  bool onoff = toBool(p[0]);
  //bool successWLED = canvas.setLive(onoff);
  return (onoff) ? KinoAPI::hyperion_startBroadcast() : KinoAPI::hyperion_stopBroadcast();
}

bool hue_init(String* p, uint8_t n) {
  return KinoAPI::hue_init();
}

bool hue_setTicker(String*p, uint8_t n) {
  return KinoAPI::hue_setTicker(p[0].toInt());
}

bool hue_setPower(String* p, uint8_t n) {
  if (!KinoAPI::hueLight_setPower(p[0],toBool(p[1]))) return false;
  return KinoAPI::hueLight_commit(p[0]);
}

bool hue_setBri(String* p, uint8_t n) {
  if (!KinoAPI::hueLight_setBri(p[0],p[1].toInt())) return false;
  return KinoAPI::hueLight_commit(p[0]);
}

bool hue_setGroup(String* p, uint8_t n) {
  if (!KinoAPI::hueGroup_setPower(p[0],toBool(p[1]))) return false;
  if (!KinoAPI::hueGroup_setBri(p[0],p[2].toInt())) return false;
  return KinoAPI::hueGroup_commit(p[0]);
}

bool hue_setScene(String* p, uint8_t n) {
  return KinoAPI::hueScene_set(p[0]);
}
