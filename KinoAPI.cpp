#include "KinoAPI.h"
#include "YamahaReceiver.h"
#include "WLEDDevice.h"
#include "OptomaBeamer.h"
#include "HyperionDevice.h"
#include "HueBridge.h"
#include "KinoMacroEngine.h"

// Externe Geräte aus dem Sketch
extern YamahaReceiver* _yamaha;
extern WLEDDevice* _canvas;
extern WLEDDevice* _sound;
extern OptomaBeamer* _beamer;
extern HyperionDevice* _hyperion;
extern HueBridge* _hue;



namespace KinoAPI {
  bool isPause = false;
  bool isAlarm = false;

  // =================================================
  //            MACROS
  // =================================================

  // Macros sind ein Teil der API, also ist das hier genau richtig aufgehoben
  static KinoMacroEngine macroEngine;

  bool startMacroEngine() {
    if (macroEngine.isReady()) return true;
    return macroEngine.begin();
  }

  std::vector<String> listMacros() {
    return macroEngine.listMacros();
  }

  bool getMacroLines(const String& macroName, std::vector<String>& lines) {
    return macroEngine.getMacroLines(macroName, lines);
  }

  bool addMacroCommand(const String& macroName, size_t index, const String& jsonActionElement) {
    return macroEngine.addCommand(macroName, index, jsonActionElement);
  }

  bool deleteMacroCommand(const String& macroName, size_t index) {
    return macroEngine.deleteCommand(macroName, index);
  }

  bool updateMacroCommand(const String& macroName, size_t index, const String& jsonActionElement) {
    return macroEngine.updateCommand(macroName, index, jsonActionElement);
  }
  
  bool executeMacro(const String& name,MacroFinishedCallback cb/*=nullptr*/) {
    return macroEngine.startMacro(name, cb);
  }

  String getCurrentMacroName() {
    return macroEngine.getName();
  }

  bool handleMacroTicks() {
    macroEngine.tick();
    return (macroEngine.errorCount() == 0);
  }

  bool addOrUpdateMacro(const String& json) {
    return macroEngine.addOrUpdateMacro(json);
  }

  bool deleteMacro(const String& macroName) {
    return macroEngine.deleteMacro(macroName);
  }

  size_t getMacroErrorCount() {
    return macroEngine.errorCount();
  }
  
  const MacroError& getMacroError(size_t i) {
    return macroEngine.getError(i);
  }
  
  // =================================================
  //            YAMAHA
  // =================================================
  bool yamaha_init() {
    return _yamaha->init();
  }

  bool yamaha_setTicker(int ms) {
    return _yamaha->setTickInterval(ms);
   /*KinoError e = _yamaha->set("tickInterval",KinoVariant::fromInt(ms));
   return (e == KinoError::OK);*/
  }
  
  bool yamaha_setPower(bool onoff) {
    if (_yamaha->getPowerStatus() == onoff) return true;
    return _yamaha->setPower(onoff);
    /*KinoVariant p;
    KinoError e = _yamaha->get("power",p);
    if (p == onoff) return true;
    e = _yamaha->set("power",KinoVariant::fromBool(onoff));
    return (e == KinoError::OK);*/
  }
  
  bool yamaha_powerOn() {
    if (_yamaha->getPowerStatus()) return true;
    return _yamaha->setPower(true);
    /*bool p;
    KinoError e = _yamaha->get("power",p);
    if (p) return true;
    e = _yamaha->set("power",KinoVariant::fromBool(true));
    return (e == KinoError::OK);*/
  }
  
  bool yamaha_powerOff() {
    if (!_yamaha->getPowerStatus()) return true;
    return _yamaha->setPower(false);
    /*bool p;
    KinoError e = _yamaha->get("power",p);
    if (!p) return true;
    e = _yamaha->set("power",KinoVariant::fromBool(false));
    return (e == KinoError::OK);*/
  }
  
