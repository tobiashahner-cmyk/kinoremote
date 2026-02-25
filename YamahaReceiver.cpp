#include "YamahaReceiver.h"
#include "NetworkHelper.h"
#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
#include <locale.h>
#include <ArduinoJson.h>

YamahaReceiver::YamahaReceiver(IPAddress ip) : _ip(ip) {}
YamahaReceiver::YamahaReceiver(const String& ip) { _ip.fromString(ip); }

bool YamahaReceiver::begin() {
  return (init() == KinoError::OK);
}

KinoError YamahaReceiver::init() {
  if (!getStatus()) return KinoError::DeviceNotReady;
  if (_powerStatus) {
    readInputSources();
  }
  return KinoError::OK;
}

// ----------------------------------------------------
// für KinoAPI: overrides für set und get aus KinoDevice
// ----------------------------------------------------

bool YamahaReceiver::isDirty() {
  return (_dirty > 0);
}

void YamahaReceiver::clearDirty() {
  _dirty = NONE;
}

bool YamahaReceiver::getStatusUpdate(const char* devName, JsonObject& root) {
  if (!_dirty) return false;
  root["dev"].set(devName);
  if (_dirty & POWER)   root["on"].set(_powerStatus);
  if (_dirty & MUTE)    root["mute"].set(_mute);
  if (_dirty & VOLUME)  root["vol"].set(_volume);
  if (_dirty & ENHANCER)root["enhancer"].set(_enhancer);
  if (_dirty & TREBLE)  root["treble"].set(_treble);
  if (_dirty & BASS)    root["bass"].set(_bass);
  if (_dirty & SWTRIM)  root["swtrim"].set(_subwooferTrim);
  if (_dirty & SOURCE)  root["input"].set(_source);
  if (_dirty & DSP)     root["dsp"].set(_soundProgram);
  if (_dirty & STRAIGHT)root["straight"].set(_straight);
  if (_dirty & TRACK) {
    NetRadioTrackInfo nri = readCurrentlyPlayingNetRadio();
    root["station"].set(nri.station);
    cleanSong(nri.song);
    root["song"].set(nri.song);
    root["elapsed"].set(nri.elapsed);
  }
  _dirty = NONE;
  return true;
}

void YamahaReceiver::cleanSong(char* str) {
    const char* search = "&amp;amp;";
    const size_t searchLen = 9;
    const char replace = '&';
    
    char* p;
    // Suche alle Vorkommen von "&amp;amp;"
    while ((p = strstr(str, search)) != NULL) {
      // Ersetze das erste Zeichen mit '&'
      *p = replace;
      // Den Rest des Strings (nach der Fundstelle + 9) nach vorne rücken
      // memmove ist sicher bei überlappenden Bereichen
      memmove(p + 1, p + searchLen, strlen(p + searchLen) + 1);
    }
}

KinoError YamahaReceiver::set(const char* property, const KinoVariant& value) {
    if (!property) {
        return KinoError::PropertyNotSupported;
    }
    /*if (strcmp(property,"tickInterval")==0) {
      if (!setTickInterval(value.asInt())) return KinoError::InvalidValue;
      return KinoError::OK;
    }*/
    if ((strcmp(property,"power")==0)||(strcmp(property,"on")==0)) {
        setPower(value.asBool());
        return KinoError::OK;
    }
    if ((strcmp(property,"volume")==0)||(strcmp(property,"vol")==0)) {
        setVolume(value.asInt());
        return KinoError::OK;
    }
    if ((strcmp(property,"percent")==0)||(strcmp(property,"pct")==0)) {
        setVolumePercent(value.asInt());
        return KinoError::OK;
    }
    if (strcmp(property, "mute") == 0) {
        setMute(value.asBool());
        return KinoError::OK;
    }
    if ((strcmp(property,"input")==0)||(strcmp(property,"source")==0)) {
        if (value.type != KinoVariant::STRING) {
            return KinoError::InvalidType;
        }
        setSource(value.c_str());
        return KinoError::OK;
    }
    if (strcmp(property,"station") == 0) {
      if (value.type != KinoVariant::STRING) return KinoError::InvalidType;
      if (!selectNetRadioFavorite(value.c_str())) return KinoError::InvalidValue;
      return KinoError::OK;
    }
    /*if (strcmp(property, "inputname") == 0) {
      if (value.type != KinoVariant::STRING) return KinoError::InvalidType;
      for (auto &s : _InputSources) {
        if (s.custom == value.s) {
          bool success = setSource(FPSTR(s.internal));
          if (!success) return KinoError::InternalError;
          return KinoError::OK;
        }
      }
      return KinoError::InvalidValue;
    }*/
    if (strcmp(property, "treble") == 0) {
      int v = value.asInt();
      if ((v < -6)||(v > 6)) return KinoError::InvalidValue;
      setTreble(v);
      return KinoError::OK;
    }
    if (strcmp(property, "bass") == 0) {
      int v = value.asInt();
      if ((v<-6)||(v>6)) return KinoError::InvalidValue;
      setBass(v);
      return KinoError::OK;
    }
    if (strcmp(property, "swtrim") == 0) {
      int v = value.asInt();
      if ((v<-6)||(v>6)) return KinoError::InvalidValue;
      setSubwooferTrim(v);
      return KinoError::OK;
    }
    if (strcmp(property, "straight") == 0) {
      setStraight(value.asBool());
      return KinoError::OK;
    }
    if (strcmp(property, "enhancer") == 0) {
      setEnhancer(value.asBool());
      return KinoError::OK;
    }
    if (strcmp(property, "dsp") == 0) {
      if (value.type != KinoVariant::STRING) return KinoError::InvalidType;
      if (!setSoundProgram(value.c_str())) return KinoError::InvalidValue;
      return KinoError::OK;
    }
    return KinoError::PropertyNotSupported;
}

