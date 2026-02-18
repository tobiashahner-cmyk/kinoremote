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
    enum DirtyBit : uint16_t {
      NONE  = 0,
      ON    = 1<<1,
      BRI   = 1<<2,
      FX    = 1<<3,
      SX    = 1<<4,
      IX    = 1<<5,
      FGCOL = 1<<6,
      BGCOL = 1<<7,
      FXCOL = 1<<8,
      LOR   = 1<<9,
      C1X   = 1<<10,
      C2X   = 1<<11,
      C3X   = 1<<12,
      PAL   = 1<<13
    };
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
    bool getStatusUpdate(const char* devName, JsonObject& root) override;

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
    char _chunkPath[64];
    
    static constexpr size_t CHUNK_SIZE = 1024;
    char   _chunkBuf[CHUNK_SIZE];
    size_t _chunkLen = 0;
    size_t _chunkPos = 0;
    
    bool   _chunkActive = false;
    int    _chunkLastIndex = -1;
    void   closeChunkStream();
    void   copyAndTrim(char* dest, const char* src, size_t srcLen, size_t destSize);
    
    bool readLineSmart(const char* path, int index, char* out, size_t outLen);
    static void stripAfterAt(char* s);
  
    IPAddress _ip;
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
    uint16_t _dirty = NONE;
    uint16_t _pendingDirty = NONE;
  
    void initFilter();
    bool readEffects(bool forceRefresh = false);
    bool readPalettes(bool forceRefresh= false);
    void effectsFile(char* filename, size_t filenameLen);
    void paletteFile(char* filename, size_t filenameLen);
    int countParams(size_t linenr);
    bool getParamLabel(size_t linenr, size_t paramnr, char* out, size_t outLen);
    bool getParamField(size_t linenr, size_t paramnr, char* out, size_t outLen);
    static StaticJsonDocument<512> _jsonFilter; 
};
