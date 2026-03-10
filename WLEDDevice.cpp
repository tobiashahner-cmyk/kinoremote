#include "WLEDDevice.h"
#include "NetworkHelper.h"
#include "FileHelper.h"
#include <ESP8266HTTPClient.h>

extern StaticJsonDocument<2048> httpJson;

bool WLEDDevice::readLineSmart(const char* path, int index, char* out, size_t outLen) {
  // ---------- Streaming möglich? ----------
  if (_chunkActive && strcmp(_chunkPath, path) == 0 && index == _chunkLastIndex + 1) {
    // Weiter im Stream...
  }
  else if (index == 0) {
    closeChunkStream();
    if (!FileHelper::begin()) return false;
    _chunkFile = LittleFS.open(path, "r");
    if (!_chunkFile) {
      closeChunkStream();
      return false;
    }
    strncpy(_chunkPath, path, sizeof(_chunkPath)); _chunkPath[sizeof(_chunkPath)-1] = '\0';
    _chunkActive = true;
    _chunkLastIndex = -1;
    _chunkLen = 0;
    _chunkPos = 0;
  }
  else {
    // Fallback auf die bereits umgestellte readLineAt
    return FileHelper::readLineAt(path, (size_t)index, out, outLen);
  }

  // ---------- Zeile aus Chunk lesen ----------
  while (true) {
    for (size_t i = _chunkPos; i < _chunkLen; i++) {
      if (_chunkBuf[i] == '\n') {
        size_t len = i - _chunkPos;
        copyAndTrim(out, _chunkBuf + _chunkPos, len, outLen);
        
        _chunkPos = i + 1;
        _chunkLastIndex++;
        return true;
      }
    }

    if (!_chunkFile.available()) {
      if (_chunkPos < _chunkLen) {
        size_t len = _chunkLen - _chunkPos;
        copyAndTrim(out, _chunkBuf + _chunkPos, len, outLen);
        _chunkLastIndex++;
        closeChunkStream();
        return true;
      }
      closeChunkStream();
      return false;
    }

    // Puffer-Rotation
    size_t rest = _chunkLen - _chunkPos;
    if (rest > 0) memmove(_chunkBuf, _chunkBuf + _chunkPos, rest);
    _chunkPos = 0;
    _chunkLen = rest;

    size_t readBytes = _chunkFile.readBytes(_chunkBuf + _chunkLen, CHUNK_SIZE - _chunkLen);
    _chunkLen += readBytes;
  }
}

// Hilfsfunktion für sauberes Kopieren und Trimmen ohne String-Objekte
void WLEDDevice::copyAndTrim(char* dest, const char* src, size_t srcLen, size_t destSize) {
  if (destSize == 0) return;

  // Führende Leerzeichen überspringen
  while (srcLen > 0 && isspace(*src)) { src++; srcLen--; }
  // Abschließende Leerzeichen ignorieren
  while (srcLen > 0 && isspace(src[srcLen - 1])) { srcLen--; }

  size_t toCopy = (srcLen < destSize - 1) ? srcLen : destSize - 1;
  memcpy(dest, src, toCopy);
  dest[toCopy] = '\0'; // Manuelle Null-Terminierung
}

void WLEDDevice::closeChunkStream() {
  if (_chunkFile) {
    _chunkFile.close();
  }
  _chunkActive = false;
  _chunkLen = 0;
  _chunkPos = 0;
  _chunkLastIndex = -1;
  _chunkPath[0] = '\0'; // Pfad-Puffer leeren
}

void WLEDDevice::stripAfterAt(char* s) {
  char* atPos = strchr(s, '@');
  if (atPos) *atPos = '\0'; // String am @ abschneiden
}

WLEDDevice::WLEDDevice(const IPAddress& ip)
: _ip(ip) {}

WLEDDevice::WLEDDevice(const String& ip) {
  _ip.fromString(ip);
}

bool WLEDDevice::needsCommit() {
  return true;
}

bool WLEDDevice::commit() {
  return applyChanges();
}

