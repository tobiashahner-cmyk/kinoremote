#include "HueBridge.h"
#include <ArduinoJson.h>


// ===== Konstruktoren =====

HueBridge::HueBridge(WiFiClient& wfc, const IPAddress& ip, const String& user)
: _client(wfc), _ip(ip), _user(user) {}

HueBridge::HueBridge(WiFiClient& wfc, const String& ip, const String& user)
: _client(wfc), _user(user) {
  _ip.fromString(ip);
}


/**
 * Zerlegt einen Pfad im Format "dev/name/act" in drei Teile.
 * Gibt true zurück, wenn alle 3 Teile gefunden wurden und in die Puffer passten.
 */
bool HueBridge::splitPath(const char* input, char* dev, size_t devLen, char* name, size_t nameLen, char* act, size_t actLen) {
    if (!input) return false;

    // 1. Finde ersten Doppelpunkt
    const char* firstColon = strchr(input, '/');
    if (!firstColon) return false;

    // 2. Finde zweiten Doppelpunkt
    const char* secondColon = strchr(firstColon + 1, '/');
    if (!secondColon) return false;

    // Längen berechnen
    size_t dLen = firstColon - input;
    size_t nLen = secondColon - (firstColon + 1);
    size_t aLen = strlen(secondColon + 1);

    // Prüfen, ob die Zielpuffer groß genug sind (inklusive Null-Terminator)
    if (dLen >= devLen || nLen >= nameLen || aLen >= actLen) return false;

    // Teile kopieren
    strncpy(dev, input, dLen);
    dev[dLen] = '\0';

    strncpy(name, firstColon + 1, nLen);
    name[nLen] = '\0';

    strcpy(act, secondColon + 1); // act ist der Rest bis zum Ende

    return true;
}

const KinoPropertyInfo HueBridge::_properties[] = {
  { "on",         "Power",      Prop_Read  | Prop_Write           },
  { "bri",        "Helligkeit", Prop_Read  | Prop_Write           },
  { "lights",     "Lampen",     Prop_Query | Prop_hasLabel | Prop_hasParams },
  { "groups",     "Gruppen",    Prop_Query | Prop_hasLabel | Prop_hasParams },
  { "sensors",    "Sensoren",   Prop_Query | Prop_hasLabel | Prop_hasParams },
  { "scenes",     "Szenen",     Prop_Query | Prop_hasLabel | Prop_hasParams },
  { "ip",         "IP",         Prop_Read                         },
  { "daylight",   "Tageslicht", Prop_Read | Prop_Status                        },
  { "temp",       "Temperatur", Prop_Read | Prop_Status                        },
  { "tickInterval", "Aktualisierung [ms]", Prop_Read | Prop_Write , 0 , 20000, 500}
};

size_t HueBridge::getPropertyCount() const {
  return sizeof(_properties) / sizeof(_properties[0]);
}

const KinoPropertyInfo* HueBridge::getPropertyInfo(size_t index) const {
  if (index >= getPropertyCount()) return nullptr;
  return &_properties[index];
}

