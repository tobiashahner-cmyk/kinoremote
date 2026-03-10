#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <vector>
#include "KinoDevice.h"

#define MAX_STATIONS 20
#define MAX_DSP 20
#define INPUTNAME_MAXLEN 20

struct NetRadioTrackInfo {
  char elapsed[20];
  char station[32];
  char song[64];
  unsigned long created;
  // char album[32];
  // char albumArt[128];
};

/*
struct InputSource {
  String key = "";                                    // intern benutzter Identifier
  String internal = "";                               // echter benutzter Name der Quelle, z.B. "HDMI1". Wird als Name zum Umschalten benutzt und wird als aktuelle Quelle ausgelesen
  String custom = "";                                 // benutzerdefinierter Name der Quelle, wie er auf dem internen Display des Yamaha angezeigt wird
  bool skip = false;                                  // skip=true heisst "nicht auswählbar". Liste kann in den Einstellungen des Receivers (des echten Geräts!) bearbeitet werden
};
*/

/* InputSource  V2  2026-02-16  Verzicht auf String, Konstruktor hinzugefügt (nötig wegen char) */
struct InputSource {
  const __FlashStringHelper* key;      // Liegt im Flash (0 Byte RAM)
  const __FlashStringHelper* internal; // Liegt im Flash (0 Byte RAM)
  char custom[INPUTNAME_MAXLEN];       // Nur der benutzerdefinierte Name belegt RAM
  bool skip;
  InputSource(const __FlashStringHelper* k, const __FlashStringHelper* i, bool s) 
    : key(k), internal(i), skip(s) {
    custom[0] = '\0'; // Initialisiere den Namen als leer
  }
};



class YamahaReceiver : public KinoDevice {
  public:
    enum DirtyBit : uint16_t {
      NONE      = 0,
      POWER     = 1 << 0,  // 00000001
      VOLUME    = 1 << 1,  // 00000010
      MUTE      = 1 << 2,  // 00000100
      SOURCE    = 1 << 3,
      DSP       = 1 << 4,
      STRAIGHT  = 1 << 5,
      ENHANCER  = 1 << 6,
      BASS      = 1 << 7,
      TREBLE    = 1 << 8,
      SWTRIM    = 1 << 9,
      TRACK     = 1 << 10, // Für Station/Song/Elapsed
      ALL       = 0xFFFF   // Alles dirty (z.B. für ersten Connect)
    };

    const char* deviceType() const override {
        return "yamahareceiver";
    }
    YamahaReceiver(IPAddress ip);                     // Konstruktor
    YamahaReceiver(const String& ip);                 // Konstruktor mit IP als String
    KinoError get(const char* property, KinoVariant& out) override;
    KinoError set(const char* property, const KinoVariant& value) override;
    KinoError queryCount(const char* property, uint16_t& out) override;
    KinoError query(const char* property, uint16_t index, KinoVariant& out) override;
    size_t getPropertyCount() const override;
    const KinoPropertyInfo* getPropertyInfo(size_t index) const override;
    bool OptionalAvailable(const char* propId) override;

    IPAddress getIp() const;
    bool begin();
    KinoError init();
    bool setPower(bool on);
    bool setVolume(int vol);                          // setzt gewünschte Lautstärke für Main_Zone. Param als 3stelligen int. z.B. -420 für -42.0dB. -800 < param < -200
    bool setVolumeAlexa(int vol);                     // übersetzt gewählte Lautstärke von 0 < vol < 255 in gültige Lautstärke -800 < vol < -200 und setzt diese
    bool setVolumePercent(int vol);                   // übersetzt gewählte LS von 0 < vol < 100 in gültige LS -800 < vol < -200 und setzt diese
    bool setTreble(int treb);                         // setzt neue Treble. Param -60 bis +60 setzt -6.0dB bis +6.0dB
    bool setBass(int bas);                            // setzt neuen Bass. Param -60 bis +60 setzt -6.0dB bis +6.0dB
    bool setSubwooferTrim(int val);                   // setzt neuen Subwoofer Trim. Param -60 bis +60 setzt -6.0dB bis +6.0dB
    bool setStraight(bool onoff);                     // schaltet "Straight" ein oder aus
    bool setEnhancer(bool onoff);                     // schaltet "Enhancer" ein oder aus                         
    
    InputSource* getInputSource();                    // liefert aktuelle Input-Quelle als InputSource struct
    bool readInputSources();                          // liest alle verfügbaren InputSources aus dem Receiver und speichert sie in _InputSources
                
