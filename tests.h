
void showYamahaTicker() {
  Serial.println("\n\n========== YAMAHA TICKER ===========\n");
  Serial.print("Intervall: alle "); Serial.print(yamaha.getTickInterval()); Serial.println("ms");
  Serial.print("Power:  "); Serial.println((yamaha.getPowerStatus()) ? "An":"Aus");
  Serial.print("Volume: "); Serial.print(yamaha.getVolume()/10); Serial.println("dB");
  String src = yamaha.getSource();
  Serial.print("Source: "); Serial.println(src);
  if (src == "NET RADIO") {
    NetRadioTrackInfo nri = yamaha.readCurrentlyPlayingNetRadio();
    Serial.print("\tStation: "); Serial.println(nri.station);
    Serial.print("\tSong   : "); Serial.println(nri.song);
    Serial.print("\tElapsed: "); Serial.println(nri.elapsed);
  }
}

void showBeamerTicker() {
  Serial.println("\n\n========== BEAMER TICKER ===========\n");
  Serial.print("Intervall: alle "); Serial.print(beamer.getTickInterval()); Serial.println("ms");
  Serial.print("Power:  "); Serial.println((beamer.getPowerStatus()) ? "An" : "Aus");
  Serial.print("Source: "); Serial.println(beamer.getSourceString());
  Serial.print("Lampe:  "); Serial.println(beamer.getLampHours());
  Serial.println("\n\n");
}

void showCanvasTicker() {
  Serial.println("\n\n========== LEINWAND TICKER ===========\n");
  Serial.print("Intervall: alle "); Serial.print(canvas.getTickInterval()); Serial.println("ms");
  Serial.print("Power:      "); Serial.println((canvas.getPowerStatus()) ? "An" : "Aus");
  Serial.print("Brightness: "); Serial.println(canvas.getBrightness());
  Serial.print("Live Data:  "); Serial.print((canvas.isReceivingLiveData()) ? "incoming, " : "none, "); Serial.println((canvas.isOverridingLiveData()) ? "ignoriert" : "bearbeitet");
  Serial.print("LD Source:  "); Serial.println(canvas.getLiveSource());
  Serial.print("Effekt:     "); Serial.println(canvas.getEffect());
  Serial.print("  Speed:    "); Serial.println(canvas.getSpeed());
}

void showSoundTicker() {
  Serial.println("\n\n========== SOUND TICKER ===========\n");
  Serial.print("Intervall: alle "); Serial.print(sound.getTickInterval()); Serial.println("ms");
  Serial.print("Power:      "); Serial.println((sound.getPowerStatus()) ? "An" : "Aus");
  Serial.print("Brightness: "); Serial.println(sound.getBrightness());
  Serial.print("Effekt:     "); Serial.println(sound.getEffect());
  Serial.print("  Speed:    "); Serial.println(canvas.getSpeed());
}

void showHyperionTicker() {
  Serial.println("\n\n========== HYPERION TICKER ===========\n");
  Serial.print("Intervall: alle "); Serial.print(hyperion.getTickInterval()); Serial.println("ms");
  Serial.print("Power      : ");
  Serial.println(hyperion.getPowerStatus() ? "An":"Aus");
  Serial.print("LED Instanz: ");
  Serial.println(hyperion.getLedDeviceStatus() ? "An":"Aus");
  Serial.print("Broadcasting: ");
  Serial.println(hyperion.isBroadcasting() ? "Ja":"Nein");
}

void showHueTicker() {
  Serial.println("\n\n========== HUE TICKER ===========\n");
  Serial.print("Intervall: alle "); Serial.print(hue.getTickInterval()); Serial.println("ms");
  auto& scenes = hue.getScenes();
  Serial.print(scenes.size()); Serial.println(" Szenen");
  auto& groups = hue.getGroups();
  Serial.print(groups.size()); Serial.println(" Gruppen");
  auto& lights = hue.getLights();
  Serial.print(lights.size()); Serial.println(" Lampen");
  for (auto& l : lights) {
    Serial.print("\t"); Serial.print(l->getName()); Serial.println(" : ");
    Serial.print("\t\tOn:  "); Serial.println(l->isOn() ? "true" : "false");
    if (l->isDimmable()) {
      Serial.print("\t\tBri: "); Serial.println(l->getBrightness());
    }
    if (l->hasCTColor()) {
      Serial.print("\t\tCT  :"); Serial.println(l->getCT());
    }
  }
  Serial.println("Sensoren (nur die Wichtigsten):");
  HueSensor* sensor = hue.getSensorByName("Temp Sensor Theke");
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
  }
  sensor = hue.getSensorByName("Licht Sensor Theke");
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
  }
}