KinoError HueBridge::get(const char* prop, KinoVariant& out) {
  // zuerst Einstellungen, die technisch die Gruppe "ALL" betreffen
  if (strcmp(prop,"tickInterval")==0){
    out = KinoVariant::fromInt(_tickInterval);
    return KinoError::OK;
  }
  if (strcmp(prop,"ip")==0) {
    out = KinoVariant::fromString(_ip.toString().c_str());
    return KinoError::OK;
  }
  if ((strcmp(prop,"power")==0)||(strcmp(prop,"anyon")==0)||(strcmp(prop,"on")==0)){
    for (auto* l : _lights) {
      if (l->isOn()) {
        out = KinoVariant::fromBool(true);
        return KinoError::OK;
      }
    }
    out = KinoVariant::fromBool(false);
    return KinoError::OK;
  }
  if ((strcmp(prop,"brightness")==0)||(strcmp(prop,"bri")==0)) {
    int totalBri = 0; float averageBri=0; int nrOfLights=0;
    for (auto* l : _lights) {
      if (l->isOn()) {
        if (l->isDimmable()) {
          totalBri += l->getBrightness();
        } else {
          totalBri += 255;
        }
      }
      nrOfLights++;
    }
    if (nrOfLights == 0) return KinoError::DeviceNotReady;
    averageBri = (totalBri / nrOfLights);
    out = KinoVariant::fromInt(averageBri);
    //out = KinoVariant::fromInt(totalBri);
    return KinoError::OK;
  }
  if ((strcmp(prop,"powerall")==0)||(strcmp(prop,"allon")==0)) {
    for (auto* l : _lights) {
      if (!l->isOn()) {
        out = KinoVariant::fromBool(false);
        return KinoError::OK;
      }
    }
    out = KinoVariant::fromBool(true);
    return KinoError::OK;
  }
  if ((strcmp(prop,"temperature")==0)||(strcmp(prop,"temp")==0)) {
    int temp = 0;
    int foundSensors = 0;
    for (auto* s : _sensors) {
      if (s->hasValue("temperature")) {
        temp += s->getValue("temperature").as<int>();
        foundSensors++;
      }
    }
    if (foundSensors == 0) return KinoError::DeviceNotReady;
    float t = temp/foundSensors;
    char buf[32];
    snprintf(buf,32,"%.1f C",(t/100));
    out = KinoVariant::fromString(buf);
    return KinoError::OK;
  }
  if (strcmp(prop, "daylight")==0) {
    for (auto* s : _sensors) {
      if (s->hasValue("daylight")) {
        out = KinoVariant::fromJsonVariant(s->getValue("daylight"));
        return KinoError::OK;
      }
    }
    return KinoError::PropertyNotSupported;
  }
  // prop ist nicht eine der bekannten allgemeingültigen Eigenschaften,
  // also suche nach einem Pfad der Form {deviceClass}/{deviceName}/{action}
  char dev[12];
  char name[32];
  char act[32];
  if (!splitPath(prop, dev, sizeof(dev), name, sizeof(name), act, sizeof(act))) return KinoError::InvalidProperty;
  // deviceClass = "lights" : Es folgt eine {action} für eine bestimmte Lampe {deviceName}
  if (strcmp(dev, "lights") == 0) {
    auto* l = getLightByName(name);
    if (!l) return KinoError::DeviceNotReady;
    if ((strcmp(act,"power")==0)||(strcmp(act,"on")==0)) {
      out = KinoVariant::fromBool(l->isOn());
      return KinoError::OK;
    } 
    if ((strcmp(act,"brightness")==0)||(strcmp(act,"bri")==0)) {
      if (!l->isDimmable()) return KinoError::OutOfRange;
      out = KinoVariant::fromInt(l->getBrightness());
      return KinoError::OK;
    }
    if (strcmp(act, "ct") == 0) {
      if (!l->hasCTColor()) return KinoError::OutOfRange;
      out = KinoVariant::fromInt(l->getCT());
      return KinoError::OK;
    }
    if ((strcmp(act,"color")==0)||(strcmp(act,"rgb")==0)||(strcmp(act,"col")==0)) {
      if (!l->hasXYColor()) return KinoError::OutOfRange;
      RgbColor col = l->getRGB();
      out = KinoVariant::fromColor(col.r, col.g, col.b);
      return KinoError::OK;
    }
    if (strcmp(act,"tt")==0) {
      out = KinoVariant::fromInt(l->getTT());
      return KinoError::OK;
    }
    if (strcmp(act, "id") == 0) {
      out = KinoVariant::fromInt(l->getId());
      return KinoError::OK;
    }
    if (strcmp(act, "label") == 0) {
      out = KinoVariant::fromString(name);
      return KinoError::OK;
    }
    if (strstr(act,"param/")) { // path = "lights/<lightName>/param/<rest>"
      int found, paramIndex; char rest[32];
      found = sscanf(act,"param/%d%s", &paramIndex, rest);
      if ((found==1)||(strlen(rest)==0)) {    // path = "lights/<lightName>/param/<paramIndex>"
        // get the n-th param for the given light
        std::vector<String> params = getLightParams(l);
        if (paramIndex >= params.size()) return KinoError::OutOfRange;
        String getsetPath = "lights/";
        getsetPath += name; getsetPath += "/";
        getsetPath += params[paramIndex];
        out = KinoVariant::fromString(getsetPath.c_str());
        return KinoError::OK;
      }
      if (strcmp(rest,"/label")==0) {
        std::vector<String> params = getLightParams(l);
        if (paramIndex >= params.size()) return KinoError::OutOfRange;
        if (params[paramIndex] == "on") out = KinoVariant::fromString("Power");
        if (params[paramIndex] == "bri") out = KinoVariant::fromString("Helligkeit");
        if (params[paramIndex] == "ct") out = KinoVariant::fromString("Weisston");
        if (params[paramIndex] == "col") out = KinoVariant::fromString("Farbe");
        if (params[paramIndex] == "tt") out = KinoVariant::fromString("Trans.Time[ms]");
        //out = KinoVariant::fromString(params[paramIndex].c_str());
        return KinoError::OK;
      }
      if (strcmp(rest,"/access")==0) {
        out = KinoVariant::fromInt(3);  // alle Parameter sind read/write
      }
      if (strcmp(rest,"/minvalue")==0) {
        int mv = 0;
        std::vector<String> params = getLightParams(l);
        if (paramIndex >= params.size()) return KinoError::OutOfRange;
        if (params[paramIndex] == "bri")  out = KinoVariant::fromInt(0);
        else if (params[paramIndex] == "ct")   out = KinoVariant::fromInt(153);
        else return KinoError::PropertyNotSupported;
        return KinoError::OK;
      }
      if (strcmp(rest,"/maxvalue")==0) {
        int mv = 0;
        std::vector<String> params = getLightParams(l);
        if (paramIndex >= params.size()) return KinoError::OutOfRange;
        if (params[paramIndex] == "bri")  out = KinoVariant::fromInt(255);
        else if (params[paramIndex] == "ct")   out = KinoVariant::fromInt(555);
        else if (params[paramIndex] == "tt")   out = KinoVariant::fromInt(10000);
        else return KinoError::PropertyNotSupported;
        return KinoError::OK;
      }
      if (strcmp(rest,"/valuestep")==0) {
        int mv = 0;
        std::vector<String> params = getLightParams(l);
        if (paramIndex >= params.size()) return KinoError::OutOfRange;
        if (params[paramIndex] == "bri")  out = KinoVariant::fromInt(1);
        else if (params[paramIndex] == "ct")   out = KinoVariant::fromInt(1);
        else if (params[paramIndex] == "tt")   out = KinoVariant::fromInt(100);
        else return KinoError::PropertyNotSupported;
        return KinoError::OK;
      }
    }
    return KinoError::PropertyNotSupported;
  }
  // deviceClass = "groups" : Es folgt eine {action} für eine bestimmte Gruppe {deviceName}
  else if (strcmp(dev, "groups") == 0) {
    auto* g = getGroupByName(name);
    if (!g) return KinoError::DeviceNotReady;
    if ((strcmp(act,"power")==0)||(strcmp(act,"anyon")==0)||(strcmp(act,"on")==0)) {  // this is GET. "on" means "any_on"
      out = KinoVariant::fromBool(g->anyOn());
      return KinoError::OK;
    }
    if (strcmp(act,"bri")==0) {
      HueGroup* g = getGroupByName(name);
      if (!g) return KinoError::DeviceNotReady;
      int totalBri = 0; int totalLights = 0;
      std::vector<uint8_t> lightIds = g->getLightIds();
      for (uint8_t lid : lightIds) {
        auto* l = getLightById(lid);
        if (l->isOn()) {
          if (l->isDimmable()) totalBri += l->getBrightness();
          else totalBri += 255;
        }
        totalLights++;
      }
      float avg = (totalBri/totalLights);
      out = KinoVariant::fromInt((int)avg);
      return KinoError::OK;
    }
    if (strcmp(act,"ct")==0) {
      HueGroup* g = getGroupByName(name);
      if (!g) return KinoError::DeviceNotReady;
      int totalCt = 0; int totalLights = 0;
      std::vector<uint8_t> lightIds = g->getLightIds();
      for (uint8_t lid : lightIds) {
        auto* l = getLightById(lid);
        if (l->isOn()) {
          if (l->hasCTColor()) totalCt += l->getCT();
        }
        totalLights++;
      }
      float avg = (totalCt/totalLights);
      out = KinoVariant::fromInt((int)avg);
      return KinoError::OK;
    }
    if ((strcmp(act,"powerall")==0)||(strcmp(act,"allon")==0)) {
      out = KinoVariant::fromBool(g->allOn());
      return KinoError::OK;
    }
    if (strcmp(act, "label") == 0) {
      out = KinoVariant::fromString(name);
      return KinoError::OK;
    }
    if (strcmp(act,"tt")==0) {
      out = KinoVariant::fromInt(g->getTT());
      return KinoError::OK;
    }
    if (strstr(act,"param/")) { // path = "groups/<groupName>/param/<rest>"
      int found, paramIndex; char rest[32];
      found = sscanf(act,"param/%d%31s", &paramIndex, rest);
      if ((found==1)||(strlen(rest)==0)) {    // path = "groups/<groupName>/param/<paramIndex>"
        // get the n-th param for the given group
        std::vector<String> params = getGroupParams(g);
        if (paramIndex >= params.size()) return KinoError::OutOfRange;
        String getsetPath = "groups/";
        getsetPath += name; getsetPath += "/";
        getsetPath += params[paramIndex];
        out = KinoVariant::fromString(getsetPath.c_str());
        return KinoError::OK;
      }
      if (strcmp(rest,"/label")==0) {
        std::vector<String> params = getGroupParams(g);
        if (paramIndex >= params.size()) return KinoError::OutOfRange;
        if (params[paramIndex] == "on") out = KinoVariant::fromString("Power");
        if (params[paramIndex] == "bri") out = KinoVariant::fromString("Helligkeit");
        if (params[paramIndex] == "ct") out = KinoVariant::fromString("Weisston");
        if (params[paramIndex] == "col") out = KinoVariant::fromString("Farbe");
        if (params[paramIndex] == "tt") out = KinoVariant::fromString("Trans.Time[ms]");
        //out = KinoVariant::fromString(params[paramIndex].c_str());
        return KinoError::OK;
      }
      if (strcmp(rest,"/access")==0) {
        out = KinoVariant::fromInt(3);  // alle Parameter sind read/write
      }
      if (strcmp(rest,"/minvalue")==0) {
        int mv = 0;
        std::vector<String> params = getGroupParams(g);
        if (paramIndex >= params.size()) return KinoError::OutOfRange;
        if (params[paramIndex] == "bri")  out = KinoVariant::fromInt(0);
        else if (params[paramIndex] == "ct")   out = KinoVariant::fromInt(153);
        else return KinoError::PropertyNotSupported;
        return KinoError::OK;
      }
      if (strcmp(rest,"/maxvalue")==0) {
        int mv = 0;
        std::vector<String> params = getGroupParams(g);
        if (paramIndex >= params.size()) return KinoError::OutOfRange;
        if (params[paramIndex] == "bri")  out = KinoVariant::fromInt(255);
        else if (params[paramIndex] == "ct")   out = KinoVariant::fromInt(555);
        else if (params[paramIndex] == "tt")   out = KinoVariant::fromInt(10000);
        else return KinoError::PropertyNotSupported;
        return KinoError::OK;
      }
      if (strcmp(rest,"/valuestep")==0) {
        int mv = 0;
        std::vector<String> params = getGroupParams(g);
        if (paramIndex >= params.size()) return KinoError::OutOfRange;
        if (params[paramIndex] == "bri")  out = KinoVariant::fromInt(1);
        else if (params[paramIndex] == "ct")   out = KinoVariant::fromInt(1);
        else if (params[paramIndex] == "tt")   out = KinoVariant::fromInt(100);
        else return KinoError::PropertyNotSupported;
        return KinoError::OK;
      }
    }
    return KinoError::PropertyNotSupported;
  }
  // deviceClass = "sensors" : Es folgt eine {action} für einen bestimmten Sensor {deviceName}
  else if (strcmp(dev,"sensors") == 0) {
    auto* s = getSensorByName(name);
    if (!s) return KinoError::DeviceNotReady;
    if (strcmp(act,"label") == 0) {
      out = KinoVariant::fromString(name);
      return KinoError::OK;
    }
    if (strcmp(act,"writable")==0) {
      out = KinoVariant::fromBool(s->isWritable());
      return KinoError::OK;
    }
    if (s->hasValue(act)) {
      out = KinoVariant::fromJsonVariant(s->getValue(act));
      return KinoError::OK;
    }
    int paramIndex; char rest[32];
    int found = sscanf(act,"param/%d%31s", &paramIndex, rest);
    if ((found == 1) || (strlen(rest)==0)) {    // path = "sensors/<sensorName>/param/<paramIndex>"
      std::vector<String> params = getSensorParams(s);
      if (paramIndex >= params.size()) return KinoError::OutOfRange;
      String getsetPath = "sensors/"; 
      getsetPath += name;
      getsetPath += "/";
      getsetPath += params[paramIndex];
      out = KinoVariant::fromString(getsetPath.c_str());
      return KinoError::OK;
    }
    if (strcmp(rest,"/label")==0) {
      std::vector<String> params = getSensorParams(s);
      if (paramIndex >= params.size()) return KinoError::OutOfRange;
      out = KinoVariant::fromString(params[paramIndex].c_str());
      return KinoError::OK;
    }
    if (strcmp(rest,"/access")==0) {
      std::vector<String> params = getSensorParams(s);
      if (paramIndex >= params.size()) return KinoError::OutOfRange;
      if ((s->isWritable()) && (params[paramIndex] == "status")) {
        out = KinoVariant::fromInt(3); // 3 = read/write
      } else {
        out = KinoVariant::fromInt(1); // 1 = read-only
      }
      return KinoError::OK;
    }
    if (strcmp(rest,"/minvalue")==0) {
      std::vector<String> params = getSensorParams(s);
      if (paramIndex >= params.size()) return KinoError::OutOfRange;
      if ((s->isWritable()) && (params[paramIndex] == "status")) {
        out = KinoVariant::fromInt(0);
        return KinoError::OK;
      }
      return KinoError::PropertyNotSupported;
    }
    if (strcmp(rest,"/maxvalue")==0) {
      std::vector<String> params = getSensorParams(s);
      if (paramIndex >= params.size()) return KinoError::OutOfRange;
      if ((s->isWritable()) && (params[paramIndex] == "status")) {
        out = KinoVariant::fromInt(255);
        return KinoError::OK;
      }
      return KinoError::PropertyNotSupported;
    }
    if (strcmp(rest,"/minvalue")==0) {
      std::vector<String> params = getSensorParams(s);
      if (paramIndex >= params.size()) return KinoError::OutOfRange;
      if ((s->isWritable()) && (params[paramIndex] == "status")) {
        out = KinoVariant::fromInt(1);
        return KinoError::OK;
      }
      return KinoError::PropertyNotSupported;
    }
  }
  else if (strcmp(dev,"scenes")==0) {
    auto* s = getSceneByName(name);
    if (strcmp(act,"label")==0) {
      out = KinoVariant::fromString(name);
      return KinoError::OK;
    }
    if ((strcmp(act,"set")==0)||(strcmp(act,"savestate")==0)) {
      out = KinoVariant::fromBool(false);
      return KinoError::OK;
    }
    if (strcmp(act,"tt")==0) {
      out = KinoVariant::fromInt(s->getTT());
      return KinoError::OK;
    }
    int paramIndex; char rest[32];
    int found = sscanf(act,"param/%d%31s", &paramIndex, rest);
    if ((found == 1) || (strlen(rest) == 0)) {  // path = "scenes/<sceneName>/param/<paramIndex>"
      if (paramIndex > 2) return KinoError::OutOfRange; // Es gibt zu jeder Szene genau 2 Parameter: "set" und "savestate", beide write-only
      String getsetPath = "scenes/";
      getsetPath += name;
      getsetPath += "/";
      if (paramIndex == 0) getsetPath += "set";
      if (paramIndex == 1) getsetPath += "savestate";
      if (paramIndex == 2) getsetPath += "tt";
      out = KinoVariant::fromString(getsetPath.c_str());
      return KinoError::OK;
    }
    if (strcmp(rest,"/label")==0) {
      if (paramIndex > 2) return KinoError::OutOfRange; // Es gibt zu jeder Szene genau 2 Parameter: "set" und "savestate"
      if (paramIndex == 0) out = KinoVariant::fromString("setzen");
      if (paramIndex == 1) out = KinoVariant::fromString("speichern");
      if (paramIndex == 2) out = KinoVariant::fromString("Transition Time [ms]");
      return KinoError::OK;
    }
    if (strcmp(rest,"/access")==0) {
      if (paramIndex > 2) return KinoError::OutOfRange;
      if ((paramIndex == 0)||(paramIndex == 1)) {
        out = KinoVariant::fromInt(2);  // set und savestate sind write-only
        return KinoError::OK;
      }
      out = KinoVariant::fromInt(3);    // tt ist read-write
      return KinoError::OK;
    }
    if (strcmp(rest,"/minvalue")==0) {
      if (paramIndex == 2) out = KinoVariant::fromInt(0);   // tt hat einen Minimalwert von 0ms
    }
    if (strcmp(rest,"/maxvalue")==0) {
      if (paramIndex == 2) out = KinoVariant::fromInt(10000); // tt hat einen Maximalwert von 10000ms
    }
    if (strcmp(rest,"/valuestep")==0) {
      if (paramIndex == 2) out = KinoVariant::fromInt(100); // tt hat eine Schrittweite von 100ms
    }
    return KinoError::PropertyNotSupported;
  }
  // Wenn deviceClass unbekannt 
  return KinoError::PropertyNotSupported;
}