KinoError WLEDDevice::get(const char* prop, KinoVariant& out) {
  if (!prop) return KinoError::PropertyNotSupported;
  if (strcmp(prop,"ip")==0) {
    char buf[20];
    snprintf(buf,20,"%d.%d.%d.%d", _ip[0], _ip[1], _ip[2], _ip[3]);
    out.setString(buf);
    return KinoError::OK;
  }
  if ((strcmp(prop,"power")==0)||(strcmp(prop,"on")==0)) {
    out.setBool(_props.onoff);
    return KinoError::OK;
  }
  if (strcmp(prop,"live")==0) {
    out.setBool(_live);
    return KinoError::OK;
  }
  if ((strcmp(prop,"override")==0)||(strcmp(prop,"lor")==0)) {
    out.setBool(_props.lor);
    return KinoError::OK;
  }
  if ((strcmp(prop,"brightness")==0)||(strcmp(prop,"bri")==0)) {
    out.setInt(_props.bri);
    return KinoError::OK;
  }
  if ((strcmp(prop,"speed")==0)||(strcmp(prop,"sx")==0)) {
    out.setInt(_props.sx);
    return KinoError::OK;
  }
  if ((strcmp(prop,"intensity")==0)||(strcmp(prop,"ix")==0)) {
    out.setInt(_props.ix);
    return KinoError::OK;
  }
  if ((strcmp(prop,"c1x")==0)||(strcmp(prop,"custom1")==0)) {
    out.setInt(_props.c1x);
    return KinoError::OK;
  }
  if ((strcmp(prop,"c2x")==0)||(strcmp(prop,"custom2")==0)) {
    out.setInt(_props.c2x);
    return KinoError::OK;
  }
  if ((strcmp(prop,"c3x")==0)||(strcmp(prop,"custom3")==0)) {
    out.setInt(_props.c3x);
    return KinoError::OK;
  }
  if ((strcmp(prop,"effect")==0)||(strcmp(prop,"fx")==0)) {
    out.setInt(_props.fx);
    return KinoError::OK;
  }
  if ((strcmp(prop,"palette")==0)||(strcmp(prop,"pal")==0)) {
    out.setInt(_props.pal);
    return KinoError::OK;
  }
  if (strcmp(prop,"input")==0) {
    out.setString(_livesource);
    return KinoError::OK;
  }
  if ((strcmp(prop,"color") == 0)||(strcmp(prop,"color1") == 0)||(strcmp(prop,"colorFg") == 0)||(strcmp(prop,"FgColor") == 0)||(strcmp(prop,"col1")==0)) {
    out.setColor(_props.col[0]);
    return KinoError::OK;
  }
  if ((strcmp(prop,"color2") == 0)||(strcmp(prop,"colorBg") == 0)||(strcmp(prop,"BgColor") == 0)||(strcmp(prop,"col2")==0)) {
    out.setColor(_props.col[1]);
    return KinoError::OK;
  }
  if ((strcmp(prop,"color3") == 0)||(strcmp(prop,"colorFx") == 0)||(strcmp(prop,"FxColor") == 0)||(strcmp(prop,"col3")==0)) {
    out.setColor(_props.col[2]);
    return KinoError::OK;
  }
  int fxnr, paramnr, found;
  char pathEnd[20];
  found = sscanf(prop,"fx/%d/param/%d%20s", &fxnr, &paramnr, pathEnd);
  if ((found == 2) && (fxnr >= 0) && (paramnr >= 0)) {// Pfad ist "fx/<effectId>/param/<paramIndex>", ohne pathEnd
    int gspLen = 64; char getsetPath[gspLen];
    if (!getParamField(fxnr, paramnr, getsetPath, gspLen)) {
      out.setNone();
      return KinoError::InvalidValue;
    }
    out.setString(getsetPath);
    return KinoError::OK;
  }
  if ((found == 3) && (fxnr >= 0) && (paramnr >= 0)) {
    // Pfad ist "fx/<effectId>/param/<paramIndex>" plus pathEnd
    if (strcmp(pathEnd,"/label")==0) {
      char label[32];
      if (!getParamLabel(fxnr, paramnr, label, 32)) { out.setNone(); return KinoError::InvalidValue; }
      out.setString(label);
      return KinoError::OK;
    }
    if (strcmp(pathEnd,"/access")==0) {
      out.setInt(3);
      return KinoError::OK;
    }
    if (strcmp(pathEnd,"/minvalue")==0) {
      out.setInt(0);
      return KinoError::OK;
    }
    if (strcmp(pathEnd,"/maxvalue")==0) {
      out.setInt(255);
      return KinoError::OK;
    }
    if (strcmp(pathEnd,"/valuestep")==0) {
      out.setInt(1);
      return KinoError::OK;
    }
    return KinoError::PropertyNotSupported;
  }
  found = sscanf(prop,"fx/%d/label%20s", &fxnr, pathEnd);
  if ((found == 1) && (fxnr >= 0)) {
    char fxFile[48]; effectsFile(fxFile, sizeof(fxFile));
    constexpr size_t lineLen = 128;
    char line[lineLen];

    // >>> Lazy-Streaming
    if (!readLineSmart(fxFile, fxnr, line, lineLen)) { out.setNone(); return KinoError::InvalidValue; }
    stripAfterAt(line);

    out.setString(line);
    return KinoError::OK;
  }
  int palnr; char rest1[32]; int paramIndex; char rest2[32];
  found = sscanf(prop,"pal/%d/%31[^/]/%d/%31s", &palnr, rest1, &paramIndex, rest2);
  if ((found == 2) && (palnr >= 0) && (strlen(rest1)>0)) {      // path = "pal/<palNr>/<rest1>
    if (strcmp(rest1,"label")==0) {
      char palFile[48];
      paletteFile(palFile, sizeof(palFile));
      
      constexpr size_t lineLen = 128;
      char line[lineLen];

      if (!readLineSmart(palFile, palnr, line, lineLen)) {out.setNone(); return KinoError::InvalidValue; }

      out.setString(line);
      return KinoError::OK;
    }
  }
  if ((found ==3) && (palnr >= 0) && (strlen(rest1)>0) && strcmp(rest1,"param")==0) { // path = "pal/<palNr>/param/<paramIndex>"
    KinoPropertyParam* p = getPaletteParam(palnr, paramIndex);
    if (!p) { out.setNone(); return KinoError::OutOfRange; }
    out.setString(p->getsetPath);
    return KinoError::OK;
  }
  if ((found==4) && (palnr>=0) && (strlen(rest1)>0) && (strcmp(rest1,"param")==0) && (strlen(rest2)>0)) {
    KinoPropertyParam* p = getPaletteParam(palnr, paramIndex);
    if (!p) { out.setNone(); return KinoError::OutOfRange; }
    
    if (strcmp(rest2,"label")==0) {
      out.setString(p->label);
      return KinoError::OK;
    }
    if (strcmp(rest2,"access")==0) {
      out.setInt(p->access);
      return KinoError::OK;
    }
    if (strcmp(rest2,"minvalue")==0) {
      out.setInt(p->minvalue.value_or(0));
      return KinoError::OK;
    }
    if (strcmp(rest2,"maxvalue")==0) {
      out.setInt(p->maxvalue.value_or(100));
      return KinoError::OK;
    }
    if (strcmp(rest2,"valuestep")==0) {
      out.setInt(p->valuestep.value_or(1));
      return KinoError::OK;
    }
  }
  return KinoError::PropertyNotSupported;
}