KinoError YamahaReceiver::get(const char* property, KinoVariant& out) {
  if (!property) {
      return KinoError::PropertyNotSupported;
  }
  /*if (strcmp(property,"tickInterval")==0) {
    out.setInt(_tickInterval);
    return KinoError::OK;
  }*/
  if ((strcmp(property,"power")==0)||(strcmp(property,"on")==0)) {
      out.setBool(_powerStatus);
      return KinoError::OK;
  }
  if ((strcmp(property,"volume")==0)||(strcmp(property,"vol")==0)) {
      out.setInt(_volume);
      return KinoError::OK;
  }
  if (strcmp(property, "mute") == 0) {
      out.setBool(_mute);
      return KinoError::OK;
  }
  if ((strcmp(property,"input")==0)||(strcmp(property,"source")==0)) {
      out.setString(_source);
      return KinoError::OK;
  }
  if (strcmp(property, "station") == 0) {
    //if (_source == "NET RADIO") {
    if (strcmp(_source, "NET RADIO")==0) {
      NetRadioTrackInfo nri = readCurrentlyPlayingNetRadio();
      out.setString(nri.station);
      return KinoError::OK;
    } else {
      out.setString("");
      return KinoError::OK;
    }
  }
  if (strcmp(property, "song") == 0) {
    //if (_source == "NET RADIO") {
    if (strcmp(_source,"NET RADIO")==0) {
      NetRadioTrackInfo nri = readCurrentlyPlayingNetRadio();
      cleanSong(nri.song);
      out.setString(nri.song);
      return KinoError::OK;
    } else {
      out.setString("");
      return KinoError::OK;
    }
  }
  if (strcmp(property, "elapsed") == 0) {
    //if (_source == "NET RADIO") {
    if (strcmp(_source, "NET RADIO")==0) {
      NetRadioTrackInfo nri = readCurrentlyPlayingNetRadio();
      out.setString(nri.elapsed);
      return KinoError::OK;
    } else {
      out.setString("");
      return KinoError::OK;
    }
  }
  if (strcmp(property, "treble") == 0) {
    out.setInt(_treble);
    return KinoError::OK;
  }
  if (strcmp(property, "bass") == 0) {
    out.setInt(_bass);
    return KinoError::OK;
  }
  if (strcmp(property, "swtrim") == 0) {
    out.setInt(_subwooferTrim);
    return KinoError::OK;
  }
  if (strcmp(property, "ip") == 0) {
    char buf[20];
    snprintf(buf, sizeof(buf), "%d.%d.%d.%d", _ip[0], _ip[1], _ip[2], _ip[3]);
    out.setString(buf);
    return KinoError::OK;
  }
  if (strcmp(property, "straight") == 0) {
    out.setBool(_straight);
    return KinoError::OK;
  }
  if (strcmp(property, "enhancer") == 0) {
    out.setBool(_enhancer);
    return KinoError::OK;
  }
  if (strcmp(property, "dsp") == 0) {
    out.setString(_soundProgram);
    return KinoError::OK;
  }
  int found;
  char inp[32]; char rest[32];
  
  int paramIndex;
  found = sscanf(property,"input/%31[^/]/param/%i%31s", inp, &paramIndex, rest);
  if ((found == 2) && (strlen(inp)>0)) {          // path = "input/<inputName>/param/<paramIndex>
    const KinoPropertyParam* p = getInputParam(inp, paramIndex);
    if (!p) { out.setNone(); return KinoError::OutOfRange; }
    out.setString(p->getsetPath);
    return KinoError::OK;
  }
  if ((found == 3) && (strlen(inp)>0)) {          // path = "input/<inputName>/param/<paramIndex>/<rest>
    const KinoPropertyParam* p = getInputParam(inp, paramIndex);
    if (!p) { out.setNone(); return KinoError::OutOfRange; }
    if (strcmp(rest,"/label")==0) {
      out.setString(p->label);
      return KinoError::OK;
    }
    if (strcmp(rest,"/access")==0) {
      out.setInt(p->access);
      return KinoError::OK;
    }
    if (strcmp(rest,"/minvalue")==0) {
      out.setInt(p->minvalue.value_or(0));
      return KinoError::OK;
    }
    if (strcmp(rest,"/maxvalue")==0) {
      out.setInt(p->maxvalue.value_or(100));
      return KinoError::OK;
    }
    if (strcmp(rest,"/valuestep")==0) {
      out.setInt(p->valuestep.value_or(1));
      return KinoError::OK;
    }
    return KinoError::PropertyNotSupported;
  }

  found = sscanf(property,"input/%31[^/]/%31s",inp, rest);  // Pfad "input/<inputName>/<rest>"
  if ((found == 2) && (strlen(inp)>0) && (strlen(rest) > 0)) {
    if (strcmp(rest,"label")==0) {
      //Serial.print("matched path: input/"); Serial.print(inp); Serial.println("/label");
      initInputSources();
      for (int i=0; i<_InputSources.size(); i++) {
        if (strcmp_P(inp, _InputSources[i].internal)==0) {
          if (strlen(_InputSources[i].custom)>0) {
            out.setString(_InputSources[i].custom);
            return KinoError::OK;
          } else {
            out.setString(_InputSources[i].internal); // KinoVariant kann jetzt FlashStringHelper*
            return KinoError::OK;
          }
        }
      }
      //Serial.print("did not find an input with internal == "); Serial.println(inp);
    }
    return KinoError::InvalidProperty;
  }
  
  found = sscanf(property,"audio/param/%i%31s", &paramIndex, rest);
  if ((found == 1) || (strlen(rest) == 0)) {  // path = "audio/param/<paramIndex>"
    const KinoPropertyParam* p = getAudioParam(paramIndex);
    if (!p) { out.setNone(); return KinoError::OutOfRange; }
    out.setString(p->getsetPath);
    return KinoError::OK;
  }
  if ((found == 2) && (strlen(rest)>0)) {
    const KinoPropertyParam* p = getAudioParam(paramIndex);
    if (!p) { out.setNone(); return KinoError::OutOfRange; }
    if (strcmp(rest,"/label")==0) {
      out.setString(p->label);
      return KinoError::OK;
    }
    if (strcmp(rest,"/access")==0) {
      out.setInt(p->access);
      return KinoError::OK;
    }
    if (strcmp(rest,"/minvalue")==0) {
      out.setInt(p->minvalue.value_or(0));
      return KinoError::OK;
    }
    if (strcmp(rest,"/maxvalue")==0) {
      out.setInt(p->maxvalue.value_or(100));
      return KinoError::OK;
    }
    if (strcmp(rest,"/valuestep")==0) {
      out.setInt(p->valuestep.value_or(1));
      return KinoError::OK;
    }
    return KinoError::PropertyNotSupported;
  }
  char dspname[32];
  found = sscanf(property,"dsp/%31[^/]/param/%i%31s", dspname, &paramIndex, rest);
  if ((found == 2) || (strlen(rest)==0)) {
    const KinoPropertyParam* p = getDspParam(dspname, paramIndex);
    if (!p) { out.setNone(); return KinoError::OutOfRange; }
    out.setString(p->getsetPath);
    return KinoError::OK;
  }
  if ((found == 3) && (strlen(rest)>0)) {
    const KinoPropertyParam* p = getDspParam(dspname, paramIndex);
    if (!p) { out.setNone(); return KinoError::OutOfRange; }
    if (strcmp(rest,"/label")==0) {
      out.setString(p->label);
      return KinoError::OK;
    }
    if (strcmp(rest,"/access")==0) {
      out.setInt(p->access);
      return KinoError::OK;
    }
    if (strcmp(rest,"/minvalue")==0) {
      out.setInt(p->minvalue.value_or(0));
      return KinoError::OK;
    }
    if (strcmp(rest,"/maxvalue")==0) {
      out.setInt(p->maxvalue.value_or(100));
      return KinoError::OK;
    }
    if (strcmp(rest,"/valuestep")==0) {
      out.setInt(p->valuestep.value_or(1));
      return KinoError::OK;
    }
    return KinoError::PropertyNotSupported;
  }  
  //Serial.println("das war unnötig...");
  return KinoError::PropertyNotSupported;
}

KinoError YamahaReceiver::queryCount(const char* property, uint16_t &out) {
  if ((strcmp(property, "favorites") == 0) || (strcmp(property,"station") == 0)) {
    if (_stationCount == 0) readNetRadioFavorites();
    out = _stationCount;
    return KinoError::OK;
  }
  if (strcmp(property, "input") == 0) {
    initInputSources();
    out = _InputSources.size();
    return KinoError::OK;
  }
  /*if (strcmp(property, "inputname") == 0) {
    initInputSources();
    out = _InputSources.size();
    return KinoError::OK;
  }*/
  if (strcmp(property, "dsp") == 0) {
    if (_dspCount == 0) readDspNames();
    out = _dspCount;
    return KinoError::OK;
  }
  if (strcmp(property, "audio/param")==0) {
    out = (int)getAudioParamCount();
    return KinoError::OK;
  }

  char inputname[32]; char rest[32];
  int found = sscanf(property,"input/%31[^/]/%31s", inputname, rest);
  if ((found==2)&&(strlen(inputname)>0)&&(strcmp(rest,"param")==0)) {   // path = "input/<inputName>/param"
    out = (int)getInputParamCount(inputname);
    return KinoError::OK;
  }
  char dspname[32];
  found = sscanf(property,"dsp/%31[^/]/param%31s", dspname, rest);
  if ((found == 1)&&(strlen(dspname)>0)) {
    out = (int)getDspParamCount(dspname);
    return KinoError::OK;
  }
  
  out = 0;
  return KinoError::PropertyNotSupported;
}

