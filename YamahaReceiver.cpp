#include "YamahaReceiver.h"
#include "NetworkHelper.h"


YamahaReceiver::YamahaReceiver(IPAddress ip) : _ip(ip) {}
YamahaReceiver::YamahaReceiver(const String& ip) { _ip.fromString(ip); }

bool YamahaReceiver::begin() {
  return init();
}

bool YamahaReceiver::init() {
  if (!getStatus()) return false;
  if (_powerStatus) {
    readInputSources();
  }
  return true;
}

// ----------------------------------------------------
// für KinoAPI: overrides für set und get aus KinoDevice
// ----------------------------------------------------

KinoError YamahaReceiver::set(const char* property, const KinoVariant& value) {
    if (!property) {
        return KinoError::PropertyNotSupported;
    }
    if (strcmp(property, "power") == 0) {
        if (value.type != KinoVariant::BOOL) {
            return KinoError::InvalidType;
        }
        setPower(value.b);
        return KinoError::OK;
    }
    if (strcmp(property, "volume") == 0) {
        if (value.type != KinoVariant::INT) {
            return KinoError::InvalidType;
        }
        setVolume(value.i);
        return KinoError::OK;
    }
    if (strcmp(property, "mute") == 0) {
        if (value.type != KinoVariant::BOOL) {
            return KinoError::InvalidType;
        }
        setMute(value.b);
        return KinoError::OK;
    }
    if (strcmp(property, "input") == 0) {
        if (value.type != KinoVariant::STRING) {
            return KinoError::InvalidType;
        }
        setSource(value.s);
        return KinoError::OK;
    }
    if (strcmp(property,"station") == 0) {
      if (value.type != KinoVariant::STRING) return KinoError::InvalidType;
      if (!selectNetRadioFavorite(value.s)) return KinoError::InvalidValue;
      return KinoError::OK;
    }
    if (strcmp(property, "inp_custom") == 0) {
      if (value.type != KinoVariant::STRING) return KinoError::InvalidType;
      for (auto &s : _InputSources) {
        if (s.custom == value.s) {
          bool success = setSource(s.internal);
          if (!success) return KinoError::InternalError;
          return KinoError::OK;
        }
      }
      return KinoError::InvalidValue;
    }
    if (strcmp(property, "treble") == 0) {
      if (value.type != KinoVariant::INT) return KinoError::InvalidType;
      int v = value.i;
      if ((v < -6)||(v > 6)) return KinoError::InvalidValue;
      setTreble(v);
      return KinoError::OK;
    }
    if (strcmp(property, "bass") == 0) {
      if (value.type != KinoVariant::INT) return KinoError::InvalidType;
      int v = value.i;
      if ((v<-6)||(v>6)) return KinoError::InvalidValue;
      setBass(v);
      return KinoError::OK;
    }
    if (strcmp(property, "swtrim") == 0) {
      if (value.type != KinoVariant::INT) return KinoError::InvalidType;
      int v = value.i;
      if ((v<-6)||(v>6)) return KinoError::InvalidValue;
      setSubwooferTrim(v);
      return KinoError::OK;
    }
    if (strcmp(property, "straight") == 0) {
      if (value.type != KinoVariant::BOOL) return KinoError::InvalidType;
      setStraight(value.b);
      return KinoError::OK;
    }
    if (strcmp(property, "enhancer") == 0) {
      if (value.type != KinoVariant::BOOL) return KinoError::InvalidType;
      setEnhancer(value.b);
      return KinoError::OK;
    }
    if (strcmp(property, "dsp") == 0) {
      if (value.type != KinoVariant::STRING) return KinoError::InvalidType;
      String d = value.s;
      setSoundProgram(d);
      return KinoError::OK;
    }
    if (strcmp(property, "tickInterval") == 0) {
      if (value.type != KinoVariant::INT) return KinoError::InvalidType;
      if (!setTickInterval(value.i)) return KinoError::InvalidValue;
    }
    return KinoError::PropertyNotSupported;
}