KinoError HueBridge::set(const char* prop, const KinoVariant& val) {
  if (strcmp(prop,"tickInterval")==0) {
    //if (val.type != KinoVariant::INT) return KinoError::InvalidType;
    if (!setTickInterval(val.asInt())) return KinoError::InternalError;
    return KinoError::OK;
  }
  // zuerst Einstellungen, die technisch die Gruppe "ALL" betreffen
  if ((strcmp(prop, "power")==0)||(strcmp(prop,"on")==0)) {
    char jsonString[20];
    snprintf(jsonString,20,"{\"on\":%s}", ((val.asBool())?"true":"false"));
    if (!sendGroupState(0,String(jsonString))) return KinoError::InternalError;
    for (auto* l : _lights) { // aktualisiere die Lights im cache
      l->forceOn(val.asBool());
    }
    return KinoError::OK;
  }
  if ((strcmp(prop, "brightness")==0)||(strcmp(prop,"bri")==0)) {
    char jsonString[20];
    snprintf(jsonString,20,"{\"bri\":%i}", (val.asInt()));
    if (!sendGroupState(0,String(jsonString))) return KinoError::InternalError;
    for (auto* l : _lights) { // aktualisiere _lights- cache
      l->forceBri(val.asInt());
    }
    return KinoError::OK;
  }
  if (strcmp(prop, "scene")==0) {
    HueScene* s = getSceneByName(val.toString());
    if (!s) return KinoError::DeviceUnknown;
    if (!s->setActive(this)) return KinoError::InternalError;
    return KinoError::OK;
  }
  // prop ist nicht eine der bekannten allgemeingültigen Eigenschaften,
  // also suche nach einem Pfad der Form {deviceClass}/{deviceName}/{action}
  char dev[12];
  char name[32];
  char act[32];
  if (!splitPath(prop, dev, sizeof(dev), name, sizeof(name), act, sizeof(act))) return KinoError::InvalidProperty;
  // deviceClass = "lights" : Es folgt eine {action} für eine bestimmte Lampe {deviceName}
  if (strcmp(dev, "lights") == 0) {
    auto* l = getLightByName(name);
    if (!l) return KinoError::DeviceUnknown;
    if ((strcmp(act,"power")==0)||(strcmp(act,"on")==0)) {
        l->setOn(val.asBool());
        return KinoError::OK;
    } 
    if ((strcmp(act,"brightness")==0)||(strcmp(act,"bri")==0)) {
      if (!l->isDimmable()) return KinoError::PropertyNotSupported;
      if (!l->setBri(val.asInt())) return KinoError::InternalError;
      return KinoError::OK;
    }
    if (strcmp(act, "ct") == 0) {
      if (!l->hasCTColor()) return KinoError::PropertyNotSupported;
      if (!l->setCT(val.asInt())) return KinoError::InternalError;
      return KinoError::OK;
    }
    if ((strcmp(act,"color")==0)||(strcmp(act,"rgb")==0)||(strcmp(act,"col")==0)) {
      //if(val.type != KinoVariant::RGB_COLOR) return KinoError::InvalidType;
      RGBColor col = val.asColor();
      if (!l->hasXYColor()) return KinoError::PropertyNotSupported;
      //if (!l->setRGB(val.color.r, val.color.g, val.color.b)) return KinoError::InternalError;
      if (!l->setRGB(col.r, col.g, col.b)) return KinoError::InternalError;
      return KinoError::OK;
    }
    if ((strcmp(act,"tt")==0)||(strcmp(act,"transitiontime")==0)) {
      l->setTT(val.asInt());
      return KinoError::OK;
    }
    return KinoError::PropertyNotSupported;
  }
  // deviceClass = "groups" : Es folgt eine {action} für eine bestimmte Gruppe {deviceName}
  else if (strcmp(dev, "groups") == 0) {
    auto* g = getGroupByName(name);
    if (!g) return KinoError::DeviceUnknown;
    if ((strcmp(act,"power")==0)||(strcmp(act,"allon")==0)||(strcmp(act,"on")==0)) {
      if (!g->setOn(val.asBool())) return KinoError::InternalError;
      return KinoError::OK;
    }
    if ((strcmp(act,"brightness")==0)||(strcmp(act,"bri")==0)) {
      if (!g->setBri(val.asInt())) return KinoError::InternalError;
      return KinoError::OK;
    }
    if (strcmp(act,"ct")==0) {
      if (!g->setCT(val.asInt())) return KinoError::InternalError;
      return KinoError::OK;
    }
    if ((strcmp(act,"tt")==0)||(strcmp(act,"transitiontime")==0)) {
      g->setTT(val.asInt());
      return KinoError::OK;
    }
    return KinoError::PropertyNotSupported;
  }
  // deviceClass = "scenes" : Es folgt eine {action} für eine bestimmte Scene {deviceName}
  else if (strcmp(dev, "scenes") == 0) {
    auto* s = getSceneByName(name);
    if (!s) return KinoError::DeviceNotReady;
    if (strcmp(act, "set") == 0) {
      if (!s->setActive(this)) return KinoError::InternalError;
      return KinoError::OK;
    }
    if (strcmp(act, "savestate") == 0) {
      if (!s->captureLightStates(this)) return KinoError::InternalError;
      return KinoError::OK;
    }
    if ((strcmp(act,"tt")==0)||(strcmp(act,"transitiontime")==0)) {
      s->setTT(val.asInt());
      return KinoError::OK;
    }
    return KinoError::PropertyNotSupported;
  }
  // deviceClass = "sensors" : Es folgt eine {action} für eine bestimmte Gruppe {deviceName}
  else if (strcmp(dev,"sensors")==0) {
    auto *s = getSensorByName(name);
    if (!s) return KinoError::DeviceUnknown;
    if (!s->isWritable()) return KinoError::PropertyNotSupported;
    if (!s->hasValue(act)) return KinoError::OutOfRange;
    if (!s->setValue(act,val.asInt())) return KinoError::InternalError;
    Serial.println("returning");
    return KinoError::OK;
  }
  // Wenn deviceClass unbekannt 
  return KinoError::PropertyNotSupported;
}

