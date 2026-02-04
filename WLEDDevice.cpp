#include "WLEDDevice.h"
#include "NetworkHelper.h"
#include "FileHelper.h"

// ===== Lazy Streaming (pro Instanz) =====

void WLEDDevice::closeLazyStream() {
  if (_lazyActive) {
    if (_lazyFile) _lazyFile.close();
    _lazyActive = false;
    _lazyPath = "";
    _lazyLastIndex = -1;
  }
}

bool WLEDDevice::ensureLazyStream(const String& path, int index) {
  // Stream passt und ist genau an der nächsten erwarteten Zeile
  if (_lazyActive &&
      _lazyPath == path &&
      index == _lazyLastIndex + 1 &&
      _lazyFile)
  {
    return true;
  }

  // Index 0: wir starten neu
  if (index == 0) {
    closeLazyStream();
    if (!FileHelper::begin()) return false;

    _lazyFile = LittleFS.open(path, "r");
    if (!_lazyFile) {
      closeLazyStream();
      return false;
    }

    _lazyPath = path;
    _lazyLastIndex = -1;
    _lazyActive = true;
    Serial.print("starting file stream "); Serial.println(path);
    return true;
  }

  // Alles andere: kein Streaming (Sprung oder andere Datei)
  Serial.print("using classic file reader for "); Serial.println(path);
  return false;
}

/*
// deprecated: char* version below!
bool WLEDDevice::readLineSmart(const String& path, int index, String& out) {
  // ---------- Streaming möglich? ----------
  if (_chunkActive &&
      String(_chunkPath) == path &&
      index == _chunkLastIndex + 1)
  {
    // OK, weiter im Chunk-Stream
  }
  else if (index == 0) {
    // Neuer Stream
    closeChunkStream();
    //Serial.println("starting chunk stream");
    if (!FileHelper::begin()) return false;
    _chunkFile = LittleFS.open(path, "r");
    if (!_chunkFile) {
      closeChunkStream();
      return false;
    }

    _chunkPath = path;
    _chunkActive = true;
    _chunkLastIndex = -1;
    _chunkLen = 0;
    _chunkPos = 0;
  }
  else {
    // Kein sequentieller Zugriff -> Fallback
    // Das bezieht sich noch auf die alte Version von readLineAt
    // Die neue Signatur ist 
    // bool readLineAt(const char* path, size_t index, char* out, size_t outLen)
    return FileHelper::readLineAt(path.c_str(), index, out);  
  }

  // ---------- Zeile aus Chunk lesen ----------
  //Serial.print(".");
  while (true) {
    // Suche '\n' im vorhandenen Buffer
    for (size_t i = _chunkPos; i < _chunkLen; i++)
    {
      if (_chunkBuf[i] == '\n') {
        out = "";
        out.reserve(i - _chunkPos);
        out.concat(_chunkBuf + _chunkPos, i - _chunkPos);
        out.trim();

        _chunkPos = i + 1;
        _chunkLastIndex++;
        return true;
      }
    }

    // Kein '\n' gefunden → mehr Daten nötig
    if (!_chunkFile.available()) {
      // EOF: evtl. letzte Zeile ohne '\n'
      if (_chunkPos < _chunkLen) {
        out = "";
        out.reserve(_chunkLen - _chunkPos);
        out.concat(_chunkBuf + _chunkPos, _chunkLen - _chunkPos);
        out.trim();

        _chunkLastIndex++;
        closeChunkStream();
        return true;
      }

      closeChunkStream();
      return false;
    }

    // Rest nach vorne schieben
    size_t rest = _chunkLen - _chunkPos;
    if (rest > 0) {
      memmove(_chunkBuf, _chunkBuf + _chunkPos, rest);
    }

    _chunkPos = 0;
    _chunkLen = rest;

    // Neu lesen (großer Block!)
    //Serial.print("O");
    size_t space = CHUNK_SIZE - _chunkLen;
    size_t readBytes = _chunkFile.readBytes(
      _chunkBuf + _chunkLen,
      space
    );

    _chunkLen += readBytes;
  }
}*/

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
    //_chunkPath = path; // String-Zuweisung (falls _chunkPath noch String ist)
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

// deprecated: char* version below!
void WLEDDevice::stripAfterAt(String& s) {
  int at = s.indexOf('@');
  if (at != -1) s = s.substring(0, at);
}

void WLEDDevice::stripAfterAt(char* s) {
  char* atPos = strchr(s, '@');
  if (atPos) *atPos = '\0'; // String am @ abschneiden
}

// ===== Konstruktoren =====

WLEDDevice::WLEDDevice(const IPAddress& ip)
: _ip(ip) {}

WLEDDevice::WLEDDevice(const String& ip) {
  _ip.fromString(ip);
}

bool WLEDDevice::needsCommit() {
  return true;
}

// neue Public API: generischer setter/getter
bool WLEDDevice::commit() {
  return applyChanges();
}