  bool yamaha_setVolume(int vol) {
    return _yamaha->setVolume(vol);
    /*KinoError e = _yamaha->set("volume",KinoVariant::fromInt(vol));
    return (e == KinoError::OK);*/
  }
  
  bool yamaha_setVolumePercent(int percentage) {
    return _yamaha->setVolumePercent(percentage);
  }
  
  bool yamaha_setInput(const String& inputName) {
    if (_yamaha->getSource() == inputName) return true;
    return _yamaha->setSource(inputName);
  }
  
  bool yamaha_setDsp(const String& dspName) {
    if (_yamaha->getSoundProgram() == dspName) return true;
    return _yamaha->setSoundProgram(dspName);  
  }
  
  bool yamaha_setStraight(bool onoff) {
    if (_yamaha->getStraight() == onoff) return true;
    return _yamaha->setStraight(onoff);
  }

  bool yamaha_straightOn() {
    if (_yamaha->getStraight()) return true;
    return _yamaha->setStraight(true);
  }

  bool yamaha_straightOff() {
    if (!_yamaha->getStraight()) return true;
    return _yamaha->setStraight(false);
  }
  
  bool yamaha_setEnhancer(bool onoff) {
    if (_yamaha->getEnhancer() == onoff) return true;
    return _yamaha->setEnhancer(onoff);
  }

  bool yamaha_enhancerOn() {
    if (_yamaha->getEnhancer()) return true;
    return _yamaha->setEnhancer(true);
  }

  bool yamaha_enhancerOff() {
    if (!_yamaha->getEnhancer()) return true;
    return _yamaha->setEnhancer(false);
  }
  
  bool yamaha_setMute(bool onoff) {
    if (_yamaha->getMute() == onoff) return true;
    return _yamaha->setMute(onoff);
  }

  bool yamaha_muteOn() {
    if (_yamaha->getMute()) return true;
    return _yamaha->setMute(true);
  }

  bool yamaha_muteOff() {
    if (!_yamaha->getMute()) return true;
    return _yamaha->setMute(false);
  }

  bool yamaha_setStation(const String& stationname) {
    return _yamaha->selectNetRadioFavorite(stationname);
  }

  // =================================================
  //            BEAMER
  // =================================================
  bool beamer_setTicker(int ms) {
    return _beamer->setTickInterval(ms);
  }
  
  bool beamer_setPower(bool onoff) {
    if (_beamer->getPowerStatus() == onoff) return true;
    return _beamer->setPower(onoff);
  }
  
  bool beamer_powerOn() {
    if (_beamer->getPowerStatus()) return true;
    return _beamer->setPower(true);
  }
  
  bool beamer_powerOff() {
    if (!_beamer->getPowerStatus()) return true;
    return _beamer->setPower(false);
  }

  bool beamer_setInput(const String& inputname) {
    if(!_beamer->getPowerStatus()) _beamer->setPower(true);
    return _beamer->setSource(inputname);
  }

  // =================================================
  //            LEINWAND
  // =================================================
  bool canvas_setTicker(int ms) {
    return _canvas->setTickInterval(ms);
  }
  
  bool canvas_setPower(bool onoff, bool commit/*=false*/) {
    if (_canvas->getPowerStatus() == onoff) return true;
    bool ok = _canvas->setPowerStatus(onoff);
    if (ok && commit) return _canvas->applyChanges();
    return ok;
  }
  
  bool canvas_powerOn(bool commit/*=false*/) {
    if (_canvas->getPowerStatus()) return true;
    bool ok = _canvas->setPowerStatus(true);
    if (ok && commit) return _canvas->applyChanges();
    return ok;
  }
  
  bool canvas_powerOff(bool commit/*=false*/) {
    if (!_canvas->getPowerStatus()) return true;
    bool ok = _canvas->setPowerStatus(false);
    if (ok && commit) return _canvas->applyChanges();
    return ok;
  }
  