KinoError HueBridge::queryCount(const char* property, uint16_t& out) {
  if (strcmp(property, "lights")==0) {
    out = _lights.size();
    return KinoError::OK;
  }
  if (strcmp(property, "groups")==0) {
    out = _groups.size();
    return KinoError::OK;
  }
  if (strcmp(property, "scenes")==0) {
    out = _scenes.size();
    return KinoError::OK;
  }
  if (strcmp(property, "sensors")==0) {
    out = _sensors.size();
    return KinoError::OK;
  }

  int found;
  char tmpName[32]; char rest[32];

  // Pfad "lights/<lightName>/<rest>"
  found = sscanf(property, "lights/%31[^/]/%31s", tmpName, rest);
  if ((found == 2) && (strlen(tmpName)>0)) {
    auto* l = getLightByName(tmpName);
    if (!l) return KinoError::DeviceNotReady;
    if (strcmp(rest,"param")==0) {
      std::vector<String> params = getLightParams(l);
      out = params.size();
      return KinoError::OK;
    }
    return KinoError::PropertyNotSupported;
  }

  // Pfad "groups/<groupName>/<rest>"
  found = sscanf(property, "groups/%31[^/]/%31s", tmpName, rest);
  if ((found == 2)&&(strlen(tmpName)>0)) {
    HueGroup* g=getGroupByName(tmpName);
    if (!g) return KinoError::DeviceNotReady;
    if (strcmp(rest,"lights")==0) {
      std::vector<uint8_t> lightids = g->getLightIds();
      out = lightids.size();
      return KinoError::OK;
    }
    if (strcmp(rest,"param")==0) {
      std::vector<String> params = getGroupParams(g);
      out = params.size();
      return KinoError::OK;
    }
    return KinoError::PropertyNotSupported;
  }

  // Pfad "sensors/<sensorName>/<rest>"
  found = sscanf(property, "sensors/%31[^/]/%31s", tmpName, rest);
  if ((found == 2) && (strlen(tmpName)>0)) {
    HueSensor *s = getSensorByName(tmpName);
    if (!s) return KinoError::DeviceNotReady;
    if ((strcmp(rest,"states")==0)||(strcmp(rest,"param")==0)) {
      out = s->getStateSize();
      return KinoError::OK;
    }
    return KinoError::PropertyNotSupported;
  }

  // Pfad "scenes/<sceneName>/<rest>"
  found = sscanf(property, "scenes/%31[^/]/%31s", tmpName, rest);
  if ((found == 2)&&(strlen(tmpName)>0)) {
    auto* s = getSceneByName(tmpName);
    if (!s) return KinoError::DeviceNotReady;
    if (strcmp(rest,"param")==0) {
      out = 3;    // immer "set", "savestate" und "tt"
      return KinoError::OK;
    }
    return KinoError::PropertyNotSupported;
  }
  return KinoError::PropertyNotSupported;
}