KinoError WLEDDevice::get(const char* prop, KinoVariant& out) {
  if (!prop) return KinoError::PropertyNotSupported;
  if (strcmp(prop,"tickInterval")==0) {
    //out = KinoVariant::fromInt(_tickInterval);        // <= vorher
    out.setInt(_tickInterval);
    return KinoError::OK;
  }
  if (strcmp(prop,"ip")==0) {
    //out = KinoVariant::fromString(_ip.toString().c_str());
    char buf[20];
    snprintf(buf,20,"%d.%d.%d.%d", _ip[0], _ip[1], _ip[2], _ip[3]);
    out.setString(buf);
    return KinoError::OK;
  }
  if ((strcmp(prop,"power")==0)||(strcmp(prop,"on")==0)) {
    //out = KinoVariant::fromBool(_props["state"]["on"] | false);
    out.setBool(_props["state"]["on"]|false);
    return KinoError::OK;
  }
  if (strcmp(prop,"live")==0) {
    //out = KinoVariant::fromBool(_props["info"]["live"]|false);
    out.setBool(_props["info"]["live"]|false);
    return KinoError::OK;
  }
  if ((strcmp(prop,"override")==0)||(strcmp(prop,"lor")==0)) {
    //out = KinoVariant::fromBool(_props["state"]["lor"]|false);
    out.setBool(_props["state"]["lor"]|false);
    return KinoError::OK;
  }
  if ((strcmp(prop,"brightness")==0)||(strcmp(prop,"bri")==0)) {
    //out = KinoVariant::fromInt(_props["state"]["bri"] | 0);
    out.setInt(_props["state"]["bri"] | 0);
    return KinoError::OK;
  }
  if ((strcmp(prop,"speed")==0)||(strcmp(prop,"sx")==0)) {
    //out = KinoVariant::fromInt(_props["state"]["seg"][0]["sx"] | 0);
    out.setInt(_props["state"]["seg"][0]["sx"] | 0);
    return KinoError::OK;
  }
  if ((strcmp(prop,"intensity")==0)||(strcmp(prop,"ix")==0)) {
    //out = KinoVariant::fromInt(_props["state"]["seg"][0]["ix"] | 0);
    out.setInt(_props["state"]["seg"][0]["ix"] | 0);
    return KinoError::OK;
  }
  if ((strcmp(prop,"c1x")==0)||(strcmp(prop,"custom1")==0)) {
    //out = KinoVariant::fromInt(_props["state"]["seg"][0]["c1x"] | 0);
    out.setInt(_props["state"]["seg"][0]["c1x"] | 0);
    return KinoError::OK;
  }
  if ((strcmp(prop,"c2x")==0)||(strcmp(prop,"custom2")==0)) {
    //out = KinoVariant::fromInt(_props["state"]["seg"][0]["c2x"] | 0);
    out.setInt(_props["state"]["seg"][0]["c2x"] | 0);
    return KinoError::OK;
  }
  if ((strcmp(prop,"c3x")==0)||(strcmp(prop,"custom3")==0)) {
    //out = KinoVariant::fromInt(_props["state"]["seg"][0]["c13"] | 0);
    out.setInt(_props["state"]["seg"][0]["c13"] | 0);
    return KinoError::OK;
  }
  if ((strcmp(prop,"effect")==0)||(strcmp(prop,"fx")==0)) {
    //out = KinoVariant::fromInt(_props["state"]["seg"][0]["fx"] | 0);
    out.setInt(_props["state"]["seg"][0]["fx"] | 0);
    return KinoError::OK;
  }
  if (strcmp(prop,"effectname")==0) {
    /*String ip = _ip.toString();
    ip.replace(".","_");
    String effectsFile = "/wled/" + ip + "/effects.txt";
    String line;
    int index = _props["state"]["seg"][0]["fx"] | 0;

    // >>> Lazy-Streaming statt readLineAt()
    if (!readLineSmart(effectsFile, index, line)) return KinoError::InvalidValue;
    stripAfterAt(line);

    //out = KinoVariant::fromString(line.c_str());
    out.setString(line.c_str());
    return KinoError::OK;*/
    return KinoError::PropertyNotSupported;
  }
  if ((strcmp(prop,"palette")==0)||(strcmp(prop,"pal")==0)) {
    //out = KinoVariant::fromInt(_props["state"]["seg"][0]["pal"] | 0);
    out.setInt(_props["state"]["seg"][0]["pal"] | 0);
    return KinoError::OK;
  }
  if (strcmp(prop,"input")==0) {
    //out = KinoVariant::fromString(_props["info"]["lm"] | "");
    out.setString(_props["info"]["lm"] | "");
    return KinoError::OK;
  }
  if ((strcmp(prop,"color") == 0)||(strcmp(prop,"color1") == 0)||(strcmp(prop,"colorFg") == 0)||(strcmp(prop,"FgColor") == 0)||(strcmp(prop,"col1")==0)) {
    /*
     // vorher
     RGBColor c = {
      _props["state"]["seg"][0]["col"][0][0] | 0,
      _props["state"]["seg"][0]["col"][0][1] | 0,
      _props["state"]["seg"][0]["col"][0][2] | 0
    };
    out = KinoVariant::fromColor(c);
    */
    // neu
    JsonVariant col = _props["state"]["seg"][0]["col"][0];
    out.setColor(col[0]|0, col[1]|0, col[2]|0);
    return KinoError::OK;
  }
  if ((strcmp(prop,"color2") == 0)||(strcmp(prop,"colorBg") == 0)||(strcmp(prop,"BgColor") == 0)||(strcmp(prop,"col2")==0)) {
    /*
    RGBColor c = {
      _props["state"]["seg"][0]["col"][1][0] | 0,
      _props["state"]["seg"][0]["col"][1][1] | 0,
      _props["state"]["seg"][0]["col"][1][2] | 0
    };
    out = KinoVariant::fromColor(c);
    */
    JsonVariant col = _props["state"]["seg"][0]["col"][1];
    out.setColor(col[0]|0, col[1]|0, col[2]|0);
    return KinoError::OK;
  }
  if ((strcmp(prop,"color3") == 0)||(strcmp(prop,"colorFx") == 0)||(strcmp(prop,"FxColor") == 0)||(strcmp(prop,"col3")==0)) {
    /*
    RGBColor c = {
      _props["state"]["seg"][0]["col"][2][0] | 0,
      _props["state"]["seg"][0]["col"][2][1] | 0,
      _props["state"]["seg"][0]["col"][2][2] | 0
    };
    out = KinoVariant::fromColor(c);
    */
    JsonVariant col = _props["state"]["seg"][0]["col"][2];
    out.setColor(col[0]|0, col[1]|0, col[2]|0);
    return KinoError::OK;
  }
  int fxnr, paramnr, found;
  char pathEnd[20];
  found = sscanf(prop,"fx/%d/param/%d%20s", &fxnr, &paramnr, pathEnd);
  if ((found == 2) && (fxnr >= 0) && (paramnr >= 0)) {// Pfad ist "fx/<effectId>/param/<paramIndex>", ohne pathEnd
    /*
    String getsetPath;
    if (!getParamField(fxnr, paramnr, getsetPath)) return KinoError::InvalidValue;
    out = KinoVariant::fromString(getsetPath.c_str());
    */
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
      /*
      String paramLabel;
      if (!getParamLabel(fxnr,paramnr,paramLabel)) return KinoError::InvalidValue;
      out = KinoVariant::fromString(paramLabel.c_str());*/
      char label[32];
      if (!getParamLabel(fxnr, paramnr, label, 32)) { out.setNone(); return KinoError::InvalidValue; }
      out.setString(label);
      return KinoError::OK;
    }
    if (strcmp(pathEnd,"/access")==0) {
      //out = KinoVariant::fromInt(3);  // immer read/write
      out.setInt(3);
      return KinoError::OK;
    }
    if (strcmp(pathEnd,"/minvalue")==0) {
      //out = KinoVariant::fromInt(0);
      out.setInt(0);
      return KinoError::OK;
    }
    if (strcmp(pathEnd,"/maxvalue")==0) {
      //out = KinoVariant::fromInt(255);
      out.setInt(255);
      return KinoError::OK;
    }
    if (strcmp(pathEnd,"/valuestep")==0) {
      //out = KinoVariant::fromInt(1);
      out.setInt(1);
      return KinoError::OK;
    }
    return KinoError::PropertyNotSupported;
  }
  found = sscanf(prop,"fx/%d/label%20s", &fxnr, pathEnd);
  if ((found == 1) && (fxnr >= 0)) {
    //String fxFile = effectsFile();
    char fxFile[48]; effectsFile(fxFile, sizeof(fxFile));
    //String line;
    constexpr size_t lineLen = 128;
    char line[lineLen];

    // >>> Lazy-Streaming
    //if (!readLineSmart(fxFile, fxnr, line)) return KinoError::InvalidValue;
    if (!readLineSmart(fxFile, fxnr, line, lineLen)) { out.setNone(); return KinoError::InvalidValue; }
    stripAfterAt(line);

    //out = KinoVariant::fromString(line.c_str());
    out.setString(line);
    return KinoError::OK;
  }
  int palnr; char rest1[32]; int paramIndex; char rest2[32];
  found = sscanf(prop,"pal/%d/%31[^/]/%d/%31s", &palnr, rest1, &paramIndex, rest2);
  if ((found == 2) && (palnr >= 0) && (strlen(rest1)>0)) {      // path = "pal/<palNr>/<rest1>
    if (strcmp(rest1,"label")==0) {
      //String palFile = paletteFile();
      char palFile[48];
      paletteFile(palFile, sizeof(palFile));
      
      //String line;
      constexpr size_t lineLen = 128;
      char line[lineLen];

      //if (!readLineSmart(palFile, palnr, line)) return KinoError::InvalidValue;
      if (!readLineSmart(palFile, palnr, line, lineLen)) {out.setNone(); return KinoError::InvalidValue; }

      //out = KinoVariant::fromString(line.c_str());
      out.setString(line);
      return KinoError::OK;
    }
  }
  if ((found ==3) && (palnr >= 0) && (strlen(rest1)>0) && strcmp(rest1,"param")==0) { // path = "pal/<palNr>/param/<paramIndex>"
    /*
    std::vector<KinoPropertyParam> params = getPaletteParams(palnr);
    if (paramIndex >= params.size()) return KinoError::OutOfRange;
    out = KinoVariant::fromString(params[paramIndex].getsetPath);
    */
    KinoPropertyParam* p = getPaletteParam(palnr, paramIndex);
    if (!p) { out.setNone(); return KinoError::OutOfRange; }
    out.setString(p->getsetPath);
    return KinoError::OK;
  }
  if ((found==4) && (palnr>=0) && (strlen(rest1)>0) && (strcmp(rest1,"param")==0) && (strlen(rest2)>0)) {
    /*
    std::vector<KinoPropertyParam> params = getPaletteParams(palnr);
    if (paramIndex >= params.size()) return KinoError::OutOfRange;
    */
    KinoPropertyParam* p = getPaletteParam(palnr, paramIndex);
    if (!p) { out.setNone(); return KinoError::OutOfRange; }
    
    if (strcmp(rest2,"label")==0) {
      //out = KinoVariant::fromString(params[paramIndex].label);
      out.setString(p->label);
      return KinoError::OK;
    }
    if (strcmp(rest2,"access")==0) {
      //out = KinoVariant::fromInt(params[paramIndex].access);
      out.setInt(p->access);
      return KinoError::OK;
    }
    if (strcmp(rest2,"minvalue")==0) {
      //out = KinoVariant::fromInt(params[paramIndex].minvalue.value_or(0));
      out.setInt(p->minvalue.value_or(0));
      return KinoError::OK;
    }
    if (strcmp(rest2,"maxvalue")==0) {
      //out = KinoVariant::fromInt(params[paramIndex].maxvalue.value_or(100));
      out.setInt(p->maxvalue.value_or(100));
      return KinoError::OK;
    }
    if (strcmp(rest2,"valuestep")==0) {
      //out = KinoVariant::fromInt(params[paramIndex].valuestep.value_or(1));
      out.setInt(p->valuestep.value_or(1));
      return KinoError::OK;
    }
  }
  return KinoError::PropertyNotSupported;
}