KinoError YamahaReceiver::get(const char* property, KinoVariant& out) {
    if (!property) {
        return KinoError::PropertyNotSupported;
    }

    if (strcmp(property, "power") == 0) {
        out = KinoVariant::fromBool(_powerStatus);
        return KinoError::OK;
    }
    if (strcmp(property, "volume") == 0) {
        out = KinoVariant::fromInt(_volume);
        return KinoError::OK;
    }
    if (strcmp(property, "mute") == 0) {
        out = KinoVariant::fromBool(_mute);
        return KinoError::OK;
    }
    if (strcmp(property, "input") == 0) {
        out = KinoVariant::fromString(_source.c_str());
        return KinoError::OK;
    }
    if (strcmp(property, "station") == 0) {
      if (_source == "NET RADIO") {
        NetRadioTrackInfo nri = readCurrentlyPlayingNetRadio();
        out = KinoVariant::fromString(nri.station.c_str());
        return KinoError::OK;
      } else {
        out = KinoVariant::fromString("");
        return KinoError::OK;
      }
    }
    if (strcmp(property, "song") == 0) {
      if (_source == "NET RADIO") {
        NetRadioTrackInfo nri = readCurrentlyPlayingNetRadio();
        out = KinoVariant::fromString(nri.station.c_str());
        return KinoError::OK;
      } else {
        out = KinoVariant::fromString("");
        return KinoError::OK;
      }
    }
    if (strcmp(property, "inp_custom") == 0) {
      InputSource inp = getInputSource();
      out = KinoVariant::fromString(inp.custom.c_str());
    }
    if (strcmp(property, "treble") == 0) {
      out = KinoVariant::fromInt(_treble);
      return KinoError::OK;
    }
    if (strcmp(property, "bass") == 0) {
      out = KinoVariant::fromInt(_bass);
      return KinoError::OK;
    }
    if (strcmp(property, "swtrim") == 0) {
      out = KinoVariant::fromInt(_subwooferTrim);
      return KinoError::OK;
    }
    if (strcmp(property, "ip") == 0) {
      out = KinoVariant::fromString(_ip.toString().c_str());
      return KinoError::OK;
    }
    if (strcmp(property, "straight") == 0) {
      out = KinoVariant::fromBool(_straight);
      return KinoError::OK;
    }
    if (strcmp(property, "enhancer") == 0) {
      out = KinoVariant::fromBool(_enhancer);
      return KinoError::OK;
    }
    if (strcmp(property, "dsp") == 0) {
      out = KinoVariant::fromString(_soundProgram.c_str());
      return KinoError::OK;
    }
    if (strcmp(property,"tickInterval")) {
      out = KinoVariant::fromInt(_tickInterval);
      return KinoError::OK;
    }
    return KinoError::PropertyNotSupported;
}



// ----------------------------------------------------
// Public Getter
// ----------------------------------------------------
IPAddress YamahaReceiver::getIp()     const { return _ip; }
bool YamahaReceiver::getPowerStatus() const { return _powerStatus; }
int  YamahaReceiver::getVolume()      const { return _volume; }
int  YamahaReceiver::getTreble()      const { return _treble; }
int  YamahaReceiver::getBass()        const { return _bass; }
int  YamahaReceiver::getSubTrim()     const { return _subwooferTrim; }
bool YamahaReceiver::getStraight()    const { return _straight; }
bool YamahaReceiver::getEnhancer()    const { return _enhancer; }
String YamahaReceiver::getSource()    const { return _source; }
String YamahaReceiver::getSoundProgram() const { return _soundProgram; }
bool YamahaReceiver::getMute()        const { return _mute; }

