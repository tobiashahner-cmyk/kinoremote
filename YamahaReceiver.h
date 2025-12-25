#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <vector>

struct NetRadioTrackInfo {
    String elapsed;                                   // aktuelle Spielzeit des Streams
    String station;                                   // Sendername
    String album;                                     // Album (meist leer)
    String song;                                      // Songtitel (wie auf Radio angezeigt)
    String albumArt;                                  // Thumbnail des Senders
};

/*
struct InputSource {
  String key = "";                                    // intern benutzter Identifier
  String internal = "";                               // echter benutzter Name der Quelle, z.B. "HDMI1". Wird als Name zum Umschalten benutzt und wird als aktuelle Quelle ausgelesen
  String custom = "";                                 // benutzerdefinierter Name der Quelle, wie er auf dem internen Display des Yamaha angezeigt wird
  bool skip = false;                                  // skip=true heisst "nicht auswählbar". Liste kann in den Einstellungen des Receivers (des echten Geräts!) bearbeitet werden
};
*/
struct InputSource {
  const __FlashStringHelper* key;      // Liegt im Flash (0 Byte RAM)
  const __FlashStringHelper* internal; // Liegt im Flash (0 Byte RAM)
  String custom;                       // Nur der benutzerdefinierte Name belegt RAM
  bool skip;
};



class YamahaReceiver {
  public:
    YamahaReceiver(IPAddress ip);                     // Konstruktor
    YamahaReceiver(const String& ip);                 // Konstruktor mit IP als String
    IPAddress getIp() const;
    bool begin();
    bool init();
    bool getPowerStatus() const;
    bool setPower(bool on);
    int  getVolume() const;                           // liefert aktuelle Lautstärke für Main_Zone, als 3stelligen int. z.B.: -35.5dB ist -355
    bool setVolume(int vol);                          // setzt gewünschte Lautstärke für Main_Zone. Param als 3stelligen int. z.B. -420 für -42.0dB. -800 < param < -200
    bool setVolumeAlexa(int vol);                     // übersetzt gewählte Lautstärke von 0 < vol < 255 in gültige Lautstärke -800 < vol < -200 und setzt diese
    bool setVolumePercent(int vol);                   // übersetzt gewählte LS von 0 < vol < 100 in gültige LS -800 < vol < -200 und setzt diese
    int  getTreble() const;                           // liefert aktuelle Treble- Einstellung von -60 bis +60, bedeutend -6.0dB bis +6.0dB
    bool setTreble(int treb);                         // setzt neue Treble. Param -60 bis +60 setzt -6.0dB bis +6.0dB
    int  getBass() const;                             // liefert aktuelle Bass-Einstellung von -60 bis +60, entsprechend -6.0dB bis +6.0dB
    bool setBass(int bas);                            // setzt neuen Bass. Param -60 bis +60 setzt -6.0dB bis +6.0dB
    int  getSubTrim() const;                          // liefert aktuellen Subwoofer Trim von -60 bis +60, entsprechend -6.0dB bis +6.0dB
    bool setSubwooferTrim(int val);                   // setzt neuen Subwoofer Trim. Param -60 bis +60 setzt -6.0dB bis +6.0dB
    bool getStraight() const;                         // liefert Einstellung für "Straight"
    bool setStraight(bool onoff);                     // schaltet "Straight" ein oder aus
    bool getEnhancer() const;                         // liefert Einstellung für "Enhancer"
    bool setEnhancer(bool onoff);                     // schaltet "Enhancer" ein oder aus
    String getSource() const;                         // liefert aktuelle Input-Quelle als String ("HDMI1", "HDMI2",..."NET RADIO", "AUX"...)
    InputSource getInputSource();                     // liefert aktuelle Input-Quelle als InputSource struct
    std::vector<InputSource> readInputSources();      // liefert eine Liste aller möglichen Input-Quellen, siehe oben
    bool setSource(const String& srcName);            // setzt Input-Quelle. Als srcName kann jeder InputSources.internal genutzt werden (solange InputSources.skip = false)
    String getSoundProgram() const;                   // liefert aktuellen DSP als String
    std::vector<String> readDspNames();               // liefert eine Liste aller verfügbaren DSPs
    bool setSoundProgram(const String& dspname);      // setzt den DSP
    bool getMute() const;                             // liefert aktuellen Stand des Mutings
    bool setMute(bool onoff);                         // schaltet Muting ein oder aus
    bool getStatus();                                 // liest den BasicStatus aus
    std::vector<String> readNetRadioFavorites();      // liefert eine Liste der NETRADIO- Favoriten als Strings
    bool selectNetRadioFavorite(const String& radioname); // wählt den übergebenen NETRADIO- Favoriten aus
    NetRadioTrackInfo readCurrentlyPlayingNetRadio(); // liefert Infos über den aktuellen NETRADIO Tracks
    bool tick();                                      // zum regelmässigen Auslesen des aktuellen Status. Ist true, wenn ausgeführt, sonst false
    bool setTickInterval(int ms);                     // setzt das Intervall für tick() in Millisekunden. Erlaubt: 0 oder 2000 bis unendlich
    int getTickInterval();                            // gibt das Intervall für tick() in Millisekunden zurück
    