KinoError WLEDDevice::set(const char* prop, const KinoVariant& val) {
  if (strcmp(prop,"tickInterval")==0) {
    if (!setTickInterval(val.asInt())) return KinoError::InvalidValue;
    return KinoError::OK;
  }
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
  // sauber: bei neuem "Batch" alten Stream schließen
  closeLazyStream();
  constexpr size_t filenameLen = 48;
  char filename[filenameLen];

  if ((strcmp(property, "palette")==0)||(strcmp(property,"pal")==0)) {
    //String fxFile = paletteFile();
    paletteFile(filename, filenameLen);
    //out = FileHelper::countLines(fxFile.c_str());
    out = FileHelper::countLines(filename);
    return KinoError::OK;
  }
  if ((strcmp(property, "effect")==0)||(strcmp(property, "effectname")==0)||(strcmp(property, "fx")==0)) {
    //String fxFile = effectsFile();
    effectsFile(filename, filenameLen);
    //out = FileHelper::countLines(fxFile.c_str());
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
    /*
    std::vector<KinoPropertyParam> params = getPaletteParams(palnr);
    out = params.size();*/
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
  //static String filename = "";
  static char filename[48];
  static int nrOfLines = 0;
  if ((strcmp(property,"effect")==0)||(strcmp(property,"fx")==0)) {
    char buf[10];
    itoa(0, buf, 10);
    if (index < 0) {
      //out = KinoVariant::fromString("0"); 
      out.setString(buf);
      return KinoError::OutOfRange; 
    }
    /*String ip = _ip.toString();
    ip.replace(".","_");
    String effectsFile = "/wled/" + ip + "/effects.txt";*/
    char fxFile[48];
    effectsFile(fxFile,48);
    int maxLines = nrOfLines;
    
    //if (filename != effectsFile) {
    if (strcmp(filename, fxFile)!=0) {
      //nrOfLines = FileHelper::countLines(effectsFile.c_str());
      nrOfLines = FileHelper::countLines(fxFile);
      maxLines = nrOfLines;
      //filename = effectsFile;
      strncpy(filename, fxFile, 48); filename[47] = '\0';
    }
    if (index > maxLines) {/*out = KinoVariant::fromString("0");*/out.setString(buf); return KinoError::OutOfRange; }
    //out = KinoVariant::fromString(String(index).c_str());
    itoa(index, buf, 10);
    out.setString(buf);
    return KinoError::OK;
  }
  if (strcmp(property,"effectname")==0) {
    //String fxFile = effectsFile();
    char fxFile[48];
    //String line;
    constexpr size_t lineLen = 128;
    char line[lineLen];

    // >>> Lazy-Streaming
    //if (!readLineSmart(fxFile, index, line)) return KinoError::InvalidValue;
    if (!readLineSmart(fxFile, index, line, lineLen)) return KinoError::InvalidValue;
    stripAfterAt(line);

    //out = KinoVariant::fromString(line.c_str());
    out.setString(line);
    return KinoError::OK;
  }
  if ((strcmp(property,"palette")==0)||(strcmp(property,"pal")==0)) {
    char buf[10];
    itoa(0,buf,10);
    if (index < 0) {/*out = KinoVariant::fromString("0");*/out.setString(buf); return KinoError::OutOfRange; }
    //String palFile = paletteFile();
    char palFile[48];
    paletteFile(palFile,48);
    int maxLines = nrOfLines;
    //if(palFile != filename) {
    if (strcmp(filename, palFile)!=0) {
      //nrOfLines = FileHelper::countLines(palFile.c_str());
      nrOfLines = FileHelper::countLines(palFile);
      maxLines = nrOfLines;
      //filename = palFile;
      strncpy(filename, palFile, sizeof(filename)); filename[sizeof(filename)-1] = '\0';
    }
    if (index > maxLines) {/*out = KinoVariant::fromString("0");*/out.setString(buf); return KinoError::OutOfRange; }
    itoa(index, buf, 10);
    //out = KinoVariant::fromString(String(index).c_str());
    out.setString(buf);
    return KinoError::OK;
  }
  return KinoError::PropertyNotSupported;
}

// deprecated: char* version below!
String WLEDDevice::effectsFile() {
  String ip = _ip.toString();
  ip.replace(".","_");
  String fxFile = "/wled/" + ip + "/effects.txt";
  return fxFile;
}

// Helperfunktion zum Erstellen des Dateinamens "wled/xxx_xx_xx_xx/effects.txt
void WLEDDevice::effectsFile(char* out, size_t outLen) {
  snprintf(out, outLen, "/wled/%d_%d_%d_%d/effects.txt", _ip[0], _ip[1], _ip[2], _ip[3]);
}

// deprecated: char* version below!
String WLEDDevice::paletteFile() {
  String ip = _ip.toString();
  ip.replace(".","_");
  String fxFile = "/wled/" + ip + "/palette.txt";
  return fxFile;
}

// Helperfunktion zum Erstellen des Dateinamens "/wled/xxx_xx_xx_xx_palette.txt"
void WLEDDevice::paletteFile(char* out, size_t outLen) {
  snprintf(out, outLen, "/wled/%d_%d_%d_%d/palette.txt", _ip[0], _ip[1], _ip[2], _ip[3]);
}

// Helperfunktion zum Zählen von Parametern eines bestimmten Effekts
/* countParams(size_t linenr) V1
int WLEDDevice::countParams(size_t linenr) {
    String line; line.reserve(128);
    //String fxfile = effectsFile();
    char fxfile[48];
    effectsFile(fxfile,48);
    if (!FileHelper::readLineAt(fxfile, linenr, line)) return 0;

    int atPos = line.indexOf('@');
    int firstSemi = line.indexOf(';');
    if (atPos == -1) return 5;  // Keine Parameter-Spezifikation: Fallback für alte WLED: alle 5 Slider

    String part = (firstSemi == -1) ? line.substring(atPos + 1) : line.substring(atPos + 1, firstSemi);

    int count = 0;
    int start = 0;

    // Wir prüfen nur die ersten 5 möglichen Felder (Slider)
    for (int i = 0; i < 5; i++) {
        int commaPos = part.indexOf(',', start);
        String label = (commaPos == -1) ? part.substring(start) : part.substring(start, commaPos);
        label.trim();

        // Aktiv, wenn Feld Text oder "!" enthält
        if (label.length() > 0) {
            count++;
        }

        if (commaPos == -1) break; // Ende des Segments erreicht
        start = commaPos + 1;
    }
    return count;
}*/
/* countParams(size_t linenr) V2 2026-02-03 : Strings entfernt und durch char* ersetzt */
int WLEDDevice::countParams(size_t linenr) {
    char line[128]; 
    char fxfile[48];
    effectsFile(fxfile, sizeof(fxfile));

    // Deine neue readLineAt Funktion nutzen
    if (!FileHelper::readLineAt(fxfile, linenr, line, sizeof(line))) return 0;

    // indexOf('@') -> strchr(line, '@')
    char* atPtr = strchr(line, '@');
    // indexOf(';') -> strchr(line, ';')
    char* semiPtr = strchr(line, ';');

    if (atPtr == NULL) return 5; // Fallback für alte WLED

    // "part" simulieren: Startet nach '@', endet vor ';' oder am String-Ende
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

        // "trim()" und "length() > 0" Logik
        // Wir überspringen führende Leerzeichen
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


// deprecated: char* version below!
bool WLEDDevice::getParamLabel(size_t linenr, size_t paramnr, String& out) {
  String line;
  //String fxfile = effectsFile();
  char fxfile[48];
  effectsFile(fxfile,48);
    if (!FileHelper::readLineAt(fxfile, linenr, line)) return false;

    // Standardnamen nur für die 5 Slider
    const char* defaults[] = {"sx", "ix", "c1x", "c2x", "c3x"};

    int atPos = line.indexOf('@');
    int firstSemi = line.indexOf(';');
    if (atPos == -1) {
      if ((paramnr < 0) || (paramnr > 4)) return false;
      out = defaults[paramnr];
      return true;
    }

    String part = (firstSemi == -1) ? line.substring(atPos + 1) : line.substring(atPos + 1, firstSemi);

    int currentActiveIdx = 0;
    int start = 0;

    for (int i = 0; i < 5; i++) {
        int commaPos = part.indexOf(',', start);
        String label = (commaPos == -1) ? part.substring(start) : part.substring(start, commaPos);
        label.trim();

        if (label.length() > 0) {
            if (currentActiveIdx == paramnr) {
                if (label == "!") {
                    out = defaults[i];
                } else {
                    int equalsPos = label.indexOf('=');
                    out = (equalsPos == -1) ? label : label.substring(0,equalsPos);
                }
                return true;
            }
            currentActiveIdx++;
        }

        if (commaPos == -1) break;
        start = commaPos + 1;
    }
    Serial.println("found nothing, returning false");
    return false;
}

// Helperfunktion, um den Anzeigenamen eines Effektparameters nr paramnr des Effekts linenr zu bestimmen
// Der String wird direkt aus der Effekt-Datei gelesen. Ist der Parameter aktiv, aber kein spezielles Label
// dafür definiert, wird der Default "sx", "ix"... zurückgegeben
bool WLEDDevice::getParamLabel(size_t linenr, size_t paramnr, char* out, size_t outLen) {
  char line[128]; 
  char fxfile[32];
  effectsFile(fxfile, sizeof(fxfile));

  if (!FileHelper::readLineAt(fxfile, linenr, line, sizeof(line))) return false;

  const char* defaults[] = {"sx", "ix", "c1x", "c2x", "c3x"};

  char* atPos = strchr(line, '@');
  char* firstSemi = strchr(line, ';');

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

// deprecated: char* version below!
bool WLEDDevice::getParamField(size_t linenr, size_t paramnr, String& out) {
  String line;
  String fxfile = effectsFile();
  //Serial.print("X");
  if (!FileHelper::readLineAt(fxfile.c_str(), linenr, line)) return false;

  // Standardnamen nur für die 5 Slider
  const char* defaults[] = {"sx", "ix", "c1x", "c2x", "c3x"};

  int atPos = line.indexOf('@');
  int firstSemi = line.indexOf(';');
  if (atPos == -1) {
    if ((paramnr <0) || (paramnr > 4)) return false;
    out = defaults[paramnr];    // Fallback für Effekte, bei denen keinerlei Parameter angegeben sind (alte WLED Versionen)
    return true;
  }

  String part = (firstSemi == -1) ? line.substring(atPos + 1) : line.substring(atPos + 1, firstSemi);

  int currentActiveIdx = 0;
  int start = 0;

  for (int i = 0; i < 5; i++) {
    int commaPos = part.indexOf(',', start);
    String label = (commaPos == -1) ? part.substring(start) : part.substring(start, commaPos);
    label.trim();

    if (label.length() > 0) {
      if (currentActiveIdx == paramnr) {
        out = defaults[i];
        return true;
      }
      currentActiveIdx++;
    }

    if (commaPos == -1) break;
    start = commaPos + 1;
  }
  return false;
}

// Helperfunktion, um den internen Namen eines Effektparameters nr paramnr des Effekts linenr zu bestimmen
// Es werden die aktiv unterstützten Parameter zu dem Effekt aus der Effekt-Datei ausgelesen, und der dem
// paramnr- Index entsprechende Name ("sx","ix"...) zurückgegeben
bool WLEDDevice::getParamField(size_t linenr, size_t paramnr, char* out, size_t outLen) {
  char line[128]; // Lokaler Puffer für die Zeile
  char fxfile[48]; 
  effectsFile(fxfile, sizeof(fxfile));

  if (!FileHelper::readLineAt(fxfile, linenr, line, sizeof(line))) return false;

  const char* defaults[] = {"sx", "ix", "c1x", "c2x", "c3x"};

  // Suche nach @ und ; (Ersatz für indexOf)
  char* atPos = strchr(line, '@');
  char* firstSemi = strchr(line, ';');

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

const KinoPropertyInfo WLEDDevice::_properties[] = {
    { "ip",           "IP",                 Prop_Read },

    { "tickInterval", "Tick Interval",      Prop_Read | Prop_Write,     0, 20000, 500 },

    { "on",           "Power",              Prop_Read | Prop_Write},

    { "live",         "Live Mode",          Prop_Read },

    { "lor",          "Live Override",      Prop_Read | Prop_Write },

    { "bri",          "Brightness",         Prop_Read | Prop_Write,     0, 255 },

 /*   { "sx",           "Speed",              Prop_Read | Prop_Write,     0, 255 },

    { "ix",           "Intensity",          Prop_Read | Prop_Write,     0, 255 },

    { "c1x",          "Custom 1",           Prop_Read | Prop_Write,     0, 255 },

    { "c2x",          "Custom 2",           Prop_Read | Prop_Write,     0, 255 },

    { "c3x",          "Custom 3",           Prop_Read | Prop_Write,     0, 255 },*/

    { "fx",           "Effect",             Prop_Read | Prop_Write | Prop_Query | Prop_hasLabel | Prop_hasParams },

    { "pal",          "Palette",            Prop_Read | Prop_Write | Prop_Query | Prop_hasLabel | Prop_hasParams },

    { "input",        "Input",              Prop_Read },

 //   { "col1",         "Vordergrund",   Prop_Read | Prop_Write  },

 //   { "col2",         "Hintergrund",   Prop_Read | Prop_Write },

 //   { "col3",         "Effekt",       Prop_Read | Prop_Write },

};

size_t WLEDDevice::getPropertyCount() const {
  return sizeof(_properties) / sizeof(_properties[0]);
}

const KinoPropertyInfo* WLEDDevice::getPropertyInfo(size_t index) const {
  if (index >= getPropertyCount()) return nullptr;
  return &_properties[index];
}

KinoError WLEDDevice::getEffectMetadata(int effectnr, KinoVariant& out) {
  //String ip = _ip.toString();
  //ip.replace(".", "_");
  //String effectsFile = "/wled/" + ip + "/effects.txt";
  char fxFile[48];
  effectsFile(fxFile, sizeof(fxFile));
  String line; line.reserve(128);

  //if (!FileHelper::readLineAt(effectsFile.c_str(), effectnr, line)) return KinoError::InvalidValue;
  if (!FileHelper::readLineAt(fxFile, effectnr, line)) return KinoError::InvalidValue;

  int start = line.indexOf('@');
  // Wenn kein @ da ist oder nichts dahinter kommt
  if (start == -1 || (unsigned int)(start + 1) >= line.length()) {
    //out = KinoVariant::fromString("");
    out.setString("");
    return KinoError::OK; // Kein Fehler, nur eben keine Metadaten
  }

  //out = KinoVariant::fromString(line.substring(start + 1).c_str());
  out.setString(line.substring(start + 1).c_str());
  return KinoError::OK;
}

KinoError WLEDDevice::getEffectParamName(const KinoVariant& in, int paramIndex, KinoVariant& out) {
  String metadata = in.toString();

  // Spezialfall: Wenn gar keine Metadaten vorhanden sind,
  // geben wir einen leeren String zurück, damit die UI die Defaults wählt.
  if (metadata.length() == 0) {
    //out = KinoVariant::fromString("");
    out.setString("");
    return KinoError::OK;
  }

  int start = 0;
  int end = metadata.indexOf(';');

  // Wir suchen das n-te Segment zwischen den Semikolons
  for (int i = 0; i < paramIndex; i++) {
    if (end == -1) {
      // Wir sind am Ende der Liste angekommen, bevor wir den paramIndex erreicht haben.
      // Das bedeutet: Dieser Parameter wird im Metadaten-String nicht erwähnt.
      // Ergo: Standardverhalten (leerer String).
      //out = KinoVariant::fromString("");
      out.setString("");
      return KinoError::OK;
    }
    start = end + 1;
    end = metadata.indexOf(';', start);
  }

  String result = (end == -1) ? metadata.substring(start) : metadata.substring(start, end);
  result.trim();

  //out = KinoVariant::fromString(result.c_str());
  out.setString(result.c_str());
  return KinoError::OK;
}

std::vector<KinoPropertyParam> WLEDDevice::getPaletteParams(int palnr) {
  std::vector<KinoPropertyParam> params;
  params.push_back({"col1","Vordergrund",3});
  params.push_back({"col2","Hintergrund",3});
  params.push_back({"col3","Effektfarbe",3});
  return params;
}

KinoPropertyParam* WLEDDevice::getPaletteParam(int palnr, int paramIndex) {
  // static sorgt dafür, dass das Array im Datensegment lebt und nicht auf dem Stack
  static KinoPropertyParam params[] = {
    {"col1", "Vordergrund", 3},
    {"col2", "Hintergrund", 3},
    {"col3", "Effektfarbe", 3}
  };

  // Ermittlung der Anzahl der Elemente in einem statischen Array
  constexpr size_t paramsSize = sizeof(params) / sizeof(params[0]);

  if (paramIndex >= 0 && (size_t)paramIndex < paramsSize) {
    return &params[paramIndex]; // Adresse des Elements zurückgeben
  }

  return nullptr; // Sicherer als ein leeres Objekt
}

// ===== Public API =====

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
  //String filePath = effectsFile();
  char filePath[48];
  effectsFile(filePath, sizeof(filePath));

  //if (FileHelper::exists(filePath.c_str())) {
  if (FileHelper::exists(filePath)) {
    if (!forceRefresh) return true;
    //FileHelper::remove(filePath.c_str());
    FileHelper::remove(filePath);
  }

  //NetworkHelper::resetClient(_client);
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
  String currentEffect = "";
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
  while (client.connected() || client.available() > 0) {
    if (client.available() > 0) {
      char c = client.read();

      if (c == '"') {
        if (inString) {
          // Ende des Namens erreicht -> Speichern
          if (currentEffect.length() > 0) {
            //FileHelper::writeLine(filePath.c_str(), currentEffect);
            FileHelper::writeLine(filePath, currentEffect);
            count++;
            currentEffect = "";
          }
          inString = false;
          inParams = false;
        } else {
          // Anfang eines Namens gefunden
          inString = true;
          inParams = false;
        }
      }
      else if (inString) {
        // Sonderfall: Escape-Zeichen (z.B. \" im Namen) ignorieren oder behandeln
        if (c == '\\') {
          char escaped = client.read(); // Nächstes Zeichen einfach mitnehmen
          currentEffect += escaped;
        } else {
          currentEffect += c;
        }
      }

      // Abbruch bei Ende des Arrays (optional, spart Zeit)
      if (!inString && c == ']') break;
    }
    yield(); // Watchdog füttern (wichtig bei langen Listen auf dem ESP8266)
  }

  // ========================================================================
  NetworkHelper::resetClient(client);
  return true;
}

bool WLEDDevice::readPalettes(bool forceRefresh/*=false*/) {
  //String filePath = paletteFile();
  char filePath[48];
  paletteFile(filePath, sizeof(filePath));

  //if (FileHelper::exists(filePath.c_str())) {
  if (FileHelper::exists(filePath)) {
    if (!forceRefresh) return true;
    //FileHelper::remove(filePath.c_str());
    FileHelper::remove(filePath);
  }

  //NetworkHelper::resetClient(_client);
  WiFiClient client;
  EnsureTimeoutBeforeRequest(200);
  if (!client.connect(_ip, 80)) {
    Serial.print("WLED: GET ");
    Serial.println("/json/palettes");
    Serial.println("could not connect");
    client.stop();
    return false;
  }
  client.printf(
    "GET /json/palettes HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Connection: close\r\n\r\n",
    _ip
  );
  if (!NetworkHelper::skipHeader(client)) return false;

  // =============== Effektnamen aus dem Stream extrahieren und wegschreiben

  unsigned long startTime = millis();
  bool inString = false;
  bool inParams = false;
  String currentEffect = "";
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
  while (client.connected() || client.available() > 0) {
    if (client.available() > 0) {
      char c = client.read();

      if (c == '"') {
        if (inString) {
          // Ende des Namens erreicht -> Speichern
          if (currentEffect.length() > 0) {
            //FileHelper::writeLine(filePath.c_str(), currentEffect);
            FileHelper::writeLine(filePath, currentEffect);
            count++;
            currentEffect = "";
          }
          inString = false;
          inParams = false;
        } else {
          // Anfang eines Namens gefunden
          inString = true;
          inParams = false;
        }
      }
      else if (inString) {
        // Sonderfall: Escape-Zeichen (z.B. \" im Namen) ignorieren oder behandeln
        if (c == '\\') {
          char escaped = client.read(); // Nächstes Zeichen einfach mitnehmen
          currentEffect += escaped;
        } else {
          currentEffect += c;
        }
      }

      // Abbruch bei Ende des Arrays (optional, spart Zeit)
      if (!inString && c == ']') break;
    }
    yield(); // Watchdog füttern (wichtig bei langen Listen auf dem ESP8266)
  }

  // ========================================================================
  NetworkHelper::resetClient(client);
  return true;
}

bool WLEDDevice::getStatus() {
  if (!readState()) return false;
  return true;
}

KinoError WLEDDevice::tick() {
  if (WiFi.status() != WL_CONNECTED) return KinoError::NothingToDo;
  if (_tickInterval == 0) return KinoError::NothingToDo;
  if (_refreshing) return KinoError::NothingToDo;

  unsigned long now = millis();
  if (now - _lastTick >= _tickInterval) {
    _lastTick = now;
    _refreshing = true;
    //showMemory();
    bool ok = readState();
    //showMemory();
    _refreshing = false;
    return (ok ? KinoError::OK : KinoError::DeviceNotReady);
  }
  return KinoError::NothingToDo;
}

bool WLEDDevice::setTickInterval(int ms) {
  if (ms == 0) { _tickInterval = 0; return true; }
  if (ms < 0) return false;       // nur zur besseren Lesbarkeit hier aufgeführt
  if (ms < 2000) return false;    // unter 2 Sekunden Interval führt zu übermässigem Traffic
  _tickInterval = ms;
  return true;
}

int WLEDDevice::getTickInterval() {
  return _tickInterval;
}

// ===== Getter =====

bool WLEDDevice::getPowerStatus() const {
  return _props["state"]["on"] | false;
}

bool WLEDDevice::inAlarm() const {
  return _alarm;
}

bool WLEDDevice::inPause() const {
  return _pause;
}

uint8_t WLEDDevice::getBrightness() const {
  return _props["state"]["bri"] | 0;
}

bool WLEDDevice::isReceivingLiveData() const {
  return _props["info"]["live"] | false;
}

bool WLEDDevice::isOverridingLiveData() const {
  return _props["state"]["lor"] | true;
}

uint8_t WLEDDevice::getSpeed() const {
  return _props["state"]["seg"][0]["sx"] | 0;
}

uint8_t WLEDDevice::getIntensity() const {
  return _props["state"]["seg"][0]["ix"] | 0;
}

uint16_t WLEDDevice::getEffect() const {
  return _props["state"]["seg"][0]["fx"] | 0;
}

uint8_t WLEDDevice::getPalette() const {
  return _props["state"]["seg"][0]["pal"] | 0;
}

String WLEDDevice::getLiveSource() const {
  return _props["info"]["lm"] | "";
}

void WLEDDevice::getLiveSource(char* src, size_t srcLen) {
  const char* lm = _props["info"]["lm"]|"";
  strncpy(src, lm, srcLen);
  src[srcLen-1] = '\0';
}

WLEDColor WLEDDevice::getColFg() const {
  /*WLEDColor c = {
    _props["state"]["seg"][0]["col"][0][0] | 0,
    _props["state"]["seg"][0]["col"][0][1] | 0,
    _props["state"]["seg"][0]["col"][0][2] | 0
  };*/
  JsonArrayConst  col = _props["state"]["seg"][0]["col"][0];
  /*WLEDColor c = {
    col[0]|0,
    col[1]|0,
    col[2]|0
  };
  return c;*/
  return {
    col[0]|0,
    col[1]|0,
    col[2]|0
  };
}

WLEDColor WLEDDevice::getColBg() const {
  /*WLEDColor c = {
    _props["state"]["seg"][0]["col"][1][0] | 0,
    _props["state"]["seg"][0]["col"][1][1] | 0,
    _props["state"]["seg"][0]["col"][1][2] | 0
  };*/
  JsonArrayConst  col = _props["state"]["seg"][0]["col"][1];
  /*WLEDColor c = {
    col[0]|0,
    col[1]|0,
    col[2]|0
  };
  return c;*/
  return {
    col[0]|0,
    col[1]|0,
    col[2]|0
  };
}

WLEDColor WLEDDevice::getColFx() const {
  /*WLEDColor c = {
    _props["state"]["seg"][0]["col"][2][0] | 0,
    _props["state"]["seg"][0]["col"][2][1] | 0,
    _props["state"]["seg"][0]["col"][2][2] | 0
  };*/
  JsonArrayConst  col = _props["state"]["seg"][0]["col"][2];
  /*WLEDColor c = {
    col[0]|0,
    col[1]|0,
    col[2]|0
  };
  return c;*/
  return {
    col[0]|0,
    col[1]|0,
    col[2]|0
  };
}

// ===== Setter =====

bool WLEDDevice::setPowerStatus(bool onoff) {
  _newProps["state"]["on"] = onoff;
  return true;
}

bool WLEDDevice::setBrightness(uint8_t bri) {
  if (bri < 0)   return false;
  if (bri > 255) return false;

  _newProps["state"]["bri"] = bri;
  return true;
}

bool WLEDDevice::setTransitionTime(int tt) {
  if (tt < 0) return false;
  if (tt < 100) tt = 100;
  _newProps["state"]["tt"] = (int)(tt/100);
  if (_newProps["state"]["tt"] == 0) _newProps["state"]["tt"] = 1;
  return true;
}

bool WLEDDevice::setEffect(uint16_t effect) {
  _newProps["state"]["seg"][0]["fx"] = effect;
  return true;
}

bool WLEDDevice::setSpeed(uint8_t sx) {
  if (sx < 0)   return false;
  if (sx > 255) return false;
  _newProps["state"]["seg"][0]["sx"] = sx;
  return true;
}

bool WLEDDevice::setIntensity(uint8_t ix) {
  if (ix < 0)   return false;
  if (ix > 255) return false;
  _newProps["state"]["seg"][0]["ix"] = ix;
  return true;
}

bool WLEDDevice::setFgColor(uint8_t R, uint8_t G, uint8_t B) {
  if ((R<0)||(R>255)||(G<0)||(G>255)||(B<0)||(B>255)) return false;
  _newProps["state"]["seg"][0]["col"][0][0] = R;
  _newProps["state"]["seg"][0]["col"][0][1] = G;
  _newProps["state"]["seg"][0]["col"][0][2] = B;
  return true;
}

bool WLEDDevice::setFgColor(WLEDColor c) {
  return setFgColor(c.r, c.g, c.b);
}

bool WLEDDevice::setBgColor(uint8_t R, uint8_t G, uint8_t B) {
  if ((R<0)||(R>255)||(G<0)||(G>255)||(B<0)||(B>255)) return false;
  _newProps["state"]["seg"][0]["col"][1][0] = R;
  _newProps["state"]["seg"][0]["col"][1][1] = G;
  _newProps["state"]["seg"][0]["col"][1][2] = B;
  return true;
}

bool WLEDDevice::setBgColor(WLEDColor c) {
  return setBgColor(c.r, c.g, c.b);
}

bool WLEDDevice::setFxColor(uint8_t R, uint8_t G, uint8_t B) {
  if ((R<0)||(R>255)||(G<0)||(G>255)||(B<0)||(B>255)) return false;
  _newProps["state"]["seg"][0]["col"][2][0] = R;
  _newProps["state"]["seg"][0]["col"][2][1] = G;
  _newProps["state"]["seg"][0]["col"][2][2] = B;
  return true;
}

bool WLEDDevice::setFxColor(WLEDColor c) {
  return setFxColor(c.r, c.g, c.b);
}

bool WLEDDevice::setLive(bool onoff) {
  _newProps["state"]["lor"] = !onoff;
  return true;
}

// Wrapper für besondere Effekte, die oft genutzt werden:

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
  _newProps["state"]["seg"][0]["c1x"] = c1x;
  _newProps["state"]["seg"][0]["c2x"] = c2x;
  _newProps["state"]["seg"][0]["c3x"] = c3x;
  return true;
}

bool WLEDDevice::setCustom1(uint8_t c1x) {
  _newProps["state"]["seg"][0]["c1x"] = c1x;
  return true;
}

bool WLEDDevice::setCustom2(uint8_t c2x) {
  _newProps["state"]["seg"][0]["c2x"] = c2x;
  return true;
}

bool WLEDDevice::setCustom3(uint8_t c3x) {
  _newProps["state"]["seg"][0]["c3x"] = c3x;
  return true;
}

bool WLEDDevice::setPalette(uint8_t pal) {
  _newProps["state"]["seg"][0]["pal"] = pal;
  return true;
}

// ===== HTTP Helper =====
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
  //NetworkHelper::resetClient(_client);
  WiFiClient client;
  EnsureTimeoutBeforeRequest(200);
  // Verbindung aufbauen mit etwas mehr Zeit (WLAN-Latenz!)
  client.setTimeout(1000); 
  if (!client.connect(_ip, 80)) {
    Serial.println(F("WLED: could not connect"));
    return false;
  }

  client.print(F("GET /json HTTP/1.1\r\n"));
  client.print(F("Host: ")); client.print(_ip); client.print(F("\r\n"));
  client.print(F("Connection: close\r\n\r\n"));

  // 1. Header überspringen (kurzes Timeout ist hier okay)
  if (!NetworkHelper::skipHeader(client)) {
    NetworkHelper::resetClient(client);
    return false;
  }

  // 2. Timeout für den Body HOCHSETZEN
  // JSON bei WLED kann groß sein, hier brauchen wir Geduld!
  client.setTimeout(2000); 

  initFilter();
  // deserializeJson liest direkt vom Stream
  DeserializationError error = deserializeJson(_props, client, DeserializationOption::Filter(_jsonFilter));

  if (!error) {
    _newProps = _props;
    NetworkHelper::resetClient(client);
    return true;
  } else {
    Serial.print(F("WLED Parsing error: "));
    Serial.println(error.c_str());
  }
  
  NetworkHelper::resetClient(client);
  return false;
}

/*
bool WLEDDevice::readState() {
  NetworkHelper::resetClient(_client);
  EnsureTimeoutBeforeRequest(200);
  if (!_client.connect(_ip, 80)) {
    Serial.print("WLED: GET ");
    Serial.println("/json");
    Serial.println("could not connect");
    NetworkHelper::resetClient(_client);
    return false;
  }
  _client.printf(
    "GET /json HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Connection: close\r\n\r\n",
    _ip.toString().c_str()
  );
  if (!NetworkHelper::skipHeader(_client)) {
    NetworkHelper::resetClient(_client);
    return false;
  }

  initFilter();
  DeserializationError error = deserializeJson(_props, _client, DeserializationOption::Filter(_jsonFilter));

  if (!error) {
    _newProps.clear();
    _newProps = _props;   // den aktuellen Status in newProps kopieren. Das spart später aufwendige Prüfungen einzelner Felder
    NetworkHelper::resetClient(_client);
    return true;
  } else {
    Serial.print(F("Parsing fehlgeschlagen: "));
    Serial.println(error.c_str());
  }
  NetworkHelper::resetClient(_client);
  return false;
}*/

bool WLEDDevice::applyChanges() {
  //NetworkHelper::resetClient(_client);
  WiFiClient client;
  EnsureTimeoutBeforeRequest(200);
  if (!client.connect(_ip, 80)) {client.stop(); return false;}

  client.printf(
    "POST /json/state HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: %d\r\n"
    "Connection: close\r\n\r\n",
    _ip,
    measureJson(_newProps["state"])
  );

  serializeJson(_newProps["state"], client);

  String resp = client.readStringUntil('\n');
  bool success = resp.startsWith("HTTP/1.1 200");
  NetworkHelper::resetClient(client);
  bool lor = _newProps["state"]["lor"];
  if (success) {
    _props.clear();
    _props = _newProps;
  } else {
    _newProps.clear();
    _newProps = _props;
  }

  return success;
}

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