KinoError HueBridge::query(const char* property, uint16_t index, KinoVariant &out) {
  if (strcmp(property, "lights")==0) {
    if (index > _lights.size()) return KinoError::OutOfRange;
    out = KinoVariant::fromString(_lights[index]->getName().c_str());
    return KinoError::OK;
  }
  if (strcmp(property, "groups")==0) {
    if (index > _groups.size()) return KinoError::OutOfRange;
    out = KinoVariant::fromString(_groups[index]->getName().c_str());
    return KinoError::OK;
  }
  if (strcmp(property, "scenes")==0) {
    if (index > _scenes.size()) return KinoError::OutOfRange;
    out = KinoVariant::fromString(_scenes[index]->getName().c_str());
    return KinoError::OK;
  }
  if (strcmp(property, "sensors")==0) {
    if (index > _sensors.size()) return KinoError::OutOfRange;
    out = KinoVariant::fromString(_sensors[index]->getName().c_str());
    return KinoError::OK;
  }
  // Versuche, Pfade zu erkennen
  char tmpName[32]; char rest[32];

  // Pfad "groups/<groupName>/<rest>"
  int found = sscanf(property, "groups/%31[^/]/%31s", tmpName, rest);
  if ((found == 2)&&(strlen(tmpName)>0)) {
    HueGroup* g=getGroupByName(tmpName);
    if (!g) return KinoError::PropertyNotSupported;
    if (strcmp(rest,"lights")==0) {
      std::vector<uint8_t> lightids = g->getLightIds();
      if (index > lightids.size()) return KinoError::OutOfRange;
      HueLight* l = getLightById(lightids[index]);
      if (!l) return KinoError::PropertyNotSupported;
      out = KinoVariant::fromString(l->getName().c_str());
      return KinoError::OK;
    }
    return KinoError::PropertyNotSupported;
  }
  // Pfad "sensors/{Sensorname}/<rest>"
  found = sscanf(property, "sensors/%31[^/]/%31s", tmpName, rest);
  if ((found == 2)&& (strlen(tmpName)>0)) {
    HueSensor *s = getSensorByName(tmpName);
    if (!s) return KinoError::DeviceNotReady;
    if (strcmp(rest,"states")==0) {
      JsonObjectConst sensorState = s->getState();
      if (index > sensorState.size()) return KinoError::OutOfRange;
      int i=0;
      for (JsonPairConst kv : sensorState) { 
        if (i==index) {
          out = KinoVariant::fromString(kv.key().c_str());
          return KinoError::OK;
        }
        i++;
      }
      return KinoError::OK;
    }
    return KinoError::PropertyNotSupported;
  }

  return KinoError::PropertyNotSupported;
}

bool HueBridge::needsCommit() {
  return true;
}

bool HueBridge::commit() {
  for (auto& g : _groups) {
    g->applyChanges(this);
  }
  for (auto& l : _lights) {
    l->applyChanges(this);
  }
  for (auto& s : _sensors) {
    s->applyChanges(this);
  }
  return true;
}

// Helperfunktion zum Bestimmen der verfügbaren Parameter für eine Lampe
std::vector<String> HueBridge::getLightParams(const HueLight* l) {
  std::vector<String> params;
  params.push_back("on");
  if (l->isDimmable()) params.push_back("bri");
  if (l->hasCTColor()) params.push_back("ct");
  if (l->hasXYColor()) params.push_back("col");
  if ((l->isDimmable()) || (l->hasCTColor()) || (l->hasXYColor())) params.push_back("tt");
  return params;
}

// Helperfunktion zum Bestimmen der verfügbaren Parameter für eine Lampe
std::vector<String> HueBridge::getGroupParams(const HueGroup* g) {
  std::vector<uint8_t> lightIds = g->getLightIds();
  bool hasBri = false;
  bool hasCT = false;
  std::vector<String> params;
  params.push_back("on");
  for (uint8_t lightId : lightIds) {
    HueLight* l = getLightById(lightId);
    if (l->isDimmable()) hasBri = true;
    if (l->hasCTColor()) hasCT = true;
  }
  if (hasBri) params.push_back("bri");
  if (hasCT)  params.push_back("ct");
  if (hasBri || hasCT) params.push_back("tt");
  return params;
}

std::vector<String> HueBridge::getSensorParams(const HueSensor* s) {
  JsonObjectConst states = s->getState();
  std::vector<String> params;
  for (JsonPairConst kv : states) {
    params.push_back(String(kv.key().c_str()));
  }
  return params;
}
// ===== Public API =====