  bool canvas_setBrightness(uint8_t bri, bool commit/*=false*/) {
    if (_canvas->getBrightness() == bri) return true;
    bool ok = _canvas->setBrightness(bri);
    if (ok && commit) return _canvas->applyChanges();
    return ok;
  }
  
  bool canvas_setEffect(uint16_t effectId, bool commit/*=false*/) {
    if (_canvas->getEffect() == effectId) return true;
    bool ok = _canvas->setEffect(effectId);
    if (ok && commit) return _canvas->applyChanges();
    return ok;
  }
  
  bool canvas_setSpeed(uint8_t sx, bool commit/*=false*/) {
    if (_canvas->getSpeed() == sx) return true;
    bool ok = _canvas->setSpeed(sx);
    if (ok && commit) return _canvas->applyChanges();
    return ok;
  }
  
  bool canvas_setIntensity(uint8_t ix, bool commit/*=false*/) {
    if (_canvas->getIntensity() == ix) return true;
    bool ok = _canvas->setIntensity(ix);
    if (ok && commit) return _canvas->applyChanges();
    return ok;
  }
  
  bool canvas_setLive(bool onoff, bool commit/*=false*/) {
    //if (_canvas->isOverridingLiveData() == (!onoff)) return true;
    bool ok = _canvas->setLive(onoff);
    if (ok && commit) return _canvas->applyChanges();
    return ok;
  }
  
  bool canvas_setSolid(uint8_t r, uint8_t g, uint8_t b, uint8_t bri) {
    //return _canvas->setColor(r,g,b,bri);    
    _canvas->setFgColor(r,g,b);
    _canvas->setBrightness(bri);
    return _canvas->applyChanges();
  }
  
  bool canvas_setTransitionTime(int tt, bool commit/*=false*/) {
    bool ok = _canvas->setTransitionTime(tt);
    if (ok && commit) return _canvas->applyChanges();
    return ok;
  }
  
  bool canvas_fadeSync(uint8_t bri, int ms) {
    if (!_canvas->fade(bri, ms)) return false;
    unsigned long start = millis();
    unsigned long now = millis();
    while(now - start <= ms) {
      yield();
    }
    return true;
  }

  bool canvas_fadeAsync(uint8_t bri, int ms) {
    if (!_canvas->fade(bri, ms)) return false;
    return true;
  }
  
  bool canvas_setFgColor(uint8_t R, uint8_t G, uint8_t B, bool commit/*=false*/) {
    bool ok = _canvas->setFgColor(R,G,B);
    if (ok && commit) return _canvas->applyChanges();
    return ok;
  }
  
  bool canvas_setBgColor(uint8_t R, uint8_t G, uint8_t B, bool commit/*=false*/) {
    bool ok = _canvas->setBgColor(R,G,B);
    if (ok && commit) return _canvas->applyChanges();
    return ok;
  }
  
  bool canvas_setFxColor(uint8_t R, uint8_t G, uint8_t B, bool commit/*=false*/) {
    bool ok = _canvas->setFxColor(R,G,B);
    if (ok && commit) return _canvas->applyChanges();
    return ok;
  }
  
  bool canvas_setCustomParams(uint8_t c1x, uint8_t c2x, uint8_t c3x, bool commit/*=false*/) {
    bool ok = _canvas->setCustom(c1x, c2x, c3x);
    if (ok && commit) return _canvas->applyChanges();
    return ok;
  }
  
  bool canvas_setPalette(uint8_t pal, bool commit/*=false*/) {
    if (_canvas->getPalette() == pal) return true;
    bool ok = _canvas->setPalette(pal);
    if (ok && commit) return _canvas->applyChanges();
    return ok;
  }
  