KinoError WLEDDevice::set(const char* prop, const KinoVariant& val) {
  if ((strcmp(prop,"power")==0)||(strcmp(prop,"on")==0)) {
    if (!setPowerStatus(val.asBool())) return KinoError::InvalidValue;
    return KinoError::OK;
  }
  if ((strcmp(prop,"override")==0)||(strcmp(prop,"lor")==0)) {
    if (!setLive(!val.asBool())) return KinoError::InvalidValue;
    return KinoError::OK;
  }
  if (strcmp(prop,"live")==0) {
    if (!setLive(val.asBool())) return KinoError::InvalidValue;
    return KinoError::OK;
  }
  if ((strcmp(prop,"brightness")==0)||(strcmp(prop,"bri")==0)) {
    if (!setBrightness(val.asInt())) return KinoError::InvalidValue;
    return KinoError::OK;
  }
  if ((strcmp(prop,"speed")==0)||(strcmp(prop,"sx")==0)) {
    if (!setSpeed(val.asInt())) return KinoError::InvalidValue;
    return KinoError::OK;
  }
  if ((strcmp(prop,"intensity")==0)||(strcmp(prop,"ix")==0)) {
    if (!setIntensity(val.asInt())) return KinoError::InvalidValue;
    return KinoError::OK;
  }
  if ((strcmp(prop,"c1x")==0)||(strcmp(prop,"custom1")==0)) {
    if (!setCustom1(val.asInt())) return KinoError::InvalidValue;
    return KinoError::OK;
  }
  if ((strcmp(prop,"c2x")==0)||(strcmp(prop,"custom2")==0)) {
    if (!setCustom2(val.asInt())) return KinoError::InvalidValue;
    return KinoError::OK;
  }
  if ((strcmp(prop,"c3x")==0)||(strcmp(prop,"custom3")==0)) {
    if (!setCustom3(val.asInt())) return KinoError::InvalidValue;
    return KinoError::OK;
  }
  if ((strcmp(prop,"transitiontime")==0)||(strcmp(prop,"tt")==0)) {
    if (!setTransitionTime(val.asInt())) return KinoError::InvalidValue;
    return KinoError::OK;
  }
  if ((strcmp(prop,"effect")==0)||(strcmp(prop,"fx")==0)) {
    if (!setEffect(val.asInt())) return KinoError::InvalidValue;
    return KinoError::OK;
  }
  if ((strcmp(prop,"palette")==0)||(strcmp(prop,"pal")==0)) {
    if (!setPalette(val.asInt())) return KinoError::InvalidValue;
    return KinoError::OK;
  }
  if ((strcmp(prop,"color") == 0)||(strcmp(prop,"color1") == 0)||(strcmp(prop,"colorFg") == 0)||(strcmp(prop,"FgColor") == 0)||(strcmp(prop,"col1") == 0)) {
    RGBColor col = val.asColor();
    if (!setFgColor(col.r, col.g, col.b)) return KinoError::InvalidValue;
    return KinoError::OK;
  }
  if ((strcmp(prop,"color2") == 0)||(strcmp(prop,"colorBg") == 0)||(strcmp(prop,"BgColor") == 0)||(strcmp(prop,"col2") == 0)) {
    RGBColor col = val.asColor();
    if (!setBgColor(col.r, col.g, col.b)) return KinoError::InvalidValue;
    return KinoError::OK;
  }
  if ((strcmp(prop,"color3") == 0)||(strcmp(prop,"colorFx") == 0)||(strcmp(prop,"FxColor") == 0)||(strcmp(prop,"col3") == 0)) {
    RGBColor col = val.asColor();
    if (!setFxColor(col.r, col.g, col.b)) return KinoError::InvalidValue;
    return KinoError::OK;
  }

  return KinoError::PropertyNotSupported;
}

KinoError WLEDDevice::queryCount(const char* property, uint16_t& out) {
  constexpr size_t filenameLen = 48;
  char filename[filenameLen];

  if ((strcmp(property, "palette")==0)||(strcmp(property,"pal")==0)) {
    paletteFile(filename, filenameLen);
    out = FileHelper::countLines(filename);
    return KinoError::OK;
  }
  if ((strcmp(property, "effect")==0)||(strcmp(property, "effectname")==0)||(strcmp(property, "fx")==0)) {
    effectsFile(filename, filenameLen);
    out = FileHelper::countLines(filename);
    return KinoError::OK;
  }
  int fxnr;
  // Versuche, Pfad zu erkennen: "fx/<effectId>/param"
  int found = sscanf(property, "fx/%d/param", &fxnr);
  if ((found == 1)&&(fxnr >= 0)) {
    out = countParams(fxnr);
    return KinoError::OK;
  }
  int palnr;
  found = sscanf(property,"pal/%d/param", &palnr);
  if ((found == 1) && (palnr >= 0)) {
    KinoPropertyParam* p = getPaletteParam(palnr, 0);
    if (!p) {
      out = 0;
      return KinoError::OutOfRange;
    }
    int ct = 1;
    while(true) {
      p = getPaletteParam(palnr, ct);
      if (!p) { out = ct; return KinoError::OK; }
      ct++;
    }
    return KinoError::OK;
  }
  out = 0;
  return KinoError::PropertyNotSupported;
}

KinoError WLEDDevice::query(const char* property, uint16_t index, KinoVariant& out) {
  static char filename[48];
  static int nrOfLines = 0;
  if ((strcmp(property,"effect")==0)||(strcmp(property,"fx")==0)) {
    char buf[10];
    itoa(0, buf, 10);
    if (index < 0) { 
      out.setString(buf);
      return KinoError::OutOfRange; 
    }
    char fxFile[48];
    effectsFile(fxFile,48);
    int maxLines = nrOfLines;
    
    if (strcmp(filename, fxFile)!=0) {
      nrOfLines = FileHelper::countLines(fxFile);
      maxLines = nrOfLines;
      strncpy(filename, fxFile, 48); filename[47] = '\0';
    }
    if (index > maxLines) { out.setString(buf); return KinoError::OutOfRange; }
    itoa(index, buf, 10);
    out.setString(buf);
    return KinoError::OK;
  }
  if (strcmp(property,"effectname")==0) {
    char fxFile[48];
    constexpr size_t lineLen = 128;

    // >>> Lazy-Streaming
    // wir missbrauchen _bodybuffer
    if (!readLineSmart(fxFile, index, _bodybuffer, sizeof(_bodybuffer))) return KinoError::InvalidValue;
    stripAfterAt(_bodybuffer);

    out.setString(_bodybuffer);
    return KinoError::OK;
  }
  if ((strcmp(property,"palette")==0)||(strcmp(property,"pal")==0)) {
    char buf[10];
    itoa(0,buf,10);
    if (index < 0) { out.setString(buf); return KinoError::OutOfRange; }
    char palFile[48];
    paletteFile(palFile,48);
    int maxLines = nrOfLines;
    if (strcmp(filename, palFile)!=0) {
      nrOfLines = FileHelper::countLines(palFile);
      maxLines = nrOfLines;
      strncpy(filename, palFile, sizeof(filename)); filename[sizeof(filename)-1] = '\0';
    }
    if (index > maxLines) { out.setString(buf); return KinoError::OutOfRange; }
    itoa(index, buf, 10);
    out.setString(buf);
    return KinoError::OK;
  }
  return KinoError::PropertyNotSupported;
}

void WLEDDevice::effectsFile(char* out, size_t outLen) {
  snprintf(out, outLen, "/wled/%d_%d_%d_%d/effects.txt", _ip[0], _ip[1], _ip[2], _ip[3]);
}

void WLEDDevice::paletteFile(char* out, size_t outLen) {
  snprintf(out, outLen, "/wled/%d_%d_%d_%d/palette.txt", _ip[0], _ip[1], _ip[2], _ip[3]);
}