bool YamahaReceiver::getStatus() {
    //Serial.println(F("Yamaha: Start Request..."));
    if (!sendXMLRequest(FPSTR(XML_GET_STATUS))) return false;

    // Wir gehen die Tags in der Reihenfolge durch, wie Yamaha sie schickt:
    // 1. Power
    if (_client.find("<Power>")) {
        _powerStatus = (_client.readStringUntil('<') == "On");
    }
    
    // 2. Volume (liegt in <Volume><Lvl><Val>...</Val></Lvl></Volume>)
    if (_client.find("<Val>")) {
        _volume = _client.readStringUntil('<').toInt();
    }
    // 5. Mute
    if (_client.find("<Mute>")) {
        _mute = (_client.readStringUntil('<') == "On");
    }
    // 2a. Subwoofer Trim (liegt im nächsten <Val>...</Val>
    if (_client.find("<Val>")) {
        _subwooferTrim = _client.readStringUntil('<').toInt();
    }
    // 3. Input
    if (_client.find("<Input_Sel>")) {
        _source = _client.readStringUntil('<');
    }
    // Straight (liegt in <Straight>...</Straight>
    if (_client.find("<Straight>")) {
        _straight = (_client.readStringUntil('<') == "On");
    }
    // Enhancer
    if (_client.find("<Enhancer>")) {
        _enhancer = (_client.readStringUntil('<') == "On");
    }
    // 4. Sound Program
    if (_client.find("<Sound_Program>")) {
        _soundProgram = _client.readStringUntil('<');
    }
    // Bass: (liegt im nächsten <Val>...</Val>
    if (_client.find("<Val>")) {
        _bass = _client.readStringUntil('<').toInt();
    }
    // Treble: liegt im nächsten <Val>...</Val>
    if (_client.find("<Val>")) {
        _treble = _client.readStringUntil('<').toInt();
    }
    

    // WICHTIG: Den Rest des Streams verwerfen und schließen
    _client.stop();
    return true;
}

void YamahaReceiver::initInputSources() {
  if (_gotInputSources) return; // Nur einmal im Leben des ESP machen
  
  _InputSources.clear();
  _InputSources.reserve(28); // Einmalig Platz reservieren

  // Wir füllen den Vektor mit Pointern auf Flash-Strings
  // Das spart ca. 1.5 KB RAM im Vergleich zur alten Version!
  _InputSources.push_back({F("TUNER"), F("TUNER"), "", false});
  _InputSources.push_back({F("PHONO"), F("PHONO"), "", false});
  _InputSources.push_back({F("HDMI_1"), F("HDMI1"), "", false});
  _InputSources.push_back({F("HDMI_2"), F("HDMI2"), "", false});
  _InputSources.push_back({F("HDMI_3"), F("HDMI3"), "", false});
  _InputSources.push_back({F("HDMI_4"), F("HDMI4"), "", false});
  _InputSources.push_back({F("HDMI_5"), F("HDMI5"), "", false});
  _InputSources.push_back({F("AV_1"), F("AV1"), "", false});
  _InputSources.push_back({F("AV_2"), F("AV2"), "", false});
  _InputSources.push_back({F("AUX"), F("AUX"), "", false});
  _InputSources.push_back({F("AUDIO_1"), F("AUDIO1"), "", false});
  _InputSources.push_back({F("AUDIO_2"), F("AUDIO2"), "", false});
  _InputSources.push_back({F("AUDIO_3"), F("AUDIO3"), "", false});
  _InputSources.push_back({F("AUDIO_4"), F("AUDIO4"), "", false});
  _InputSources.push_back({F("AUDIO_5"), F("AUDIO5"), "", false});
  _InputSources.push_back({F("Napster"), F("Napster"), "", false});
  _InputSources.push_back({F("Spotify"), F("Spotify"), "", false});
  _InputSources.push_back({F("Qobuz"), F("Qobuz"), "", false});
  _InputSources.push_back({F("TIDAL"), F("TIDAL"), "", false});
  _InputSources.push_back({F("Deezer"), F("Deezer"), "", false});
  _InputSources.push_back({F("Amazon_Music"), F("Amazon Music"), "", false});
  _InputSources.push_back({F("Alexa"), F("Alexa"), "", false});
  _InputSources.push_back({F("AirPlay"), F("AirPlay"), "", false});
  _InputSources.push_back({F("MusicCast_Link"), F("MusicCast Link"), "", false});
  _InputSources.push_back({F("SERVER"), F("SERVER"), "", false});
  _InputSources.push_back({F("NET_RADIO"), F("NET RADIO"), "", false});
  _InputSources.push_back({F("Bluetooth"), F("Bluetooth"), "", false});
  _InputSources.push_back({F("USB"), F("USB"), "", false});
}

