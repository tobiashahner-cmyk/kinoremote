#include "KinoMacroActions.h"
#include "KinoAPI.h"

// === Handler ===

static ActionResult act_yamaha_power(JsonObject a) {
  if (!a.containsKey("on")) return { ActionError::MissingParam, "on" };
  bool onoff = a["on"]|false;
  bool ok = KinoAPI::yamaha_setPower(onoff);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_yamaha_volume(JsonObject a) {
  if (!a.containsKey("vol")) return { ActionError::MissingParam, "vol" };
  int vol = a["vol"]|(-800);
  bool ok = KinoAPI::yamaha_setVolume(vol);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""}; 
}
static ActionResult act_yamaha_volume_percent(JsonObject a) {
  if (!a.containsKey("percent")) return { ActionError::MissingParam, "percent" };
  int pct = a["percent"]|20;
  bool ok = KinoAPI::yamaha_setVolumePercent(pct);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_yamaha_input(JsonObject a) {
  if (!a.containsKey("input")) return { ActionError::MissingParam, "input" };
  String input=a["input"]|"";
  bool ok = KinoAPI::yamaha_setInput(input);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_yamaha_dsp(JsonObject a) {
  if (!a.containsKey("dsp")) return { ActionError::MissingParam, "dsp" };
  String dsp = a["dsp"]|"";
  bool ok = KinoAPI::yamaha_setDsp(dsp);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_yamaha_straight(JsonObject a) {
  if (!a.containsKey("on")) return { ActionError::MissingParam, "on" };
  bool onoff = a["on"]|false;
  bool ok = KinoAPI::yamaha_setStraight(onoff);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_yamaha_enhancer(JsonObject a) {
  if (!a.containsKey("on")) return { ActionError::MissingParam, "on" };
  bool onoff = a["on"]|false;
  bool ok = KinoAPI::yamaha_setEnhancer(onoff);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_yamaha_mute(JsonObject a) {
  if (!a.containsKey("on")) return { ActionError::MissingParam, "on" };
  bool onoff = a["on"]|false;
  bool ok = KinoAPI::yamaha_setMute(onoff);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_yamaha_station(JsonObject a) {
  if (!a.containsKey("station")) return { ActionError::MissingParam, "station" };
  String station = a["station"]|"";
  bool ok = KinoAPI::yamaha_setStation(station);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_beamer_power(JsonObject a) {
  if (!a.containsKey("on")) return { ActionError::MissingParam, "on" };
  bool onoff = a["on"]|false;
  bool ok = KinoAPI::beamer_setPower(onoff);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_beamer_input(JsonObject a) {
  if (!a.containsKey("input")) return { ActionError::MissingParam, "input" };
  String input = a["input"]|"";
  bool ok = KinoAPI::beamer_setInput(input);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_canvas_power(JsonObject a) {
  if (!a.containsKey("on")) return { ActionError::MissingParam, "on" };
  bool onoff = a["on"]|false;
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::canvas_setPower(onoff,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_canvas_brightness(JsonObject a) {
  if (!a.containsKey("bri")) return { ActionError::MissingParam, "bri" };
  int bri = a["bri"]|1;
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::canvas_setBrightness(bri,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_canvas_effect(JsonObject a) {
  if (!a.containsKey("effect")) return { ActionError::MissingParam, "effect" };
  int effect = a["effect"]|0;
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::canvas_setEffect(effect,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_canvas_speed(JsonObject a) {
  if (!a.containsKey("speed")) return { ActionError::MissingParam, "speed" };
  int sx = a["sx"]|1;
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::canvas_setSpeed(sx,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_canvas_intensity(JsonObject a) {
  if (!a.containsKey("intensity")) return { ActionError::MissingParam, "intensity" };
  int si = a["intensity"]|1;
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::canvas_setIntensity(si,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_canvas_live(JsonObject a) {
  if (!a.containsKey("on")) return { ActionError::MissingParam, "on" };
  bool onoff = a["on"]|false;
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::canvas_setLive(onoff,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_canvas_tt(JsonObject a) {
  if (!a.containsKey("tt")) return { ActionError::MissingParam, "tt" };
  int tt = a["tt"]|1;
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::canvas_setTransitionTime(tt,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_canvas_fgcol(JsonObject a) {
  int r,g,b;
  if ((a.containsKey("col"))&&(a["col"].is<JsonArray>())) {
    r = a["col"][0]|0;
    g = a["col"][1]|0;
    b = a["col"][2]|0;
  } else if ((a.containsKey("r"))&&(a.containsKey("g"))&&(a.containsKey("b"))) {
    r = a["r"]|0;
    g = a["g"]|0;
    b = a["b"]|0;
  } else return { ActionError::MissingParam, "col or r,g,b" };
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::canvas_setFgColor(r,g,b,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_canvas_bgcol(JsonObject a) {
  int r,g,b;
  if ((a.containsKey("col"))&&(a["col"].is<JsonArray>())) {
    r = a["col"][0]|0;
    g = a["col"][1]|0;
    b = a["col"][2]|0;
  } else if ((a.containsKey("r"))&&(a.containsKey("g"))&&(a.containsKey("b"))) {
    r = a["r"]|0;
    g = a["g"]|0;
    b = a["b"]|0;
  } else return { ActionError::MissingParam, "col or r,g,b" };
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::canvas_setBgColor(r,g,b,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_canvas_fxcol(JsonObject a) {
  int r,g,b;
  if ((a.containsKey("col"))&&(a["col"].is<JsonArray>())) {
    r = a["col"][0]|0;
    g = a["col"][1]|0;
    b = a["col"][2]|0;
  } else if ((a.containsKey("r"))&&(a.containsKey("g"))&&(a.containsKey("b"))) {
    r = a["r"]|0;
    g = a["g"]|0;
    b = a["b"]|0;
  } else return { ActionError::MissingParam, "col or r,g,b" };
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::canvas_setFxColor(r,g,b,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_canvas_custom(JsonObject a) {
  int c1x = a["c1x"] | 0;
  int c2x = a["c2x"] | 0;
  int c3x = a["c3x"] | 0;
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::canvas_setCustomParams(c1x,c2x,c3x,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_canvas_palette(JsonObject a) {
  if (!a.containsKey("pal")) return { ActionError::MissingParam, "pal" };
  int pal = a["pal"]|0;
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::canvas_setPalette(pal,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_canvas_fade(JsonObject a) {
  if (!a.containsKey("bri")) return { ActionError::MissingParam, "bri" };
  if (!a.containsKey("ms")) return { ActionError::MissingParam, "ms" };
  int bri = a["bri"]|0;
  int ms = a["ms"]|0;
  bool sync = a["sync"]|false;
  bool ok = false;
  if (sync) ok = KinoAPI::canvas_fadeSync(bri,ms);
  if(!sync) ok = KinoAPI::canvas_fadeAsync(bri,ms);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_canvas_solid(JsonObject a) {
  int r,g,b;
  if ((a.containsKey("col"))&&(a["col"].is<JsonArray>())) {
    r = a["col"][0]|0;
    g = a["col"][1]|0;
    b = a["col"][2]|0;
  } else if ((a.containsKey("r"))&&(a.containsKey("g"))&&(a.containsKey("b"))) {
    r = a["r"]|0;
    g = a["g"]|0;
    b = a["b"]|0;
  } else return { ActionError::MissingParam, "col or r,g,b" };
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::canvas_setSolid(r,g,b,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_canvas_music(JsonObject a) {
  bool ok = KinoAPI::canvas_setMusicEffect();
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_canvas_lightning(JsonObject a) {
  bool ok = KinoAPI::canvas_setLightningEffect();
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_canvas_alarm(JsonObject a) {
  if (!a.containsKey("on")) return { ActionError::MissingParam, "on" };
  bool onoff = a["on"]|false;
  bool ok = false;
  if (onoff) ok = KinoAPI::canvas_setAlarm();
  if(!onoff) ok = KinoAPI::canvas_stopAlarm();
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_canvas_pause(JsonObject a) {
  if (!a.containsKey("on")) return { ActionError::MissingParam, "on" };
  bool onoff = a["on"]|false;
  bool ok = false;
  if (onoff) ok = KinoAPI::canvas_setPause();
  if(!onoff) ok = KinoAPI::canvas_setResume();
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}


static ActionResult act_sound_power(JsonObject a) {
  if (!a.containsKey("on")) return { ActionError::MissingParam, "on" };
  bool onoff = a["on"]|false;
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::sound_setPower(onoff,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_sound_brightness(JsonObject a) {
  if (!a.containsKey("bri")) return { ActionError::MissingParam, "bri" };
  int bri = a["bri"]|1;
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::sound_setBrightness(bri,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_sound_effect(JsonObject a) {
  if (!a.containsKey("effect")) return { ActionError::MissingParam, "effect" };
  int effect = a["effect"]|1;
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::sound_setEffect(effect,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_sound_speed(JsonObject a) {
  if (!a.containsKey("speed")) return { ActionError::MissingParam, "speed" };
  int sx = a["speed"]|1;
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::sound_setSpeed(sx,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_sound_intensity(JsonObject a) {
  if (!a.containsKey("intensity")) return { ActionError::MissingParam, "intensity" };
  int intensity = a["intensity"]|1;
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::sound_setIntensity(intensity,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_sound_live(JsonObject a) {
  if (!a.containsKey("on")) return { ActionError::MissingParam, "on" };
  bool onoff = a["on"]|false;
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::sound_setLive(onoff,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_sound_tt(JsonObject a) {
  if (!a.containsKey("tt")) return { ActionError::MissingParam, "tt" };
  int tt = a["tt"]|1;
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::sound_setTransitionTime(tt,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_sound_fgcol(JsonObject a) {
  int r,g,b;
  if ((a.containsKey("col"))&&(a["col"].is<JsonArray>())) {
    r = a["col"][0]|0;
    g = a["col"][1]|0;
    b = a["col"][2]|0;
  } else if ((a.containsKey("r"))&&(a.containsKey("g"))&&(a.containsKey("b"))) {
    r = a["r"]|0;
    g = a["g"]|0;
    b = a["b"]|0;
  } else return { ActionError::MissingParam, "col or r,g,b" };
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::sound_setFgColor(r,g,b,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_sound_bgcol(JsonObject a) {
  int r,g,b;
  if ((a.containsKey("col"))&&(a["col"].is<JsonArray>())) {
    r = a["col"][0]|0;
    g = a["col"][1]|0;
    b = a["col"][2]|0;
  } else if ((a.containsKey("r"))&&(a.containsKey("g"))&&(a.containsKey("b"))) {
    r = a["r"]|0;
    g = a["g"]|0;
    b = a["b"]|0;
  } else return { ActionError::MissingParam, "col or r,g,b" };
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::sound_setBgColor(r,g,b,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_sound_fxcol(JsonObject a) {
  int r,g,b;
  if ((a.containsKey("col"))&&(a["col"].is<JsonArray>())) {
    r = a["col"][0]|0;
    g = a["col"][1]|0;
    b = a["col"][2]|0;
  } else if ((a.containsKey("r"))&&(a.containsKey("g"))&&(a.containsKey("b"))) {
    r = a["r"]|0;
    g = a["g"]|0;
    b = a["b"]|0;
  } else return { ActionError::MissingParam, "col or r,g,b" };
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::sound_setFxColor(r,g,b,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_sound_custom(JsonObject a) {
  int c1x = a["c1x"] | 0;
  int c2x = a["c2x"] | 0;
  int c3x = a["c3x"] | 0;
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::sound_setCustomParams(c1x,c2x,c3x,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_sound_palette(JsonObject a) {
  if (!a.containsKey("pal")) return { ActionError::MissingParam, "pal" };
  int pal = a["pal"]|1;
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::sound_setPalette(pal,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_sound_fade(JsonObject a) {
  if (!a.containsKey("bri")) return { ActionError::MissingParam, "bri" };
  if (!a.containsKey("ms")) return { ActionError::MissingParam, "ms" };
  int bri = a["bri"]|1;
  int ms = a["ms"]|1;
  bool sync = a["sync"]|false;
  bool ok = false;
  if (sync) ok = KinoAPI::sound_fadeSync(bri,ms);
  if(!sync) ok = KinoAPI::sound_fadeAsync(bri,ms);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};

}
static ActionResult act_sound_solid(JsonObject a) {
  int r,g,b;
  if ((a.containsKey("col"))&&(a["col"].is<JsonArray>())) {
    r = a["col"][0]|0;
    g = a["col"][1]|0;
    b = a["col"][2]|0;
  } else if ((a.containsKey("r"))&&(a.containsKey("g"))&&(a.containsKey("b"))) {
    r = a["r"]|0;
    g = a["g"]|0;
    b = a["b"]|0;
  } else return { ActionError::MissingParam, "col or r,g,b" };
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::sound_setSolid(r,g,b,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_sound_music(JsonObject a) {
  bool ok = KinoAPI::sound_setMusicEffect();
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_sound_lightning(JsonObject a) {
  bool ok = KinoAPI::sound_setLightningEffect();
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_sound_alarm(JsonObject a) {
  if (!a.containsKey("on")) return { ActionError::MissingParam, "on" };
  bool onoff = a["on"]|false;
  bool ok = false;
  if (onoff) ok = KinoAPI::sound_setAlarm();
  if(!onoff) ok = KinoAPI::sound_stopAlarm();
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_sound_pause(JsonObject a) {
  if (!a.containsKey("on")) return { ActionError::MissingParam, "on" };
  bool onoff = a["on"]|false;
  bool ok = false;
  if (onoff) ok = KinoAPI::sound_setPause();
  if(!onoff) ok = KinoAPI::sound_setResume();
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_hyperion_broadcast(JsonObject a) {
  if (!a.containsKey("on")) return { ActionError::MissingParam, "on" };
  bool onoff = a["on"]|false;
  bool ok = false;
  if (onoff) ok = KinoAPI::hyperion_startBroadcast();
  if(!onoff) ok = KinoAPI::hyperion_stopBroadcast();
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_huelight_power(JsonObject a) {
  if (!a.containsKey("on")) return { ActionError::MissingParam, "on" };
  if (!a.containsKey("light")) return { ActionError::MissingParam, "light" };
  String light = a["light"]|"";
  bool onoff = a["on"]|false;
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::hueLight_setPower(light,onoff,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_huelight_brightness(JsonObject a) {
  if (!a.containsKey("bri")) return { ActionError::MissingParam, "bri" };
  if (!a.containsKey("light")) return { ActionError::MissingParam, "light" };
  String light = a["light"]|"";
  int bri = a["bri"]|1;
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::hueLight_setBri(light,bri,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_huelight_temp(JsonObject a) {
  if (!a.containsKey("ct")) return { ActionError::MissingParam, "ct" };
  if (!a.containsKey("light")) return { ActionError::MissingParam, "light" };
  String light = a["light"]|"";
  int ct = a["ct"]|1;
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::hueLight_setCT(light,ct,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_huelight_color(JsonObject a) {
  if (!a.containsKey("light")) return { ActionError::MissingParam, "light" };
  int r,g,b;
  String light = a["light"]|"";
  if ((a.containsKey("col"))&&(a["col"].is<JsonArray>())) {
    r = a["col"][0]|0;
    g = a["col"][1]|0;
    b = a["col"][2]|0;
  } else if ((a.containsKey("r"))&&(a.containsKey("g"))&&(a.containsKey("b"))) {
    r = a["r"]|0;
    g = a["g"]|0;
    b = a["b"]|0;
  } else return { ActionError::MissingParam, "col or r,g,b" };
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::hueLight_setRGB(light,r,g,b,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_huegroup_power(JsonObject a) {
  if (!a.containsKey("on")) return { ActionError::MissingParam, "on" };
  if (!a.containsKey("group")) return { ActionError::MissingParam, "group" };
  String group = a["group"]|"";
  bool onoff = a["on"]|false;
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::hueGroup_setPower(group,onoff,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_huegroup_brightness(JsonObject a) {
  if (!a.containsKey("bri")) return { ActionError::MissingParam, "bri" };
  if (!a.containsKey("group")) return { ActionError::MissingParam, "group" };
  String group = a["group"]|"";
  int bri = a["bri"]|1;
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::hueGroup_setBri(group,bri,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_huegroup_temp(JsonObject a) {
  if (!a.containsKey("ct")) return { ActionError::MissingParam, "ct" };
  if (!a.containsKey("group")) return { ActionError::MissingParam, "group" };
  String group = a["group"]|"";
  int ct = a["ct"]|1;
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::hueGroup_setCT(group,ct,commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_huescene_set(JsonObject a) {
  if (!a.containsKey("scene")) return { ActionError::MissingParam, "scene" };
  String scene = a["scene"]|"";
  bool ok = KinoAPI::hueScene_set(scene);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_huescene_save(JsonObject a) {
  if (!a.containsKey("scene")) return { ActionError::MissingParam, "scene" };
  String scene = a["scene"]|"";
  bool ok = KinoAPI::hueScene_save(scene);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_huesensor_set(JsonObject a) {
  if (!a.containsKey("sensor")) return { ActionError::MissingParam, "sensor" };
  if (!a.containsKey("key")) return { ActionError::MissingParam, "key" };
  if (!a.containsKey("val")) return { ActionError::MissingParam, "val" };
  String sensor = a["sensor"]|"";
  String key = a["key"]|"";
  int val = a["val"]|0;
  bool commit = a["commit"]|false;
  bool ok = KinoAPI::hueSensor_setState(sensor, key, val, commit);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_kino_radio(JsonObject a) {
  bool ok = KinoAPI::radioMode();
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_kino_bluray(JsonObject a) {
  bool ok = KinoAPI::blurayMode();
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_kino_streaming(JsonObject a) {
  bool ok = KinoAPI::streamingMode();
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_kino_gaming(JsonObject a) {
  bool ok = KinoAPI::GamingMode();
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_kino_pause(JsonObject a) {
  if (!a.containsKey("on")) return { ActionError::MissingParam, "on" };
  bool onoff = a["on"]|false;
  bool ok = false;
  if (onoff) ok = KinoAPI::startPause();
  if(!onoff) ok = KinoAPI::endPause();
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}
static ActionResult act_kino_power(JsonObject a) {
  if (!a.containsKey("on")) return { ActionError::MissingParam, "on" };
  bool onoff = a["on"]|false;
  bool ok = KinoAPI::Power(onoff);
  if (!ok) return { ActionError::ExecutionFailed, "" };
  return {ActionError::OK,""};
}


// === Lookup Table ===


static const MacroActions::ActionEntry actionTable[] = {
    {"yamaha_power"            ,act_yamaha_power            },
    {"yamaha_volume"            ,act_yamaha_volume           },
    {"yamaha_volume_percent"    ,act_yamaha_volume_percent   },
    {"yamaha_input"           ,act_yamaha_input            },
    {"yamaha_dsp"             ,act_yamaha_dsp              },
    {"yamaha_straight"          ,act_yamaha_straight         },
    {"yamaha_enhancer"          ,act_yamaha_enhancer         },
    {"yamaha_mute"            ,act_yamaha_mute             },
    {"yamaha_station"           ,act_yamaha_station          },
    {"beamer_power"           ,act_beamer_power            },
    {"beamer_input"           ,act_beamer_input            },
    {"canvas_power"           ,act_canvas_power            },
    {"canvas_brightness"        ,act_canvas_brightness       },
    {"canvas_effect"            ,act_canvas_effect           },
    {"canvas_speed"           ,act_canvas_speed            },
    {"canvas_intensity"         ,act_canvas_intensity        },
    {"canvas_live"            ,act_canvas_live             },
    {"canvas_tt"              ,act_canvas_tt               },
    {"canvas_fgcol"           ,act_canvas_fgcol            },
    {"canvas_bgcol"           ,act_canvas_bgcol            },
    {"canvas_fxcol"           ,act_canvas_fxcol            },
    {"canvas_custom"            ,act_canvas_custom           },
    {"canvas_palette"           ,act_canvas_palette          },
    {"canvas_fade"            ,act_canvas_fade             },
    {"canvas_solid"           ,act_canvas_solid            },
    {"canvas_music"           ,act_canvas_music            },
    {"canvas_lightning"         ,act_canvas_lightning        },
    {"canvas_alarm"           ,act_canvas_alarm            },
    {"canvas_pause"           ,act_canvas_pause            },
    {"sound_power"            ,act_sound_power             },
    {"sound_brightness"         ,act_sound_brightness        },
    {"sound_effect"           ,act_sound_effect            },
    {"sound_speed"            ,act_sound_speed             },
    {"sound_intensity"          ,act_sound_intensity         },
    {"sound_live"             ,act_sound_live              },
    {"sound_tt"             ,act_sound_tt                },
    {"sound_fgcol"            ,act_sound_fgcol             },
    {"sound_bgcol"            ,act_sound_bgcol             },
    {"sound_fxcol"            ,act_sound_fxcol             },
    {"sound_custom"           ,act_sound_custom            },
    {"sound_palette"            ,act_sound_palette           },
    {"sound_fade"             ,act_sound_fade              },
    {"sound_solid"            ,act_sound_solid             },
    {"sound_music"            ,act_sound_music             },
    {"sound_alarm"            ,act_sound_alarm             },
    {"sound_pause"            ,act_sound_pause             },
    {"hyperion_broadcast"       ,act_hyperion_broadcast      },
    {"huelight_power"           ,act_huelight_power          },
    {"huelight_brightness"      ,act_huelight_brightness     },
    {"huelight_temp"            ,act_huelight_temp           },
    {"huelight_color"           ,act_huelight_color          },
    {"huegroup_power"           ,act_huegroup_power          },
    {"huegroup_brightness"      ,act_huegroup_brightness     },
    {"huegroup_temp"            ,act_huegroup_temp           },
    {"huescene_set"           ,act_huescene_set            },
    {"huescene_save"            ,act_huescene_save           },
    {"huesensor_set"            ,act_huesensor_set           },
    {"kino_radio"             ,act_kino_radio              },
    {"kino_bluray"            ,act_kino_bluray             },
    {"kino_streaming"           ,act_kino_streaming          },
    {"kino_gaming"            ,act_kino_gaming             },
    {"kino_pause"             ,act_kino_pause              },
    {"kino_power"             ,act_kino_power              },
    { nullptr, nullptr }
};

static constexpr size_t actionCount =
    sizeof(actionTable) / sizeof(actionTable[0]);

// === Dispatcher ===

ActionResult MacroActions::execute(const JsonObject& action) {
  if (!action.containsKey("cmd")) {
    return { ActionError::MissingCmd, "missing cmd" };
  }

  const char* cmd = action["cmd"];

  for (size_t i = 0; actionTable[i].cmd; ++i) {
    if (strcmp(cmd, actionTable[i].cmd) == 0) {
      return actionTable[i].handler(action);
    }
  }

  return { ActionError::UnknownCommand, cmd };
}