int WLEDDevice::countParams(size_t linenr) {
  char fxfile[48];
  effectsFile(fxfile, sizeof(fxfile));
  // wir missbrauchen _bodybuffer
  if (!FileHelper::readLineAt(fxfile, linenr, _bodybuffer, sizeof(_bodybuffer))) return 0;

  char* atPtr = strchr(_bodybuffer, '@');
  char* semiPtr = strchr(_bodybuffer, ';');

  if (atPtr == NULL) return 5; // Fallback für alte WLED

  // "part" startet nach '@', endet vor ';' oder am String-Ende
  char* part = atPtr + 1;
  if (semiPtr != NULL) {
    *semiPtr = '\0'; // String am Semikolon kappen, um 'part' zu isolieren
  }

  int count = 0;
  char* currentField = part;

  // Wir prüfen nur die ersten 5 Felder (Slider)
  for (int i = 0; i < 5; i++) {
    // Suche nächstes Komma
    char* commaPos = strchr(currentField, ',');
    if (commaPos != NULL) {
      *commaPos = '\0'; // Feld temporär terminieren
    }

    // überspringe führende Leerzeichen
    while (*currentField == ' ') currentField++;
    
    // Wenn nach dem Trimmen noch Zeichen da sind (und es kein "!" ist, falls gewünscht)
    if (*currentField != '\0') {
      count++;
    }

    if (commaPos == NULL) break; // Kein weiteres Komma -> Ende
    currentField = commaPos + 1; // Nächstes Feld beginnt nach dem Komma
  }

  return count;
}

// Helperfunktion, um den Anzeigenamen eines Effektparameters nr paramnr des Effekts linenr zu bestimmen
// Der String wird direkt aus der Effekt-Datei gelesen. Ist der Parameter aktiv, aber kein spezielles Label
// dafür definiert, wird der Default "sx", "ix"... zurückgegeben
bool WLEDDevice::getParamLabel(size_t linenr, size_t paramnr, char* out, size_t outLen) {
  //char line[128];   // wir missbrauchen den _bodybuffer
  char fxfile[32];
  effectsFile(fxfile, sizeof(fxfile));

  if (!FileHelper::readLineAt(fxfile, linenr, _bodybuffer, sizeof(_bodybuffer))) return false;

  const char* defaults[] = {"sx", "ix", "c1x", "c2x", "c3x"};

  char* atPos = strchr(_bodybuffer, '@');
  char* firstSemi = strchr(_bodybuffer, ';');

  if (!atPos) {
    if (paramnr > 4) return false;
    strncpy(out, defaults[paramnr], outLen);
    return true;
  }

  char* currentPos = atPos + 1;
  if (firstSemi) *firstSemi = '\0';

  int currentActiveIdx = 0;

  for (int i = 0; i < 5; i++) {
    char* commaPos = strchr(currentPos, ',');
    if (commaPos) *commaPos = '\0';

    // Trim leading spaces
    while (*currentPos == ' ') currentPos++;
    
    // Trim trailing spaces
    char* end = currentPos + strlen(currentPos) - 1;
    while (end > currentPos && *end == ' ') { *end = '\0'; end--; }

    if (*currentPos != '\0') {
      if (currentActiveIdx == (int)paramnr) {
        if (strcmp(currentPos, "!") == 0) {
          // Fallback auf Default bei "!"
          strncpy(out, defaults[i], outLen);
        } else {
          // Prüfen auf '=' im Label
          char* equalsPos = strchr(currentPos, '=');
          if (equalsPos) *equalsPos = '\0'; // Label vor dem '=' abschneiden
          
          strncpy(out, currentPos, outLen);
        }
        out[outLen - 1] = '\0';
        return true;
      }
      currentActiveIdx++;
    }

    if (!commaPos) break;
    currentPos = commaPos + 1;
  }
  return false;
}

// Helperfunktion, um den internen Namen eines Effektparameters nr paramnr des Effekts linenr zu bestimmen
// Es werden die aktiv unterstützten Parameter zu dem Effekt aus der Effekt-Datei ausgelesen, und der dem
// paramnr- Index entsprechende Name ("sx","ix"...) zurückgegeben
bool WLEDDevice::getParamField(size_t linenr, size_t paramnr, char* out, size_t outLen) {
  char fxfile[48]; 
  effectsFile(fxfile, sizeof(fxfile));
  // wir missbrauchen _bodybuffer
  if (!FileHelper::readLineAt(fxfile, linenr, _bodybuffer, sizeof(_bodybuffer))) return false;

  const char* defaults[] = {"sx", "ix", "c1x", "c2x", "c3x"};

  char* atPos = strchr(_bodybuffer, '@');
  char* firstSemi = strchr(_bodybuffer, ';');

  if (atPos == nullptr) {
    if (paramnr > 4) return false;
    strncpy(out, defaults[paramnr], outLen);
    return true;
  }

  // "part" ist der Bereich nach '@' bis zum ';' (oder Ende)
  char* partStart = atPos + 1;
  if (firstSemi) *firstSemi = '\0'; // String am Semikolon abschneiden für einfachere Verarbeitung

  int currentActiveIdx = 0;
  char* currentPos = partStart;

  for (int i = 0; i < 5; i++) {
    char* commaPos = strchr(currentPos, ',');
    if (commaPos) *commaPos = '\0'; // Temporär terminieren, um das Label zu isolieren

    // Trim-Logik (führende Leerzeichen überspringen)
    while (*currentPos == ' ') currentPos++;
    
    if (*currentPos != '\0') { // Wenn Label nicht leer
      if (currentActiveIdx == (int)paramnr) {
        strncpy(out, defaults[i], outLen);
        out[outLen - 1] = '\0'; // Sicherstellen, dass terminierte Null vorhanden ist
        return true;
      }
      currentActiveIdx++;
    }
    if (!commaPos) break;
    currentPos = commaPos + 1;
  }
  return false;
}

const KinoPropertyInfo WLEDDevice::_properties[] PROGMEM = {
    { "ip",           "IP",                 Prop_Read },
    { "on",           "Power",              Prop_Read | Prop_Write},
    { "live",         "Live Mode",          Prop_Read },
    { "lor",          "Live Override",      Prop_Read | Prop_Write },
    { "bri",          "Brightness",         Prop_Read | Prop_Write,     0, 255 },
    { "fx",           "Effect",             Prop_Read | Prop_Write | Prop_Query | Prop_hasLabel | Prop_hasParams },
    { "pal",          "Palette",            Prop_Read | Prop_Write | Prop_Query | Prop_hasLabel | Prop_hasParams },
    { "input",        "Input",              Prop_Read },
};

size_t WLEDDevice::getPropertyCount() const {
  return sizeof(_properties) / sizeof(_properties[0]);
}

const KinoPropertyInfo* WLEDDevice::getPropertyInfo(size_t index) const {
  if (index >= getPropertyCount()) return nullptr;
  //return &_properties[index];
  static KinoPropertyInfo buffer; 
  memcpy_P(&buffer, &_properties[index], sizeof(KinoPropertyInfo));
  
  return &buffer;
}