KinoError YamahaReceiver::query(const char* property, uint16_t index, KinoVariant& out) {
  if ((strcmp(property, "favorites") == 0) || (strcmp(property,"station") == 0)) {
    if (index >= _stationCount) { out.setNone(); return KinoError::OutOfRange; }
    char buf[48];
    bool ok = getNetRadioFavorite(index, buf, sizeof(buf));
    if (!ok) { out.setNone(); return KinoError::InternalError; }
    out.setString(buf);
    return KinoError::OK;
  }
  if (strcmp(property, "input") == 0) {
    initInputSources();
    if (index >= _InputSources.size()) return KinoError::OutOfRange;
    out.setString(_InputSources[index].internal);
    return KinoError::OK;
  }
  /*if (strcmp(property, "inputname") == 0) {
    initInputSources();
    if (index >= _InputSources.size()) return KinoError::OutOfRange;
    String tmpInternal = FPSTR(_InputSources[index].internal);
    String tmpCustom   = _InputSources[index].custom;
    if (tmpCustom.length() == 0) tmpCustom = tmpInternal;
    out.setString(tmpCustom.c_str());
    return KinoError::OK;
  }*/
  /*if (strcmp(property, "inputskip") == 0) {
    initInputSources();
    if (index >= _InputSources.size()) return KinoError::OutOfRange;
    out.setBool(_InputSources[index].skip);
    return KinoError::OK;
  }*/
  if (strcmp(property, "dsp") == 0) {
    if (_dspCount == 0) readDspNames();
    char buf[32];
    if (!getDspName(index, buf, sizeof(buf))) {
      out.setNone();
      return KinoError::OutOfRange;
    }
    out.setString(buf);
    return KinoError::OK;
  }
  
  return KinoError::PropertyNotSupported;
}

const KinoPropertyInfo YamahaReceiver::_props[] = {

  { "on",      "Power",        Prop_Read | Prop_Write },

  { "mute",       "Mute",         Prop_Read | Prop_Write },

  { "vol",     "Volume",       Prop_Read | Prop_Write, -800, 0, 5 },

  { "audio",      "Audio",        Prop_hasParams },

  //{ "treble",     "Treble",       Prop_Read | Prop_Write, -60, 60, 5 },

  //{ "bass",       "Bass",         Prop_Read | Prop_Write, -60, 60, 5 },

  //{ "swtrim",     "Subwoofer",    Prop_Read | Prop_Write, -60, 60, 5 },

  { "input",      "Input",        Prop_Read | Prop_Write | Prop_Query | Prop_hasLabel | Prop_hasParams},

  { "dsp",        "Sound Program",Prop_Read | Prop_Write | Prop_Query | Prop_hasParams },

  { "station",    "Station",      Prop_Read | Prop_Write | Prop_Query },

  { "song",       "Song",         Prop_Read | Prop_Status },

  { "elapsed",    "Elapsed",      Prop_Read | Prop_Status },

  { "ip",         "IP Address",   Prop_Read | Prop_Internal }//,

  //{ "tickInterval","Akt.Intervall[ms]", Prop_Read | Prop_Write, 0, 20000, 500}
};

size_t YamahaReceiver::getPropertyCount() const {
  return sizeof(_props) / sizeof(_props[0]);
}

const KinoPropertyInfo* YamahaReceiver::getPropertyInfo(size_t index) const {
  if (index >= getPropertyCount()) return nullptr;
  return &_props[index];
}

const KinoPropertyParam YamahaReceiver::_AudioParams[] = {
  {"enhancer","Sound Enhancer",3},
  {"treble","Hoehen",3,-60,60,5},
  {"bass","Tiefen",3,-60,60,5},
  {"swtrim","Subwoofer Trim",3,-60-60,5}
};

size_t YamahaReceiver::getAudioParamCount() {
  return 4;
}

const KinoPropertyParam YamahaReceiver::_InputParams[] = {
  {"station","Sender",1},
  {"song","Song",1},
  {"elapsed","Spielzeit",1}
};

const KinoPropertyParam YamahaReceiver::_DspParams[] = {
  {"straight", "Pure Straight",3}
};

size_t YamahaReceiver::getDspParamCount(const char* dspname) {
  return sizeof(_DspParams) / sizeof(_DspParams[0]);
}

const KinoPropertyParam* YamahaReceiver::getAudioParam(size_t index) {
  if (index >= getAudioParamCount()) return nullptr;
  return &_AudioParams[index];
}

size_t YamahaReceiver::getInputParamCount(const char* inp) {
  if (strcmp(inp, "NET RADIO")==0) return (sizeof(_InputParams) / sizeof(_InputParams[0]));
  return 0;
}

const KinoPropertyParam* YamahaReceiver::getInputParam(const char* inp, size_t index) {
  if (strcmp(inp, "NET RADIO")==0) {
    if (index >= getInputParamCount(inp)) return nullptr;
    return &_InputParams[index];
  }
  return nullptr;
}

const KinoPropertyParam* YamahaReceiver::getDspParam(const char* dspname, size_t index) {
  if (index >= getDspParamCount(dspname)) return nullptr;
  return &_DspParams[index];
}

