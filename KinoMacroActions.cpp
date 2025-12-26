#include "KinoMacroActions.h"
#include "KinoAPI.h"

// === Handler ===
// Muster:
/*
static bool act_yamaha_power(JsonObject a) {
    if (!a.containsKey("on")) return false;
    return KinoAPI::yamahaPower(a["on"].as<bool>());
}
*/
// "yamaha_power"             => KinoAPI::yamaha_setPower(a["on"] | false);
static bool act_yamaha_power(JsonObject a) {
  if (!a.containsKey("on")) return false;
  bool onoff = a["on"]|false;
  return KinoAPI::yamaha_setPower(onoff);
}
static bool act_yamaha_volume(JsonObject a) {
  if (!a.containsKey("vol")) return false;
  int vol = a["vol"]|(-800);
  return KinoAPI::yamaha_setVolume(vol);
}
static bool act_yamaha_volume_percent(JsonObject a) {
  if (!a.containsKey("percent")) return false;
  int pct = a["percent"]|20;
  return KinoAPI::yamaha_setVolumePercent(pct);
}
static bool act_yamaha_input(JsonObject a) {
  if (!a.containsKey("input")) return false;
  String input=a["input"]|"";
  return KinoAPI::yamaha_setInput(input);
}
static bool act_yamaha_dsp(JsonObject a) {
  if (!a.containsKey("dsp")) return false;
  String dsp = a["dsp"]|"";
  return KinoAPI::yamaha_setDsp(dsp);
}
static bool act_yamaha_straight(JsonObject a) {
  if (!a.containsKey("on")) return false;
  bool onoff = a["on"]|false;
  return KinoAPI::yamaha_setStraight(onoff);
}
static bool act_yamaha_enhancer(JsonObject a) {
  if (!a.containsKey("on")) return false;
  bool onoff = a["on"]|false;
  return KinoAPI::yamaha_setEnhancer(onoff);
}
static bool act_yamaha_mute(JsonObject a) {
  if (!a.containsKey("on")) return false;
  bool onoff = a["on"]|false;
  return KinoAPI::yamaha_setMute(onoff);
}
static bool act_yamaha_station(JsonObject a) {
  if (!a.containsKey("station")) return false;
  String station = a["station"]|"";
  return KinoAPI::yamaha_setStation(station);
}
static bool act_beamer_power(JsonObject a) {
  if (!a.containsKey("on")) return false;
  bool onoff = a["on"]|false;
  return KinoAPI::beamer_setPower(onoff);
}
static bool act_beamer_input(JsonObject a) {
  if (!a.containsKey("input")) return false;
  String input = a["input"]|"";
  return KinoAPI::beamer_setInput(input);
}
static bool act_canvas_power(JsonObject a) {
  if (!a.containsKey("on")) return false;
  bool onoff = a["on"]|false;
  bool commit = a["commit"]|false;
  return KinoAPI::canvas_setPower(onoff,commit);
}
static bool act_canvas_brightness(JsonObject a) {
  if (!a.containsKey("bri")) return false;
  int bri = a["bri"]|1;
  bool commit = a["commit"]|false;
  return KinoAPI::canvas_setBrightness(bri,commit);
}
static bool act_canvas_effect(JsonObject a) {
  if (!a.containsKey("effect")) return false;
  int effect = a["effect"]|0;
  bool commit = a["commit"]|false;
  return KinoAPI::canvas_setEffect(effect,commit);
}
static bool act_canvas_speed(JsonObject a) {
  if (!a.containsKey("speed")) return false;
  int sx = a["sx"]|1;
  bool commit = a["commit"]|false;
  return KinoAPI::canvas_setSpeed(sx,commit);
}
static bool act_canvas_intensity(JsonObject a) {
  if (!a.containsKey("intensity")) return false;
  int si = a["intensity"]|1;
  bool commit = a["commit"]|false;
  return KinoAPI::canvas_setIntensity(si,commit);
}
static bool act_canvas_live(JsonObject a) {
  if (!a.containsKey("on")) return false;
  bool onoff = a["on"]|false;
  bool commit = a["commit"]|false;
  return KinoAPI::canvas_setLive(onoff,commit);
}
static bool act_canvas_tt(JsonObject a) {
  if (!a.containsKey("tt")) return false;
  int tt = a["tt"]|1;
  bool commit = a["commit"]|false;
  return KinoAPI::canvas_setTransitionTime(tt,commit);
}
static bool act_canvas_fgcol(JsonObject a) {
  int r,g,b;
  if ((a.containsKey("col"))&&(a["col"].is<JsonArray>())) {
    r = a["col"][0]|0;
    g = a["col"][1]|0;
    b = a["col"][2]|0;
  } else if ((a.containsKey("r"))&&(a.containsKey("g"))&&(a.containsKey("b"))) {
    r = a["r"]|0;
    g = a["g"]|0;
    b = a["b"]|0;
  } else return false;
  bool commit = a["commit"]|false;
  return KinoAPI::canvas_setFgColor(r,g,b,commit);
}
static bool act_canvas_bgcol(JsonObject a) {
  int r,g,b;
  if ((a.containsKey("col"))&&(a["col"].is<JsonArray>())) {
    r = a["col"][0]|0;
    g = a["col"][1]|0;
    b = a["col"][2]|0;
  } else if ((a.containsKey("r"))&&(a.containsKey("g"))&&(a.containsKey("b"))) {
    r = a["r"]|0;
    g = a["g"]|0;
    b = a["b"]|0;
  } else return false;
  bool commit = a["commit"]|false;
  return KinoAPI::canvas_setBgColor(r,g,b,commit);
}
static bool act_canvas_fxcol(JsonObject a) {
  int r,g,b;
  if ((a.containsKey("col"))&&(a["col"].is<JsonArray>())) {
    r = a["col"][0]|0;
    g = a["col"][1]|0;
    b = a["col"][2]|0;
  } else if ((a.containsKey("r"))&&(a.containsKey("g"))&&(a.containsKey("b"))) {
    r = a["r"]|0;
    g = a["g"]|0;
    b = a["b"]|0;
  } else return false;
  bool commit = a["commit"]|false;
  return KinoAPI::canvas_setFxColor(r,g,b,commit);
}
static bool act_canvas_custom(JsonObject a) {
  int c1x = a["c1x"] | 0;
  int c2x = a["c2x"] | 0;
  int c3x = a["c3x"] | 0;
  bool commit = a["commit"]|false;
  return KinoAPI::canvas_setCustomParams(c1x,c2x,c3x,commit);
}
static bool act_canvas_palette(JsonObject a) {
  if (!a.containsKey("pal")) return false;
  int pal = a["pal"]|0;
  bool commit = a["commit"]|false;
  return KinoAPI::canvas_setPalette(pal,commit);
}
static bool act_canvas_fade(JsonObject a) {
  if (!a.containsKey("bri")) return false;
  if (!a.containsKey("ms")) return false;
  int bri = a["bri"]|0;
  int ms = a["ms"]|0;
  bool sync = a["sync"]|false;
  if (sync) return KinoAPI::canvas_fadeSync(bri,ms);
  return KinoAPI::canvas_fadeAsync(bri,ms);
}
static bool act_canvas_solid(JsonObject a) {
  int r,g,b;
  if ((a.containsKey("col"))&&(a["col"].is<JsonArray>())) {
    r = a["col"][0]|0;
    g = a["col"][1]|0;
    b = a["col"][2]|0;
  } else if ((a.containsKey("r"))&&(a.containsKey("g"))&&(a.containsKey("b"))) {
    r = a["r"]|0;
    g = a["g"]|0;
    b = a["b"]|0;
  } else return false;
  bool commit = a["commit"]|false;
  return KinoAPI::canvas_setSolid(r,g,b,commit);
}
static bool act_canvas_music(JsonObject a) {
  return KinoAPI::canvas_setMusicEffect();
}
static bool act_canvas_lightning(JsonObject a) {
  return KinoAPI::canvas_setLightningEffect();
}
static bool act_canvas_alarm(JsonObject a) {
  if (!a.containsKey("on")) return false;
  bool onoff = a["on"]|false;
  if (onoff) return KinoAPI::canvas_setAlarm();
  return KinoAPI::canvas_stopAlarm();
}
static bool act_canvas_pause(JsonObject a) {
  if (!a.containsKey("on")) return false;
  bool onoff = a["on"]|false;
  if (onoff) return KinoAPI::canvas_setPause();
  return KinoAPI::canvas_setResume();
}