  bool canvas_setMusicEffect() {
    //return _canvas->setMusicMode();
    if (_canvas->fade(0,200)) delay(200);
    _canvas->setPowerStatus(true);
    _canvas->setLive(false);
    _canvas->setBrightness(128);
    _canvas->setEffect(9);
    _canvas->setSpeed(64);
    _canvas->setIntensity(128);
    _canvas->setFgColor(255,0,0);
    _canvas->setBgColor(0,255,0);
    _canvas->setFxColor(0,0,255);
    _canvas->setTransitionTime(100);
    _canvas->setPalette(0);
    return _canvas->applyChanges();
  }
  
  bool canvas_setLightningEffect() {
    //return _canvas->setLightningMode();
    if (_canvas->fade(0,200)) delay(200);
    _canvas->setPowerStatus(true);
    _canvas->setLive(false);
    _canvas->setBrightness(200);
    _canvas->setEffect(57);
    _canvas->setSpeed(250);
    _canvas->setIntensity(128);
    _canvas->setFgColor(255,255,255);
    _canvas->setBgColor(0,0,0);
    _canvas->setFxColor(0,0,0);
    _canvas->setPalette(5);
    return _canvas->applyChanges();
  }
  
  bool canvas_setAlarm() {
    if (!_canvas->setAlarm(true)) return false;
    if (_canvas->fade(0,200)) delay(200);
    _canvas->setPowerStatus(true);
    _canvas->setLive(false);
    _canvas->setBrightness(200);
    _canvas->setEffect(11);
    _canvas->setSpeed(255);
    _canvas->setIntensity(255);
    _canvas->setFgColor(255,0,0);
    _canvas->setBgColor(160,160,160);
    _canvas->setFxColor(0,0,255);
    _canvas->setPalette(5);
    if (!_canvas->applyChanges()) {
      _canvas->setAlarm(false); // hier wird im schlimmsten Fall das Backup wiederhergestellt, obwohl sich seitdem der Status nicht geändert hat
      return false;
    }
    return true;
  }

  bool canvas_stopAlarm() {
    return _canvas->setAlarm(false); // _canvas->setAlarm kümmert sich um die interne Logik
  }

  bool canvas_setPause() {
    if (!_canvas->setPause(true)) return false;   // _canvas->setPause kümmert sich um das Backup, falls nötig
    
    if (_canvas->fade(0,200)) delay(200);
    _canvas->setPowerStatus(true);
    _canvas->setLive(false);
    _canvas->setBrightness(100);
    _canvas->setEffect(2);
    _canvas->setSpeed(100);
    _canvas->setIntensity(100);
    _canvas->setFgColor(255,0,0);
    _canvas->setBgColor(0,0,255);
    _canvas->setFxColor(0,0,0);
    _canvas->setPalette(5);
    if (!_canvas->applyChanges()) {
      _canvas->setPause(false);   // hier wird im schlimmsten Fall ein bereits wiederhergestelltes Backup nochmal wiederhergestellt
      return false;
    }
    return true;
  }

  bool canvas_setResume() {
    return _canvas->setPause(false); // setPause kümmert sich um das Wiederherstellen des Backups
  }
  
  bool canvas_commit() {
    return _canvas->applyChanges();
  }

  // =================================================
  //            SOUND LED
  // =================================================

  bool sound_setTicker(int ms) {
    return _sound->setTickInterval(ms);
  }
  
  bool sound_setPower(bool onoff, bool commit/*=false*/) {
    if (_sound->getPowerStatus() == onoff) return true;
    bool ok = _sound->setPowerStatus(onoff);
    if (ok && commit) return _sound->applyChanges();
    return ok;
  }
  
  bool sound_powerOn(bool commit/*=false*/) {
    if (_sound->getPowerStatus()) return true;
    bool ok = _sound->setPowerStatus(true);
    if (ok && commit) return _sound->applyChanges();
    return ok;
  }
  
  bool sound_powerOff(bool commit/*=false*/) {
    if (!_sound->getPowerStatus()) return true;
    bool ok = _sound->setPowerStatus(false);
    if (ok && commit) return _sound->applyChanges();
    return ok;
  }
  