/* boolYamahaReceiver::getStatus()  V3: 2026-02-21 Umstieg auf HttpClient */
bool YamahaReceiver::getStatus() {
  WiFiClient wifi;
  HTTPClient http;
  if (!sendXMLRequest(wifi, http, FPSTR(XML_GET_STATUS))) {
    Serial.println(F("could not get status from yamaha"));
    http.end();
    wifi.stop();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();

  // 1. Power
  if (stream->find("<Power>")) {
    bool val = readIsOn(*stream);
    if (_powerStatus != val) { _powerStatus = val; _dirty |= POWER; }
  }
  
  // 2. Volume
  if (stream->find("<Val>")) {
    int val = readIntUntil(*stream);
    if (_volume != val) { _volume = val; _dirty |= VOLUME; }
  }

  // 5. Mute
  if (stream->find("<Mute>")) {
    bool val = readIsOn(*stream);
    if (_mute != val) { _mute = val; _dirty |= MUTE; }
  }

  // 2a. Subwoofer Trim
  if (stream->find("<Val>")) {
    int val = readIntUntil(*stream);
    if (_subwooferTrim != val) { _subwooferTrim = val; _dirty |= SWTRIM; }
  }

  // 3. Input (Beispiel mit festem Puffer statt dynamischem String)
  if (stream->find("<Input_Sel>")) {
    char buf[32];
    readSanitizedUntil(*stream, '<', buf, sizeof(buf));
    if (strcmp(_source, buf)!=0) {
      strncpy(_source, buf, sizeof(_source));
      _source[sizeof(_source)-1] = '\0';
      _dirty |= SOURCE;
    }
  }


  // Straight / Enhancer
  if (stream->find("<Straight>")) {
    bool val = readIsOn(*stream);
    if (_straight != val) { _straight = val; _dirty |= STRAIGHT; }
  }
  if (stream->find("<Enhancer>")) {
    bool val = readIsOn(*stream);
    if (_enhancer != val) { _enhancer = val; _dirty |= ENHANCER; }
  }

  // 4. Sound Program
  if (stream->find("<Sound_Program>")) {
    char buf[32];
    readSanitizedUntil(*stream, '<', buf, sizeof(buf));
    if (strcmp(_soundProgram, buf)!=0) {
      strlcpy(_soundProgram, buf, sizeof(_soundProgram));
      _dirty |= DSP;
    }
  }

  // Bass / Treble
  if (stream->find("<Val>")) {
    int val = readIntUntil(*stream);
    if (_bass != val) { _bass = val; _dirty |= BASS; }
  }
  if (stream->find("<Val>")) {
    int val = readIntUntil(*stream);
    if (_treble != val) { _treble = val; _dirty |= TREBLE; }
  }

  // Stream sauber schliessen
  http.end();
  wifi.stop();

  //erzwinge refresh von NetRadioInfo:
  if (_powerStatus && (strcmp(_source,"NET RADIO")==0)) _dirty |= TRACK;
  return true;
}

/* bool YamahaReceiver::readIsOn(Stream& s) V1  2026-02-10 */
bool YamahaReceiver::readIsOn(Stream& s) {
  // Liest bis '<', gibt true zurück wenn der Inhalt "On" ist
  char buf[10]; // Reicht locker für "On", "Off", "Standby"
  size_t len = s.readBytesUntil('<', buf, sizeof(buf) - 1);
  buf[len] = '\0';
  return (strcmp(buf, "On") == 0);
}

/* bool YamahaReceiver::readIntUntil(Stream& s) V1  2026-02-10 */
int YamahaReceiver::readIntUntil(Stream& s) {
  // Liest bis '<' und wandelt direkt in Integer um
  int val = s.parseInt();
  s.find("<"); 
  return val;
}

/* bool YamahaReceiver::readSanitizedUntil(...) V2  2026-02-15  Umlaute werden in HTML-Entities übersetzt */
size_t YamahaReceiver::readSanitizedUntil(Stream& s, char terminator, char* buffer, size_t maxLen) {
  size_t count = 0;
  
  // Wir lesen, solange Platz für mindestens ein Zeichen + Null-Terminator ist
  while (count < maxLen - 1) {
    int c = s.read();
    if (c < 0 || c == terminator) break; // Timeout oder Ende

    // 1. Standard ASCII (Druckbare Zeichen 32-126)
    if (c >= 32 && c <= 126) {
      buffer[count++] = (char)c;
    } 
    // 2. UTF-8 Multi-Byte Sequenz (Start-Byte für Umlaute ist meist 0xC3)
    else if (c == 0xC3) {
      // Wir müssen auf das nächste Byte warten (Folge-Byte)
      int next = s.read();
      if (next < 0) break; // Unerwartetes Ende des Streams

      const char* entity = nullptr;
      switch (next) {
        case 0xA4: entity = "&auml;";  break; // ä
        case 0xB6: entity = "&ouml;";  break; // ö
        case 0xBC: entity = "&uuml;";  break; // ü
        case 0x84: entity = "&Auml;";  break; // Ä
        case 0x96: entity = "&Ouml;";  break; // Ö
        case 0x9C: entity = "&Uuml;";  break; // Ü
        case 0x9F: entity = "&szlig;"; break; // ß
        default:   entity = "_";       break; // Unbekannt/Nicht unterstützt
      }

      // Das Entity Zeichen für Zeichen in den Puffer kopieren
      size_t entityLen = strlen(entity);
      if (count + entityLen < maxLen) {
        while (*entity) {
          buffer[count++] = *entity++;
        }
      } else {
        // Falls das Entity nicht mehr ganz passt, 
        // schreiben wir stattdessen den Platzhalter '_'
        buffer[count++] = '_';
      }
    }
    // 3. Fallback für alle anderen Sonderzeichen (z.B. 0xC2 Präfixe oder High-ASCII)
    else {
      // Wenn wir es nicht kennen, nehmen wir einen Unterstrich als Platzhalter
      buffer[count++] = '_';
    }
  }
  
  buffer[count] = '\0';
  return count;
}

void YamahaReceiver::initInputSources() {
  if (_gotInputSources) return; // Nur einmal im Leben des ESP machen
  
  _InputSources.clear();
  _InputSources.reserve(28); // Einmalig Platz reservieren

  // Wir füllen den Vektor mit Pointern auf Flash-Strings
  // Das spart ca. 1.5 KB RAM im Vergleich zur alten Version!
  _InputSources.push_back({F("TUNER"), F("TUNER"), false});
  _InputSources.push_back({F("PHONO"), F("PHONO"), false});
  _InputSources.push_back({F("HDMI_1"), F("HDMI1"), false});
  _InputSources.push_back({F("HDMI_2"), F("HDMI2"), false});
  _InputSources.push_back({F("HDMI_3"), F("HDMI3"), false});
  _InputSources.push_back({F("HDMI_4"), F("HDMI4"), false});
  _InputSources.push_back({F("HDMI_5"), F("HDMI5"), false});
  _InputSources.push_back({F("AV_1"), F("AV1"), false});
  _InputSources.push_back({F("AV_2"), F("AV2"), false});
  _InputSources.push_back({F("AUX"), F("AUX"), false});
  _InputSources.push_back({F("AUDIO_1"), F("AUDIO1"), false});
  _InputSources.push_back({F("AUDIO_2"), F("AUDIO2"), false});
  _InputSources.push_back({F("AUDIO_3"), F("AUDIO3"), false});
  _InputSources.push_back({F("AUDIO_4"), F("AUDIO4"), false});
  _InputSources.push_back({F("AUDIO_5"), F("AUDIO5"), false});
  _InputSources.push_back({F("Napster"), F("Napster"), false});
  _InputSources.push_back({F("Spotify"), F("Spotify"), false});
  _InputSources.push_back({F("Qobuz"), F("Qobuz"), false});
  _InputSources.push_back({F("TIDAL"), F("TIDAL"), false});
  _InputSources.push_back({F("Deezer"), F("Deezer"), false});
  _InputSources.push_back({F("Amazon_Music"), F("Amazon Music"), false});
  _InputSources.push_back({F("Alexa"), F("Alexa"), false});
  _InputSources.push_back({F("AirPlay"), F("AirPlay"), false});
  _InputSources.push_back({F("MusicCast_Link"), F("MusicCast Link"), false});
  _InputSources.push_back({F("SERVER"), F("SERVER"), false});
  _InputSources.push_back({F("NET_RADIO"), F("NET RADIO"), false});
  _InputSources.push_back({F("Bluetooth"), F("Bluetooth"), false});
  _InputSources.push_back({F("USB"), F("USB"), false});
}


bool YamahaReceiver::readInputSources() {
  initInputSources();
  
  char keyname[32];
  WiFiClient wifi;
  HTTPClient http;
  // 1. Namen abfragen
  if (!sendXMLRequest(wifi, http, XML_GET_INPUTNAMES)) {
    http.end();
    wifi.stop();
    return false;
  }
  WiFiClient* stream = http.getStreamPtr();
  // Finde "<Input_Name>"
  if (stream->find("<Input_Name>")) {
    while(true) {
      if (stream->find('<')) {
        readSanitizedUntil(*stream, '>', keyname, sizeof(keyname));
        if (strcmp(keyname, "/Input_Name")==0) {
          // end of list
          break;
        }
        InputSource* is = getInputSourceByKey(keyname);
        if (is) {
          readSanitizedUntil(*stream, '<', is->custom, sizeof(is->custom));
        }
        if (!stream->find('>')) {
          // closing tag not terminated, something is wrong
          break;
        }
      } else {
        // didnt find '<' => no tag opener, something is wrong
        break;
      }
      yield();
    }
  }
  http.end();

  // 2. Skip-Status abfragen (analog zum ersten Teil)
  if (!sendXMLRequest(wifi, http, XML_GET_INPUTSKIP)) {
    http.end();
    wifi.stop();
    return false;
  }
  stream = http.getStreamPtr(); // vielleicht nicht nötig, aber ich gehe mal lieber sicher
  // Finde "<Input_Name>"
  if (stream->find("<Input_Skip>")) {
    while (true) {
      if (stream->find('<')) {
        readSanitizedUntil(*stream, '>', keyname, sizeof(keyname));
        if (strcmp(keyname, "/Input_Skip")==0) {
          // end of list
          break;
        }
        InputSource* is = getInputSourceByKey(keyname);
        if (is) {
          is->skip = readIsOn(*stream);
        }
        if (!stream->find('>')) {
          // closing tag not terminated, something is wrong
          break;
        }
      } else {
        // no tag opener, something is wrong
        break;
      }
      yield();
    }
  }
  
  http.end();
  wifi.stop();

  _gotInputSources = true;
  return true;
}

/* getInputSource   V2  2026-02-15  Rückgabetyp geändert auf Pointer */
InputSource* YamahaReceiver::getInputSource() {
  if (!_gotInputSources) readInputSources();
  for (auto& s : _InputSources) {
    if (strcmp_P(_source, s.internal)==0) return &s;
  }
  return nullptr;
}

/* getInputSorceByKey   V2  2026-02-15  Strings entfernt */
InputSource* YamahaReceiver::getInputSourceByKey(const char* keyname) {
  for (auto& s: _InputSources) {
    if (strcmp_P(keyname, s.key)==0) return &s;
  }
  // no such InputSource
  return nullptr;
}

// ----------------------------------------------------
// Liste der NET RADIO Items auslesen
// ----------------------------------------------------

bool YamahaReceiver::waitForNetRadioList(WiFiClient& wifi, HTTPClient& http, bool keepalive) {
  unsigned long startTime = millis();
  bool ready = false;

  while (millis() - startTime < 2000) {
    if (!sendXMLRequest(wifi, http, XML_GET_NETRADIO_LIST)) {
      //http.end();
      NetworkHelper::resetHttpClient(http);
      yield();
      delay(100);
      continue;
    }

    WiFiClient* stream = http.getStreamPtr();
    if (stream->find("<Menu_Status>")) {
      char menuStatus[20];
      size_t len = stream->readBytesUntil('<', menuStatus, sizeof(menuStatus) - 1);
      menuStatus[len] = '\0';
      if (strcmp(menuStatus, "Ready") == 0) {
        ready = true;
        break; // Wir lassen http offen, damit der Stream gelesen werden kann!
      }
    }

    //http.end(); // Nicht bereit? Dann sauber schließen für den nächsten Versuch
    NetworkHelper::resetHttpClient(http);
    yield();
    delay(50);
  }

  if (!ready || !keepalive) {
    //http.end();
    NetworkHelper::resetHttpClient(http);
  }
  return ready;
}

bool YamahaReceiver::moveToFavorites(WiFiClient& wifi, HTTPClient& http) {
  // 1. Home
  if (!sendXMLRequest(wifi, http, XML_SET_MOVEHOME)) return false;
  bool ok = http.getStreamPtr()->find("RC=\"0\"");
  //http.end(); 
  NetworkHelper::resetHttpClient(http);
  if (!ok || !waitForNetRadioList(wifi, http, false)) return false;

  // 2. Erster Select (Zeile 1)
  if (!sendXMLRequest(wifi, http, XML_SET_SELECT_LINE_ONE)) return false;
  ok = http.getStreamPtr()->find("RC=\"0\"");
  //http.end();
  NetworkHelper::resetHttpClient(http);
  if (!ok || !waitForNetRadioList(wifi, http, false)) return false;

  // 3. Zweiter Select (Favoriten-Ordner öffnen)
  if (!sendXMLRequest(wifi, http, XML_SET_SELECT_LINE_ONE)) return false;
  ok = http.getStreamPtr()->find("RC=\"0\"");
  //http.end();
  NetworkHelper::resetHttpClient(http);
  
  return ok;
}

// ----------------------------------------------------
// Navigiert zu Favoriten
// ----------------------------------------------------

bool YamahaReceiver::moveToNextPage(WiFiClient& wifi, HTTPClient& http) {
  if (!waitForNetRadioList(wifi, http, false)) return false; // Stream ist auf jeden Fall leer und geschlossen
  bool ok = sendXMLRequest(wifi, http, XML_SELECT_NEXT_PAGE);
  
  // sendXMLRequest gibt den Stream offen zurück. Antwort auswerten:
  WiFiClient* stream = http.getStreamPtr();
  if (ok) ok = stream->find((char*)"RC=\"0\"");
  // Rest der Antwort interessiert nicht:
  //http.end();
  NetworkHelper::resetHttpClient(http);
  return ok;
}

// ----------------------------------------------------
// Favoriten auslesen
// ----------------------------------------------------

bool YamahaReceiver::readNetRadioFavorites(bool reload) {
  if (_stationCount > 0 && !reload) return true;
  WiFiClient wifi;
  HTTPClient http;
  if (!moveToFavorites(wifi, http)) {
    //http.end();
    //wifi.stop();
    NetworkHelper::resetClients(wifi, http, false);
    return false;
  }

  _stationCount = 0;
  bool hasNextPage = true;

  while (hasNextPage && _stationCount < MAX_STATIONS) {
    // Wir warten auf die Liste und halten den Stream offen (keepalive = true)
    if (!waitForNetRadioList(wifi, http, true)) break;

    WiFiClient* stream = http.getStreamPtr();
    // Die Liste hat typischerweise 8 Einträge pro Seite
    for (int i = 0; i < 8; i++) {
      if (_stationCount >= MAX_STATIONS) break;

      // 1. Suche den Sendernamen
      if (!stream->find("<Txt>")) break; 
      
      char tempName[48];
      //size_t nLen = client.readBytesUntil('<', tempName, sizeof(tempName) - 1);
      size_t nLen = readSanitizedUntil(*stream, '<', tempName, sizeof(tempName) - 1);
      tempName[nLen] = '\0';

      // 2. Suche das zugehörige Attribut (muss nach <Txt> kommen)
      if (!stream->find("<Attribute>")) break;
      
      char tempAttr[20];
      //size_t aLen = client.readBytesUntil('<', tempAttr, sizeof(tempAttr) - 1);
      size_t aLen = readSanitizedUntil(*stream, '<', tempAttr, sizeof(tempAttr) - 1);
      tempAttr[aLen] = '\0';

      // 3. Validierung
      if (strcmp(tempAttr, "Unselectable") == 0) {
        hasNextPage = false; // Ende der Liste erreicht
        break;
      }

      // 4. In das statische Array kopieren
      strncpy(_stations[_stationCount], tempName, 47);
      _stations[_stationCount][47] = '\0';
      _stationCount++;
    }

    if (hasNextPage && _stationCount < MAX_STATIONS) {
        // Seite voll, versuche nächste Seite
        // moveToNextPage muss die Verbindung schließen, damit wir neu pollen können
        NetworkHelper::resetHttpClient(http);
        yield();
        delay(50);
        if (!moveToNextPage(wifi, http)) {
            hasNextPage = false; 
        }
        // Der nächste Schleifendurchlauf ruft wieder waitForNetRadioList auf
    }
  }
  
  // Am Ende sicherstellen, dass alles zu ist
  //http.end();
  //wifi.stop();
  NetworkHelper::resetClients(wifi, http, true);
  return _stationCount > 0;
}

/* getNetRadioFavorite  V1  2026-02-14  Getter für neuen private _stations */
bool YamahaReceiver::getNetRadioFavorite(size_t index, char* buf, int buflen) {
  if (index >= _stationCount) {
    buf[0] = '\0';
    return false;
  }
  strncpy(buf, _stations[index], buflen-1);
  buf[buflen-1] = '\0';
  return true;
}

bool YamahaReceiver::selectNetRadioFavorite(const char* station) {
  // gültige Favoritenliste sicherstellen
  if (_stationCount == 0) readNetRadioFavorites();
  if (_stationCount == 0) return false;
  // bereite Parameter vor: wir vergleichen nur die ersten 10 Buchstaben
  char wanted[11];
  strncpy(wanted, station, sizeof(wanted));
  wanted[sizeof(wanted)-1] = '\0';
  // bereite Menü vor
  WiFiClient wifi;
  HTTPClient http;
  if (!moveToFavorites(wifi, http)) {
    //http.end();
    //wifi.stop();
    NetworkHelper::resetClients(wifi, http, false);
    return false;
  }
  bool hasNextPage = true;
  int found = 0;
  while (hasNextPage && found <= _stationCount) {
    // Wir warten auf die Liste und halten den Stream offen (keepalive = true)
    if (!waitForNetRadioList(wifi, http, true)) {
      break;
    }
    WiFiClient* stream = http.getStreamPtr();
    // Die Liste hat typischerweise 8 Einträge pro Seite
    for (int linenr = 1; linenr < 9; linenr++) {
      // 1. Suche den Sendernamen
      if (!stream->find("<Txt>")) {
        break; 
      }
      
      char tempName[11];
      size_t nLen = readSanitizedUntil(*stream, '<', tempName, sizeof(tempName));
      tempName[nLen] = '\0';

      // 2. Suche das zugehörige Attribut (muss nach <Txt> kommen)
      if (!stream->find("<Attribute>")) {
        break;
      }
      
      char tempAttr[20];
      size_t aLen = readSanitizedUntil(*stream, '<', tempAttr, sizeof(tempAttr));
      tempAttr[aLen] = '\0';

      // 3. Validierung
      if (strcmp(tempAttr, "Unselectable") == 0) {
        hasNextPage = false; // Ende der Liste erreicht
        break;
      }

      // 4. vergleichen und ggf auswählen
      if (strcmp(tempName, wanted)==0) {
        // Stream vorbereiten für den Auswahlbefehl
        //http.end();
        NetworkHelper::resetHttpClient(http);
        bool ok = executeSetCommand(wifi, http, XML_SET_SELECT_LINENR_START, linenr, XML_SET_SELECT_LINENR_END);
        //http.end();
        //wifi.stop();
        NetworkHelper::resetClients(wifi, http, ok);
        return ok;
      }
      // wenn wir hier sind, war noch nicht der richtige Sender dabei
      found++;
    }

    if (hasNextPage && _stationCount < MAX_STATIONS) {
        // Seite voll, versuche nächste Seite
        // moveToNextPage wird die Verbindung schließen, damit wir neu pollen können
        //http.end();
        NetworkHelper::resetHttpClient(http);
        yield();
        delay(50);
        if (!moveToNextPage(wifi, http)) {
          hasNextPage = false; 
        }
        // Der nächste Schleifendurchlauf ruft wieder waitForNetRadioList auf
    }
  }
  // Ende der Liste erreicht oder alle bekannten Favoriten abgegrast. Schliesse den Client und melde den Misserfolg:
  //http.end();
  //wifi.stop();
  NetworkHelper::resetClients(wifi, http, true);  // Es war kein Fehler in der Verbindung, also true
  return false;
}

bool YamahaReceiver::readDspNames(bool reload) {
  if ((_dspCount > 0) && (!reload)) return true;
  _dspCount = 0;
  WiFiClient wifi;
  HTTPClient http;
  if (!sendXMLRequest(wifi, http, FPSTR(XML_GET_DSP_SKIP))) {
    Serial.println(F("readDspNames: sendXMLRequest failed, returning false"));
    //http.end();
    //wifi.stop();
    NetworkHelper::resetClients(wifi, http, false);
    return false;
  }
  WiFiClient* stream = http.getStreamPtr();
  if (!stream->find((char*)"RC=\"0\"")) {
    //http.end();
    //wifi.stop();
    NetworkHelper::resetClients(wifi, http, false);
    return false;
  }
  char keyname[32];
  unsigned long now = millis();
  unsigned long wdStart = now;
  unsigned long maxTimeout = 1000;
  // Finde "<DSP_Skip>"
  if (stream->find("<DSP_Skip>")) {
    while(now - wdStart < maxTimeout) { // ensure max timeout
      if (stream->find('<')) {
        readSanitizedUntil(*stream,'>',keyname, sizeof(keyname));
        // end of list?
        if (strcmp(keyname, "/DSP_Skip")== 0) {
          break;
        }
        bool skip = readIsOn(*stream);
        if (!skip && (_dspCount < MAX_DSP)) {
          sanitizeDspName(keyname);
          strncpy(_dsps[_dspCount], keyname, 31);
          _dsps[_dspCount][31] = '\0';
          _dspCount++;
        }
        // try skipping the closing tag (in fact, ANY tag)
        // closing tag not terminated => something is wrong
        if (!stream->find('>')) {
          break;
        }
      } else {
        // no tag opener, something is wrong
        break;
      }
      now = millis();
      yield();  // take this, watchdog ;-)
    }
  }
  //http.end();
  //wifi.stop();
  NetworkHelper::resetClients(wifi, http, true);
  return (_dspCount > 0);
}

bool YamahaReceiver::getDspName(size_t index, char* buf, size_t bufLen) {
  if (index >= _dspCount) {
    if (bufLen > 0) buf[0] = '\0';
    return false;
  }
  strncpy(buf, _dsps[index], bufLen-1);
  buf[bufLen-1] = '\0';
  return true;
}

void YamahaReceiver::sanitizeDspName(char* name) {
    // Unterstriche ersetzen
    for (char* p = name; *p; p++) if (*p == '_') *p = ' ';
    
    // Trimmen
    char* start = name;
    while (isspace((unsigned char)*start)) start++;
    if (start != name) memmove(name, start, strlen(start) + 1);
    
    int len = strlen(name);
    while (len > 0 && isspace((unsigned char)name[len - 1])) len--;
    name[len] = '\0';
}

NetRadioTrackInfo YamahaReceiver::readCurrentlyPlayingNetRadio() {
  static NetRadioTrackInfo info;
  unsigned long now = millis();
  if ((info.created != 0)&&(now-info.created < 2000)) return info;
  WiFiClient wifi;
  HTTPClient http;
  if (!sendXMLRequest(wifi, http, FPSTR(XML_GET_NETRADIO_PLAYINFO))){
    //http.end();
    //wifi.stop();
    NetworkHelper::resetClients(wifi, http, false);
    return info;
  }
  WiFiClient* stream = http.getStreamPtr();
  //char buf[64];
  size_t readLen;
  if (stream->find("<Elapsed>")) {
    readSanitizedUntil(*stream, '<', info.elapsed, sizeof(info.elapsed));
  }
  if (stream->find("<Station>")) {
    readSanitizedUntil(*stream, '<', info.station, sizeof(info.station));
  }
  if (stream->find("<Song>")) {
    readSanitizedUntil(*stream, '<', info.song, sizeof(info.song));
  }
  //http.end();
  //wifi.stop();
  NetworkHelper::resetClients(wifi, http, true);
  info.created = millis();
  return info;
}

bool YamahaReceiver::setPower(bool onoff) {
  bool ok = executeSetCommand(XML_SET_POWER_START, onoff ? "On" : "Standby", XML_SET_POWER_END);
  if (ok) {
    if (_powerStatus != onoff) _dirty |= POWER;
    _powerStatus = onoff;
  }
  return ok;
}

bool YamahaReceiver::setVolume(int vol) {
  if (vol > 0) vol *= -1;
  bool ok = executeSetCommand(XML_SET_VOLUME_START, vol, XML_SET_VOLUME_END);
  if (ok) {
    if (_volume != vol) _dirty |= VOLUME;
    _volume = vol;
    _mute = false;  // Yamaha schaltet Mute aus, wenn Lautstärke gesetzt wird
  }
  return ok;
}

bool YamahaReceiver::setVolumeAlexa(int vol) {
  int realVal = map(vol, 0, 255, -800, -200);
  // Yamaha akzeptiert nur 5er Schritte, also runden:
  int remainder = realVal%10;
  realVal -= remainder;
  if (remainder <= (-5)) realVal -= 5;    // remainder is <0, because realVal is <0
  return setVolume(realVal);
}

bool YamahaReceiver::setVolumePercent(int vol) {
  int realVal = map(vol, 0, 100, -800, -200);
  int remainder = realVal%10;
  realVal -= remainder;
  if (remainder <= (-5)) realVal -= 5;    // remainder is <0, because realVal is <0
  return setVolume(realVal);
}

bool YamahaReceiver::setMute(bool onoff) {
  bool ok = executeSetCommand(XML_SET_MUTE_START, onoff ? "On" : "Off", XML_SET_MUTE_END);
  if (ok) {
    if (_mute != onoff) _dirty |= MUTE;
    _mute = onoff;
  }
  return ok;
}

bool YamahaReceiver::setTreble(int treb) {
  bool ok = executeSetCommand(XML_SET_TREBLE_START, treb, XML_SET_TREBLE_END);
  if (ok) {
    if(_treble != treb) _dirty |= TREBLE;
    _treble = treb;
  }
  return ok;
}

bool YamahaReceiver::setBass(int bas) {
  bool ok = executeSetCommand(XML_SET_BASS_START, bas, XML_SET_BASS_END);
  if (ok) {
    if (_bass != bas) _dirty |= BASS;
    _bass = bas;
  }
  return ok;
}

bool YamahaReceiver::setSubwooferTrim(int val) {
  bool ok = executeSetCommand(XML_SET_SWTRIM_START, val, XML_SET_SWTRIM_END);
  if (ok) {
    if (_subwooferTrim != val) _dirty |= SWTRIM;
    _subwooferTrim = val;
  }
  return ok;
}

bool YamahaReceiver::setSource(const char* srcName) {
  bool ok = executeSetCommand(XML_SET_SOURCE_START, srcName, XML_SET_SOURCE_END);
  if (ok && (strcmp(_source, srcName)!=0)) {
    strncpy(_source, srcName, sizeof(_source));
    _source[sizeof(_source)-1] = '\0';
    _dirty |= SOURCE;
  }
  return ok;
}

bool YamahaReceiver::setStraight(bool onoff) {
  bool ok = executeSetCommand(XML_SET_STRAIGHT_START, onoff ? "On" : "Off", XML_SET_STRAIGHT_END);
  if ( ok && (_straight != onoff)) {
    _straight = onoff;
    _dirty |= STRAIGHT;
  }
  return ok;
}

bool YamahaReceiver::setEnhancer(bool onoff) {
  bool ok = executeSetCommand(XML_SET_ENHANCER_START, onoff ? "On" : "Off", XML_SET_ENHANCER_END);
  if (ok && (_enhancer != onoff)) {
    _enhancer = onoff;
    _dirty |= ENHANCER;
  }
  return ok;
}

bool YamahaReceiver::setSoundProgram(const char* dspname) {
  bool ok = executeSetCommand(XML_SET_DSP_START, dspname, XML_SET_DSP_END);
  if (ok && (strcmp(dspname, _soundProgram)!=0)) {
        strncpy(_soundProgram, dspname, sizeof(_soundProgram));
        _soundProgram[sizeof(_soundProgram)-1] = '\0';
        _dirty |= DSP;
  }
  return ok;
}

// ----------------------------------------------------
// HTTP XML Sender (mit Timeout)
// ----------------------------------------------------

void YamahaReceiver::EnsureDelayBeforeRequest(unsigned long timeout) {
    static unsigned long LastRequest = 0;
    unsigned long now = millis();
    while(now - LastRequest < timeout) {
      delay(10);
      now = millis();
    }
    return;
}

bool YamahaReceiver::sendXMLRequest(WiFiClient& wifi, HTTPClient& http, const __FlashStringHelper* xml) {
  EnsureDelayBeforeRequest(100);
  // URL zusammenbauen
  //char url[56];
  snprintf(_url, sizeof(_url), "http://%d.%d.%d.%d/YamahaRemoteControl/ctrl", _ip[0], _ip[1], _ip[2], _ip[3]);
  
  if (!http.begin(wifi, _url)) {
    return false;
  }

  http.addHeader(F("Content-Type"), F("text/xml; charset=UTF-8"));
  http.setReuse(false); // Yamaha-Schnittstellen mögen oft keine persistenten Verbindungen

  // Body zusammensetzen (Header + XML)
  // Wir nutzen einen lokalen Puffer auf dem Stack (ca. 160-200 Bytes reichen locker)
  //char body[256]; 
  // Zusammenbauen mit snprintf_P (spart RAM, nutzt Flash-Strings)
  // %S = Flash-String (__FlashStringHelper*), %s = RAM-String (char*)
  int len = snprintf_P(_body, sizeof(_body), PSTR("%S%S"), 
                       (PGM_P)XML_HEADER, (PGM_P)xml);

  if (len >= (int)sizeof(_body)) {
      Serial.println(F("[Yamaha] XML too long for buffer!"));
      //http.end();
      NetworkHelper::resetHttpClient(http);
      return false;
  }

  int httpCode = http.POST((uint8_t*)_body, len);

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[YamahaReceiver::sendXMLRequest] POST failed, error: %s\n", http.errorToString(httpCode).c_str());
    //http.end();
    NetworkHelper::resetHttpClient(http);
    return false;
  }

  return true;
}

