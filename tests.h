
void showYamahaTicker() {
  Serial.println("\n\n========== YAMAHA TICKER ===========\n");
  KinoVariant v;
  /* KinoError err; */  // Da dies nur Tests sind, gehen wir graceful über eventuelle Fehler hinweg
  KinoAPI::getProperty("yamaha","tickInterval",v);
  Serial.print("Intervall: alle "); Serial.print(v.i); Serial.println("ms");
  KinoAPI::getProperty("yamaha","power",v);
  Serial.print("Power:  "); Serial.println(v.b ? "An":"Aus");
  KinoAPI::getProperty("yamaha","volume",v);
  Serial.print("Volume: "); Serial.print(v.asInt()/10); Serial.println("dB");
  KinoAPI::getProperty("yamaha","source",v);
  Serial.print("Source: "); Serial.println(v.s);
  if (strcmp(v.s, "NET RADIO")==0) {
    KinoAPI::getProperty("yamaha","station",v);
    Serial.printf("\tStation: %s\n",v.s);
    KinoAPI::getProperty("yamaha","song",v);
    Serial.printf("\tSong   : %s\n",v.s);
    KinoAPI::getProperty("yamaha","elapsed",v);
    Serial.printf("\tElapsed: %s\n",v.s);
  }
}

void showBeamerTicker() {
  Serial.println("\n\n========== BEAMER TICKER ===========\n");
  KinoVariant v;
  KinoAPI::getProperty("beamer","tickInterval",v);
  Serial.print("Intervall: alle "); Serial.print(v.i); Serial.println("ms");
  KinoAPI::getProperty("beamer","power",v);
  Serial.print("Power:  "); Serial.println((v.b) ? "An" : "Aus");
  KinoAPI::getProperty("beamer","input",v);
  Serial.print("Source: "); Serial.println(v.s);
  KinoAPI::getProperty("beamer","uptime",v);
  Serial.print("Lampe:  "); Serial.println(v.i);
  Serial.println("\n\n");
}

void showCanvasTicker() {
  Serial.println("\n\n========== LEINWAND TICKER ===========\n");
  KinoVariant v;
  KinoAPI::getProperty("canvas","tickInterval",v);
  Serial.print("Intervall: alle "); Serial.print(v.i); Serial.println("ms");
  KinoAPI::getProperty("canvas","power",v);
  Serial.print("Power:      "); Serial.println((v.b) ? "An" : "Aus");
  KinoAPI::getProperty("canvas","bri",v);
  Serial.print("Brightness: "); Serial.println(v.i);
  KinoAPI::getProperty("canvas","live",v);
  Serial.print("Live Data:  "); Serial.print((v.b) ? "incoming, " : "none, "); 
  KinoAPI::getProperty("canvas","lor",v);
  Serial.println((!v.b) ? "ignoriert" : "bearbeitet");
  KinoAPI::getProperty("canvas","input",v);
  Serial.print("LD Source:  "); Serial.println(v.s);
  KinoAPI::getProperty("canvas","fx",v);
  Serial.print("Effekt:     "); Serial.println(v.i);
  KinoAPI::getProperty("canvas","speed",v);
  Serial.print("  Speed:    "); Serial.println(v.i);
}

void showSoundTicker() {
  Serial.println("\n\n========== SOUND TICKER ===========\n");
  KinoVariant v;
  KinoAPI::getProperty("sound","tickInterval",v);
  Serial.print("Intervall: alle "); Serial.print(v.i); Serial.println("ms");
  KinoAPI::getProperty("sound","on",v);
  Serial.print("Power:      "); Serial.println((v.b) ? "An" : "Aus");
  KinoAPI::getProperty("sound","bri",v);
  Serial.print("Brightness: "); Serial.println(v.i);
  KinoAPI::getProperty("sound","fx",v);
  Serial.print("Effekt:     "); Serial.println(v.i);
  KinoAPI::getProperty("sound","sx",v);
  Serial.print("  Speed:    "); Serial.println(v.i);
}

void showHyperionTicker() {
  Serial.println("\n\n========== HYPERION TICKER ===========\n");
  KinoVariant v;
  KinoAPI::getProperty("hyperion","tickInterval",v);
  Serial.print("Intervall: alle "); Serial.print(v.i); Serial.println("ms");
  Serial.print("Power       : ");
  KinoAPI::getProperty("hyperion","power",v);
  Serial.println(v.b ? "An":"Aus");
  Serial.print("Broadcasting: ");
  KinoAPI::getProperty("hyperion","live",v);
  Serial.println(v.b ? "An":"Aus");
}