bool HueBridge::begin() {
    return (init()==KinoError::OK);
}

KinoError HueBridge::init() {
    if (!readLights()) return KinoError::DeviceNotReady;
    if (!readGroups()) return KinoError::DeviceNotReady;
    if (!readScenes()) return KinoError::DeviceNotReady;
    readSensors();
    return KinoError::OK;
}

KinoError HueBridge::tick() {
  if (WiFi.status() != WL_CONNECTED) return KinoError::NothingToDo;
  if (_tickInterval == 0) return KinoError::NothingToDo;
  if (_refreshing) return KinoError::NothingToDo;
  unsigned long now = millis();
  if (now - _lastTick >= _tickInterval) {
    _lastTick = now;
    _refreshing = true;
    showMemory();
    bool ok = (readLights()&&readSensors());
    showMemory();
    _refreshing = false;
    return (ok ? KinoError::OK : KinoError::DeviceNotReady);
  }
  return KinoError::NothingToDo;
}

bool HueBridge::setTickInterval(int ms) {
  if (ms == 0) { _tickInterval = 0; return true; }
  if (ms < 0) return false;       // nur für bessere Lesbarkeit hier. negative Werte sind unerlaubt
  if (ms < 2000) return false;    // schneller als alle 2 Sekunden erzeugt zu viel Traffic
  _tickInterval = ms;
  return true;
}

int HueBridge::getTickInterval() {
  return _tickInterval;
}

void HueBridge::EnsureTimeoutBeforeRequest(unsigned long timeout) {
  static unsigned long LastRequest = 0;
  unsigned long now = millis();
  while (now - LastRequest < timeout) {
    yield();
    delay(10);
    now = millis();
  }
  return;
}

// helper function for read* functions to stop the client and return false
bool HueBridge::httpError(const char* cause) {
  _client.stop();
  Serial.print("Hue HTTP Error: ");
  Serial.println(cause);
  return false;
}

bool HueBridge::readLights() {
  //Serial.println("Lese Lampen aus der Hue Bridge");
    String path = "/api/" + _user + "/lights";

    if (!httpGET(path)) return httpError("httpGet lights failed");
    if (!skipHttpHeader()) return httpError("skipHttpHeader lights failed");

    // Anti Memory Leak: Lösche alle bisherigen Lampen-Objekte
    //for (auto* l : _lights) delete l;
    //_lights.clear(); // Vorherige Lampen entfernen

    String buffer; buffer.reserve(200);
    char c;
    bool inLight = false;
    int depth = 0;

    uint8_t id = 0;
    String lightJson; // Hier sammeln wir den JSON-Block einer Lampe
    lightJson.reserve(1024);

    while (_client.connected() || _client.available()) {
        if (!_client.available()) {
            yield();
            continue;
        }

        c = _client.read();
        buffer += c;

        // Buffer kürzen, damit er nicht zu groß wird
        if (buffer.length() > 128) buffer.remove(0, buffer.length() - 128);

        // --- Start einer Lampe erkennen: "ID":{ ---
        if (!inLight && buffer.endsWith("\":{")) {
            int q2 = buffer.lastIndexOf('"');
            int q1 = buffer.lastIndexOf('"', q2 - 1);
            if (q1 >= 0 && q2 > q1) {
                String idStr = buffer.substring(q1 + 1, q2);
                if (idStr.length() > 0 && isDigit(idStr[0])) {
                    id = idStr.toInt();
                    inLight = true;
                    depth = 1; // Erste { gezählt
                    buffer = "";
                    lightJson = "{"; // Start JSON-Block sammeln

                    //Serial.print("Start Lampe ID ");
                    //Serial.println(id);
                }
            }
            continue;
        }

        // --- Innerhalb Lampe ---
        if (inLight) {
            lightJson += c;
            if (c == '{') depth++;
            if (c == '}') depth--;

            // --- Ende Lampe ---
            if (depth == 0) {
                // lightJson enthält nun das vollständige Lampe-Objekt
                DynamicJsonDocument doc(2048); // Für eine einzelne Lampe reicht 2KB locker
                DeserializationError error = deserializeJson(doc, lightJson);
                if (!error) {
                    String name = doc["name"] | "";
                    bool on = doc["state"]["on"] | false;
                    uint8_t bri = doc["state"]["bri"] | 0;
                    bool hasBri = false;
                    if (doc["state"].containsKey("bri")) {
                      hasBri = true;
                    }
                    bool hasXY = false;
                    float x = 0, y = 0;
                    if (doc["state"].containsKey("xy") && doc["state"]["xy"].size() == 2) {
                        x = doc["state"]["xy"][0].as<float>();
                        y = doc["state"]["xy"][1].as<float>();
                        hasXY = true;
                    }
                    bool hasCT = doc["state"].containsKey("ct");
                    uint16_t ct = hasCT ? doc["state"]["ct"].as<uint16_t>() : 0;

                    HueLight* existing = getLightById(id);
                    if (existing) {
                      existing->updateValues(name, on, bri, hasXY, x, y, hasCT, ct, hasBri);
                    } else {
                      _lights.push_back(new HueLight(id, name, on, bri, hasXY, x, y, hasCT, ct, hasBri));
                    }

                    //Serial.print("Lampe gefunden namens ");
                    //Serial.println(name);
                } else {
                    //Serial.print("Fehler beim Parsen von Lampe ID ");
                    //Serial.println(id);
                }

                inLight = false;
                buffer = "";
                lightJson = "";
            }
        }
    }

    _client.stop();
    //Serial.print(_lights.size());
    //Serial.println(" Lampen gefunden");

    return !_lights.empty();
}

HueLight* HueBridge::getLightById(uint8_t id) {
    for (auto* l : _lights) {
        if (l->getId() == id)
            return l;
    }
    return nullptr;
}

HueLight* HueBridge::getLightByName(const String& name) {
  for (auto* l : _lights) {
    if (l->getName() == name) return l;
  }
  return nullptr;
}

const std::vector<HueLight*>& HueBridge::getLights() const {
  return _lights;
}

HueGroup* HueBridge::getGroupByName(const String& name) {
    for (auto* g : _groups) {
        if (g->getName() == name) return g;
    }
    return nullptr;
}

const std::vector<HueGroup*>& HueBridge::getGroups() const {
    return _groups;
}