bool YamahaReceiver::sendXMLRequest(WiFiClient& wifi, HTTPClient& http, const char* xml) {
  return sendXMLRequest(wifi, http, FPSTR(xml));
}

bool YamahaReceiver::executeSetCommand(WiFiClient& wifi, HTTPClient& http, const __FlashStringHelper* xmlstart, const char* xmlval, const __FlashStringHelper* xmlend) {
  EnsureDelayBeforeRequest(100);

  //char url[56];
  snprintf(_url, sizeof(_url), "http://%d.%d.%d.%d/YamahaRemoteControl/ctrl", _ip[0], _ip[1], _ip[2], _ip[3]);

  if (!http.begin(wifi, _url)) {
    NetworkHelper::resetHttpClient(http);
    return false;
  }

  http.addHeader(F("Content-Type"), F("text/xml; charset=UTF-8"));
  http.setReuse(false);

  // Wir nutzen einen lokalen Puffer auf dem Stack (ca. 160-200 Bytes reichen locker)
  //char body[256]; 
  // %S = Flash-String (__FlashStringHelper*), %s = RAM-String (char*)
  int len = snprintf_P(_body, sizeof(_body), PSTR("%S%S%s%S"), 
                       (PGM_P)XML_HEADER, (PGM_P)xmlstart, xmlval, (PGM_P)xmlend);

  if (len >= (int)sizeof(_body)) {
      Serial.println(F("[Yamaha] XML too long for buffer!"));
      //http.end();
      NetworkHelper::resetHttpClient(http);
      return false;
  }

  int httpCode = http.POST((uint8_t*)_body, len);

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[YamahaReceiver] POST failed, error: %s (%d)\n", http.errorToString(httpCode).c_str(), httpCode);
    //http.end();
    NetworkHelper::resetHttpClient(http);
    return false;
  }

  return true;
}