KinoPropertyParam* WLEDDevice::getPaletteParam(int palnr, int paramIndex) {
  // static sorgt dafür, dass das Array im Datensegment lebt und nicht auf dem Stack
  static KinoPropertyParam params[] = {
    {"col1", "Vordergrund", 3},
    {"col2", "Hintergrund", 3},
    {"col3", "Effektfarbe", 3}
  };

  // Ermittlung der Anzahl der Elemente im statischen Array
  constexpr size_t paramsSize = sizeof(params) / sizeof(params[0]);

  if (paramIndex >= 0 && (size_t)paramIndex < paramsSize) {
    return &params[paramIndex]; // Adresse des Elements zurückgeben
  }

  return nullptr;
}

bool WLEDDevice::begin() {
  if (!readState()) return false;
  return true;
}

KinoError WLEDDevice::init() {
  readEffects();
  readPalettes();
  if (readState()) return KinoError::OK;
  return KinoError::DeviceNotReady;
}

bool WLEDDevice::readEffects(bool forceRefresh/*=false*/) {
  char filePath[48];
  effectsFile(filePath, sizeof(filePath));

  if (FileHelper::exists(filePath)) {
    if (!forceRefresh) return true;
    FileHelper::remove(filePath);
  }

  WiFiClient client;
  EnsureTimeoutBeforeRequest(200);
  if (!client.connect(_ip, 80)) {
    Serial.print("WLED: GET ");
    Serial.println("/json");
    Serial.println("could not connect");
    return false;
  }
  client.printf(
    "GET /json/effects HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Connection: close\r\n\r\n",
    _ip
  );
  if (!NetworkHelper::skipHeader(client)) return false;

  // =============== Effektnamen aus dem Stream extrahieren und wegschreiben

  unsigned long startTime = millis();
  bool inString = false;
  bool inParams = false;
  int count = 0;

  // 1. Warte auf Daten (Timeout 5s)
  while (client.available() == 0) {
    if (millis() - startTime > 5000) {
      NetworkHelper::resetClient(client);
      return false;
    }
    delay(1);
  }

  // 2. Stream zeichenweise verarbeiten
  char currentEffect[128];
  size_t index = 0; // Unser Zeiger innerhalb des Puffers

  while (client.connected() || client.available() > 0) {
    if (client.available() > 0) {
      char c = client.read();

      if (c == '"') {
        if (inString) {
          // Ende des Namens erreicht
          currentEffect[index] = '\0'; // String terminieren
          
          if (index > 0) { // index > 0 ist schneller als strlen() > 0
            FileHelper::writeLine(filePath, currentEffect);
            count++;
          }
          
          inString = false;
          index = 0; // <== ENTSCHEIDEND: Zurück auf Anfang für den nächsten Effekt!
        } else {
          inString = true;
          index = 0; // Sicherstellen, dass wir vorne anfangen
        }
      }
      else if (inString) {
        // Überlaufschutz: Nur schreiben, wenn noch Platz für das Zeichen + \0 ist
        if (index < sizeof(currentEffect) - 1) {
          if (c == '\\') {
            // Escape-Logik: Nächstes Zeichen direkt lesen
            if (client.available() > 0 || client.peek() != -1) {
              currentEffect[index++] = client.read();
            }
          } else {
            currentEffect[index++] = c;
          }
        }
      }

      if (!inString && c == ']') break;
    }
    yield();
  }

  // ========================================================================
  NetworkHelper::resetClient(client);
  return true;
}

bool WLEDDevice::readPalettes(bool forceRefresh) {
  char filePath[48];
  paletteFile(filePath, sizeof(filePath));

  if (FileHelper::exists(filePath)) {
    if (!forceRefresh) return true;
    FileHelper::remove(filePath);
  }

  WiFiClient wifi;
  EnsureTimeoutBeforeRequest(200);
  if (!wifi.connect(_ip, 80)) {
    Serial.print("WLED: GET ");
    Serial.println("/json/palettes");
    Serial.println("could not connect");
    //wifi.stop();
    NetworkHelper::resetWiFiClient(wifi);
    yield();
    delay(1000);
    return false;
  }
  wifi.printf(
    "GET /json/palettes HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Connection: close\r\n\r\n",
    _ip
  );
  if (!NetworkHelper::skipHeader(wifi)) return false;

  // =============== Palettennamen aus dem Stream extrahieren und wegschreiben

  unsigned long startTime = millis();
  bool inString = false;
  bool inParams = false;
  int count = 0;

  // 1. Warte auf Daten (Timeout 5s)
  while (wifi.available() == 0) {
    if (millis() - startTime > 5000) {
      //NetworkHelper::resetClient(wifi);
      NetworkHelper::resetWiFiClient(wifi);
      yield();
      delay(1000);
      return false;
    }
    delay(1);
  }

  // 2. Stream zeichenweise verarbeiten
  char currentPalette[128];
  size_t index = 0; // Unser Zeiger innerhalb des Puffers

  while (wifi.connected() || wifi.available() > 0) {
    if (wifi.available() > 0) {
      char c = wifi.read();

      if (c == '"') {
        if (inString) {
          // Ende des Namens erreicht
          currentPalette[index] = '\0'; // String terminieren
          
          if (index > 0) { // index > 0 ist schneller als strlen() > 0
            FileHelper::writeLine(filePath, currentPalette);
            count++;
          }
          
          inString = false;
          index = 0; // <== ENTSCHEIDEND: Zurück auf Anfang für den nächsten Effekt!
        } else {
          inString = true;
          index = 0; // Sicherstellen, dass wir vorne anfangen
        }
      }
      else if (inString) {
        // Überlaufschutz: Nur schreiben, wenn noch Platz für das Zeichen + \0 ist
        if (index < sizeof(currentPalette) - 1) {
          if (c == '\\') {
            // Escape-Logik: Nächstes Zeichen direkt lesen
            if (wifi.available() > 0 || wifi.peek() != -1) {
              currentPalette[index++] = wifi.read();
            }
          } else {
            currentPalette[index++] = c;
          }
        }
      }

      if (!inString && c == ']') break;
    }
    yield();
  }

  // ========================================================================
  //NetworkHelper::resetClient(wifi);
  NetworkHelper::resetWiFiClient(wifi);
  return true;
}

bool WLEDDevice::getStatus() {
  if (!readState()) return false;
  return true;
}

KinoError WLEDDevice::tick() {
  if (WiFi.status() != WL_CONNECTED) return KinoError::NothingToDo;
  bool ok = readState();
  return (ok ? KinoError::OK : KinoError::DeviceNotReady);
}