  bool sound_setBrightness(uint8_t bri, bool commit/*=false*/) {
    bool ok = _sound->setBrightness(bri);
    if (ok && commit) return _sound->applyChanges();
    return ok;
  }
  
  bool sound_setEffect(uint16_t effectId, bool commit/*=false*/) {
    bool ok = _sound->setEffect(effectId);
    if (ok && commit) return _sound->applyChanges();
    return ok;
  }
  
  bool sound_setSpeed(uint8_t sx, bool commit/*=false*/) {
    bool ok = _sound->setSpeed(sx);
    if (ok && commit) return _sound->applyChanges();
    return ok;
  }
  
  bool sound_setIntensity(uint8_t ix, bool commit/*=false*/) {
    bool ok = _sound->setIntensity(ix);
    if (ok && commit) return _sound->applyChanges();
    return ok;
  }

  bool sound_setLive(bool onoff, bool commit/*=false*/) {
    return true;
  }
  
  bool sound_setSolid(uint8_t r, uint8_t g, uint8_t b, uint8_t bri) {
    //return _sound->setColor(r,g,b,bri);
    _sound->setFgColor(r,g,b);
    _sound->setBrightness(bri);
    _sound->setEffect(0);
    return _sound->applyChanges();
  }
  
  bool sound_setTransitionTime(int tt, bool commit/*=false*/) {
    bool ok = _sound->setTransitionTime((int)(tt/100));
    if (ok && commit) return _sound->applyChanges();
    return ok;
  }

  bool sound_fadeSync(uint8_t bri, int ms) {
    if (!_sound->fade(bri, ms)) return false;
    unsigned long start = millis();
    unsigned long now = millis();
    while(now - start <= ms) {
      yield();
    }
    return true;
  }

  bool sound_fadeAsync(uint8_t bri, int ms) {
    if (!_sound->fade(bri, ms)) return false;
    return true;
  }
  
  bool sound_setFgColor(uint8_t R, uint8_t G, uint8_t B, bool commit/*=false*/) {
    bool ok = _sound->setFgColor(R,G,B);
    if (ok && commit) return _sound->applyChanges();
    return ok;
  }
  
  bool sound_setBgColor(uint8_t R, uint8_t G, uint8_t B, bool commit/*=false*/) {
    bool ok = _sound->setBgColor(R,G,B);
    if (ok && commit) return _sound->applyChanges();
    return ok;
  }
  
  bool sound_setFxColor(uint8_t R, uint8_t G, uint8_t B, bool commit/*=false*/) {
    bool ok = _sound->setFxColor(R,G,B);
    if (ok && commit) return _sound->applyChanges();
    return ok;
  }
  
  bool sound_setCustomParams(uint8_t c1x, uint8_t c2x, uint8_t c3x, bool commit/*=false*/) {
    bool ok = _sound->setCustom(c1x, c2x, c3x);
    if (ok && commit) return _sound->applyChanges();
    return ok;
  }
  
  bool sound_setPalette(uint8_t pal, bool commit/*=false*/) {
    if (_sound->getPalette() == pal) return true;
    bool ok = _sound->setPalette(pal);
    if (ok && commit) return _sound->applyChanges();
    return ok;
  }
  
  bool sound_setMusicEffect() {
    if (_sound->fade(0,200)) delay(200);
    _sound->setPowerStatus(true);
    _sound->setLive(false);
    _sound->setBrightness(128);
    _sound->setEffect(157);
    _sound->setSpeed(250);
    _sound->setIntensity(128);
    _sound->setCustom(93,94,130);
    _sound->setFgColor(255,160,0);
    _sound->setBgColor(0,0,0);
    _sound->setFxColor(0,0,0);
    _sound->setPalette(1);
    _sound->setTransitionTime(200);
    return _sound->applyChanges();
  }
  