bool YamahaReceiver::executeSetCommand(const __FlashStringHelper* xmlstart, const char* xmlval, const __FlashStringHelper* xmlend) {
  WiFiClient wifi;
  HTTPClient http;
  bool success = executeSetCommand(wifi, http, xmlstart, xmlval, xmlend);
  //http.end();
  //wifi.stop(); 
  NetworkHelper::resetClients(wifi, http, success);
  return success;
}

bool YamahaReceiver::executeSetCommand(WiFiClient& wifi, HTTPClient& http, const char* start, const char* val, const char* end) {
  return executeSetCommand(wifi, http, FPSTR(start), val, FPSTR(end));
}

bool YamahaReceiver::executeSetCommand(const char* start, const char* val, const char* end) {
  return executeSetCommand(FPSTR(start), val, FPSTR(end));
}

bool YamahaReceiver::executeSetCommand(WiFiClient& wifi, HTTPClient& http, const char* xmlstart, int val, const char* xmlend) {
  char buf[12];
  itoa(val, buf, 10); // schneller und kleiner als snprintf
  return executeSetCommand(wifi, http, FPSTR(xmlstart), (const char*)buf, FPSTR(xmlend));
}

bool YamahaReceiver::executeSetCommand(const char* xmlstart, int val, const char* xmlend) {
  char buf[12];
  itoa(val, buf, 10); // schneller und kleiner als snprintf
  return executeSetCommand(FPSTR(xmlstart), (const char*)buf, FPSTR(xmlend));
}