void testBasicStatus() {
    // Basic Status
    Serial.println("--- GET BASIC STATUS ---");
    if (!yamaha.getStatus()) {
      Serial.println("Konnte Basic Status nicht lesen!");
    } else {
      bool power = yamaha.getPowerStatus();
      Serial.print("Power:  "); Serial.println(power ? "An":"Aus");
      if (!power) {
        Serial.print("Schalte ein: ");
        Serial.println(yamaha.setPower(true) ? "OK" : "Fehler");
        Serial.print("Setze Lautstärke -60: ");
        Serial.println(yamaha.setVolume(-600) ? "OK" : "Fehler");
        Serial.print("starte NET RADIO: ");
        Serial.println(yamaha.setSource("NET RADIO") ? "OK" : "Fehler");
      }
      Serial.println("Lese Status:");
      Serial.print("Volume: "); Serial.println(yamaha.getVolume());
      Serial.print("Treble: "); Serial.println(yamaha.getTreble());
      Serial.print("Bass: ");   Serial.println(yamaha.getBass());
      Serial.print("Mute:   "); Serial.println(yamaha.getMute() ? "An":"Aus");
      Serial.print("SW Trim:"); Serial.println(yamaha.getSubTrim());
      Serial.print("Source: "); Serial.println(yamaha.getSource());
      Serial.print("Straight:"); Serial.println(yamaha.getStraight() ? "An":"Aus");
      Serial.print("Enhancer:"); Serial.println(yamaha.getEnhancer() ? "An":"Aus");
      Serial.print("SoundPrg:"); Serial.println(yamaha.getSoundProgram());
    }
}

void testReadNetRadioFavorites() {
    Serial.println("\n\nLade NET RADIO Favoriten...");
    std::vector<String> favs = yamaha.readNetRadioFavorites();
    Serial.println("NET RADIO Favoriten:");
    for(String s : favs) {
      Serial.println(s);
    }
}

void testSetNetRadioFavorite() {
    Serial.println("\n\nWähle BOB aus: ");
    if (yamaha.selectNetRadioFavorite("bob")) {
        Serial.println("Sender ausgewählt!");
    } else {
        Serial.println("Sender nicht gefunden.");
    }
}

void testReadNetRadioPlayInfo() {
    auto info = yamaha.readCurrentlyPlayingNetRadio();
    Serial.println("Aktueller NET RADIO Track:");
    Serial.println("Station: " + info.station);
    Serial.println("Song:    " + info.song);
    Serial.println("Album:   " + info.album);
    Serial.println("Elapsed: " + info.elapsed);
    Serial.println("Art:     " + info.albumArt);
}

void testReadDsps() {
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
}

void testReadInputSources() {
    Serial.println("\n\nLese alle Sources aus");
    yamaha.readInputSources();
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
}

void testAlexaVolume(int tvol) {
  Serial.print("\n\nSetze Lautsärke im Alexa-Style: ");
  Serial.println((yamaha.setVolumeAlexa(tvol)) ? " OK" : " Fehler");
}

// BEAMER
void BeamerTestStatus() {
  beamer.begin();
  Serial.println("Status:");
  Serial.print("Power: "); Serial.println((beamer.getPowerStatus() ? "An" : "Aus"));
  Serial.print("Source: "); Serial.println(beamer.getSourceString());
  Serial.print("LampHours: "); Serial.println(beamer.getLampHours());
  Serial.println();
}


// WLED
void canvasTestStatus() {
  canvas.begin();
  Serial.println("Status:");
  Serial.print("Power:      "); Serial.println((canvas.getPowerStatus()) ? "An" : "Aus");
  Serial.print("Brightness: "); Serial.println(canvas.getBrightness());
  Serial.print("Live Data:  "); Serial.print((canvas.isReceivingLiveData()) ? "incoming, " : "none, "); Serial.println((canvas.isOverridingLiveData()) ? "ignoriert" : "bearbeitet");
  Serial.print("Effekt:     "); Serial.println(canvas.getEffect());
  Serial.print("  Speed:    "); Serial.println(canvas.getSpeed());
}