void showHueTicker() {
  Serial.println("\n\n========== HUE TICKER ===========\n");
  KinoVariant v;
  KinoAPI::getProperty("hue","tickInterval",v);
  Serial.print("Intervall: alle "); Serial.print(v.i); Serial.println("ms");
  uint16_t propCount;
  KinoAPI::getQueryCount("hue","scenes",propCount);
  Serial.print(propCount); Serial.println(" Szenen");
  KinoAPI::getQueryCount("hue","groups",propCount);
  Serial.print(propCount); Serial.println(" Gruppen");
  KinoAPI::getQueryCount("hue","lights",propCount);
  Serial.print(propCount); Serial.println(" Lampen");
  for (int i=0; i<propCount; i++) {
    KinoAPI::getQuery("hue","lights",i,v);
    char lightProp[64];
    KinoVariant l;
    KinoError err;
    Serial.print("\t"); Serial.print(v.s); Serial.println(" : ");
    snprintf(lightProp,64,"lights/%s/on",v.s);
    KinoAPI::getProperty("hue",lightProp,l);
    Serial.print("\t\tOn:  "); Serial.println(l.b ? "true" : "false");
    snprintf(lightProp,64,"lights/%s/bri",v.s);
    err = KinoAPI::getProperty("hue",lightProp,l);
    if (err==KinoError::OK) {
      Serial.print("\t\tBri: "); Serial.println(l.i);
    }
    snprintf(lightProp,64,"lights/%s,ct",v.s);
    err = KinoAPI::getProperty("hue",lightProp,l);
    if (err==KinoError::OK) {
      Serial.print("\t\tCT  :"); Serial.println(l.i);
    }
    snprintf(lightProp,64,"lights/%s/color",v.s);
    err = KinoAPI::getProperty("hue",lightProp,l);
    if (err == KinoError::OK) {
      Serial.print("\t\tCol :"); 
      Serial.printf(" [ %i , %i , %i ] (oder auch %s )\n", l.color.r, l.color.g, l.color.b, l.toString());
    }
  }
  Serial.println("Sensoren (nur die Wichtigsten):");
  /*HueSensor* sensor = _hue->getSensorByName("Temp Sensor Theke");
  if (sensor) {
    Serial.print(sensor->getName());
    if (sensor->hasValue("temperature")) {
      float t=(sensor->getValue("temperature").as<int>()/100.0f);
      Serial.print(t); Serial.println("°C");
    } else {
      Serial.println("temperature nicht gefunden");
    } 
  } else {
      Serial.println("Temp Sensor Theke nicht gefunden");
  }*/
  KinoError sensorErr = KinoAPI::getProperty("hue","sensors/Temp Sensor Theke/temperature",v);
  if (sensorErr == KinoError::OK) {
    Serial.printf("\tTemp Sensor Theke meldet: temperature = %2.1f\n",(v.asFloat()/100.0f));
  } else {
    Serial.print(kinoErrorToString(sensorErr));
  }
  sensorErr = KinoAPI::getProperty("hue","sensors/Licht Sensor Theke/dark",v);
  bool isDark = v.asBool();
  if (sensorErr == KinoError::OK) {
    Serial.printf("\tLicht Sensor Theke meldet dark = %s\n", v.toString());
  } else {
    Serial.print(kinoErrorToString(sensorErr));
  }
  /*sensor = _hue->getSensorByName("Licht Sensor Theke");
  if (sensor) {
    Serial.println(sensor->getName());
    Serial.print("\tdark: ");
    if (sensor->hasValue("dark")) {
      bool t=(sensor->getValue("dark").as<bool>());
      Serial.println(t ? "true" : "false");
    } else {
      Serial.println("nicht gefunden");
    } 
    Serial.print("\tdaylight: ");
    if (sensor->hasValue("daylight")) {
      bool t=(sensor->getValue("daylight").as<bool>());
      Serial.println(t ? "true" : "false");
    } else {
      Serial.println("nicht gefunden");
    } 
  } else {
      Serial.println("Licht Sensor Theke nicht gefunden");
  }*/
}