KinoError YamahaReceiver::tick() {
  if (WiFi.status() != WL_CONNECTED) return KinoError::NothingToDo;
  //if (_tickInterval == 0) return KinoError::NothingToDo;
  //if (_refreshing) return KinoError::NothingToDo;
  
  //int now = millis();
  //if (now - _lastTick >= _tickInterval) {
    //_lastTick = now;
    //_refreshing = true;
    bool ok = getStatus();
    //_refreshing = false;
    return (ok ? KinoError::OK : KinoError::DeviceNotReady);
  //}
  //return KinoError::NothingToDo;
}
/*
bool YamahaReceiver::setTickInterval(int ms) {
  if (ms == 0) { _tickInterval = 0; return true; }
  if (ms < 0) return false;       // nur zur besseren Lesbarkeit hier aufgeführt
  if (ms < 2000) return false;    // unter 2 Sekunden Interval führt zu übermässigem Traffic
  _tickInterval = ms;
  _lastTick = millis();
  return true;
}*/

char YamahaReceiver::_body[256];
char YamahaReceiver::_url[56];

// ------------------------------------------------------------
// XML Template Definition (PROGMEM)
// ------------------------------------------------------------
const char YamahaReceiver::XML_HEADER[] PROGMEM = 
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>";

const char YamahaReceiver::XML_GET_STATUS[] PROGMEM =
    "<YAMAHA_AV cmd=\"GET\"><Main_Zone><Basic_Status>GetParam</Basic_Status></Main_Zone></YAMAHA_AV>";