static bool act_sound_power(JsonObject a) {
  if (!a.containsKey("on")) return false;
  bool onoff = a["on"]|false;
  bool commit = a["commit"]|false;
  return KinoAPI::sound_setPower(onoff,commit);
}
static bool act_sound_brightness(JsonObject a) {
  if (!a.containsKey("bri")) return false;
  int bri = a["bri"]|1;
  bool commit = a["commit"]|false;
  return KinoAPI::sound_setBrightness(bri,commit);
}
static bool act_sound_effect(JsonObject a) {
  if (!a.containsKey("effect")) return false;
  int effect = a["effect"]|1;
  bool commit = a["commit"]|false;
  return KinoAPI::sound_setEffect(effect,commit);
}
static bool act_sound_speed(JsonObject a) {
  if (!a.containsKey("speed")) return false;
  int sx = a["speed"]|1;
  bool commit = a["commit"]|false;
  return KinoAPI::sound_setSpeed(sx,commit);
}
static bool act_sound_intensity(JsonObject a) {
  if (!a.containsKey("intensity")) return false;
  int intensity = a["intensity"]|1;
  bool commit = a["commit"]|false;
  return KinoAPI::sound_setIntensity(intensity,commit);
}
static bool act_sound_live(JsonObject a) {
  if (!a.containsKey("on")) return false;
  bool onoff = a["on"]|false;
  bool commit = a["commit"]|false;
  return KinoAPI::sound_setLive(onoff,commit);
}
static bool act_sound_tt(JsonObject a) {
  if (!a.containsKey("tt")) return false;
  int tt = a["tt"]|1;
  bool commit = a["commit"]|false;
  return KinoAPI::sound_setTransitionTime(tt,commit);
}
static bool act_sound_fgcol(JsonObject a) {
  int r,g,b;
  if ((a.containsKey("col"))&&(a["col"].is<JsonArray>())) {
    r = a["col"][0]|0;
    g = a["col"][1]|0;
    b = a["col"][2]|0;
  } else if ((a.containsKey("r"))&&(a.containsKey("g"))&&(a.containsKey("b"))) {
    r = a["r"]|0;
    g = a["g"]|0;
    b = a["b"]|0;
  } else return false;
  bool commit = a["commit"]|false;
  return KinoAPI::sound_setFgColor(r,g,b,commit);
}
static bool act_sound_bgcol(JsonObject a) {
  int r,g,b;
  if ((a.containsKey("col"))&&(a["col"].is<JsonArray>())) {
    r = a["col"][0]|0;
    g = a["col"][1]|0;
    b = a["col"][2]|0;
  } else if ((a.containsKey("r"))&&(a.containsKey("g"))&&(a.containsKey("b"))) {
    r = a["r"]|0;
    g = a["g"]|0;
    b = a["b"]|0;
  } else return false;
  bool commit = a["commit"]|false;
  return KinoAPI::sound_setBgColor(r,g,b,commit);
}
static bool act_sound_fxcol(JsonObject a) {
  int r,g,b;
  if ((a.containsKey("col"))&&(a["col"].is<JsonArray>())) {
    r = a["col"][0]|0;
    g = a["col"][1]|0;
    b = a["col"][2]|0;
  } else if ((a.containsKey("r"))&&(a.containsKey("g"))&&(a.containsKey("b"))) {
    r = a["r"]|0;
    g = a["g"]|0;
    b = a["b"]|0;
  } else return false;
  bool commit = a["commit"]|false;
  return KinoAPI::sound_setFxColor(r,g,b,commit);
}
static bool act_sound_custom(JsonObject a) {
  int c1x = a["c1x"] | 0;
  int c2x = a["c2x"] | 0;
  int c3x = a["c3x"] | 0;
  bool commit = a["commit"]|false;
  return KinoAPI::sound_setCustomParams(c1x,c2x,c3x,commit);
}
static bool act_sound_palette(JsonObject a) {
  if (!a.containsKey("pal")) return false;
  int pal = a["pal"]|1;
  bool commit = a["commit"]|false;
  return KinoAPI::sound_setPalette(pal,commit);
}
static bool act_sound_fade(JsonObject a) {
  if (!a.containsKey("bri")) return false;
  if (!a.containsKey("ms")) return false;
  int bri = a["bri"]|1;
  int ms = a["ms"]|1;
  bool sync = a["sync"]|false;
  if (sync) return KinoAPI::sound_fadeSync(bri,ms);
  return KinoAPI::sound_fadeAsync(bri,ms);
}
static bool act_sound_solid(JsonObject a) {
  int r,g,b;
  if ((a.containsKey("col"))&&(a["col"].is<JsonArray>())) {
    r = a["col"][0]|0;
    g = a["col"][1]|0;
    b = a["col"][2]|0;
  } else if ((a.containsKey("r"))&&(a.containsKey("g"))&&(a.containsKey("b"))) {
    r = a["r"]|0;
    g = a["g"]|0;
    b = a["b"]|0;
  } else return false;
  bool commit = a["commit"]|false;
  return KinoAPI::sound_setSolid(r,g,b,commit);
}
static bool act_sound_music(JsonObject a) {
  return KinoAPI::sound_setMusicEffect();
}
static bool act_sound_lightning(JsonObject a) {
  return KinoAPI::sound_setLightningEffect();
}
static bool act_sound_alarm(JsonObject a) {
  if (!a.containsKey("on")) return false;
  bool onoff = a["on"]|false;
  if (onoff) return KinoAPI::sound_setAlarm();
  return KinoAPI::sound_stopAlarm();
}
static bool act_sound_pause(JsonObject a) {
  if (!a.containsKey("on")) return false;
  bool onoff = a["on"]|false;
  if (onoff) return KinoAPI::sound_setPause();
  return KinoAPI::sound_setResume();
}
static bool act_hyperion_broadcast(JsonObject a) {
  if (!a.containsKey("on")) return false;
  bool onoff = a["on"]|false;
  if (onoff) return KinoAPI::hyperion_startBroadcast();
  return KinoAPI::hyperion_stopBroadcast();
}
static bool act_huelight_power(JsonObject a) {
  if (!a.containsKey("on")) return false;
  if (!a.containsKey("light")) return false;
  String light = a["light"]|"";
  bool onoff = a["on"]|false;
  bool commit = a["commit"]|false;
  return KinoAPI::hueLight_setPower(light,onoff,commit);
}
static bool act_huelight_brightness(JsonObject a) {
  if (!a.containsKey("bri")) return false;
  if (!a.containsKey("light")) return false;
  String light = a["light"]|"";
  int bri = a["bri"]|1;
  bool commit = a["commit"]|false;
  return KinoAPI::hueLight_setBri(light,bri,commit);
}
static bool act_huelight_temp(JsonObject a) {
  if (!a.containsKey("ct")) return false;
  if (!a.containsKey("light")) return false;
  String light = a["light"]|"";
  int ct = a["ct"]|1;
  bool commit = a["commit"]|false;
  return KinoAPI::hueLight_setCT(light,ct,commit);
}
static bool act_huelight_color(JsonObject a) {
  if (!a.containsKey("light")) return false;
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
  } else return false;
  bool commit = a["commit"]|false;
  return KinoAPI::hueLight_setRGB(light,r,g,b,commit);
}
static bool act_huegroup_power(JsonObject a) {
  if (!a.containsKey("on")) return false;
  if (!a.containsKey("group")) return false;
  String group = a["group"]|"";
  bool onoff = a["on"]|false;
  bool commit = a["commit"]|false;
  return KinoAPI::hueGroup_setPower(group,onoff,commit);
}
static bool act_huegroup_brightness(JsonObject a) {
  if (!a.containsKey("bri")) return false;
  if (!a.containsKey("group")) return false;
  String group = a["group"]|"";
  int bri = a["bri"]|1;
  bool commit = a["commit"]|false;
  return KinoAPI::hueGroup_setBri(group,bri,commit);
}
static bool act_huegroup_temp(JsonObject a) {
  if (!a.containsKey("ct")) return false;
  if (!a.containsKey("group")) return false;
  String group = a["group"]|"";
  int ct = a["ct"]|1;
  bool commit = a["commit"]|false;
  return KinoAPI::hueGroup_setCT(group,ct,commit);
}
static bool act_huescene_set(JsonObject a) {
  if (!a.containsKey("scene")) return false;
  String scene = a["scene"]|"";
  return KinoAPI::hueScene_set(scene);
}
static bool act_huescene_save(JsonObject a) {
  if (!a.containsKey("scene")) return false;
  String scene = a["scene"]|"";
  return KinoAPI::hueScene_save(scene);
}
static bool act_huesensor_set(JsonObject a) {
  if (!a.containsKey("sensor")) return false;
  if (!a.containsKey("key")) return false;
  if (!a.containsKey("val")) return false;
  String sensor = a["sensor"]|"";
  String key = a["key"]|"";
  int val = a["val"]|0;
  bool commit = a["commit"]|false;
  return KinoAPI::hueSensor_setState(sensor, key, val, commit);
}
static bool act_kino_radio(JsonObject a) {
  return KinoAPI::radioMode();
}
static bool act_kino_bluray(JsonObject a) {
  return KinoAPI::blurayMode();
}
static bool act_kino_streaming(JsonObject a) {
  return KinoAPI::streamingMode();
}
static bool act_kino_gaming(JsonObject a) {
  return KinoAPI::GamingMode();
}
static bool act_kino_pause(JsonObject a) {
  if (!a.containsKey("on")) return false;
  bool onoff = a["on"]|false;
  return ((onoff) ? KinoAPI::startPause() : KinoAPI::endPause());
}
static bool act_kino_power(JsonObject a) {
  if (!a.containsKey("on")) return false;
  bool onoff = a["on"]|false;
  return KinoAPI::Power(onoff);
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
};

static constexpr size_t actionCount =
    sizeof(actionTable) / sizeof(actionTable[0]);

// === Dispatcher ===

bool MacroActions::execute(JsonObject action) {
    if (!action.containsKey("cmd")) return false;

    const char* cmd = action["cmd"];

    for (size_t i = 0; i < actionCount; ++i) {
        if (strcmp(cmd, actionTable[i].cmd) == 0) {
            return actionTable[i].handler(action);
        }
    }
    Serial.print("unknown cmd: ");
    Serial.println(cmd);
    return _unknown(action);
}

bool MacroActions::_unknown(JsonObject action) {
    return false;
}