  bool sound_setLightningEffect() {
    if (_sound->fade(0,200)) delay(200);
    _sound->setPowerStatus(true);
    _sound->setLive(false);
    _sound->setBrightness(200);
    _sound->setEffect(74);
    _sound->setSpeed(44);
    _sound->setIntensity(13);
    _sound->setCustom(93,94,130);
    _sound->setFgColor(0,0,255);
    _sound->setBgColor(0,0,0);
    _sound->setFxColor(0,0,0);
    _sound->setPalette(7);
    _sound->setTransitionTime(200);
    return _sound->applyChanges();
  }

  bool sound_setAlarm() {
    if (!_sound->setAlarm(true)) return false;
    if (_sound->fade(0,200)) delay(200);
    _sound->setPowerStatus(true);
    _sound->setLive(false);
    _sound->setBrightness(250);
    _sound->setEffect(110);
    _sound->setSpeed(255);
    _sound->setIntensity(96);
    _sound->setFgColor(255,0,0);
    _sound->setBgColor(255,255,255);
    _sound->setFxColor(0,0,255);
    _sound->setPalette(5);
    if (!_sound->applyChanges()) {
      _sound->setAlarm(false); // hier wird im schlimmsten Fall das Backup wiederhergestellt, obwohl sich seitdem der Status nicht geändert hat
      return false;
    }
    return true;
  }

  bool sound_stopAlarm() {
    return _sound->setAlarm(false); // _sound->setAlarm kümmert sich um die interne Logik
  }

  bool sound_setPause() {
    if (!_sound->setPause(true)) return false;   // _sound->setPause kümmert sich um das Backup, falls nötig
    if (_sound->fade(0,200)) delay(200);
    _sound->setPowerStatus(true);
    _sound->setLive(false);
    _sound->setBrightness(100);
    _sound->setEffect(69);
    _sound->setSpeed(120);
    _sound->setIntensity(64);
    _sound->setFgColor(255,160,0);
    _sound->setBgColor(0,0,0);
    _sound->setFxColor(0,0,0);
    _sound->setPalette(35);
    if (!_sound->applyChanges()) {
      _sound->setPause(false);   // hier wird im schlimmsten Fall ein bereits wiederhergestelltes Backup nochmal wiederhergestellt
      return false;
    }
    return true;
  }

  bool sound_setResume() {
    return _sound->setPause(false); // setPause kümmert sich um das Wiederherstellen des Backups
  }

  bool sound_commit() {
    return _sound->applyChanges();
  }

  // =================================================
  //            WLED (alle gleichzeitig)
  // =================================================
  
  bool wled_setMusicEffect() {
    return (canvas_setMusicEffect() && sound_setMusicEffect());
  }
  
  bool wled_setLightningEffect() {
    return (canvas_setLightningEffect() && sound_setLightningEffect());
  }
  
  // =================================================
  //            Hyperion
  // =================================================

  bool hyperion_setTicker(int ms) {
    return _hyperion->setTickInterval(ms);
  }

  bool hyperion_startBroadcast() {
    return _hyperion->startBroadcast();
  }
  
  bool hyperion_stopBroadcast() {
    return _hyperion->stopBroadcast();
  }

  
  // =================================================
  //            HUE
  // =================================================

  bool hue_init() {
    return _hue->readLights();
  }

  bool hue_setTicker(int ms) {
    return _hue->setTickInterval(ms);
  }
  
  bool hueLight_setPower(const String& lampName, bool onoff, bool commit/*=false*/) {
    HueLight* l = _hue->getLightByName(lampName);
    if (!l) return false;
    bool ok = l->setOn(onoff);
    if (ok && commit) return l->applyChanges(_hue);
    return ok;
  }
  
  bool hueLight_powerOn(const String& lampName, bool commit/*=false*/) {
    HueLight* l = _hue->getLightByName(lampName);
    if (!l) return false;
    bool ok = l->setOn(true);
    if (ok && commit) return l->applyChanges(_hue);
    return ok;
  }
  