void testBasicStatus() {
    // Basic Status
    Serial.println("--- GET BASIC STATUS ---");
    if (!_yamaha->getStatus()) {
      Serial.println("Konnte Basic Status nicht lesen!");
    } else {
      bool power = _yamaha->getPowerStatus();
      Serial.print("Power:  "); Serial.println(power ? "An":"Aus");
      if (!power) {
        Serial.print("Schalte ein: ");
        Serial.println(_yamaha->setPower(true) ? "OK" : "Fehler");
        Serial.print("Setze Lautstärke -60: ");
        Serial.println(_yamaha->setVolume(-600) ? "OK" : "Fehler");
        Serial.print("starte NET RADIO: ");
        Serial.println(_yamaha->setSource("NET RADIO") ? "OK" : "Fehler");
      }
      Serial.println("Lese Status:");
      Serial.print("Volume: "); Serial.println(_yamaha->getVolume());
      Serial.print("Treble: "); Serial.println(_yamaha->getTreble());
      Serial.print("Bass: ");   Serial.println(_yamaha->getBass());
      Serial.print("Mute:   "); Serial.println(_yamaha->getMute() ? "An":"Aus");
      Serial.print("SW Trim:"); Serial.println(_yamaha->getSubTrim());
      Serial.print("Source: "); Serial.println(_yamaha->getSource());
      Serial.print("Straight:"); Serial.println(_yamaha->getStraight() ? "An":"Aus");
      Serial.print("Enhancer:"); Serial.println(_yamaha->getEnhancer() ? "An":"Aus");
      Serial.print("SoundPrg:"); Serial.println(_yamaha->getSoundProgram());
    }
}

void testReadNetRadioFavorites() {
    Serial.println("\n\nLade NET RADIO Favoriten...");
    std::vector<String> favs = _yamaha->readNetRadioFavorites();
    Serial.println("NET RADIO Favoriten:");
    for(String s : favs) {
      Serial.println(s);
    }
}

void testSetNetRadioFavorite() {
    Serial.println("\n\nWähle BOB aus: ");
    if (_yamaha->selectNetRadioFavorite("bob")) {
        Serial.println("Sender ausgewählt!");
    } else {
        Serial.println("Sender nicht gefunden.");
    }
}

void testReadNetRadioPlayInfo() {
    auto info = _yamaha->readCurrentlyPlayingNetRadio();
    Serial.println("Aktueller NET RADIO Track:");
    Serial.println("Station: " + info.station);
    Serial.println("Song:    " + info.song);
    Serial.println("Album:   " + info.album);
    Serial.println("Elapsed: " + info.elapsed);
    Serial.println("Art:     " + info.albumArt);
}

void testReadDsps() {
    Serial.println("\n\nLese alle DSPs aus");
    std::vector<String> dsps = _yamaha->readDspNames();
    for (String d : dsps) {
      Serial.print(d);
      if (d == _yamaha->getSoundProgram()) {
        Serial.println(" !!");
      } else {
        Serial.println();
      }
    }
}

void testReadInputSources() {
    Serial.println("\n\nLese alle Sources aus");
    _yamaha->readInputSources();
    String currentSource = _yamaha->getSource();
    for (const auto& src : _yamaha->readInputSources()) {
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
}

void testAlexaVolume(int tvol) {
  Serial.print("\n\nSetze Lautsärke im Alexa-Style: ");
  Serial.println((_yamaha->setVolumeAlexa(tvol)) ? " OK" : " Fehler");
}

// BEAMER
void BeamerTestStatus() {
  _beamer->begin();
  Serial.println("Status:");
  Serial.print("Power: "); Serial.println((_beamer->getPowerStatus() ? "An" : "Aus"));
  Serial.print("Source: "); Serial.println(_beamer->getSourceString());
  Serial.print("LampHours: "); Serial.println(_beamer->getLampHours());
  Serial.println();
}


// WLED
void canvasTestStatus() {
  _canvas->begin();
  Serial.println("Status:");
  Serial.print("Power:      "); Serial.println((_canvas->getPowerStatus()) ? "An" : "Aus");
  Serial.print("Brightness: "); Serial.println(_canvas->getBrightness());
  Serial.print("Live Data:  "); Serial.print((_canvas->isReceivingLiveData()) ? "incoming, " : "none, "); Serial.println((_canvas->isOverridingLiveData()) ? "ignoriert" : "bearbeitet");
  Serial.print("Effekt:     "); Serial.println(_canvas->getEffect());
  Serial.print("  Speed:    "); Serial.println(_canvas->getSpeed());
}