bool HueBridge::readGroups() {
    //Serial.println("Lese Gruppen aus der Hue Bridge");
    String path = "/api/" + _user + "/groups";

    if (!httpGET(path)) {
      //Serial.println("httpGet groups failed");
      return httpError("httpGet groups failed");
    }
    if (!skipHttpHeader()) {
      //Serial.println("skipHttpHeader groups failed");
      return httpError("httpGet groups failed");
    }
      

    // Alte Gruppen löschen (Anti Memory Leak)
    for (auto* g : _groups) delete g;
    _groups.clear();

    String buffer;
    char c;
    bool inGroup = false;
    int depth = 0;

    uint16_t groupId = 0;
    String groupJson;

    while (_client.connected() || _client.available()) {
        if (!_client.available()) {
            yield();
            continue;
        }

        c = _client.read();
        buffer += c;

        if (buffer.length() > 128)
            buffer.remove(0, buffer.length() - 128);

        // --- Start Gruppe erkennen: "ID":{ ---
        if (!inGroup && buffer.endsWith("\":{")) {
            int q2 = buffer.lastIndexOf('"');
            int q1 = buffer.lastIndexOf('"', q2 - 1);
            if (q1 >= 0 && q2 > q1) {
                String idStr = buffer.substring(q1 + 1, q2);
                if (idStr.length() > 0 && isDigit(idStr[0])) {
                    //Serial.print("Start von Gruppe ");
                    //Serial.println(idStr);
                    groupId = idStr.toInt();
                    inGroup = true;
                    depth = 1;
                    buffer = "";
                    groupJson = "{";
                }
            }
            continue;
        }

        // --- Innerhalb Gruppe ---
        if (inGroup) {
            groupJson += c;
            if (c == '{') depth++;
            if (c == '}') depth--;

            if (depth == 0) {
                DynamicJsonDocument doc(1024);
                DeserializationError err = deserializeJson(doc, groupJson);
                if (!err) {
                    String name = doc["name"] | "";

                    std::vector<uint8_t> lightIds;

                    JsonArray lightsArr = doc["lights"].as<JsonArray>();
                    for (JsonVariant v : lightsArr) {
                        lightIds.push_back(
                            String(v.as<const char*>()).toInt()
                        );
                    }
                    
                    _groups.push_back(
                        new HueGroup(groupId, name, *this, lightIds)
                    );
                } else {
                  //Serial.println("Fehler beim Parsen des Json: ");
                  //Serial.println(err.f_str());
                }

                inGroup = false;
                buffer = "";
                groupJson = "";
            }
        }
    }

    _client.stop();
    //Serial.print(_groups.size());
    //Serial.println(" Gruppen gefunden");

    return !_groups.empty();
}

bool HueBridge::readScenes() {
    String path = "/api/" + _user + "/scenes";

    if (!httpGET(path)) return httpError("httpGet scenes failed");
    if (!skipHttpHeader()) return httpError("skipHeader scenes failed");

    // Alte Szenen löschen
    for (auto* s : _scenes) delete s;
    _scenes.clear();

    String buffer;
    char c;
    bool inScene = false;
    int depth = 0;

    String sceneId;
    String sceneJson;

    while (_client.connected() || _client.available()) {
        if (!_client.available()) {
            yield();
            continue;
        }

        c = _client.read();
        buffer += c;

        if (buffer.length() > 128)
            buffer.remove(0, buffer.length() - 128);

        // --- Start Szene erkennen: "ID":{ ---
        if (!inScene && buffer.endsWith("\":{")) {
            int q2 = buffer.lastIndexOf('"');
            int q1 = buffer.lastIndexOf('"', q2 - 1);
            if (q1 >= 0 && q2 > q1) {
                sceneId = buffer.substring(q1 + 1, q2);
                inScene = true;
                depth = 1;
                buffer = "";
                sceneJson = "{";
            }
            continue;
        }

        // --- Innerhalb Szene ---
        if (inScene) {
            sceneJson += c;
            if (c == '{') depth++;
            if (c == '}') depth--;

            if (depth == 0) {
                DynamicJsonDocument doc(2048);
                if (deserializeJson(doc, sceneJson) == DeserializationError::Ok) {

                    String type = doc["type"] | "";
                    if (type == "LightScene") {

                        String name = doc["name"] | "";

                        std::vector<uint8_t> lightIds;
                        for (JsonVariant v : doc["lights"].as<JsonArray>()) {
                            lightIds.push_back(
                                String(v.as<const char*>()).toInt()
                            );
                        }

                        _scenes.push_back(
                            new HueScene(sceneId, name, lightIds)
                        );
                    }
                }

                inScene = false;
                sceneJson = "";
                buffer = "";
            }
        }
    }

    _client.stop();
    return !_scenes.empty();
}

HueScene* HueBridge::getSceneByName(const String& name) {
    for (auto* s : _scenes) {
        if (s->getName() == name)
            return s;
    }
    return nullptr;
}

/*
const std::vector<HueScene*>& HueBridge::getScenes() const {
    return _scenes;
}*/

bool HueBridge::setScene(const String& sceneName) {
  HueScene* s = getSceneByName(sceneName);
  if (s) return s->setActive(this);
  return false;
}

std::map<uint8_t, bool> HueBridge::getScenePowerStates(const String& sceneId) {
    std::map<uint8_t, bool> result;

    String path = "/api/" + _user + "/scenes/" + sceneId;

    if (!httpGET(path)) {_client.stop(); return result; }
    if (!skipHttpHeader()) {_client.stop(); return result; }

    String buffer;
    char c;
    int depth = 0;
    bool inLightStates = false;
    String json;

    while (_client.connected() || _client.available()) {
        if (!_client.available()) {
            yield();
            continue;
        }

        c = _client.read();

        // lightstates-Block finden
        buffer += c;
        if (buffer.length() > 64)
            buffer.remove(0, buffer.length() - 64);

        if (!inLightStates && buffer.endsWith("\"lightstates\":{")) {
            inLightStates = true;
            depth = 1;
            json = "{";
            continue;
        }

        if (inLightStates) {
            json += c;
            if (c == '{') depth++;
            if (c == '}') depth--;

            if (depth == 0) {
                DynamicJsonDocument doc(2048);
                if (deserializeJson(doc, json) == DeserializationError::Ok) {

                    for (JsonPair kv : doc.as<JsonObject>()) {
                        uint8_t lightId = String(kv.key().c_str()).toInt();
                        bool on = kv.value()["on"] | false;
                        result[lightId] = on;
                    }
                }
                break;
            }
        }
    }

    _client.stop();
    return result;
}

SceneLightStates HueBridge::getSceneLightStates(const String& sceneId) {
    SceneLightStates result;

    String path = "/api/" + _user + "/scenes/" + sceneId;
    if (!httpGET(path)) return result;
    if (!skipHttpHeader()) return result;

    String buffer;
    char c;
    bool inLightStates = false;
    int depth = 0;
    String json;

    while (_client.connected() || _client.available()) {
        if (!_client.available()) {
            yield();
            continue;
        }

        c = _client.read();

        // lightstates-Block finden
        buffer += c;
        if (buffer.length() > 64)
            buffer.remove(0, buffer.length() - 64);

        if (!inLightStates && buffer.endsWith("\"lightstates\":{")) {
            inLightStates = true;
            depth = 1;
            json = "{";
            continue;
        }

        if (inLightStates) {
            json += c;

            if (c == '{') depth++;
            if (c == '}') depth--;

            if (depth == 0) {
                DynamicJsonDocument doc(2048);
                if (deserializeJson(doc, json) == DeserializationError::Ok) {

                    for (JsonPair kv : doc.as<JsonObject>()) {
                        uint8_t lightId =
                            String(kv.key().c_str()).toInt();

                        SceneLightState state;
                        JsonObject obj = kv.value().as<JsonObject>();

                        if (obj.containsKey("on")) {
                            state.hasOn = true;
                            state.on = obj["on"];
                        }
                        if (obj.containsKey("bri")) {
                            state.hasBri = true;
                            state.bri = obj["bri"];
                        }
                        if (obj.containsKey("ct")) {
                            state.hasCT = true;
                            state.ct = obj["ct"];
                        }

                        result[lightId] = state;
                    }
                }
                break;
            }
        }
    }

    _client.stop();
    return result;
}