const char YamahaReceiver::XML_GET_INPUTNAMES[] PROGMEM = 
    "<YAMAHA_AV cmd=\"GET\"><System><Input_Output><Input_Name>GetParam</Input_Name></Input_Output></System></YAMAHA_AV>";

const char YamahaReceiver::XML_GET_INPUTSKIP[] PROGMEM = 
    "<YAMAHA_AV cmd=\"GET\"><System><Input_Output><Input_Skip>GetParam</Input_Skip></Input_Output></System></YAMAHA_AV>";

const char YamahaReceiver::XML_GET_NETRADIO_LIST[] PROGMEM = 
    "<YAMAHA_AV cmd=\"GET\"><NET_RADIO><List_Info>GetParam</List_Info></NET_RADIO></YAMAHA_AV>";

const char YamahaReceiver::XML_SET_MOVEHOME[] PROGMEM = 
    "<YAMAHA_AV cmd=\"PUT\"><NET_RADIO><List_Control><Cursor>Return to Home</Cursor></List_Control></NET_RADIO></YAMAHA_AV>";

const char YamahaReceiver::XML_SELECT_NEXT_PAGE[] PROGMEM = 
    "<YAMAHA_AV cmd=\"PUT\"><NET_RADIO><List_Control><Page>Down</Page></List_Control></NET_RADIO></YAMAHA_AV>";

const char YamahaReceiver::XML_SET_SELECT_LINE_ONE[] PROGMEM = 
    "<YAMAHA_AV cmd=\"PUT\"><NET_RADIO><List_Control><Direct_Sel>Line_1</Direct_Sel></List_Control></NET_RADIO></YAMAHA_AV>";

const char YamahaReceiver::XML_SET_SELECT_LINENR_START[] PROGMEM = 
    "<YAMAHA_AV cmd=\"PUT\"><NET_RADIO><List_Control><Direct_Sel>Line_";

const char YamahaReceiver::XML_SET_SELECT_LINENR_END[] PROGMEM = 
    "</Direct_Sel></List_Control></NET_RADIO></YAMAHA_AV>";

const char YamahaReceiver::XML_GET_DSP_SKIP[] PROGMEM = 
    "<YAMAHA_AV cmd=\"GET\"><System><Surround><DSP_Skip>GetParam</DSP_Skip></Surround></System></YAMAHA_AV>";

const char YamahaReceiver::XML_GET_NETRADIO_PLAYINFO[] PROGMEM = 
    "<YAMAHA_AV cmd=\"GET\"><NET_RADIO><Play_Info>GetParam</Play_Info></NET_RADIO></YAMAHA_AV>";

const char YamahaReceiver::XML_SET_POWER_START[] PROGMEM = 
    "<YAMAHA_AV cmd=\"PUT\"><System><Power_Control><Power>";

const char YamahaReceiver::XML_SET_POWER_END[] PROGMEM = 
    "</Power></Power_Control></System></YAMAHA_AV>";

const char YamahaReceiver::XML_SET_VOLUME_START[] PROGMEM = 
    "<YAMAHA_AV cmd=\"PUT\"><Main_Zone><Volume><Lvl><Val>";

const char YamahaReceiver::XML_SET_VOLUME_END[] PROGMEM = 
    "</Val><Exp>1</Exp><Unit>dB</Unit></Lvl></Volume></Main_Zone></YAMAHA_AV>";

const char YamahaReceiver::XML_SET_MUTE_START[] PROGMEM = 
    "<YAMAHA_AV cmd=\"PUT\"><Main_Zone><Volume><Mute>";

const char YamahaReceiver::XML_SET_MUTE_END[] PROGMEM = 
    "</Mute></Volume></Main_Zone></YAMAHA_AV>";

const char YamahaReceiver::XML_SET_TREBLE_START[] PROGMEM = 
    "<YAMAHA_AV cmd=\"PUT\"><Main_Zone><Sound_Video><Tone><Treble><Val>";

const char YamahaReceiver::XML_SET_TREBLE_END[] PROGMEM = 
    "</Val><Exp>1</Exp><Unit>dB</Unit></Treble></Tone></Sound_Video></Main_Zone></YAMAHA_AV>";

const char YamahaReceiver::XML_SET_BASS_START[] PROGMEM = 
    "<YAMAHA_AV cmd=\"PUT\"><Main_Zone><Sound_Video><Tone><Bass><Val>";

const char YamahaReceiver::XML_SET_BASS_END[] PROGMEM = 
    "</Val><Exp>1</Exp><Unit>dB</Unit></Bass></Tone></Sound_Video></Main_Zone></YAMAHA_AV>";

const char YamahaReceiver::XML_SET_SWTRIM_START[] PROGMEM = 
    "<YAMAHA_AV cmd=\"PUT\"><Main_Zone><Volume><Subwoofer_Trim><Val>";

const char YamahaReceiver::XML_SET_SWTRIM_END[] PROGMEM = 
    "</Val></Subwoofer_Trim></Volume></Main_Zone></YAMAHA_AV>";

const char YamahaReceiver::XML_SET_SOURCE_START[] PROGMEM = 
    "<YAMAHA_AV cmd=\"PUT\"><Main_Zone><Input><Input_Sel>";

const char YamahaReceiver::XML_SET_SOURCE_END[] PROGMEM = 
    "</Input_Sel></Input></Main_Zone></YAMAHA_AV>";

const char YamahaReceiver::XML_SET_STRAIGHT_START[] PROGMEM = 
    "<YAMAHA_AV cmd=\"PUT\"><Main_Zone><Surround><Program_Sel><Current><Straight>";

const char YamahaReceiver::XML_SET_STRAIGHT_END[] PROGMEM = 
    "</Straight></Current></Program_Sel></Surround></Main_Zone></YAMAHA_AV>";

const char YamahaReceiver::XML_SET_ENHANCER_START[] PROGMEM = 
    "<YAMAHA_AV cmd=\"PUT\"><Main_Zone><Surround><Program_Sel><Current><Enhancer>";

const char YamahaReceiver::XML_SET_ENHANCER_END[] PROGMEM = 
    "</Enhancer></Current></Program_Sel></Surround></Main_Zone></YAMAHA_AV>";

const char YamahaReceiver::XML_SET_DSP_START[] PROGMEM = 
    "<YAMAHA_AV cmd=\"PUT\"><Main_Zone><Surround><Program_Sel><Current><Sound_Program>";

const char YamahaReceiver::XML_SET_DSP_END[] PROGMEM = 
    "</Sound_Program></Current></Program_Sel></Surround></Main_Zone></YAMAHA_AV>";
