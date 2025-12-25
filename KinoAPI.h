#pragma once
#include <Arduino.h>
#include "KinoMacroEngine.h"

namespace KinoAPI {
  // Macros
  bool handleMacroTicks();
  bool executeMacro(const String& name);
  bool loadMacroJson(const String& json);
  size_t getMacroErrorCount();
  const MacroError& getMacroError(size_t i);
  // Yamaha
  bool yamaha_init();
  bool yamaha_setTicker(int ms);
  bool yamaha_setPower(bool onoff);
  bool yamaha_powerOn();
  bool yamaha_powerOff();
  bool yamaha_setVolume(int vol);
  bool yamaha_setVolumePercent(int percentage);
  bool yamaha_setInput(const String& inputName);
  bool yamaha_setDsp(const String& dspName);
  bool yamaha_setStraight(bool onoff);
  bool yamaha_straightOn();
  bool yamaha_straightOff();
  bool yamaha_setEnhancer(bool onoff);
  bool yamaha_enhancerOn();
  bool yamaha_enhancerOff();
  bool yamaha_setMute(bool onoff);
  bool yamaha_muteOn();
  bool yamaha_muteOff();
  bool yamaha_setStation(const String& stationname);
  // Beamer
  bool beamer_setPower(bool onoff);
  bool beamer_setTicker(int ms);
  bool beamer_powerOn();
  bool beamer_powerOff();
  bool beamer_setInput(const String& inputname);
  // Leinwand
  bool canvas_setTicker(int ms);
  bool canvas_setPower(bool onoff, bool commit=false);
  bool canvas_powerOn();
  bool canvas_powerOff();
  bool canvas_setBrightness(uint8_t bri, bool commit=false);
  bool canvas_setEffect(uint16_t effectId, bool commit=false);
  bool canvas_setSpeed(uint8_t sx, bool commit=false);
  bool canvas_setIntensity(uint8_t ix, bool commit=false);
  bool canvas_setLive(bool onoff, bool commit=false);
  bool canvas_setTransitionTime(int tt, bool commit=false);
  bool canvas_setFgColor(uint8_t R, uint8_t G, uint8_t B, bool commit=false);
  bool canvas_setBgColor(uint8_t R, uint8_t G, uint8_t B, bool commit=false);
  bool canvas_setFxColor(uint8_t R, uint8_t G, uint8_t B, bool commit=false);
  bool canvas_setCustomParams(uint8_t c1x, uint8_t c2x, uint8_t c3x, bool commit=false);
  bool canvas_setPalette(uint8_t pal, bool commit=false);
  bool canvas_fadeSync(uint8_t bri, int ms);
  bool canvas_fadeAsync(uint8_t bri, int ms);
  bool canvas_setSolid(uint8_t r, uint8_t g, uint8_t b, uint8_t bri);
  bool canvas_setMusicEffect();
  bool canvas_setLightningEffect();
  bool canvas_setAlarm();
  bool canvas_stopAlarm();
  bool canvas_setPause();
  bool canvas_setResume();
  bool canvas_commit();
  // sound
  bool sound_setTicker(int ms);
  bool sound_setPower(bool onoff, bool commit=false);
  bool sound_powerOn();
  bool sound_powerOff();
  bool sound_setBrightness(uint8_t bri, bool commit=false);
  bool sound_setEffect(uint16_t effectId, bool commit=false);
  bool sound_setSpeed(uint8_t sx, bool commit=false);
  bool sound_setIntensity(uint8_t ix, bool commit=false);
  bool sound_setLive(bool onoff, bool commit=false);
  bool sound_setTransitionTime(int tt, bool commit=false);
  bool sound_setFgColor(uint8_t R, uint8_t G, uint8_t B, bool commit=false);
  bool sound_setBgColor(uint8_t R, uint8_t G, uint8_t B, bool commit=false);
  bool sound_setFxColor(uint8_t R, uint8_t G, uint8_t B, bool commit=false);
  bool sound_setCustomParams(uint8_t c1x, uint8_t c2x, uint8_t c3x, bool commit=false);
  bool sound_setPalette(uint8_t pal, bool commit=false);
  bool sound_setSolid(uint8_t r, uint8_t g, uint8_t b, uint8_t bri);
  bool sound_fadeSync(uint8_t bri, int ms);
  bool sound_fadeAsync(uint8_t bri, int ms);
  bool sound_setMusicEffect();
  bool sound_setLightningEffect();
  bool sound_setAlarm();
  bool sound_stopAlarm();
  bool sound_setPause();
  bool sound_setResume();
  bool sound_commit();
  // Alle WLEDs gleichzeitig
  bool wled_setMusicEffect();
  bool wled_setLightningEffect();
  // Hyperion
  bool hyperion_setTicker(int ms);
  bool hyperion_startBroadcast();
  bool hyperion_stopBroadcast();
  // Hue
  bool hue_init();
  bool hue_setTicker(int ms);
  bool hueLight_setPower(const String& lampName, bool onoff, bool commit=false);
  bool hueLight_powerOn(const String& lampName);
  bool hueLight_powerOff(const String& lampName);
  bool hueLight_setBri(const String& lampName, uint8_t bri, bool commit=false);
  bool hueLight_setCT(const String& lampName, uint16_t ct, bool commit=false);
  bool hueLight_setRGB(const String& lampName, uint8_t R, uint8_t G, uint8_t B, bool commit=false);
  bool hueLight_commit(const String& lampName);
  bool hueGroup_setPower(const String& groupname, bool onoff, bool commit=false);
  bool hueGroup_powerOn(const String& groupname);
  bool hueGroup_powerOff(const String& groupname);
  bool hueGroup_setBri(const String& groupname, uint8_t bri, bool commit=false);
  bool hueGroup_setCT(const String& groupname, uint16_t ct, bool commit=false);
  bool hueGroup_commit(const String& groupname);
  bool hueScene_set(const String& sceneName);
  bool hueScene_save(const String& sceneName);
  bool hueSensor_setState(const String& sensorName, const String& key, int val, bool commit=false);
  bool hueSensor_commit(const String& sensorName);
  
  // Makros
  // Hier nur die wichtigsten Makros, weitere sollten
  // per Json definiert werden
  bool radioMode();
  bool blurayMode();
  bool streamingMode();
  bool GamingMode();
  bool startPause();
  bool endPause();
  bool togglePause();
  bool Power(bool onoff);
  
}