std::vector<InputSource> YamahaReceiver::readInputSources() {
  initInputSources();
  
  String keyname; keyname.reserve(40);
  
  // 1. Namen abfragen
  if (!sendXMLRequest(FPSTR(XML_GET_INPUTNAMES))) return _InputSources;
  // Finde "<Input_Name>"
  if (_client.find("<Input_Name>")) {
    while(true) {
      if (_client.find('<')) {
        keyname = _client.readStringUntil('>');
        if (keyname == "/Input_Name") {
          // end of list
          break;
        }
        InputSource* is = getInputSourceByKey(keyname);
        if (is) {
          is->custom = _client.readStringUntil('<');
        }
        if (!_client.find('>')) {
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

  // 2. Skip-Status abfragen (analog zum ersten Teil)
  if (!sendXMLRequest(FPSTR(XML_GET_INPUTSKIP))) return _InputSources;
  // Finde "<Input_Name>"
  if (_client.find("<Input_Skip>")) {
    while (true) {
      if (_client.find('<')) {
        keyname = _client.readStringUntil('>');
        if (keyname == "/Input_Skip") {
          // end of list
          break;
        }
        InputSource* is = getInputSourceByKey(keyname);
        if (is) {
          is->skip = (_client.readStringUntil('<')=="On");
        }
        if (!_client.find('>')) {
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
  
  _client.stop();

  _gotInputSources = true;
  return _InputSources;
}

InputSource YamahaReceiver::getInputSource() {
  if (!_gotInputSources) readInputSources();
  for (auto& s : _InputSources) {
    if (String(s.internal) == _source) return s;
  }
  InputSource unknown = {F("NONE"), F("NONE"), "NONE", true};
  return unknown;
}

InputSource* YamahaReceiver::getInputSourceByKey(const String& keyname) {
  for (auto& s: _InputSources) {
    if (String(s.key) == keyname) return &s;
  }
  // no such InputSource
  return nullptr;
}

// ----------------------------------------------------
// Liste der NET RADIO Items auslesen
// ----------------------------------------------------
String YamahaReceiver::readNetRadioList() {
  unsigned long startTime = millis();
  String resp;
  String menuStatus;
  
  String req = FPSTR(XML_GET_NETRADIO_LIST);
  do {
    resp = sendXML(req);
    menuStatus = extractTagString(resp, "Menu_Status");
    if (menuStatus == "Ready") break;
    delay(50);  // kleines Intervall warten, bevor erneut abgefragt wird
  } while (millis() - startTime < 2000); // maximal 2 Sekunden warten
  return resp; // liefert die letzte Antwort zurück
}



// ----------------------------------------------------
// Navigiert zu Favoriten
// ----------------------------------------------------
bool YamahaReceiver::moveToFavorites() {
  String resp = readNetRadioList();
  int layer = extractTagInt(resp, "Menu_Layer");
  String layername = extractTagString(resp, "Menu_Name");
  if ((layer == 3)&&(layername == "Favoriten")) return true;

  //String req = FPSTR(XML_SET_MOVEHOME);
  if (!executeSetCommand(FPSTR(XML_SET_MOVEHOME), F(""),F(""))) return false;

  // lies die aktuelle Liste aus. Da die Funktion auf eine gültige Liste wartet, ist die Auswahl definitiv erfolgt
  readNetRadioList();

  // wähle Punkt 1 der Liste aus
  if (!executeSetCommand(FPSTR(XML_SET_SELECT_LINE_ONE), F(""),F(""))) return false;

  // lies die Liste, genau wie eben
  readNetRadioList();
  
  // wähle Punkt 1 der Liste aus
  return executeSetCommand(FPSTR(XML_SET_SELECT_LINE_ONE), F(""),F(""));
  // return true;
}

// ----------------------------------------------------
// Favoriten auslesen
// ----------------------------------------------------
std::vector<String> YamahaReceiver::readNetRadioFavorites() {
    std::vector<String> stations;

    if (!moveToFavorites()) return stations;
    String resp = readNetRadioList();

    // Minimal-Parsing: suche <Txt>...</Txt>
    int pos = 0;
    while ((pos = resp.indexOf("<Txt>", pos)) >= 0) {
        int start = pos + 5;
        int end = resp.indexOf("</Txt>", start);
        if (end < 0) break;
        String name = resp.substring(start, end);
        stations.push_back(name);
        pos = end + 6;
    }

    return stations;
}

bool YamahaReceiver::selectNetRadioFavorite(const String& radioname) {
  String chk = radioname;
  chk.toLowerCase();
  std::vector<String> favorites = readNetRadioFavorites();
  for (size_t i = 0; i < favorites.size(); i++) {
    String station = favorites[i];
    station.toLowerCase();
    if (station.indexOf(chk) >= 0) {
    // Yamaha erwartet "Line_1", "Line_2", ...
      return executeSetCommand(FPSTR(XML_SET_SELECT_LINENR_START),String(i+1),FPSTR(XML_SET_SELECT_LINENR_END));
    }
  }
  return false; // kein passender Sender gefunden
}

std::vector<String> YamahaReceiver::readDspNames() {
  std::vector<String> dspnames;
  if (!sendXMLRequest(FPSTR(XML_GET_DSP_SKIP))) return dspnames;
  if (!_client.find((char*)"RC=\"0\"")) {
    _client.stop();
    return dspnames;
  }
  String keyname; keyname.reserve(32);
  // Finde "<DSP_Skip>"
  unsigned long now = millis();
  unsigned long wdStart = now;
  unsigned long maxTimeout = 1000;
  if (_client.find("<DSP_Skip>")) {
    while(now - wdStart < maxTimeout) { // ensure max timeout
      if (_client.find('<')) {
        keyname = _client.readStringUntil('>');
        // end of list?
        if (keyname == "/DSP_Skip") break;
        bool skip = (_client.readStringUntil('<')=="On");
        if (!skip) {
          keyname.replace("_"," ");
          keyname.trim();
          dspnames.push_back(keyname);
        }
        // try skipping the closing tag (in fact, ANY tag)
        // closing tag not terminated => something is wrong
        if (!_client.find('>')) break;
      } else {
        // no tag opener, something is wrong
        break;
      }
      now = millis();
      yield();  // take this, watchdog ;-)
    }
  }
  _client.stop();
  return dspnames;
}

NetRadioTrackInfo YamahaReceiver::readCurrentlyPlayingNetRadio() {
  NetRadioTrackInfo info;
  if (!sendXMLRequest(FPSTR(XML_GET_NETRADIO_PLAYINFO))) return info;

  if (_client.find("<Elapsed>"))  info.elapsed = _client.readStringUntil('<');
  if (_client.find("<Station>"))  info.station = _client.readStringUntil('<');
  if (_client.find("<Album>"))    info.album   = _client.readStringUntil('<');
  if (_client.find("<Song>"))     info.song    = _client.readStringUntil('<');
  if (_client.find("<URL>"))      info.albumArt = _client.readStringUntil('<');

  _client.stop();
  return info;
}

bool YamahaReceiver::setPower(bool onoff) {
    if (executeSetCommand(FPSTR(XML_SET_POWER_START), onoff ? "On" : "Standby", FPSTR(XML_SET_POWER_END))) {
        _powerStatus = onoff;
        return true;
    }
    return false;
}

bool YamahaReceiver::setVolume(int vol) {
    if (vol > 0) vol *= -1;
    if (executeSetCommand(FPSTR(XML_SET_VOLUME_START), String(vol), FPSTR(XML_SET_VOLUME_END))) {
        _volume = vol;
        _mute = false;
        return true;
    }
    return false;
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
    if (executeSetCommand(FPSTR(XML_SET_MUTE_START), onoff ? "On" : "Off", FPSTR(XML_SET_MUTE_END))) {
        _mute = onoff;
        return true;
    }
    return false;
}

bool YamahaReceiver::setTreble(int treb) {
    if (executeSetCommand(FPSTR(XML_SET_TREBLE_START), String(treb), FPSTR(XML_SET_TREBLE_END))) {
        _treble = treb;
        return true;
    }
    return false;
}

bool YamahaReceiver::setBass(int bas) {
    if (executeSetCommand(FPSTR(XML_SET_BASS_START), String(bas), FPSTR(XML_SET_BASS_END))) {
        _bass = bas;
        return true;
    }
    return false;
}

bool YamahaReceiver::setSubwooferTrim(int val) {
    if (executeSetCommand(FPSTR(XML_SET_SWTRIM_START), String(val), FPSTR(XML_SET_SWTRIM_END))) {
        _subwooferTrim = val;
        return true;
    }
    return false;
}

bool YamahaReceiver::setSource(const String& srcName) {
    if (executeSetCommand(FPSTR(XML_SET_SOURCE_START), srcName, FPSTR(XML_SET_SOURCE_END))) {
        _source = srcName;
        return true;
    }
    return false;
}

bool YamahaReceiver::setStraight(bool onoff) {
    if (executeSetCommand(FPSTR(XML_SET_STRAIGHT_START), onoff ? "On" : "Off", FPSTR(XML_SET_STRAIGHT_END))) {
        _straight = onoff;
        return true;
    }
    return false;
}

bool YamahaReceiver::setEnhancer(bool onoff) {
    if (executeSetCommand(FPSTR(XML_SET_ENHANCER_START), onoff ? "On" : "Off", FPSTR(XML_SET_ENHANCER_END))) {
        _enhancer = onoff;
        return true;
    }
    return false;
}

bool YamahaReceiver::setSoundProgram(const String& dspname) {
    if (executeSetCommand(FPSTR(XML_SET_DSP_START), dspname, FPSTR(XML_SET_DSP_END))) {
        _source = dspname;
        return true;
    }
    return false;
}

// ----------------------------------------------------
// HTTP XML Sender (mit Timeout)
// ----------------------------------------------------

String YamahaReceiver::sendXML(const String& xml) {
  // Wir nutzen die neue, stabile Request-Methode
  if (!sendXMLRequest(xml)) {
    return "";
  }

  // Für die alten Funktionen lesen wir den Rest des Streams in einen String
  // Wir setzen ein Limit, damit der ESP nicht abstürzt (max 2048 Zeichen)
  String response;
  response.reserve(2048);
  
  unsigned long start = millis();
  while (_client.connected() && (millis() - start < 1500)) {
    while (_client.available()) {
      response += (char)_client.read();
      if (response.length() >= 2047) break;
    }
    yield();
  }
  _client.stop(); 
  return response;
}

bool YamahaReceiver::sendXMLRequest(const String& xml, int len/*=0*/) {
    _client.stop(); // Bestehende Verbindungen killen
    _client.setTimeout(1000); // Kurzer Timeout für das LAN

    if (!_client.connect(_ip, 80)) {
      return false;
    }

    int contentLength = (xml.length()>0)?xml.length():len;
    contentLength += strlen_P(XML_HEADER);

    _client.print(F("POST /YamahaRemoteControl/ctrl HTTP/1.1\r\n"));
    _client.print(F("Host: ")); _client.println(_ip.toString());
    _client.println(F("Content-Type: text/xml; charset=UTF-8"));
    _client.print(F("Content-Length: "));
    _client.println(contentLength);
    _client.println(F("Connection: close\r\n\r\n")); // Wichtig: Abschluss-Leerzeile
    
    _client.print(FPSTR(XML_HEADER));
    if (xml.length()==0) return true;   // just got used as a dummy-request-starter, let the caller do the rest
    _client.print(xml);

    if (!NetworkHelper::skipHeader(_client)) {
        _client.stop();
        return false;
    }
    return true;
}

bool YamahaReceiver::executeSetCommand(const __FlashStringHelper* start, const String& val, const __FlashStringHelper* end) {
    int len = strlen_P(reinterpret_cast<PGM_P>(start)) + val.length() + strlen_P(reinterpret_cast<PGM_P>(end));
    if (!sendXMLRequest("",len)) return false; // Verbindung öffnen (Dummy-Request-Starter)

    // Wir streamen den Request direkt in den Client, ohne Zwischen-String!
    _client.print(FPSTR(XML_HEADER));
    _client.print(FPSTR(start));
    _client.print(val);
    _client.print(FPSTR(end));
    // Wir lesen die Antwort (sehr kurz bei PUT) und suchen nach dem OK
    bool success = _client.find((char*)"RC=\"0\""); 
    _client.stop();
    return success;
}

// ----------------------------------------------------
// Hilfsfunktionen zum Tag-Parsing
// ----------------------------------------------------
int YamahaReceiver::extractTagInt(const String& xml, const String& tag) {
  String value = extractTagString(xml, tag);
  return value.length() ? value.toInt() : 0;
}

int YamahaReceiver::extractTagInt(const String& xml, const String& parent, const String& child) {
  String value = extractTagString(xml, parent, child);
  return value.length() ? value.toInt() : 0;
}

String YamahaReceiver::extractTagString(const String& xml, const String& tag) {
  String openTag = "<" + tag + ">";
  String closeTag = "</" + tag + ">";
  int start = xml.indexOf(openTag);
  if (start < 0) return "";
  start += openTag.length();
  int end = xml.indexOf(closeTag, start);
  if (end < 0) return "";
  return xml.substring(start, end);
}

String YamahaReceiver::extractTagString(const String& xml, const String& parent, const String& child) {
  int pStart = xml.indexOf("<" + parent + ">");
  if (pStart < 0) return "";
  int pEnd = xml.indexOf("</" + parent + ">", pStart);
  if (pEnd < 0) return "";
  String section = xml.substring(pStart, pEnd);
  return extractTagString(section, child);
}

// Check, ob die Antwort vom Yamaha "OK" ist
bool YamahaReceiver::isOk(const String& resp) {
  return resp.indexOf("<YAMAHA_AV rsp=\"PUT\" RC=\"0\"") >= 0;
}

bool YamahaReceiver::tick() {
  if (_tickInterval == 0) return false;
  int now = millis();
  if (now - _lastTick >= _tickInterval) {
    _lastTick = now;
    getStatus();
    return true;
  }
  return false;
}

bool YamahaReceiver::setTickInterval(int ms) {
  if (ms == 0) { _tickInterval = 0; return true; }
  if (ms < 0) return false;       // nur zur besseren Lesbarkeit hier aufgeführt
  if (ms < 2000) return false;    // unter 2 Sekunden Interval führt zu übermässigem Traffic
  _tickInterval = ms;
  return true;
}
int YamahaReceiver::getTickInterval() {
  return _tickInterval;
}

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