  private:
    IPAddress _ip;
    WiFiClient _client;
    // ticker
    unsigned long _tickInterval  = 10000;
    unsigned long _lastTick = 0;
    // Status Cache
    bool _powerStatus    = false;
    int  _volume         = 0;
    int  _treble         = 0;
    int  _bass           = 0;
    int  _subwooferTrim  = 0;
    bool _straight       = false;
    bool _enhancer       = false;
    String _soundProgram = "";
    String _source       = "";
    bool _mute           = false;
    // Liste aller auswählbaren Sources
    std::vector<InputSource> _InputSources;
    bool _gotInputSources = false;

    void initInputSources();                                                                // init- Helper für InputSources
    InputSource* getInputSourceByKey(const String&keyname);
    String readNetRadioList();                                                              // Helper für readNetRadioFavorites()
    bool moveToFavorites();                                                                 // Helper für readNetRadioFavorites()
    // String sendXML ist nur noch wegen Abwärtskompatibilität hier. Das fliegt bald raus, sobald
    // alle internen Methoden auf sendXMLRequest umgebaut sind!
    String sendXML(const String& xml);                                                      // Helper für das Senden von XML an den Yamaha
    // Sendet den Request und lässt den Client am Header-Ende stehen
    bool sendXMLRequest(const String& xml, int len=0);
    int extractTagInt(const String& xml, const String& tag);                                // Helper für das Auslesen von Werten aus der Antwort vom Yamaha
    int extractTagInt(const String& xml, const String& parent, const String& child);        // Helper für das Auslesen von Werten aus der Antwort vom Yamaha
    String extractTagString(const String& xml, const String& tag);                          // Helper für das Auslesen von Werten aus der Antwort vom Yamaha
    String extractTagString(const String& xml, const String& parent, const String& child);  // Helper für das Auslesen von Werten aus der Antwort vom Yamaha
    bool isOk(const String& resp);                                                          // Helper für Fehlerbehandlung nach PUT-Anfragen
    bool executeSetCommand(const __FlashStringHelper* start, const String& val, const __FlashStringHelper* end);
    

    // ----------------------------------------------------
    // XML Templates in PROGMEM
    // ----------------------------------------------------
    static const char XML_HEADER[] PROGMEM;
    static const char XML_GET_STATUS[] PROGMEM;
    static const char XML_GET_INPUTNAMES[] PROGMEM;
    static const char XML_GET_INPUTSKIP[] PROGMEM;
    static const char XML_GET_NETRADIO_LIST[] PROGMEM;
    static const char XML_SET_MOVEHOME[] PROGMEM;
    static const char XML_SET_SELECT_LINE_ONE[] PROGMEM;
    static const char XML_SET_SELECT_LINENR_START[] PROGMEM;
    static const char XML_SET_SELECT_LINENR_END[] PROGMEM;
    static const char XML_GET_DSP_SKIP[] PROGMEM;
    static const char XML_GET_NETRADIO_PLAYINFO[] PROGMEM;
    static const char XML_SET_POWER_START[] PROGMEM;
    static const char XML_SET_POWER_END[] PROGMEM;
    static const char XML_SET_VOLUME_START[] PROGMEM;
    static const char XML_SET_VOLUME_END[] PROGMEM;
    static const char XML_SET_MUTE_START[] PROGMEM;
    static const char XML_SET_MUTE_END[] PROGMEM;
    static const char XML_SET_TREBLE_START[] PROGMEM;
    static const char XML_SET_TREBLE_END[] PROGMEM;
    static const char XML_SET_BASS_START[] PROGMEM;
    static const char XML_SET_BASS_END[] PROGMEM;
    static const char XML_SET_SWTRIM_START[] PROGMEM;
    static const char XML_SET_SWTRIM_END[] PROGMEM;
    static const char XML_SET_SOURCE_START[] PROGMEM;
    static const char XML_SET_SOURCE_END[] PROGMEM;
    static const char XML_SET_STRAIGHT_START[] PROGMEM;
    static const char XML_SET_STRAIGHT_END[] PROGMEM;
    static const char XML_SET_ENHANCER_START[] PROGMEM;
    static const char XML_SET_ENHANCER_END[] PROGMEM;
    static const char XML_SET_DSP_START[] PROGMEM;
    static const char XML_SET_DSP_END[] PROGMEM;
};