  bool hueLight_powerOff(const String& lampName, bool commit/*=false*/) {
    HueLight* l = _hue->getLightByName(lampName);
    if (!l) return false;
    bool ok = l->setOn(false);
    if (ok && commit) return l->applyChanges(_hue);
    return ok;
  }
  
  bool hueLight_setBri(const String& lampName, uint8_t bri, bool commit/*=false*/) {
    HueLight* l = _hue->getLightByName(lampName);
    if (!l) return false;
    bool ok = l->setBri(bri);
    if (ok && commit) return l->applyChanges(_hue);
    return ok;
  }
  
  bool hueLight_setCT(const String& lampName, uint16_t ct, bool commit/*=false*/) {
    HueLight* l = _hue->getLightByName(lampName);
    if (!l) return false;
    bool ok = l->setCT(ct);
    if (ok && commit) return l->applyChanges(_hue);
    return ok;
  }
  
  bool hueLight_setRGB(const String& lampName, uint8_t R, uint8_t G, uint8_t B, bool commit/*=false*/) {
    HueLight* l = _hue->getLightByName(lampName);
    if (!l) return false;
    bool ok = l->setRGB(R,G,B);
    if (ok && commit) return l->applyChanges(_hue);
    return ok;
  }
  
  bool hueLight_commit(const String& lampName) {
    HueLight* l = _hue->getLightByName(lampName);
    if (!l) return false;
    return l->applyChanges(_hue);
  }
  
  bool hueGroup_setPower(const String& groupname, bool onoff, bool commit/*=false*/) {
    HueGroup* g = _hue->getGroupByName(groupname);
    if (!g) return false;
    bool ok = g->setOn(onoff);
    if (ok && commit) return g->applyChanges(_hue);
    return ok;
  }
  
  bool hueGroup_powerOn(const String& groupname, bool commit/*=false*/) {
    HueGroup* g = _hue->getGroupByName(groupname);
    if (!g) return false;
    bool ok = g->setOn(true);
    if (ok && commit) return g->applyChanges(_hue);
    return ok;
  }
  
  bool hueGroup_powerOff(const String& groupname, bool commit/*=false*/) {
    HueGroup* g = _hue->getGroupByName(groupname);
    if (!g) return false;
    bool ok = g->setOn(false);
    if (ok && commit) return g->applyChanges(_hue);
    return ok;
  }
  
  bool hueGroup_setBri(const String& groupname, uint8_t bri, bool commit/*=false*/) {
    HueGroup* g = _hue->getGroupByName(groupname);
    if (!g) return false;
    bool ok = g->setBri(bri);
    if (ok && commit) return g->applyChanges(_hue);
    return ok;
  }
  
  bool hueGroup_setCT(const String& groupname, uint16_t ct, bool commit/*=false*/) {
    HueGroup* g = _hue->getGroupByName(groupname);
    if (!g) return false;
    bool ok = g->setCT(ct);
    if (ok && commit) return g->applyChanges(_hue);
    return ok;
  }
  
  bool hueGroup_commit(const String& groupname) {
    HueGroup* g = _hue->getGroupByName(groupname);
    if (!g) return false;
    return g->applyChanges(_hue);
  }
  
  bool hueScene_set(const String& sceneName) {
    HueScene* s = _hue->getSceneByName(sceneName);
    if (!s) return false;
    return s->setActive(_hue);
  }
  
  bool hueScene_save(const String& sceneName) {
    HueScene* s = _hue->getSceneByName(sceneName);
    if (!s) return false;
    return s->captureLightStates(_hue);
  }
  
  bool hueSensor_setState(const String& sensorName, const String& statekey, int statevalue, bool commit/*=false*/) {
    HueSensor* s = _hue->getSensorByName(sensorName);
    if (!s) return false;
    bool ok = s->setValue(statekey, statevalue);
    if (ok && commit) return s->applyChanges(_hue);
    return ok;
  }