bool WLEDDevice::getStatusUpdate(const char* devName, JsonObject& root) {
  if (!_dirty) return false;
  static uint8_t part = 0;
  bool ok = false;
  if (part == 0) {
    uint16_t mask = ON|BRI|FX|SX|IX;
    if ( (_dirty & mask)>0 ) {
      root["dev"].set((char*)devName);
      if (_dirty & ON)    root["on"].set(_props.onoff);
      if (_dirty & BRI)   root["bri"].set(_props.bri);
      if (_dirty & FX)    root["fx"].set(_props.fx);
      if (_dirty & SX)    root["sx"].set(_props.sx);
      if (_dirty & IX)    root["ix"].set(_props.ix);
      _dirty &= ~mask;  // bits löschen
      ok = true;
    }
    part = 1;
    return ok;
  }
  if (part == 1) {
    uint16_t mask = FGCOL|BGCOL|FXCOL|PAL;
    if ( (_dirty & mask) > 0) {
      root["dev"].set((char*)devName);
      if (_dirty & FGCOL) { KinoVariant v = KinoVariant::fromColor(_props.col[0]); root["col1"].set(v.c_str()); }
      if (_dirty & BGCOL) { KinoVariant v = KinoVariant::fromColor(_props.col[1]); root["col2"].set(v.c_str()); }
      if (_dirty & FXCOL) { KinoVariant v = KinoVariant::fromColor(_props.col[2]); root["col3"].set(v.c_str()); }
      if (_dirty & PAL)   root["pal"].set(_props.pal);
      _dirty &= ~mask;  // bits löschen
      ok = true;
    }
    part = 2;
    return ok;
  }
  if (part == 2) {
    uint16_t mask = LOR|C1X|C2X|C3X;
    if ( (_dirty & mask) > 0) {
      root["dev"].set((char*)devName);
      if (_dirty & LOR)   root["lor"].set(_props.lor);
      if (_dirty & C1X)   root["c1x"].set(_props.c1x);
      if (_dirty & C2X)   root["c2x"].set(_props.c2x);
      if (_dirty & C3X)   root["c3x"].set(_props.c3x);
      _dirty &= ~mask;
      ok = true;
    }
    part = 0;
    return ok;
  }
  return ok; // nur für den Compiler, hier kommen wir nie an
}

bool WLEDDevice::getPowerStatus() const {
  return _props.onoff;
}

bool WLEDDevice::inAlarm() const {
  return _alarm;
}

bool WLEDDevice::inPause() const {
  return _pause;
}

uint8_t WLEDDevice::getBrightness() const {
  return _props.bri;
}

bool WLEDDevice::isReceivingLiveData() const {
  return _live;
}

bool WLEDDevice::isOverridingLiveData() const {
  return _props.lor;
}

uint8_t WLEDDevice::getSpeed() const {
  return _props.sx;
}

uint8_t WLEDDevice::getIntensity() const {
  return _props.ix;
}

uint16_t WLEDDevice::getEffect() const {
  return _props.fx;
}

uint8_t WLEDDevice::getPalette() const {
  return _props.pal;
}

void WLEDDevice::getLiveSource(char* src, size_t srcLen) {
  strlcpy(src, _livesource, srcLen);
}

RGBColor WLEDDevice::getColFg() const {
  return _props.col[0];
}

RGBColor WLEDDevice::getColBg() const {
  return _props.col[1];
}

RGBColor WLEDDevice::getColFx() const {
  return _props.col[2];
}

bool WLEDDevice::setPowerStatus(bool onoff) {
  _newProps.onoff = onoff;
  _pendingDirty |= ON;
  return true;
}

bool WLEDDevice::setBrightness(uint8_t bri) {
  if (bri < 0)   return false;
  if (bri > 255) return false;

  _newProps.bri = bri;
  _pendingDirty |= BRI;
  return true;
}

bool WLEDDevice::setTransitionTime(int tt) {
  if (tt < 0) return false;
  if (tt < 100) tt = 100;
  _tt = (int)(tt/100);
  return true;
}

bool WLEDDevice::setEffect(uint16_t effect) {
  _newProps.fx = effect;
  _pendingDirty |= FX;
  return true;
}

bool WLEDDevice::setSpeed(uint8_t sx) {
  if (sx < 0)   return false;
  if (sx > 255) return false;
  _newProps.sx = sx;
  _pendingDirty |= SX;
  return true;
}

bool WLEDDevice::setIntensity(uint8_t ix) {
  if (ix < 0)   return false;
  if (ix > 255) return false;
  _newProps.ix = ix;
  _pendingDirty |= IX;
  return true;
}

bool WLEDDevice::setFgColor(uint8_t R, uint8_t G, uint8_t B) {
  //if ((R<0)||(R>255)||(G<0)||(G>255)||(B<0)||(B>255)) return false; // sinnlos: uint8_t ist genau in diesem Bereich!
  _newProps.col[0].r = R;
  _newProps.col[0].g = G;
  _newProps.col[0].b = B;
  _pendingDirty |= FGCOL;
  return true;
}

bool WLEDDevice::setFgColor(RGBColor c) {
  return setFgColor(c.r, c.g, c.b);
}

bool WLEDDevice::setBgColor(uint8_t R, uint8_t G, uint8_t B) {
  //if ((R<0)||(R>255)||(G<0)||(G>255)||(B<0)||(B>255)) return false;
  _newProps.col[1].r = R;
  _newProps.col[1].g = G;
  _newProps.col[1].b = B;
  _pendingDirty |= BGCOL;
  return true;
}

bool WLEDDevice::setBgColor(RGBColor c) {
  return setBgColor(c.r, c.g, c.b);
}

bool WLEDDevice::setFxColor(uint8_t R, uint8_t G, uint8_t B) {
  //if ((R<0)||(R>255)||(G<0)||(G>255)||(B<0)||(B>255)) return false;
  _newProps.col[2].r = R;
  _newProps.col[2].g = G;
  _newProps.col[2].b = B;
  _pendingDirty |= FXCOL;
  return true;
}

bool WLEDDevice::setFxColor(RGBColor c) {
  return setFxColor(c.r, c.g, c.b);
}

bool WLEDDevice::setLive(bool onoff) {
  _newProps.lor = !onoff;
  _pendingDirty |= LOR;
  return true;
}

bool WLEDDevice::fade(uint8_t bri, int tt) {
  if (!setBrightness(bri)) return false;
  if (!setTransitionTime(tt)) return false;
  return applyChanges();
}