bool HueBridge::readSensors() {

    String path = "/api/" + _user + "/sensors";

    if (!httpGET(path)) return httpError("httpGet sensors failed");
    if (!skipHttpHeader()) return httpError("skipHeader sensors failed");

    // --- alte Sensoren löschen ---
    //for (auto* s : _sensors) delete s;
    //_sensors.clear();

    String buffer; buffer.reserve(200);
    char c;
    bool inSensor = false;
    int depth = 0;

    uint16_t sensorId = 0;
    String sensorJson; sensorJson.reserve(1024);

    while (_client.connected() || _client.available()) {

        if (!_client.available()) {
            yield();
            continue;
        }

        c = _client.read();
        buffer += c;

        // Buffer begrenzen
        if (buffer.length() > 128)
            buffer.remove(0, buffer.length() - 128);

        // --- Start Sensor erkennen: "ID":{ ---
        if (!inSensor && buffer.endsWith("\":{")) {

            int q2 = buffer.lastIndexOf('"');
            int q1 = buffer.lastIndexOf('"', q2 - 1);

            if (q1 >= 0 && q2 > q1) {
                String idStr = buffer.substring(q1 + 1, q2);
                if (idStr.length() > 0 && isDigit(idStr[0])) {

                    sensorId = idStr.toInt();
                    inSensor = true;
                    depth = 1;
                    buffer = "";
                    sensorJson = "{";
                }
            }
            continue;
        }

        // --- innerhalb Sensor ---
        if (inSensor) {
            sensorJson += c;

            if (c == '{') depth++;
            if (c == '}') depth--;

            // --- Ende Sensor ---
            if (depth == 0) {

                DynamicJsonDocument doc(2048);
                DeserializationError err = deserializeJson(doc, sensorJson);

                if (!err) {
                    HueSensor* s = getSensorById(sensorId);
                    if (!s) {
                      String name = doc["name"] | "";
                      String type = doc["type"] | "";

                      s = new HueSensor(sensorId, name, type);
                      _sensors.push_back(s);
                    }

                    if (doc.containsKey("state")) {
                        s->updateState(
                            doc["state"].as<JsonObject>()
                        );
                    }

                    
                }

                inSensor = false;
                sensorJson = "";
                buffer = "";
            }
        }
    }

    _client.stop();
    return !_sensors.empty();
}

HueSensor* HueBridge::getSensorById(uint16_t sensorId) {
  for (auto* s : _sensors) {
    if (s->getId() == sensorId) return s;
  }
  return nullptr;
}

HueSensor* HueBridge::getSensorByName(const String& name) {
    for (auto* s : _sensors)
        if (s->getName() == name)
            return s;
    return nullptr;
}


bool HueBridge::setSensorState(uint16_t id,
                               const JsonObject& state) {

    String path =
        "/api/" + _user + "/sensors/" +
        String(id) + "/state";

    String payload;
    serializeJson(state, payload);

    return sendState(path, payload);
}



// ===== HTTP =====

bool HueBridge::httpGET(const String& path) {
  EnsureTimeoutBeforeRequest(100);
  _client.stop();
  if (!_client.connect(_ip, 80)) {
    _client.stop();
    return false;
  }

  _client.printf(
    "GET %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Connection: close\r\n\r\n",
    path.c_str(),
    _ip.toString().c_str()
  );

  return waitForData();
}

bool HueBridge::sendLightState(uint8_t lightId, const String& jsonPayload) {
  String path = "/api/" + _user + "/lights/" + String(lightId) + "/state";
  return sendState(path, jsonPayload);
}

bool HueBridge::sendGroupState(uint16_t groupId, const String& jsonPayload) {
  String path = "/api/" + _user + "/groups/" + String(groupId) + "/action";
  return sendState(path, jsonPayload);
}

bool HueBridge::saveScene(const String& sceneId, const String& jsonPayload) {
    String path = "/api/" + _user + "/scenes/" + sceneId;
    return sendState(path, jsonPayload);
}


bool HueBridge::sendState(const String& path, const String& jsonPayload) {
  _client.stop();
  EnsureTimeoutBeforeRequest(100);
  //Serial.println("Hue: sending state");
  if (!_client.connect(_ip, 80)) {
    _client.stop();
    return false;
  }

  _client.printf(
      "PUT %s HTTP/1.1\r\n"
      "Host: %s\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: %d\r\n"
      "Connection: close\r\n\r\n"
      "%s",
      path.c_str(),
      _ip.toString().c_str(),
      jsonPayload.length(),
      jsonPayload.c_str()
  );

  return waitForData();
}

bool HueBridge::waitForData(uint32_t timeout) {
  uint32_t start = millis();
  while (_client.connected() && !_client.available()) {
    if (millis() - start > timeout) {
      _client.stop();
      return false;
    }
    yield();
  }
  return true;
}

bool HueBridge::skipHttpHeader() {
  unsigned long start = millis();
  String line;

  while (!_client.available()) {
    if (millis() - start > 2000) {
      _client.stop();
      return false;
    }
    yield();
  }

  while (_client.connected() || _client.available()) {
    if (!_client.available()) {
      if (millis() - start > 2000) {
        _client.stop();
        return false;
      }
      yield();
      continue;
    }

    line = _client.readStringUntil('\n');

    if (line.length() == 0 || line == "\r") {
      return true;
    }
  }
  return false;
}

bool HueBridge::setPower(bool onoff) {
  if (onoff) {  // einschalten
    if (anyOn()) return true;   // no action needed, light is already on
    HueSensor* sensor = getSensorByName("Daylight");
    // Tageslicht?
    bool day = (sensor && sensor->hasValue("daylight")) ? sensor->getValue("daylight").as<bool>() : false;
    if (day) return setScene("Standard");  // Tagsüber: verhalte Dich wie der Lichtschalter
    return setScene("Nachtlicht");         // Nachts: setze Nachtlicht, um einen Schock zu verhindern ;-)
  }
  // still here: ausschalten
  if (!sendGroupState(0,"{\"on\":false}")) return false;
  for (auto& l : _lights) {
    l->forceOn(false);
  }
  return true;
}

bool HueBridge::anyOn() {
  for (auto& l : _lights) {
    if (l->isOn()) return true;
  }
  return false;
}


/*
 * ALTE VERSION

bool HueBridge::skipHttpHeader() {
  unsigned long start = millis();
  String line;

  // Erst warten, bis überhaupt Daten kommen
  while (!_client.available()) {
    if (millis() - start > 2000) return false;
    yield();
  }

  while (_client.connected()) {
    if (!_client.available()) {
      if (millis() - start > 2000) return false;
      yield();
      continue;
    }

    line = _client.readStringUntil('\n');
    Serial.println(line);
    if (line == "\r" || line == "") {
        return true;
    }
  }
  return false;
}
*/