  bool hueSensor_commit(const String& sensorName) {
    HueSensor* s = _hue->getSensorByName(sensorName);
    if (!s) return false;
    return s->applyChanges(_hue);
  }

  // =================================================
  //            MAKROS
  // =================================================
  
  bool radioMode() {
    if (!_yamaha->getPowerStatus()) _yamaha->setPower(true);
    //if (_beamer->getPowerStatus()) _beamer->setPower(false);
    delay(100);
    _yamaha->setSource("NET RADIO");
    canvas_setMusicEffect();
    _yamaha->setVolume(-600);
    sound_setMusicEffect();
    _yamaha->setStraight(false);
    // _hyperion->stopBroadcast();
    delay(100);
    _yamaha->setSoundProgram("7ch Stereo");
    _hue->setPower(true);
    return true;
  }
  
  bool blurayMode() {
    if (!_yamaha->getPowerStatus()) _yamaha->setPower(true);
    if (!_beamer->getPowerStatus()) _beamer->setPower(true);
    _yamaha->setSource("HDMI2");
    _canvas->setLive(true);
    _yamaha->setVolume(-350);
    _sound->setPowerStatus(false);
    _yamaha->setStraight(true);
    _hyperion->startBroadcast();
    _hue->setScene("Film");
    return true;
  }
  
  bool streamingMode() {
    if (!_yamaha->getPowerStatus()) _yamaha->setPower(true);
    if (!_beamer->getPowerStatus()) _beamer->setPower(true);
    _yamaha->setSource("HDMI4");
    _canvas->setLive(true);
    _yamaha->setVolume(-400);
    _sound->setPowerStatus(false);
    _yamaha->setStraight(true);
    _hyperion->startBroadcast();
    _hue->setScene("Film");
    return true;
  }
  
  bool GamingMode() {
    if (!_yamaha->getPowerStatus()) _yamaha->setPower(true);
    if (!_beamer->getPowerStatus()) _beamer->setPower(true);
    _yamaha->setSource("HDMI5");
    _canvas->setLive(true);
    _yamaha->setVolume(-500);
    _sound->setPowerStatus(false);
    _yamaha->setStraight(false);
    _hyperion->startBroadcast();
    _yamaha->setSoundProgram("Surround Decoder");
    _hue->setScene("Gaming");
    return true;
  }
  
  bool startPause() {
    isPause = true;
    HueScene* pp = _hue->getSceneByName("prePause");
    if (pp) pp->captureLightStates(_hue);
    HueScene* p = _hue->getSceneByName("Pause");
    if (pp) pp->setActive(_hue);
    // Canvas Pause
    canvas_setPause();
    // sound Pause
    sound_setPause();
    // für später: yamaha->Zone2: source setzen, Lautstärke setzen, output einschalten
    return true;
  }
  
  bool endPause() {
    isPause = false;
    HueScene* pp = _hue->getSceneByName("prePause");
    if (pp) pp->setActive(_hue);
    //_hue->setScene("prePause");
    canvas_setResume();
    sound_setResume();
    return true;
  }
  
  bool togglePause() {
    if (isPause) return endPause();
    return startPause();
  }
  
  bool Power(bool onoff) {
    if (!onoff) {
      yamaha_powerOff();
      beamer_powerOff();
      canvas_powerOff();
      sound_powerOff();
      hyperion_stopBroadcast();
      _hue->setPower(false);
      return true;
    }
    _hue->setPower(true); // ToDo: Logik hierher verschieben, hue soll nur ausführen!
    yamaha_powerOn();
    delay(50);
    yamaha_setInput("NET RADIO");
    delay(50);
    yamaha_setVolume(-600);
    delay(50);
    yamaha_setDsp("7ch Stereo");
    delay(50);
    yamaha_setStraight(false);
    return true;
  }
}