bool WLEDDevice::setAlarm(bool onoff) {
  if (onoff == _alarm) return false;              // signal back to controller: no change in alarm state
  _alarm = onoff;
  if (onoff && !_pause) return backupState();     // Gehe von "normal" auf "Alarm"
  if (!onoff && _pause) return true;              // schalte "Alarm" aus. Aufrufende Funktion kümmert sich um Reaktivierung des Pausemodus, wir halten hier nur den Backupstate bereit
  if (onoff && _pause)  return true;              // Gehe von "Pause" auf "Alarm". Das Backup von "Pause" bleibt gültig
  if (!onoff && !_pause)return restoreBackup();   // Gehe von "Alarm" auf "normal". Stelle das Backup wieder her
  return true;
}

bool WLEDDevice::setPause(bool onoff) {
  if (_alarm) return false;                       // Von "Alarm" kann man nicht auf "Pause" wechseln!
  _pause = onoff;
  return (onoff ? backupState() : restoreBackup()); // Beim Pause einschalten: Backup erstellen. Beim Pause ausschalten: Backup wiederherstellen
}

bool WLEDDevice::backupState() {
  if (_alarm || _pause) return true;    // nur Backup machen, wenn wir nicht sowieso schon in Alarm oder Pause sind!
  _bkp.onoff = getPowerStatus();
  _bkp.fx = getEffect();
  _bkp.bri = getBrightness();
  _bkp.sx = getSpeed();
  _bkp.ix = getIntensity();
  _bkp.pal = getPalette();
  _bkp.fgCol = getColFg();
  _bkp.bgCol = getColBg();
  _bkp.fxCol = getColFx();
  return true;
}

bool WLEDDevice::restoreBackup() {
  setPowerStatus(_bkp.onoff);
  setEffect(_bkp.fx);
  setBrightness(_bkp.bri);
  setSpeed(_bkp.sx);
  setIntensity(_bkp.ix);
  setPalette(_bkp.pal);
  setFgColor(_bkp.fgCol);
  setBgColor(_bkp.bgCol);
  setFxColor(_bkp.fxCol);
  return applyChanges();
}

bool WLEDDevice::setCustom(uint8_t c1x, uint8_t c2x, uint8_t c3x) {
  _newProps.c1x = c1x;
  _newProps.c2x = c2x;
  _newProps.c3x = c3x;
  _pendingDirty |= (C1X | C2X | C3X);
  return true;
}

bool WLEDDevice::setCustom1(uint8_t c1x) {
  _newProps.c1x = c1x;
  _pendingDirty |= C1X;
  return true;
}

bool WLEDDevice::setCustom2(uint8_t c2x) {
  _newProps.c2x = c2x;
  _pendingDirty |= C2X;
  return true;
}

bool WLEDDevice::setCustom3(uint8_t c3x) {
  _newProps.c3x = c3x;
  _pendingDirty |= C3X;
  return true;
}

bool WLEDDevice::setPalette(uint8_t pal) {
  _newProps.pal = pal;
  _pendingDirty |= PAL;
  return true;
}

void WLEDDevice::EnsureTimeoutBeforeRequest(unsigned long timeout) {
  static unsigned long LastRequest = 0;
  unsigned long now = millis();
  while (now - LastRequest < timeout) {
    delay(10);
    now = millis();
  }
  LastRequest = millis();
  return;
}

