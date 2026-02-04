#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "KinoDevice.h"

struct WLEDColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct stateBackup {
  bool onoff;
  uint16_t fx;
  uint8_t bri;
  uint8_t sx;
  uint8_t ix;
  uint8_t pal;
  WLEDColor fgCol;
  WLEDColor bgCol;
  WLEDColor fxCol;
  uint8_t c1x;
  uint8_t c2x;
  uint8_t c3x;
};



class WLEDDevice : public KinoDevice {
  public:
    const char* deviceType() const override {
        return "wled";
    }
    
    // Konstruktoren
    explicit WLEDDevice(const IPAddress& ip);
    explicit WLEDDevice(const String& ip);

    KinoError get(const char* property, KinoVariant& out) override;
    KinoError set(const char* property, const KinoVariant& value) override;
    KinoError queryCount(const char* property, uint16_t& out) override;
    KinoError query(const char* property, uint16_t index, KinoVariant &out) override;
    KinoError init() override;                                      // wie begin(), nur semantisch anders ;-)
    bool needsCommit() override;
    bool commit() override;
    size_t getPropertyCount() const override;
    const KinoPropertyInfo* getPropertyInfo(size_t index) const override;

    KinoError getEffectMetadata(int effectnr, KinoVariant& out);
    KinoError getEffectParamName(const KinoVariant& in, int paramIndex, KinoVariant& out);
    std::vector<KinoPropertyParam> getPaletteParams(int palnr);
    KinoPropertyParam* getPaletteParam(int palnr, int paramIndex);
  
    // Lifecycle
    bool begin();
    bool getStatus();
    KinoError tick();                                      // zum regelmässigen Auslesen des aktuellen Status. Ist true, wenn ausgeführt, sonst false
    bool setTickInterval(int ms);
    int getTickInterval();
  
  
    // Getter
    bool getPowerStatus() const;
    uint8_t getBrightness() const;
    bool isReceivingLiveData() const;
    char* isReceivingFrom() const;
    bool isOverridingLiveData() const;
    uint16_t getEffect() const;
    uint8_t getSpeed() const;
    uint8_t getIntensity() const;
    String getLiveSource() const;
    void getLiveSource(char* src, size_t srcLen);
    uint8_t getPalette() const;
    bool inAlarm() const;
    bool inPause() const;
    WLEDColor getColFg() const;
    WLEDColor getColBg() const;
    WLEDColor getColFx() const;
  
    // Setter
    bool setPowerStatus(bool onoff);
    bool setBrightness(uint8_t bri);
    bool setTransitionTime(int tt);
    bool fade(uint8_t newbri, int durationMs);
    bool setEffect(uint16_t effect);
    bool setSpeed(uint8_t speed);
    bool setIntensity(uint8_t intensity);
    bool setFgColor(uint8_t R, uint8_t G, uint8_t B);
    bool setFgColor(WLEDColor col);
    bool setBgColor(uint8_t R, uint8_t G, uint8_t B);
    bool setBgColor(WLEDColor col);
    bool setFxColor(uint8_t R, uint8_t G, uint8_t B);
    bool setFxColor(WLEDColor col);
    bool setCustom(uint8_t c1x, uint8_t c2x, uint8_t c3x);
    bool setCustom1(uint8_t c1x);
    bool setCustom2(uint8_t c2x);
    bool setCustom3(uint8_t c3x);
    bool setPalette(uint8_t pal);
    bool setLive(bool onoff);
    bool setAlarm(bool onoff);
    bool setPause(bool onoff);
    bool backupState();
    bool restoreBackup();
    bool applyChanges();
  
  private:
    // ===== Lazy Streaming (pro Instanz) =====
    File _lazyFile;
    String _lazyPath;
    int _lazyLastIndex = -1;
    bool _lazyActive = false;
    // Chunk-Streaming Cache
    File   _chunkFile;
    //String _chunkPath;
    char _chunkPath[64];
    
    static constexpr size_t CHUNK_SIZE = 1024;
    char   _chunkBuf[CHUNK_SIZE];
    size_t _chunkLen = 0;
    size_t _chunkPos = 0;
    
    bool   _chunkActive = false;
    int    _chunkLastIndex = -1;
    void   closeChunkStream();
    void   copyAndTrim(char* dest, const char* src, size_t srcLen, size_t destSize);
    
    void closeLazyStream();
    bool ensureLazyStream(const String& path, int index);
    //bool readLineSmart(const String& path, int index, String& out);
    bool readLineSmart(const char* path, int index, char* out, size_t outLen);
    static void stripAfterAt(String& s);
    static void stripAfterAt(char* s);
  
    IPAddress _ip;
    //WiFiClient& _client;
    int  _tickInterval  = 0;
    unsigned long _lastTick = 0;
    void EnsureTimeoutBeforeRequest(unsigned long timeout);
    static const KinoPropertyInfo _properties[];
    // Status
    StaticJsonDocument<1024> _props;
    StaticJsonDocument<1024> _newProps;
    bool readState();
    stateBackup _bkp;
    bool _alarm = false;
    bool _pause = false;
  
    void initFilter();
    bool readEffects(bool forceRefresh = false);
    bool readPalettes(bool forceRefresh= false);
    String effectsFile();
    void effectsFile(char* filename, size_t filenameLen);
    String paletteFile();
    void paletteFile(char* filename, size_t filenameLen);
    int countParams(size_t linenr);
    bool getParamLabel(size_t linenr, size_t paramnr, String& out);
    bool getParamLabel(size_t linenr, size_t paramnr, char* out, size_t outLen);
    bool getParamField(size_t linenr, size_t paramnr, String& out);
    bool getParamField(size_t linenr, size_t paramnr, char* out, size_t outLen);
    static StaticJsonDocument<512> _jsonFilter; 
};
