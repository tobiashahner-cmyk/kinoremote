#include "KinoAPI.h"
#include "YamahaReceiver.h"
#include "WLEDDevice.h"
#include "OptomaBeamer.h"
#include "HyperionDevice.h"
#include "HueBridge.h"
#include "KinoMacroEngine.h"

// Externe Geräte aus dem Sketch
extern YamahaReceiver yamaha;
extern WLEDDevice canvas;
extern WLEDDevice sound;
extern OptomaBeamer beamer;
extern HyperionDevice hyperion;
extern HueBridge hue;



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
    return yamaha.init();
  }

  bool yamaha_setTicker(int ms) {
    return yamaha.setTickInterval(ms);
  }
  
  bool yamaha_setPower(bool onoff) {
    if (yamaha.getPowerStatus() == onoff) return true;
    return yamaha.setPower(onoff);
  }
  
  bool yamaha_powerOn() {
    if (yamaha.getPowerStatus()) return true;
    return yamaha.setPower(true);
  }
  
  bool yamaha_powerOff() {
    if (!yamaha.getPowerStatus()) return true;
    return yamaha.setPower(false);
  }
  
  bool yamaha_setVolume(int vol) {
    return yamaha.setVolume(vol);
  }
  
  bool yamaha_setVolumePercent(int percentage) {
    return yamaha.setVolumePercent(percentage);
  }
  
  bool yamaha_setInput(const String& inputName) {
    if (yamaha.getSource() == inputName) return true;
    return yamaha.setSource(inputName);
  }
  
  bool yamaha_setDsp(const String& dspName) {
    if (yamaha.getSoundProgram() == dspName) return true;
    return yamaha.setSoundProgram(dspName);  
  }
  
  bool yamaha_setStraight(bool onoff) {
    if (yamaha.getStraight() == onoff) return true;
    return yamaha.setStraight(onoff);
  }

  bool yamaha_straightOn() {
    if (yamaha.getStraight()) return true;
    return yamaha.setStraight(true);
  }

  bool yamaha_straightOff() {
    if (!yamaha.getStraight()) return true;
    return yamaha.setStraight(false);
  }
  
  bool yamaha_setEnhancer(bool onoff) {
    if (yamaha.getEnhancer() == onoff) return true;
    return yamaha.setEnhancer(onoff);
  }

  bool yamaha_enhancerOn() {
    if (yamaha.getEnhancer()) return true;
    return yamaha.setEnhancer(true);
  }

  bool yamaha_enhancerOff() {
    if (!yamaha.getEnhancer()) return true;
    return yamaha.setEnhancer(false);
  }
  
  bool yamaha_setMute(bool onoff) {
    if (yamaha.getMute() == onoff) return true;
    return yamaha.setMute(onoff);
  }

  bool yamaha_muteOn() {
    if (yamaha.getMute()) return true;
    return yamaha.setMute(true);
  }

  bool yamaha_muteOff() {
    if (!yamaha.getMute()) return true;
    return yamaha.setMute(false);
  }

  bool yamaha_setStation(const String& stationname) {
    return yamaha.selectNetRadioFavorite(stationname);
  }

  // =================================================
  //            BEAMER
  // =================================================
  bool beamer_setTicker(int ms) {
    return beamer.setTickInterval(ms);
  }
  
  bool beamer_setPower(bool onoff) {
    if (beamer.getPowerStatus() == onoff) return true;
    return beamer.setPower(onoff);
  }
  
  bool beamer_powerOn() {
    if (beamer.getPowerStatus()) return true;
    return beamer.setPower(true);
  }
  
  bool beamer_powerOff() {
    if (!beamer.getPowerStatus()) return true;
    return beamer.setPower(false);
  }

  bool beamer_setInput(const String& inputname) {
    if(!beamer.getPowerStatus()) beamer.setPower(true);
    return beamer.setSource(inputname);
  }

  // =================================================
  //            LEINWAND
  // =================================================
  bool canvas_setTicker(int ms) {
    return canvas.setTickInterval(ms);
  }
  
  bool canvas_setPower(bool onoff, bool commit/*=false*/) {
    if (canvas.getPowerStatus() == onoff) return true;
    bool ok = canvas.setPowerStatus(onoff);
    if (ok && commit) return canvas.applyChanges();
    return ok;
  }
  
  bool canvas_powerOn(bool commit/*=false*/) {
    if (canvas.getPowerStatus()) return true;
    bool ok = canvas.setPowerStatus(true);
    if (ok && commit) return canvas.applyChanges();
    return ok;
  }
  
  bool canvas_powerOff(bool commit/*=false*/) {
    if (!canvas.getPowerStatus()) return true;
    bool ok = canvas.setPowerStatus(false);
    if (ok && commit) return canvas.applyChanges();
    return ok;
  }
  
  bool canvas_setBrightness(uint8_t bri, bool commit/*=false*/) {
    if (canvas.getBrightness() == bri) return true;
    bool ok = canvas.setBrightness(bri);
    if (ok && commit) return canvas.applyChanges();
    return ok;
  }
  
  bool canvas_setEffect(uint16_t effectId, bool commit/*=false*/) {
    if (canvas.getEffect() == effectId) return true;
    bool ok = canvas.setEffect(effectId);
    if (ok && commit) return canvas.applyChanges();
    return ok;
  }
  
  bool canvas_setSpeed(uint8_t sx, bool commit/*=false*/) {
    if (canvas.getSpeed() == sx) return true;
    bool ok = canvas.setSpeed(sx);
    if (ok && commit) return canvas.applyChanges();
    return ok;
  }
  
  bool canvas_setIntensity(uint8_t ix, bool commit/*=false*/) {
    if (canvas.getIntensity() == ix) return true;
    bool ok = canvas.setIntensity(ix);
    if (ok && commit) return canvas.applyChanges();
    return ok;
  }
  
  bool canvas_setLive(bool onoff, bool commit/*=false*/) {
    //if (canvas.isOverridingLiveData() == (!onoff)) return true;
    bool ok = canvas.setLive(onoff);
    if (ok && commit) return canvas.applyChanges();
    return ok;
  }
  
  bool canvas_setSolid(uint8_t r, uint8_t g, uint8_t b, uint8_t bri) {
    //return canvas.setColor(r,g,b,bri);    
    canvas.setFgColor(r,g,b);
    canvas.setBrightness(bri);
    return canvas.applyChanges();
  }
  
  bool canvas_setTransitionTime(int tt, bool commit/*=false*/) {
    bool ok = canvas.setTransitionTime(tt);
    if (ok && commit) return canvas.applyChanges();
    return ok;
  }
  
  bool canvas_fadeSync(uint8_t bri, int ms) {
    if (!canvas.fade(bri, ms)) return false;
    unsigned long start = millis();
    unsigned long now = millis();
    while(now - start <= ms) {
      yield();
    }
    return true;
  }

  bool canvas_fadeAsync(uint8_t bri, int ms) {
    if (!canvas.fade(bri, ms)) return false;
    return true;
  }
  
  bool canvas_setFgColor(uint8_t R, uint8_t G, uint8_t B, bool commit/*=false*/) {
    bool ok = canvas.setFgColor(R,G,B);
    if (ok && commit) return canvas.applyChanges();
    return ok;
  }
  
  bool canvas_setBgColor(uint8_t R, uint8_t G, uint8_t B, bool commit/*=false*/) {
    bool ok = canvas.setBgColor(R,G,B);
    if (ok && commit) return canvas.applyChanges();
    return ok;
  }
  
  bool canvas_setFxColor(uint8_t R, uint8_t G, uint8_t B, bool commit/*=false*/) {
    bool ok = canvas.setFxColor(R,G,B);
    if (ok && commit) return canvas.applyChanges();
    return ok;
  }
  
  bool canvas_setCustomParams(uint8_t c1x, uint8_t c2x, uint8_t c3x, bool commit/*=false*/) {
    bool ok = canvas.setCustom(c1x, c2x, c3x);
    if (ok && commit) return canvas.applyChanges();
    return ok;
  }
  
  bool canvas_setPalette(uint8_t pal, bool commit/*=false*/) {
    if (canvas.getPalette() == pal) return true;
    bool ok = canvas.setPalette(pal);
    if (ok && commit) return canvas.applyChanges();
    return ok;
  }
  
  bool canvas_setMusicEffect() {
    //return canvas.setMusicMode();
    if (canvas.fade(0,200)) delay(200);
    canvas.setPowerStatus(true);
    canvas.setLive(false);
    canvas.setBrightness(128);
    canvas.setEffect(9);
    canvas.setSpeed(64);
    canvas.setIntensity(128);
    canvas.setFgColor(255,0,0);
    canvas.setBgColor(0,255,0);
    canvas.setFxColor(0,0,255);
    canvas.setTransitionTime(100);
    canvas.setPalette(0);
    return canvas.applyChanges();
  }
  
  bool canvas_setLightningEffect() {
    //return canvas.setLightningMode();
    if (canvas.fade(0,200)) delay(200);
    canvas.setPowerStatus(true);
    canvas.setLive(false);
    canvas.setBrightness(200);
    canvas.setEffect(57);
    canvas.setSpeed(250);
    canvas.setIntensity(128);
    canvas.setFgColor(255,255,255);
    canvas.setBgColor(0,0,0);
    canvas.setFxColor(0,0,0);
    canvas.setPalette(5);
    return canvas.applyChanges();
  }
  
  bool canvas_setAlarm() {
    if (!canvas.setAlarm(true)) return false;
    if (canvas.fade(0,200)) delay(200);
    canvas.setPowerStatus(true);
    canvas.setLive(false);
    canvas.setBrightness(200);
    canvas.setEffect(11);
    canvas.setSpeed(255);
    canvas.setIntensity(255);
    canvas.setFgColor(255,0,0);
    canvas.setBgColor(160,160,160);
    canvas.setFxColor(0,0,255);
    canvas.setPalette(5);
    if (!canvas.applyChanges()) {
      canvas.setAlarm(false); // hier wird im schlimmsten Fall das Backup wiederhergestellt, obwohl sich seitdem der Status nicht geändert hat
      return false;
    }
    return true;
  }

  bool canvas_stopAlarm() {
    return canvas.setAlarm(false); // canvas.setAlarm kümmert sich um die interne Logik
  }

  bool canvas_setPause() {
    if (!canvas.setPause(true)) return false;   // canvas.setPause kümmert sich um das Backup, falls nötig
    
    if (canvas.fade(0,200)) delay(200);
    canvas.setPowerStatus(true);
    canvas.setLive(false);
    canvas.setBrightness(100);
    canvas.setEffect(2);
    canvas.setSpeed(100);
    canvas.setIntensity(100);
    canvas.setFgColor(255,0,0);
    canvas.setBgColor(0,0,255);
    canvas.setFxColor(0,0,0);
    canvas.setPalette(5);
    if (!canvas.applyChanges()) {
      canvas.setPause(false);   // hier wird im schlimmsten Fall ein bereits wiederhergestelltes Backup nochmal wiederhergestellt
      return false;
    }
    return true;
  }

  bool canvas_setResume() {
    return canvas.setPause(false); // setPause kümmert sich um das Wiederherstellen des Backups
  }
  
  bool canvas_commit() {
    return canvas.applyChanges();
  }

  // =================================================
  //            SOUND LED
  // =================================================

  bool sound_setTicker(int ms) {
    return sound.setTickInterval(ms);
  }
  
  bool sound_setPower(bool onoff, bool commit/*=false*/) {
    if (sound.getPowerStatus() == onoff) return true;
    bool ok = sound.setPowerStatus(onoff);
    if (ok && commit) return sound.applyChanges();
    return ok;
  }
  
  bool sound_powerOn(bool commit/*=false*/) {
    if (sound.getPowerStatus()) return true;
    bool ok = sound.setPowerStatus(true);
    if (ok && commit) return sound.applyChanges();
    return ok;
  }
  
  bool sound_powerOff(bool commit/*=false*/) {
    if (!sound.getPowerStatus()) return true;
    bool ok = sound.setPowerStatus(false);
    if (ok && commit) return sound.applyChanges();
    return ok;
  }
  
  bool sound_setBrightness(uint8_t bri, bool commit/*=false*/) {
    bool ok = sound.setBrightness(bri);
    if (ok && commit) return sound.applyChanges();
    return ok;
  }
  
  bool sound_setEffect(uint16_t effectId, bool commit/*=false*/) {
    bool ok = sound.setEffect(effectId);
    if (ok && commit) return sound.applyChanges();
    return ok;
  }
  
  bool sound_setSpeed(uint8_t sx, bool commit/*=false*/) {
    bool ok = sound.setSpeed(sx);
    if (ok && commit) return sound.applyChanges();
    return ok;
  }
  
  bool sound_setIntensity(uint8_t ix, bool commit/*=false*/) {
    bool ok = sound.setIntensity(ix);
    if (ok && commit) return sound.applyChanges();
    return ok;
  }

  bool sound_setLive(bool onoff, bool commit/*=false*/) {
    return true;
  }
  
  bool sound_setSolid(uint8_t r, uint8_t g, uint8_t b, uint8_t bri) {
    //return sound.setColor(r,g,b,bri);
    sound.setFgColor(r,g,b);
    sound.setBrightness(bri);
    sound.setEffect(0);
    return sound.applyChanges();
  }
  
  bool sound_setTransitionTime(int tt, bool commit/*=false*/) {
    bool ok = sound.setTransitionTime((int)(tt/100));
    if (ok && commit) return sound.applyChanges();
    return ok;
  }

  bool sound_fadeSync(uint8_t bri, int ms) {
    if (!sound.fade(bri, ms)) return false;
    unsigned long start = millis();
    unsigned long now = millis();
    while(now - start <= ms) {
      yield();
    }
    return true;
  }

  bool sound_fadeAsync(uint8_t bri, int ms) {
    if (!sound.fade(bri, ms)) return false;
    return true;
  }
  
  bool sound_setFgColor(uint8_t R, uint8_t G, uint8_t B, bool commit/*=false*/) {
    bool ok = sound.setFgColor(R,G,B);
    if (ok && commit) return sound.applyChanges();
    return ok;
  }
  
  bool sound_setBgColor(uint8_t R, uint8_t G, uint8_t B, bool commit/*=false*/) {
    bool ok = sound.setBgColor(R,G,B);
    if (ok && commit) return sound.applyChanges();
    return ok;
  }
  
  bool sound_setFxColor(uint8_t R, uint8_t G, uint8_t B, bool commit/*=false*/) {
    bool ok = sound.setFxColor(R,G,B);
    if (ok && commit) return sound.applyChanges();
    return ok;
  }
  
  bool sound_setCustomParams(uint8_t c1x, uint8_t c2x, uint8_t c3x, bool commit/*=false*/) {
    bool ok = sound.setCustom(c1x, c2x, c3x);
    if (ok && commit) return sound.applyChanges();
    return ok;
  }
  
  bool sound_setPalette(uint8_t pal, bool commit/*=false*/) {
    if (sound.getPalette() == pal) return true;
    bool ok = sound.setPalette(pal);
    if (ok && commit) return sound.applyChanges();
    return ok;
  }
  
  bool sound_setMusicEffect() {
    if (sound.fade(0,200)) delay(200);
    sound.setPowerStatus(true);
    sound.setLive(false);
    sound.setBrightness(128);
    sound.setEffect(157);
    sound.setSpeed(250);
    sound.setIntensity(128);
    sound.setCustom(93,94,130);
    sound.setFgColor(255,160,0);
    sound.setBgColor(0,0,0);
    sound.setFxColor(0,0,0);
    sound.setPalette(1);
    sound.setTransitionTime(200);
    return sound.applyChanges();
  }
  
  bool sound_setLightningEffect() {
    if (sound.fade(0,200)) delay(200);
    sound.setPowerStatus(true);
    sound.setLive(false);
    sound.setBrightness(200);
    sound.setEffect(74);
    sound.setSpeed(44);
    sound.setIntensity(13);
    sound.setCustom(93,94,130);
    sound.setFgColor(0,0,255);
    sound.setBgColor(0,0,0);
    sound.setFxColor(0,0,0);
    sound.setPalette(7);
    sound.setTransitionTime(200);
    return sound.applyChanges();
  }

  bool sound_setAlarm() {
    if (!sound.setAlarm(true)) return false;
    if (sound.fade(0,200)) delay(200);
    sound.setPowerStatus(true);
    sound.setLive(false);
    sound.setBrightness(250);
    sound.setEffect(110);
    sound.setSpeed(255);
    sound.setIntensity(96);
    sound.setFgColor(255,0,0);
    sound.setBgColor(255,255,255);
    sound.setFxColor(0,0,255);
    sound.setPalette(5);
    if (!sound.applyChanges()) {
      sound.setAlarm(false); // hier wird im schlimmsten Fall das Backup wiederhergestellt, obwohl sich seitdem der Status nicht geändert hat
      return false;
    }
    return true;
  }

  bool sound_stopAlarm() {
    return sound.setAlarm(false); // sound.setAlarm kümmert sich um die interne Logik
  }

  bool sound_setPause() {
    if (!sound.setPause(true)) return false;   // sound.setPause kümmert sich um das Backup, falls nötig
    if (sound.fade(0,200)) delay(200);
    sound.setPowerStatus(true);
    sound.setLive(false);
    sound.setBrightness(100);
    sound.setEffect(69);
    sound.setSpeed(120);
    sound.setIntensity(64);
    sound.setFgColor(255,160,0);
    sound.setBgColor(0,0,0);
    sound.setFxColor(0,0,0);
    sound.setPalette(35);
    if (!sound.applyChanges()) {
      sound.setPause(false);   // hier wird im schlimmsten Fall ein bereits wiederhergestelltes Backup nochmal wiederhergestellt
      return false;
    }
    return true;
  }

  bool sound_setResume() {
    return sound.setPause(false); // setPause kümmert sich um das Wiederherstellen des Backups
  }

  bool sound_commit() {
    return sound.applyChanges();
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
    return hyperion.setTickInterval(ms);
  }

  bool hyperion_startBroadcast() {
    return hyperion.startBroadcast();
  }
  
  bool hyperion_stopBroadcast() {
    return hyperion.stopBroadcast();
  }

  
  // =================================================
  //            HUE
  // =================================================

  bool hue_init() {
    return hue.readLights();
  }

  bool hue_setTicker(int ms) {
    return hue.setTickInterval(ms);
  }
  
  bool hueLight_setPower(const String& lampName, bool onoff, bool commit/*=false*/) {
    HueLight* l = hue.getLightByName(lampName);
    if (!l) return false;
    bool ok = l->setOn(onoff);
    if (ok && commit) return l->applyChanges(hue);
    return ok;
  }
  
  bool hueLight_powerOn(const String& lampName, bool commit/*=false*/) {
    HueLight* l = hue.getLightByName(lampName);
    if (!l) return false;
    bool ok = l->setOn(true);
    if (ok && commit) return l->applyChanges(hue);
    return ok;
  }
  
  bool hueLight_powerOff(const String& lampName, bool commit/*=false*/) {
    HueLight* l = hue.getLightByName(lampName);
    if (!l) return false;
    bool ok = l->setOn(false);
    if (ok && commit) return l->applyChanges(hue);
    return ok;
  }
  
  bool hueLight_setBri(const String& lampName, uint8_t bri, bool commit/*=false*/) {
    HueLight* l = hue.getLightByName(lampName);
    if (!l) return false;
    bool ok = l->setBri(bri);
    if (ok && commit) return l->applyChanges(hue);
    return ok;
  }
  
  bool hueLight_setCT(const String& lampName, uint16_t ct, bool commit/*=false*/) {
    HueLight* l = hue.getLightByName(lampName);
    if (!l) return false;
    bool ok = l->setCT(ct);
    if (ok && commit) return l->applyChanges(hue);
    return ok;
  }
  
  bool hueLight_setRGB(const String& lampName, uint8_t R, uint8_t G, uint8_t B, bool commit/*=false*/) {
    HueLight* l = hue.getLightByName(lampName);
    if (!l) return false;
    bool ok = l->setRGB(R,G,B);
    if (ok && commit) return l->applyChanges(hue);
    return ok;
  }
  
  bool hueLight_commit(const String& lampName) {
    HueLight* l = hue.getLightByName(lampName);
    if (!l) return false;
    return l->applyChanges(hue);
  }
  
  bool hueGroup_setPower(const String& groupname, bool onoff, bool commit/*=false*/) {
    HueGroup* g = hue.getGroupByName(groupname);
    if (!g) return false;
    bool ok = g->setOn(onoff);
    if (ok && commit) return g->applyChanges(hue);
    return ok;
  }
  
  bool hueGroup_powerOn(const String& groupname, bool commit/*=false*/) {
    HueGroup* g = hue.getGroupByName(groupname);
    if (!g) return false;
    bool ok = g->setOn(true);
    if (ok && commit) return g->applyChanges(hue);
    return ok;
  }
  
  bool hueGroup_powerOff(const String& groupname, bool commit/*=false*/) {
    HueGroup* g = hue.getGroupByName(groupname);
    if (!g) return false;
    bool ok = g->setOn(false);
    if (ok && commit) return g->applyChanges(hue);
    return ok;
  }
  
  bool hueGroup_setBri(const String& groupname, uint8_t bri, bool commit/*=false*/) {
    HueGroup* g = hue.getGroupByName(groupname);
    if (!g) return false;
    bool ok = g->setBri(bri);
    if (ok && commit) return g->applyChanges(hue);
    return ok;
  }
  
  bool hueGroup_setCT(const String& groupname, uint16_t ct, bool commit/*=false*/) {
    HueGroup* g = hue.getGroupByName(groupname);
    if (!g) return false;
    bool ok = g->setCT(ct);
    if (ok && commit) return g->applyChanges(hue);
    return ok;
  }
  
  bool hueGroup_commit(const String& groupname) {
    HueGroup* g = hue.getGroupByName(groupname);
    if (!g) return false;
    return g->applyChanges(hue);
  }
  
  bool hueScene_set(const String& sceneName) {
    HueScene* s = hue.getSceneByName(sceneName);
    if (!s) return false;
    return s->setActive(hue);
  }
  
  bool hueScene_save(const String& sceneName) {
    HueScene* s = hue.getSceneByName(sceneName);
    if (!s) return false;
    return s->captureLightStates(hue);
  }
  
  bool hueSensor_setState(const String& sensorName, const String& statekey, int statevalue, bool commit/*=false*/) {
    HueSensor* s = hue.getSensorByName(sensorName);
    if (!s) return false;
    bool ok = s->setValue(statekey, statevalue);
    if (ok && commit) return s->applyChanges(hue);
    return ok;
  }

  bool hueSensor_commit(const String& sensorName) {
    HueSensor* s = hue.getSensorByName(sensorName);
    if (!s) return false;
    return s->applyChanges(hue);
  }

  // =================================================
  //            MAKROS
  // =================================================
  
  bool radioMode() {
    if (!yamaha.getPowerStatus()) yamaha.setPower(true);
    //if (beamer.getPowerStatus()) beamer.setPower(false);
    delay(100);
    yamaha.setSource("NET RADIO");
    canvas_setMusicEffect();
    yamaha.setVolume(-600);
    sound_setMusicEffect();
    yamaha.setStraight(false);
    // hyperion.stopBroadcast();
    delay(100);
    yamaha.setSoundProgram("7ch Stereo");
    hue.setPower(true);
    return true;
  }
  
  bool blurayMode() {
    if (!yamaha.getPowerStatus()) yamaha.setPower(true);
    if (!beamer.getPowerStatus()) beamer.setPower(true);
    yamaha.setSource("HDMI2");
    canvas.setLive(true);
    yamaha.setVolume(-350);
    sound.setPowerStatus(false);
    yamaha.setStraight(true);
    hyperion.startBroadcast();
    hue.setScene("Film");
    return true;
  }
  
  bool streamingMode() {
    if (!yamaha.getPowerStatus()) yamaha.setPower(true);
    if (!beamer.getPowerStatus()) beamer.setPower(true);
    yamaha.setSource("HDMI4");
    canvas.setLive(true);
    yamaha.setVolume(-400);
    sound.setPowerStatus(false);
    yamaha.setStraight(true);
    hyperion.startBroadcast();
    hue.setScene("Film");
    return true;
  }
  
  bool GamingMode() {
    if (!yamaha.getPowerStatus()) yamaha.setPower(true);
    if (!beamer.getPowerStatus()) beamer.setPower(true);
    yamaha.setSource("HDMI5");
    canvas.setLive(true);
    yamaha.setVolume(-500);
    sound.setPowerStatus(false);
    yamaha.setStraight(false);
    hyperion.startBroadcast();
    yamaha.setSoundProgram("Surround Decoder");
    hue.setScene("Gaming");
    return true;
  }
  
  bool startPause() {
    isPause = true;
    HueScene* pp = hue.getSceneByName("prePause");
    if (pp) pp->captureLightStates(hue);
    HueScene* p = hue.getSceneByName("Pause");
    if (pp) pp->setActive(hue);
    // Canvas Pause
    canvas_setPause();
    // sound Pause
    sound_setPause();
    // für später: yamaha->Zone2: source setzen, Lautstärke setzen, output einschalten
    return true;
  }
  
  bool endPause() {
    isPause = false;
    HueScene* pp = hue.getSceneByName("prePause");
    if (pp) pp->setActive(hue);
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
      hue.setPower(false);
      return true;
    }
    hue.setPower(true); // ToDo: Logik hierher verschieben, hue soll nur ausführen!
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