bool WLEDDevice::readState() {
  bool      onBefore  = _props.onoff;
  uint8_t   briBefore = _props.bri;
  uint8_t   fxBefore  = _props.fx;
  uint8_t   sxBefore  = _props.sx;
  uint8_t   ixBefore  = _props.ix;
  RGBColor  c1Before  = _props.col[0];
  RGBColor  c2Before  = _props.col[1];
  RGBColor  c3Before  = _props.col[2];
  bool      lorBefore = _props.lor;
  uint8_t   c1xBefore = _props.c1x;
  uint8_t   c2xBefore = _props.c2x;
  uint8_t   c3xBefore = _props.c3x;
  uint8_t   palBefore = _props.pal;
  WiFiClient wifi;
  HTTPClient http;
  
  EnsureTimeoutBeforeRequest(200);

  char url[32];
  snprintf(url, sizeof(url), "http://%d.%d.%d.%d/json", _ip[0], _ip[1], _ip[2], _ip[3]);
  http.setTimeout(3000);
  
  if (http.begin(wifi, url)) {
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
      initFilter();
      httpJson.clear();
      DeserializationError error = deserializeJson(httpJson, http.getStream(), DeserializationOption::Filter(_jsonFilter));

      if (!error) {
        _props.onoff        = httpJson["state"]["on"] | false;
        _props.bri          = httpJson["state"]["bri"] | 0;
        _props.fx           = httpJson["state"]["seg"][0]["fx"] | 0;
        _props.sx           = httpJson["state"]["seg"][0]["sx"] | 0;
        _props.ix           = httpJson["state"]["seg"][0]["ix"] | 0;
        _props.col[0].r     = httpJson["state"]["seg"][0]["col"][0][0] | 0;
        _props.col[0].g     = httpJson["state"]["seg"][0]["col"][0][1] | 0;
        _props.col[0].b     = httpJson["state"]["seg"][0]["col"][0][2] | 0;
        _props.col[1].r     = httpJson["state"]["seg"][0]["col"][1][0] | 0;
        _props.col[1].g     = httpJson["state"]["seg"][0]["col"][1][1] | 0;
        _props.col[1].b     = httpJson["state"]["seg"][0]["col"][1][2] | 0;
        _props.col[2].r     = httpJson["state"]["seg"][0]["col"][2][0] | 0;
        _props.col[2].g     = httpJson["state"]["seg"][0]["col"][2][1] | 0;
        _props.col[2].b     = httpJson["state"]["seg"][0]["col"][2][2] | 0;
        _props.lor          = httpJson["state"]["lor"] | 0;
        _props.c1x          = httpJson["state"]["seg"][0]["c1x"] | 0;
        _props.c2x          = httpJson["state"]["seg"][0]["c2x"] | 0;
        _props.c3x          = httpJson["state"]["seg"][0]["c3x"] | 0;
        _props.pal          = httpJson["state"]["seg"][0]["pal"] | 0;
        strlcpy(_livesource,  httpJson["info"]["lm"]|"", sizeof(_livesource));
        _live               = httpJson["info"]["live"]|false;
        if (_props.onoff    != onBefore)    _dirty |= ON;
        if (_props.bri      != briBefore)   _dirty |= BRI;
        if (_props.fx       != fxBefore)    _dirty |= FX;
        if (_props.sx       != sxBefore)    _dirty |= SX;
        if (_props.ix       != ixBefore)    _dirty |= IX;
        if (_props.col[0]   != c1Before)    _dirty |= FGCOL;
        if (_props.col[1]   != c2Before)    _dirty |= BGCOL;
        if (_props.col[2]   != c3Before)    _dirty |= FXCOL;
        if (_props.lor      != lorBefore)   _dirty |= LOR;
        if (_props.c1x      != c1xBefore)   _dirty |= C1X;
        if (_props.c2x      != c2xBefore)   _dirty |= C2X;
        if (_props.c3x      != c3xBefore)   _dirty |= C3X;
        if (_props.pal      != palBefore)   _dirty |= PAL;

        syncNewPropsAfterReadState();
        
        //http.end();
        //wifi.stop();
        NetworkHelper::resetClients(wifi, http, true);
        return true;
      } else {
        Serial.printf("WLED Parsing error: %s\n", error.c_str());
      }
    } else {
      Serial.printf("WLED HTTP failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    //http.end();
    //wifi.stop();
    //NetworkHelper::resetClients(wifi, http, false);
  }
  NetworkHelper::resetClients(wifi, http, false);
  return false;
}

void WLEDDevice::syncNewPropsAfterReadState() {
  if (!(_pendingDirty & ON))    _newProps.onoff = _props.onoff;
  if (!(_pendingDirty & BRI))   _newProps.bri   = _props.bri;
  if (!(_pendingDirty & FX))    _newProps.fx    = _props.fx;
  if (!(_pendingDirty & SX))    _newProps.sx    = _props.sx;
  if (!(_pendingDirty & IX))    _newProps.ix    = _props.ix;
  if (!(_pendingDirty & FGCOL)) _newProps.col[0]= _props.col[0];
  if (!(_pendingDirty & BGCOL)) _newProps.col[1]= _props.col[1];
  if (!(_pendingDirty & FXCOL)) _newProps.col[2]= _props.col[2];
  if (!(_pendingDirty & LOR))   _newProps.lor   = _props.lor;
  if (!(_pendingDirty & C1X))   _newProps.c1x   = _props.c1x;
  if (!(_pendingDirty & C2X))   _newProps.c2x   = _props.c2x;
  if (!(_pendingDirty & C3X))   _newProps.c3x   = _props.c3x;
  if (!(_pendingDirty & PAL))   _newProps.pal   = _props.pal;
}

bool WLEDDevice::applyChanges() {
  if (_pendingDirty == NONE) return true;

  int pos = 0;

  // Start des JSON
  pos += snprintf(_bodybuffer + pos, sizeof(_bodybuffer) - pos, "{\"on\":%s,\"bri\":%d,\"lor\":%s", 
                  _newProps.onoff ? "true" : "false", _newProps.bri, _newProps.lor ? "true" : "false");

  // Segment-Daten (FX, Speed, Intensity, Palette, Farben, etc.)
  // Wir prüfen, ob IRGENDEIN Segment-relevanter Wert dirty ist
  if (_pendingDirty & (FX | SX | IX | PAL | C1X | C2X | C3X | FGCOL | BGCOL | FXCOL)) {
    pos += snprintf(_bodybuffer + pos, sizeof(_bodybuffer) - pos, ",\"seg\":[{\"fx\":%d,\"sx\":%d,\"ix\":%d,\"pal\":%d,\"c1x\":%d,\"c2x\":%d,\"c3x\":%d",
                    _newProps.fx, _newProps.sx, _newProps.ix, _newProps.pal, _newProps.c1x, _newProps.c2x, _newProps.c3x);
    
    // Farben nur mitschicken, wenn eine davon dirty ist (WLED erwartet bei "col" ein Array von Arrays)
    if (_pendingDirty & (FGCOL | BGCOL | FXCOL)) {
      pos += snprintf(_bodybuffer + pos, sizeof(_bodybuffer) - pos, ",\"col\":[[%d,%d,%d],[%d,%d,%d],[%d,%d,%d]]",
                      _newProps.col[0].r, _newProps.col[0].g, _newProps.col[0].b,
                      _newProps.col[1].r, _newProps.col[1].g, _newProps.col[1].b,
                      _newProps.col[2].r, _newProps.col[2].g, _newProps.col[2].b);
    }
    pos += snprintf(_bodybuffer + pos, sizeof(_bodybuffer) - pos, "}]");
  }
  
  pos += snprintf(_bodybuffer + pos, sizeof(_bodybuffer) - pos, "}");

  WiFiClient wifi;
  HTTPClient http;
  char url[32];
  snprintf(url, sizeof(url), "http://%d.%d.%d.%d/json/state", _ip[0], _ip[1], _ip[2], _ip[3]);

  if (http.begin(wifi, url)) {
    http.addHeader(F("Content-Type"), F("application/json"));
    
    int httpCode = http.POST((uint8_t*)_bodybuffer, pos);
    bool success = (httpCode == HTTP_CODE_OK || httpCode == 204 || httpCode == 207);

    if (success) {
      _props = _newProps; // Synchronisieren
      _pendingDirty = NONE;
    }
    //http.end();
    //wifi.stop();
    NetworkHelper::resetClients(wifi, http, success);
    return success;
  }
  NetworkHelper::resetClients(wifi, http, false);
  return false;
}

// statischer Puffer für Anfragen (kann ggf missbraucht werden, um Stack zu sparen)
char WLEDDevice::_bodybuffer[256];

// Json Filter für die Properties und pending
StaticJsonDocument<512> WLEDDevice::_jsonFilter;

void WLEDDevice::initFilter() {
  if (_jsonFilter.size() > 0) return;
  _jsonFilter.clear();
  _jsonFilter["state"]["on"] = true;
  _jsonFilter["state"]["bri"] = true;
  _jsonFilter["state"]["lor"] = true;
  _jsonFilter["state"]["tt"] = true;
  _jsonFilter["state"]["seg"][0]["tt"] = true;
  _jsonFilter["state"]["seg"][0]["fx"] = true;
  _jsonFilter["state"]["seg"][0]["sx"] = true;
  _jsonFilter["state"]["seg"][0]["ix"] = true;
  _jsonFilter["state"]["seg"][0]["pal"] = true;
  _jsonFilter["state"]["seg"][0]["col"] = true;
  _jsonFilter["state"]["seg"][0]["c1x"] = true;
  _jsonFilter["state"]["seg"][0]["c2x"] = true;
  _jsonFilter["state"]["seg"][0]["c3x"] = true;
  _jsonFilter["info"]["live"] = true;
  _jsonFilter["info"]["lm"] = true;
}