    bool setSource(const char* srcName);              // setzt Input-Quelle. Als srcName kann jeder InputSources.internal genutzt werden (solange InputSources.skip = false)
    const char* getSoundProgram() const;              // liefert aktuellen DSP als char array
    bool setSoundProgram(const char* dspname);        // setzt den DSP
    bool getMute() const;                             // liefert aktuellen Stand des Mutings
    bool setMute(bool onoff);                         // schaltet Muting ein oder aus
    bool getStatus();                                 // liest den BasicStatus aus
    bool readNetRadioFavorites(bool reload=false);    // liest die Liste der NETRADIO- Favoriten in _stations ein
    bool getNetRadioFavorite(size_t index, char* buf, int buflen); // gibt NETRADIO- Favoriten Nr index in buf zurück
    bool selectNetRadioFavorite(const char* radioname); // wählt den übergebenen NETRADIO- Favoriten aus (checkt die ersten 10 Zeichen des Namens)
    NetRadioTrackInfo readCurrentlyPlayingNetRadio(); // liefert Infos über den aktuellen NETRADIO Tracks
    KinoError tick();                                      // zum regelmässigen Auslesen des aktuellen Status. Ist true, wenn ausgeführt, sonst false
    //bool setTickInterval(int ms);                     // setzt das Intervall für tick() in Millisekunden. Erlaubt: 0 oder 2000 bis unendlich
    bool isDirty();
    void clearDirty();
    bool getStatusUpdate(const char* devName, JsonObject& root);
  private:
    uint16_t _dirty = NONE;
    IPAddress _ip;
    static const KinoPropertyInfo _props[];
    // ticker
    //unsigned long _tickInterval  = 0;
    //unsigned long _lastTick = 0;
    // Status Cache
    bool _powerStatus    = false;
    int  _volume         = 0;
    int  _treble         = 0;
    int  _bass           = 0;
    int  _subwooferTrim  = 0;
    bool _straight       = false;
    bool _enhancer       = false;
    char _soundProgram[32];
    char _source[INPUTNAME_MAXLEN] = {};
    bool _mute           = false;

    void cleanSong(char* title);
    
    // Liste aller auswählbaren Sources
    std::vector<InputSource> _InputSources;
    bool _gotInputSources = false;
    // Liste aller verfügbaren NETRADIO Favoriten
    char _stations[MAX_STATIONS][48];
    size_t _stationCount = 0;
    
    char _dsps[MAX_DSP][32];
    size_t _dspCount;
    bool readDspNames(bool reload=false);             // liest alle verfügbaren DSP in _dsps
    void sanitizeDspName(char* dspname);              // Helperfunktion für readDspNames: ersetzt "_" durch " " und entfernt Leerzeichen
    bool getDspName(size_t index, char* buf, size_t bufLen);

    static const KinoPropertyParam _AudioParams[];
    size_t getAudioParamCount();
    const KinoPropertyParam* getAudioParam(size_t index);
    
    static const KinoPropertyParam _InputParams[];
    size_t getInputParamCount(const char* inp);
    const KinoPropertyParam* getInputParam(const char* inp, size_t index);
    
    static const KinoPropertyParam _DspParams[];
    size_t getDspParamCount(const char* dspname);
    const KinoPropertyParam* getDspParam(const char* dspname, size_t index);

    // getStatus()-helper:
    bool readIsOn(Stream& s);
    int readIntUntil(Stream& s);
    size_t readSanitizedUntil(Stream& s, char terminator, char* buffer, size_t maxLen);
    
    void initInputSources();                                                                // init- Helper für InputSources
    
    InputSource* getInputSourceByKey(const char* keyname);
                                                         
    bool waitForNetRadioList(WiFiClient& client, HTTPClient& http, bool keepalive);     // Helper für readNetRadioFavorites()
    bool moveToFavorites(WiFiClient& client, HTTPClient& http);                         // Helper für readNetRadioFavorites()
    bool moveToNextPage(WiFiClient& wifi, HTTPClient& http);
    //bool sendXMLRequest(WiFiClient& client, const __FlashStringHelper* xml);
    //bool sendXMLRequest(WiFiClient& client, const char* xml);
    bool sendXMLRequest(WiFiClient& wifi, HTTPClient& http, const __FlashStringHelper* xml);
    bool sendXMLRequest(WiFiClient& wifi, HTTPClient& http, const char* xml);
    bool executeSetCommand(WiFiClient& wifi, HTTPClient& http, const __FlashStringHelper* xmlstart, const char* val, const __FlashStringHelper* xmlend);
    bool executeSetCommand(const __FlashStringHelper* xmlstart, const char* val, const __FlashStringHelper* xmlend);
    bool executeSetCommand(WiFiClient& wifi, HTTPClient& http, const char* xmlstart, const char* val, const char* xmlend);
    bool executeSetCommand(const char* xmlstart, const char* val, const char* xmlend);
    bool executeSetCommand(WiFiClient& wifi, HTTPClient& http, const char* xmlstart, int val, const char* xmlend);
    bool executeSetCommand(const char* xmlstart, int val, const char* xmlend);
    void EnsureDelayBeforeRequest(unsigned long timeout);

    static char _url[56];
    static char _body[256];
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
    static const char XML_SELECT_NEXT_PAGE[] PROGMEM;
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
