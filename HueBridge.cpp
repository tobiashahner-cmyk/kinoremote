#include "HueBridge.h"
#include <ArduinoJson.h>
#include "NetworkHelper.h"

// ===== Konstruktoren =====

HueBridge::HueBridge(const IPAddress& ip, const char* user)
: _ip(ip) {
  _globalDepth = 0; 
  strlcpy(_user, user, sizeof(_user));
}

HueBridge::HueBridge(const String& ip, const char* user) {
  _ip.fromString(ip);
  _globalDepth = 0;
  strlcpy(_user, user, sizeof(_user));
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
    //out = KinoVariant::fromInt(_tickInterval);
    out.setInt(_tickInterval);
    return KinoError::OK;
  }
  if (strcmp(prop,"ip")==0) {
    //out = KinoVariant::fromString(_ip.toString().c_str());
    char buf[20];
    snprintf(buf, 20, "%d.%d.%d.%d", _ip[0], _ip[1], _ip[2], _ip[3]);
    out.setString(buf);
    return KinoError::OK;
  }
  if ((strcmp(prop,"power")==0)||(strcmp(prop,"anyon")==0)||(strcmp(prop,"on")==0)){
    for (auto* l : _lights) {
      if (l->isOn()) {
        //out = KinoVariant::fromBool(true);
        out.setBool(true);
        return KinoError::OK;
      }
    }
    //out = KinoVariant::fromBool(false);
    out.setBool(false);
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
    //out = KinoVariant::fromInt(averageBri);
    out.setInt(averageBri);
    return KinoError::OK;
  }
  if ((strcmp(prop,"powerall")==0)||(strcmp(prop,"allon")==0)) {
    for (auto* l : _lights) {
      if (!l->isOn()) {
        //out = KinoVariant::fromBool(false);
        out.setBool(false);
        return KinoError::OK;
      }
    }
    //out = KinoVariant::fromBool(true);
    out.setBool(true);
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
    if (foundSensors == 0) return KinoError::PropertyNotSupported;
    float t = temp/foundSensors;
    char buf[32];
    snprintf(buf,32,"%.1f C",(t/100));
    //out = KinoVariant::fromString(buf);
    out.setString(buf);
    return KinoError::OK;
  }
  if (strcmp(prop, "daylight")==0) {
    for (auto* s : _sensors) {
      if (s->hasValue("daylight")) {
        //out = KinoVariant::fromJsonVariant(s->getValue("daylight"));
        out.setFromJsonVariant(s->getValue("daylight"));
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
      //out = KinoVariant::fromBool(l->isOn());
      out.setBool(l->isOn());
      return KinoError::OK;
    } 
    if ((strcmp(act,"brightness")==0)||(strcmp(act,"bri")==0)) {
      if (!l->isDimmable()) return KinoError::OutOfRange;
      //out = KinoVariant::fromInt(l->getBrightness());
      out.setInt(l->getBrightness());
      return KinoError::OK;
    }
    if (strcmp(act, "ct") == 0) {
      if (!l->hasCTColor()) return KinoError::OutOfRange;
      //out = KinoVariant::fromInt(l->getCT());
      out.setInt(l->getCT());
      return KinoError::OK;
    }
    if ((strcmp(act,"color")==0)||(strcmp(act,"rgb")==0)||(strcmp(act,"col")==0)) {
      if (!l->hasXYColor()) return KinoError::OutOfRange;
      RgbColor col = l->getRGB();
      //out = KinoVariant::fromColor(col.r, col.g, col.b);
      out.setColor(col.r, col.g, col.b);
      return KinoError::OK;
    }
    if (strcmp(act,"tt")==0) {
      //out = KinoVariant::fromInt(l->getTT());
      out.setInt(l->getTT());
      return KinoError::OK;
    }
    if (strcmp(act, "id") == 0) {
      //out = KinoVariant::fromInt(l->getId());
      out.setInt(l->getId());
      return KinoError::OK;
    }
    if (strcmp(act, "label") == 0) {
      //out = KinoVariant::fromString(name);
      out.setString(name);
      return KinoError::OK;
    }
    if (strstr(act,"param/")) { // path = "lights/<lightName>/param/<rest>"
      int found, paramIndex; char rest[32];
      found = sscanf(act,"param/%d%s", &paramIndex, rest);
      if ((found==1)||(strlen(rest)==0)) {    // path = "lights/<lightName>/param/<paramIndex>"
        // get the n-th param for the given light
        //std::vector<String> params = getLightParams(l);
        //if (paramIndex >= params.size()) return KinoError::OutOfRange;
        //String getsetPath = "lights/";
        //getsetPath += name; getsetPath += "/";
        //getsetPath += params[paramIndex];
        //out = KinoVariant::fromString(getsetPath.c_str());
        char getsetpath[128];
        char param[32];
        if (getLightParam(l,paramIndex, param, sizeof(param))) {
          snprintf(getsetpath, sizeof(getsetpath), "lights/%s/%s", name, param);
          out.setString(getsetpath);
          return KinoError::OK;
        }
        out.setNone(); 
        return KinoError::OutOfRange;
        
      }
      if (strcmp(rest,"/label")==0) {
        /*std::vector<String> params = getLightParams(l);
        if (paramIndex >= params.size()) return KinoError::OutOfRange;
        if (params[paramIndex] == "on") out = KinoVariant::fromString("Power");
        if (params[paramIndex] == "bri") out = KinoVariant::fromString("Helligkeit");
        if (params[paramIndex] == "ct") out = KinoVariant::fromString("Weisston");
        if (params[paramIndex] == "col") out = KinoVariant::fromString("Farbe");
        if (params[paramIndex] == "tt") out = KinoVariant::fromString("Trans.Time[ms]");*/
        char param[10];
        out.setNone();
        if (getLightParam(l, paramIndex, param, sizeof(param))) {
          if (strcmp(param,"on")==0)    out.setString("Power");
          if (strcmp(param,"bri")==0)   out.setString("Helligkeit");
          if (strcmp(param,"ct")==0)    out.setString("Weisston");
          if (strcmp(param,"col")==0)   out.setString("Farbe");
          if (strcmp(param,"tt")==0)    out.setString("Trans.Time[ms]");
          if (out.type == KinoVariant::STRING) return KinoError::OK;
        } 
        //out = KinoVariant::fromString(params[paramIndex].c_str());
        return KinoError::OutOfRange;
      }
      if (strcmp(rest,"/access")==0) {
        //out = KinoVariant::fromInt(3);  // alle Parameter sind read/write
        out.setInt(3);
      }
      if (strcmp(rest,"/minvalue")==0) {
        /*std::vector<String> params = getLightParams(l);
        if (paramIndex >= params.size()) return KinoError::OutOfRange;
        if (params[paramIndex] == "bri")  out = KinoVariant::fromInt(0);
        else if (params[paramIndex] == "ct")   out = KinoVariant::fromInt(153);
        else return KinoError::PropertyNotSupported;*/
        char param[10];
        out.setInt(0);
        if (getLightParam(l, paramIndex, param, sizeof(param))) {
          if (strcmp(param, "bri")==0) out.setInt(0);
          if (strcmp(param, "ct" )==0) out.setInt(l->getMinCT());
          return KinoError::OK;
        } else {
          out.setNone();
          return KinoError::OutOfRange;
        }
        return KinoError::PropertyNotSupported;
      }
      if (strcmp(rest,"/maxvalue")==0) {
        char param[10];
        out.setInt(100);
        if (getLightParam(l, paramIndex, param, sizeof(param))) {
          if (strcmp(param, "bri")==0)  out.setInt(255);
          if (strcmp(param, "ct" )==0)  out.setInt(l->getMaxCT());
          if (strcmp(param, "tt" )==0)  out.setInt(10000);
          return KinoError::OK;
        } else {
          out.setNone();
          return KinoError::OutOfRange;
        }
        return KinoError::PropertyNotSupported;
      }
      if (strcmp(rest,"/valuestep")==0) {
        char param[10];
        out.setInt(1);
        if (getLightParam(l, paramIndex, param, sizeof(param))) {
          if (strcmp(param, "tt")==0) out.setInt(100);
          return KinoError::OK;
        } else {
          out.setNone();
          return KinoError::OutOfRange;
        }
        return KinoError::PropertyNotSupported;
      }
    }
    return KinoError::PropertyNotSupported;
  }
  // deviceClass = "groups" : Es folgt eine {action} für eine bestimmte Gruppe {deviceName}
  else if (strcmp(dev, "groups") == 0) {
    auto* g = getGroupByName(name);
    if (!g) return KinoError::DeviceNotReady;
    if ((strcmp(act,"power")==0)||(strcmp(act,"anyon")==0)||(strcmp(act,"on")==0)) {  // this is GET. "on" means "any_on"
      //out = KinoVariant::fromBool(g->anyOn());
      out.setBool(g->anyOn());
      return KinoError::OK;
    }
    if (strcmp(act,"bri")==0) {
      int totalBri = 0; int totalLights = 0;
      //std::vector<uint8_t> lightIds = g->getLightIds();
      const std::vector<uint8_t>& lightIds = g->getLightIds();
      for (uint8_t lid : lightIds) {
        auto* l = getLightById(lid);
        if (l->isOn()) {
          if (l->isDimmable()) totalBri += l->getBrightness();
          else totalBri += 255;
        }
        totalLights++;
      }
      float avg = (totalBri/totalLights);
      //out = KinoVariant::fromInt((int)avg);
      out.setInt((int)avg);
      return KinoError::OK;
    }
    if (strcmp(act,"ct")==0) {
      HueGroup* g = getGroupByName(name);
      if (!g) return KinoError::DeviceNotReady;
      int totalCt = 0; int totalLights = 0;
      //std::vector<uint8_t> lightIds = g->getLightIds();
      const std::vector<uint8_t>& lightIds = g->getLightIds();
      for (uint8_t lid : lightIds) {
        auto* l = getLightById(lid);
        if (l->isOn()) {
          if (l->hasCTColor()) totalCt += l->getCT();
        }
        totalLights++;
      }
      float avg = (totalCt/totalLights);
      //out = KinoVariant::fromInt((int)avg);
      out.setInt((int)avg);
      return KinoError::OK;
    }
    if ((strcmp(act,"powerall")==0)||(strcmp(act,"allon")==0)) {
      //out = KinoVariant::fromBool(g->allOn());
      out.setBool(g->allOn());
      return KinoError::OK;
    }
    if (strcmp(act, "label") == 0) {
      //out = KinoVariant::fromString(name);
      out.setString(name);
      return KinoError::OK;
    }
    if (strcmp(act,"tt")==0) {
      //out = KinoVariant::fromInt(g->getTT());
      out.setInt(g->getTT());
      return KinoError::OK;
    }
    if (strstr(act,"param/")) { // path = "groups/<groupName>/param/<rest>"
      int found, paramIndex; char rest[32];
      found = sscanf(act,"param/%d%31s", &paramIndex, rest);
      if ((found==1)||(strlen(rest)==0)) {    // path = "groups/<groupName>/param/<paramIndex>"
        // get the n-th param for the given group
        /*std::vector<String> params = getGroupParams(g);
        if (paramIndex >= params.size()) return KinoError::OutOfRange;
        String getsetPath = "groups/";
        getsetPath += name; getsetPath += "/";
        getsetPath += params[paramIndex];
        out = KinoVariant::fromString(getsetPath.c_str());
        return KinoError::OK;*/
        char param[10];
        out.setNone();
        if (getGroupParam(g, paramIndex, param, sizeof(param))) {
          char path[128];
          snprintf(path, sizeof(path), "groups/%s/%s", name, param);
          out.setString(path);
          return KinoError::OK;
        } else {
          return KinoError::OutOfRange;
        }
        return KinoError::PropertyNotSupported;
      }
      if (strcmp(rest,"/label")==0) {
        /*std::vector<String> params = getGroupParams(g);
        if (paramIndex >= params.size()) return KinoError::OutOfRange;
        if (params[paramIndex] == "on") out = KinoVariant::fromString("Power");
        if (params[paramIndex] == "bri") out = KinoVariant::fromString("Helligkeit");
        if (params[paramIndex] == "ct") out = KinoVariant::fromString("Weisston");
        if (params[paramIndex] == "col") out = KinoVariant::fromString("Farbe");
        if (params[paramIndex] == "tt") out = KinoVariant::fromString("Trans.Time[ms]");
        //out = KinoVariant::fromString(params[paramIndex].c_str());
        return KinoError::OK;*/
        char param[10]; out.setNone();
        if (getGroupParam(g, paramIndex, param, sizeof(param))) {
          if (strcmp(param, "on"  )==0) out.setString("Power");
          if (strcmp(param, "bri" )==0) out.setString("Helligkeit");
          if (strcmp(param, "ct"  )==0) out.setString("Weisston");
          if (strcmp(param, "col" )==0) out.setString("Farbe");
          if (strcmp(param, "tt"  )==0) out.setString("Trans.Time[ms]");
          if (out.type == KinoVariant::STRING) return KinoError::OK;
          return KinoError::OutOfRange;
        } else {
          return KinoError::OutOfRange;
        }
        return KinoError::PropertyNotSupported;
      }
      if (strcmp(rest,"/access")==0) {
        //out = KinoVariant::fromInt(3);  // alle Parameter sind read/write
        out.setInt(3);
      }
      if (strcmp(rest,"/minvalue")==0) {
        /*int mv = 0;
        std::vector<String> params = getGroupParams(g);
        if (paramIndex >= params.size()) return KinoError::OutOfRange;
        if (params[paramIndex] == "bri")  out = KinoVariant::fromInt(0);
        else if (params[paramIndex] == "ct")   out = KinoVariant::fromInt(153);
        else return KinoError::PropertyNotSupported;
        return KinoError::OK;*/
        char param[10];
        if (getGroupParam(g, paramIndex, param, sizeof(param))) {
          out.setInt(0);
          if (strcmp(param, "ct")==0) out.setInt(153);
          return KinoError::OK;
        } else {
          out.setNone();
          return KinoError::OutOfRange;
        }
        return KinoError::PropertyNotSupported;
      }
      if (strcmp(rest,"/maxvalue")==0) {
        /*int mv = 0;
        std::vector<String> params = getGroupParams(g);
        if (paramIndex >= params.size()) return KinoError::OutOfRange;
        if (params[paramIndex] == "bri")  out = KinoVariant::fromInt(255);
        else if (params[paramIndex] == "ct")   out = KinoVariant::fromInt(555);
        else if (params[paramIndex] == "tt")   out = KinoVariant::fromInt(10000);
        else return KinoError::PropertyNotSupported;
        return KinoError::OK;*/
        char param[10];
        if (getGroupParam(g, paramIndex, param, sizeof(param))) {
          out.setInt(100);
          if (strcmp(param, "bri")==0)  out.setInt(255);
          if (strcmp(param, "ct" )==0)  out.setInt(555);
          if (strcmp(param, "tt" )==0)  out.setInt(10000);
          return KinoError::OK;
        } 
        out.setNone();
        return KinoError::OutOfRange;
      }
      if (strcmp(rest,"/valuestep")==0) {
        /*int mv = 0;
        std::vector<String> params = getGroupParams(g);
        if (paramIndex >= params.size()) return KinoError::OutOfRange;
        if (params[paramIndex] == "bri")  out = KinoVariant::fromInt(1);
        else if (params[paramIndex] == "ct")   out = KinoVariant::fromInt(1);
        else if (params[paramIndex] == "tt")   out = KinoVariant::fromInt(100);
        else return KinoError::PropertyNotSupported;
        return KinoError::OK;*/
        char param[10];
        if (getGroupParam(g, paramIndex, param, sizeof(param))) {
          out.setInt(1);
          if (strcmp(param, "tt")==0) out.setInt(100);
          return KinoError::OK;
        }
        out.setNone();
        return KinoError::OutOfRange;
      }
    }
    return KinoError::PropertyNotSupported;
  }
  // deviceClass = "sensors" : Es folgt eine {action} für einen bestimmten Sensor {deviceName}
  else if (strcmp(dev,"sensors") == 0) {
    auto* s = getSensorByName(name);
    if (!s) return KinoError::DeviceNotReady;
    if (strcmp(act,"label") == 0) {
      //out = KinoVariant::fromString(name);
      out.setString(name);
      return KinoError::OK;
    }
    if (strcmp(act,"writable")==0) {
      //out = KinoVariant::fromBool(s->isWritable());
      out.setBool(s->isWritable());
      return KinoError::OK;
    }
    if (s->hasValue(act)) {
      //out = KinoVariant::fromJsonVariant(s->getValue(act));
      out.setFromJsonVariant(s->getValue(act));
      return KinoError::OK;
    }
    int paramIndex; char rest[32];
    int found = sscanf(act,"param/%d%31s", &paramIndex, rest);
    if ((found == 1) || (strlen(rest)==0)) {    // path = "sensors/<sensorName>/param/<paramIndex>"
      /*std::vector<String> params = getSensorParams(s);
      if (paramIndex >= params.size()) return KinoError::OutOfRange;
      String getsetPath = "sensors/"; 
      getsetPath += name;
      getsetPath += "/";
      getsetPath += params[paramIndex];
      out = KinoVariant::fromString(getsetPath.c_str());
      return KinoError::OK;*/
      char param[32]; // sensor-keys können länger sein
      if (getSensorParam(s, paramIndex, param, sizeof(param))) {
        char getsetpath[128];
        snprintf(getsetpath, sizeof(getsetpath), "sensors/%s/%s", name, param);
        out.setString(getsetpath);
        return KinoError::OK;
      }
      out.setNone();
      return KinoError::OutOfRange;
    }
    if (strcmp(rest,"/label")==0) {
      /*std::vector<String> params = getSensorParams(s);
      if (paramIndex >= params.size()) return KinoError::OutOfRange;
      out = KinoVariant::fromString(params[paramIndex].c_str());
      return KinoError::OK;*/
      char param[32];
      if (getSensorParam(s, paramIndex, param, sizeof(param))) {
        out.setString(param);
        return KinoError::OK;
      }
      out.setNone();
      return KinoError::OutOfRange;
    }
    if (strcmp(rest,"/access")==0) {
      /*std::vector<String> params = getSensorParams(s);
      if (paramIndex >= params.size()) return KinoError::OutOfRange;
      if ((s->isWritable()) && (params[paramIndex] == "status")) {
        out = KinoVariant::fromInt(3); // 3 = read/write
      } else {
        out = KinoVariant::fromInt(1); // 1 = read-only
      }
      return KinoError::OK;*/
      char param[32];
      out.setInt(1);  // Standard: read-only
      if (getSensorParam(s, paramIndex, param, sizeof(param))) {
        if (s->isWritable() && (strcmp(param,"status")==0)) out.setInt(3);
        return KinoError::OK;
      }
      out.setInt(0);  // kein Zugriff
      return KinoError::OutOfRange;
    }
    if (strcmp(rest,"/minvalue")==0) {
      /*std::vector<String> params = getSensorParams(s);
      if (paramIndex >= params.size()) return KinoError::OutOfRange;
      if ((s->isWritable()) && (params[paramIndex] == "status")) {
        out = KinoVariant::fromInt(0);
        return KinoError::OK;
      }
      return KinoError::PropertyNotSupported;*/
      char param[32];
      if (getSensorParam(s, paramIndex, param, sizeof(param))) {
        if ((s->isWritable()) && (strcmp(param,"status")==0)) {
          out.setInt(0);
          return KinoError::OK;
        }
        out.setNone();
        return KinoError::PropertyNotSupported;
      }
      out.setNone();
      return KinoError::OutOfRange;
    }
    if (strcmp(rest,"/maxvalue")==0) {
      /*std::vector<String> params = getSensorParams(s);
      if (paramIndex >= params.size()) return KinoError::OutOfRange;
      if ((s->isWritable()) && (params[paramIndex] == "status")) {
        out = KinoVariant::fromInt(255);
        return KinoError::OK;
      }
      return KinoError::PropertyNotSupported;*/
      char param[32];
      if (getSensorParam(s, paramIndex, param, sizeof(param))) {
        if ((s->isWritable()) && (strcmp(param, "status")==0)) {
          out.setInt(255);
          return KinoError::OK;
        }
        out.setNone();
        return KinoError::PropertyNotSupported;
      }
      out.setNone();
      return KinoError::OutOfRange;
    }
    if (strcmp(rest,"/valuestep")==0) {
      /*std::vector<String> params = getSensorParams(s);
      if (paramIndex >= params.size()) return KinoError::OutOfRange;
      if ((s->isWritable()) && (params[paramIndex] == "status")) {
        out = KinoVariant::fromInt(1);
        return KinoError::OK;
      }
      return KinoError::PropertyNotSupported;*/
      char param[32];
      if (getSensorParam(s, paramIndex, param, sizeof(param))) {
        if ((s->isWritable()) && (strcmp(param, "status")==0)) {
          out.setInt(1);
          return KinoError::OK;
        }
        out.setNone();
        return KinoError::PropertyNotSupported;
      }
      out.setNone();
      return KinoError::OutOfRange;
    }
  }
  else if (strcmp(dev,"scenes")==0) {
    auto* s = getSceneByName(name);
    if (strcmp(act,"label")==0) {
      //out = KinoVariant::fromString(name);
      out.setString(name);
      return KinoError::OK;
    }
    if ((strcmp(act,"set")==0)||(strcmp(act,"savestate")==0)) {
      //out = KinoVariant::fromBool(false);
      out.setBool(false);
      return KinoError::OK;
    }
    if (strcmp(act,"tt")==0) {
      //out = KinoVariant::fromInt(s->getTT());
      out.setInt(s->getTT());
      return KinoError::OK;
    }
    int paramIndex; char rest[32];
    int found = sscanf(act,"param/%d%31s", &paramIndex, rest);
    if ((found == 1) || (strlen(rest) == 0)) {  // path = "scenes/<sceneName>/param/<paramIndex>"
      /*if (paramIndex > 2) return KinoError::OutOfRange; // Es gibt zu jeder Szene genau 2 Parameter: "set" und "savestate", beide write-only
      String getsetPath = "scenes/";
      getsetPath += name;
      getsetPath += "/";
      if (paramIndex == 0) getsetPath += "set";
      if (paramIndex == 1) getsetPath += "savestate";
      if (paramIndex == 2) getsetPath += "tt";
      out = KinoVariant::fromString(getsetPath.c_str());*/
      if ((paramIndex < 0) || (paramIndex > 2)) { out.setNone(); return KinoError::OutOfRange; }
      char getsetpath[128]; const char* gspEnd = nullptr;
      if (paramIndex == 0) gspEnd = "set";
      if (paramIndex == 1) gspEnd = "savestate";
      if (paramIndex == 2) gspEnd = "tt";
      snprintf(getsetpath, sizeof(getsetpath), "scenes/%s/%s", name, gspEnd);
      out.setString(getsetpath);
      return KinoError::OK;
    }
    if (strcmp(rest,"/label")==0) {
      if (paramIndex > 2) return KinoError::OutOfRange; // Es gibt zu jeder Szene genau 2 Parameter: "set" und "savestate"
      /*if (paramIndex == 0) out = KinoVariant::fromString("setzen");
      if (paramIndex == 1) out = KinoVariant::fromString("speichern");
      if (paramIndex == 2) out = KinoVariant::fromString("Transition Time [ms]");
      return KinoError::OK;*/
      if (paramIndex == 0) out.setString("setzen");
      if (paramIndex == 1) out.setString("speichern");
      if (paramIndex == 2) out.setString("Trans.Time[ms]");
      return KinoError::OK;
    }
    if (strcmp(rest,"/access")==0) {
      if (paramIndex > 2) {out.setNone(); return KinoError::OutOfRange; }
      if ((paramIndex == 0)||(paramIndex == 1)) {
        //out = KinoVariant::fromInt(2);  // set und savestate sind write-only
        out.setInt(2);
        return KinoError::OK;
      }
      //out = KinoVariant::fromInt(3);    // tt ist read-write
      out.setInt(3);
      return KinoError::OK;
    }
    if (strcmp(rest,"/minvalue")==0) {
      //if (paramIndex == 2) out = KinoVariant::fromInt(0);   // tt hat einen Minimalwert von 0ms
      if (paramIndex == 2) {
        out.setInt(0);
        return KinoError::OK;
      }
      out.setNone();
      return KinoError::PropertyNotSupported;
    }
    if (strcmp(rest,"/maxvalue")==0) {
      //if (paramIndex == 2) out = KinoVariant::fromInt(10000); // tt hat einen Maximalwert von 10000ms
      if (paramIndex == 2) {
        out.setInt(10000);
        return KinoError::OK;
      }
      out.setNone();
      return KinoError::PropertyNotSupported;
    }
    if (strcmp(rest,"/valuestep")==0) {
      //if (paramIndex == 2) out = KinoVariant::fromInt(100); // tt hat eine Schrittweite von 100ms
      if (paramIndex == 2) {
        out.setInt(100);
        return KinoError::OK;
      }
      out.setNone();
      return KinoError::PropertyNotSupported;
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
    if (!sendGroupState(0,jsonString)) return KinoError::InternalError;
    for (auto* l : _lights) { // aktualisiere die Lights im cache
      l->forceOn(val.asBool());
    }
    return KinoError::OK;
  }
  if ((strcmp(prop, "brightness")==0)||(strcmp(prop,"bri")==0)) {
    char jsonString[20];
    snprintf(jsonString,20,"{\"bri\":%i}", (val.asInt()));
    if (!sendGroupState(0,jsonString)) return KinoError::InternalError;
    for (auto* l : _lights) { // aktualisiere _lights- cache
      l->forceBri(val.asInt());
    }
    return KinoError::OK;
  }
  if (strcmp(prop, "scene")==0) {
    HueScene* s = getSceneByName(val.c_str());
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
      /*std::vector<String> params = getLightParams(l);
      out = params.size();*/
      out = getLightParamCount(l);
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
      //std::vector<uint8_t> lightids = g->getLightIds();
      const std::vector<uint8_t>& lightids = g->getLightIds();
      out = lightids.size();
      return KinoError::OK;
    }
    if (strcmp(rest,"param")==0) {
      /*std::vector<String> params = getGroupParams(g);
      out = params.size();*/
      out = getGroupParamCount(g);
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
    //out = KinoVariant::fromString(_lights[index]->getName().c_str());
    out.setString(_lights[index]->getName());
    return KinoError::OK;
  }
  if (strcmp(property, "groups")==0) {
    if (index > _groups.size()) return KinoError::OutOfRange;
    //out = KinoVariant::fromString(_groups[index]->getName().c_str());
    out.setString(_groups[index]->getName());
    return KinoError::OK;
  }
  if (strcmp(property, "scenes")==0) {
    if (index > _scenes.size()) return KinoError::OutOfRange;
    //out = KinoVariant::fromString(_scenes[index]->getName().c_str());
    out.setString(_scenes[index]->getName());
    return KinoError::OK;
  }
  if (strcmp(property, "sensors")==0) {
    if (index > _sensors.size()) return KinoError::OutOfRange;
    //out = KinoVariant::fromString(_sensors[index]->getName().c_str());
    out.setString(_sensors[index]->getName());
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
      //std::vector<uint8_t> lightids = g->getLightIds();
      const std::vector<uint8_t>& lightids = g->getLightIds();
      if (index > lightids.size()) return KinoError::OutOfRange;
      HueLight* l = getLightById(lightids[index]);
      if (!l) return KinoError::PropertyNotSupported;
      //out = KinoVariant::fromString(l->getName().c_str());
      out.setString(l->getName());
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
          //out = KinoVariant::fromString(kv.key().c_str());
          out.setString(kv.key().c_str());
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

void HueBridge::setGroupsDirtyFlags(uint8_t lightId) {
  for (auto* g : _groups) {
    g->setDirty(lightId);
  }
}

bool HueBridge::getStatusUpdate(const char* devName, JsonObject& root) {
  static size_t nextLight = 0;
  static size_t nextGroup = 0;
  static size_t nextSensor = 0;
  static int scan = 0;  // 0=lights, 1=groups, 2=sensors

  if (scan == 0) {
    scan = 1;
    HueLight* l = _lights[nextLight];
    nextLight++;
    if (nextLight == _lights.size()) nextLight = 0;
    
    if (!l) return false;

    if (l->isDirty()) {
      Serial.print("found light to update: "); Serial.println(l->getName());
      root["dev"] = devName;
      char path[32];
      snprintf(path, sizeof(path), "lights/%s", l->getName());
      path[sizeof(path)-1] = '\0';
      root["path"] = path;
      root["on"] = l->isOn();
      if (l->isDimmable()) root["bri"] = l->getBrightness();
      if (l->hasCTColor()) root["ct"] = l->getCT();
      if (l->hasXYColor()) {
        RgbColor c = l->getRGB();
        KinoVariant col = KinoVariant::fromColor(c.r, c.g, c.b);
        root["col"] = col.c_str();
      }
      l->clearDirty();
      nextLight++;
      if (nextLight == _lights.size()) nextLight = 0;
      return true;
    }
    return false;
  } else if (scan ==1) {
    scan = 2;
    HueGroup* g = _groups[nextGroup];
    nextGroup++;
    if (nextGroup == _groups.size()) nextGroup = 0;
    if (!g) return false;
    if (g->isDirty()) {
      root["dev"] = devName;
      char path[32];
      snprintf(path, sizeof(path), "groups/%s", g->getName());
      path[sizeof(path)-1] = '\0';
      root["path"] = path;
      bool anyOn = false;
      int totalBri = 0; size_t lCount = 0;
      for (uint8_t lid : g->getLightIds()) {
        HueLight* l = _lights[lid];
        if (!l) continue;
        if (l->isOn()) {
          anyOn = true;
          totalBri = l->isDimmable() ? l->getBrightness() : 255;
        }
        lCount++;
      }
      root["on"] = anyOn;
      root["bri"] = (int)(totalBri/lCount);
      g->clearDirty();
      return true;
    }
    return false;
  } else if (scan == 2) {
    // check sensor
    scan = 0;
    HueSensor* s = _sensors[nextSensor];
    nextSensor++;
    if (nextSensor == _sensors.size()) nextSensor = 0;
    if (!s) return false;
    if (s->isDirty()) {
      root["dev"] = devName;
      char path[32];
      snprintf(path, sizeof(path), "sensors/%s", s->getName());
      path[sizeof(path)-1] = '\0';
      root["path"] = path;
      JsonObjectConst curState = s->getState();
      for (JsonPairConst kv : curState) {
        root[kv.key()] = kv.value();
      }
      s->clearDirty();
      return true;
    }
  }
  return false;
}

bool HueBridge::needsCommit() {
  return true;
}

bool HueBridge::commit() {
  Serial.println(F("HueBridge::commit() : groups.applyChanges() start"));
  showMemory();
  for (auto& g : _groups) {
    g->applyChanges(this);
  }
  Serial.println(F("HueBridge::commit() : lights.applyChanges() start"));
  showMemory();
  for (auto& l : _lights) {
    l->applyChanges(this);
  }
  Serial.println(F("HueBridge::commit() : sensors.applyChanges() start"));
  showMemory();
  for (auto& s : _sensors) {
    s->applyChanges(this);
  }
  Serial.println(F("HueBridge::commit() : sensors.applyChanges() end"));
  showMemory();
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

int HueBridge::getLightParamCount(const HueLight* l) {
  if (!l) return 0;
  int count = 1;  // ón
  if (l->isDimmable()) count++; // bri
  if (l->hasCTColor()) count++; // ct
  if (l->hasXYColor()) count++; // col
  return count;
}

bool HueBridge::getLightParam(const HueLight* l, int paramIndex, char* out, size_t outLen) {
  const char* result = nullptr;
  int count = 0;

  // Wir zählen einfach manuell durch, welcher Parameter an welcher Stelle stünde
  if (paramIndex == count++) result = "on";
  else if (l->isDimmable() && paramIndex == count++) result = "bri";
  else if (l->hasCTColor() && paramIndex == count++) result = "ct";
  else if (l->hasXYColor() && paramIndex == count++) result = "col";
  else if ((l->isDimmable() || l->hasCTColor() || l->hasXYColor()) && paramIndex == count++) result = "tt";

  if (result) {
    strlcpy(out, result, outLen); // strlcpy ist sicherer als strncpy
    return true;
  }

  if (outLen > 0) out[0] = '\0';
  return false;
}

int HueBridge::getGroupParamCount(const HueGroup* g) {
  if (!g) return 0;
  int count = 1;  // on
  
  bool hasBri = false;
  bool hasCT  = false;
  
  const std::vector<uint8_t>& lightIds = g->getLightIds();  // Referenz ohne Kopie
  for(uint8_t lightId : lightIds) {
    auto* l = getLightById(lightId);
    if (l->isDimmable()) hasBri = true;
    if (l->hasCTColor()) hasCT  = true;
    if (hasBri && hasCT) break;
  }
  if (hasBri) count++;
  if (hasCT)  count++;
  return count;
}

// Helperfunktion zum Bestimmen der verfügbaren Parameter für eine Gruppe
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

bool HueBridge::getGroupParam(const HueGroup* g, int paramIndex, char* out, size_t outLen) {
  const char* result = nullptr;
  int count = 0;

  // 1. "on" ist immer da (Index 0)
  if (paramIndex == count++) {
    result = "on";
  } else {
    // 2. Wir prüfen die Eigenschaften der Lampen in der Gruppe
    bool hasBri = false;
    bool hasCT = false;

    // Nutze eine Referenz auf den Vector, um keine Kopie zu erzeugen!
    const std::vector<uint8_t>& lightIds = g->getLightIds(); 
    
    for (uint8_t lightId : lightIds) {
      HueLight* l = getLightById(lightId);
      if (l) {
        if (l->isDimmable()) hasBri = true;
        if (l->hasCTColor()) hasCT = true;
      }
      if (hasBri && hasCT) break; // Frühzeitiger Abbruch, wenn alles gefunden
    }

    // 3. Logische Zuordnung zum Index
    if (hasBri && paramIndex == count++) result = "bri";
    else if (hasCT && paramIndex == count++) result = "ct";
    else if ((hasBri || hasCT) && paramIndex == count++) result = "tt";
  }

  if (result) {
    strlcpy(out, result, outLen);
    return true;
  }

  if (outLen > 0) out[0] = '\0';
  return false;
}


std::vector<String> HueBridge::getSensorParams(const HueSensor* s) {
  JsonObjectConst states = s->getState();
  std::vector<String> params;
  for (JsonPairConst kv : states) {
    params.push_back(String(kv.key().c_str()));
  }
  return params;
}

bool HueBridge::getSensorParam(const HueSensor* s, int paramIndex, char* out, size_t outLen) {
  const char* result = nullptr;
  int count = 0;
  JsonObjectConst states = s->getState();
  for (JsonPairConst kv : states) {
    if (paramIndex == count++) { result = kv.key().c_str(); break; }
  }
  if (result) {
    strlcpy(out, result, outLen);
    return true;
  }
  if (outLen > 0) out[0] = '\0';
  return false;
}
// ===== Public API =====

bool HueBridge::begin() {
    return (init()==KinoError::OK);
}

KinoError HueBridge::init() {
    if (!readLights()) return KinoError::DeviceNotReady;
    if (!readGroups()) return KinoError::DeviceNotReady;
    if (!readScenes()) return KinoError::DeviceNotReady;
    readScenes();
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
    //showMemory();
    bool ok = (readLights()&&readSensors());
    //showMemory();
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

bool HueBridge::httpError(WiFiClient& client, const char* cause) {
    client.stop();
    Serial.print(F("Hue HTTP Error: "));
    Serial.println(cause);
    return false;
}

/* alte httpError()
// helper function for read* functions to stop the client and return false
bool HueBridge::httpError(const char* cause) {
  _client.stop();
  Serial.print("Hue HTTP Error: ");
  Serial.println(cause);
  return false;
}
*/

/* readLights() V1
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
*/

/* readLights() V2 Strings entfernt
bool HueBridge::readLights() {
    // 1. Pfad bauen ohne String-Objekte
    char path[64];
    snprintf(path, sizeof(path), "/api/%s/lights", _user.c_str());

    // 2. HTTP Request absetzen
    if (!httpGET(path)) return httpError("httpGet lights failed");
    
    // 3. Header überspringen mit dem effizienten NetworkHelper
    if (!NetworkHelper::skipHeader(_client)) return httpError("skipHeader failed");

    char buffer[32]; // Kleiner Puffer für die Mustersuche ("ID":{)
    size_t bufIdx = 0;
    memset(buffer, 0, sizeof(buffer));
    
    _client.setTimeout(2000); 

    while (_client.connected() || _client.available()) {
        if (!_client.available()) { 
            yield(); 
            continue; 
        }

        char c = _client.read();
        
        // Zeichen in Such-Puffer schieben
        buffer[bufIdx++] = c;
        if (bufIdx >= sizeof(buffer) - 1) {
            memmove(buffer, buffer + 1, sizeof(buffer) - 2);
            bufIdx = sizeof(buffer) - 2;
        }
        buffer[bufIdx] = '\0';

        // Wir suchen das Ende eines Keys, gefolgt von einer öffnenden Klammer, z.B. "12":{
        if (c == '{') {
            char* lastQuote = strrchr(buffer, '"');
            if (lastQuote) {
                char* firstQuote = nullptr;
                // Suche das öffnende Anführungszeichen des Keys rückwärts
                for (char* p = lastQuote - 1; p >= buffer; p--) {
                    if (*p == '"') { firstQuote = p; break; }
                }

                if (firstQuote) {
                    *lastQuote = '\0'; // Key terminieren
                    int id = atoi(firstQuote + 1);
                    
                    if (id > 0) {
                        // JETZT: deserializeJson übernimmt ab der aktuellen Position im Stream
                        // Es liest automatisch bis zum Ende des Objekts '}'
                        DynamicJsonDocument doc(2048);
                        DeserializationError error = deserializeJson(doc, _client);
                        
                        if (!error) {
                            updateOrAddLight(id, doc);
                        } else {
                            Serial.print(F("Hue JSON Error ID "));
                            Serial.print(id);
                            Serial.print(F(": "));
                            Serial.println(error.c_str());
                        }
                        // Puffer zurücksetzen, um nach der nächsten ID zu suchen
                        bufIdx = 0;
                        memset(buffer, 0, sizeof(buffer));
                    }
                }
            }
        }
    }
    _client.stop();
    return !_lights.empty();
}
*/

/* readLights() V3 helper functions findNextKey und updateOrAddLight eingeführt
bool HueBridge::readLights() {
    char path[64];
    snprintf(path, sizeof(path), "/api/%s/lights", _user.c_str());
    if (!httpGET(path) || !NetworkHelper::skipHeader(_client)) return false;

    char idStr[32];
    while (findNextKey(idStr, sizeof(idStr))) {
        DynamicJsonDocument doc(2048);
        if (deserializeJson(doc, _client) == DeserializationError::Ok) {
            updateOrAddLight(atoi(idStr), doc);
        }
    }
    _client.stop();
    return !_lights.empty();
}
*/

/* readLights() V4 mit wiederverwendetem JsonDocument
bool HueBridge::readLights() {
  WiFiClient client;
  char path[128];
  snprintf(path, sizeof(path), "/api/%s/lights", _user.c_str());
  if (!httpGET(client, path) || !NetworkHelper::skipHeader(client)) {
    Serial.println(F("Could not connect to bridge for readLights"));
    client.stop();
    return false;
  }

  char idStr[32];
  DynamicJsonDocument doc(2048);
  while (findNextKey(client, idStr, sizeof(idStr))) {
    doc.clear();
    if (deserializeJson(doc, client) == DeserializationError::Ok) {
      updateOrAddLight(atoi(idStr), doc);
    } else {
      Serial.print(F("Deserialization failed for light "));
      Serial.println(idStr);
    }
  }
  return !_lights.empty();
}*/

/* readLights() V5 neue Funktionssignatur für findNextKey */
bool HueBridge::readLights() {
  _globalDepth = 0;
  WiFiClient client;
  char path[64];
  snprintf(path, sizeof(path), "/api/%s/lights", _user);

  if (!httpGET(client, path) || !NetworkHelper::skipHeader(client)) {
    return httpError(client, "readLights connect failed");
  }

  char idStr[16];
  // 2026-02-02 : doc ersetzt durch _httpJson(1024), filter hinzugefügt
  //DynamicJsonDocument doc(2048); 
  _filter.clear();
  _filter["name"] = true; _filter["state"] = true; _filter["capabilities"]["control"]["ct"] = true;

  // Wichtig: findNextKey muss 'true' für numericOnly erhalten
  while (findNextKey(client, idStr, sizeof(idStr), true)) {
    //doc.clear();
    _httpJson.clear();
    
    // deserializeJson liest ab der '{' das KOMPLETTE Objekt der Lampe
    //DeserializationError err = deserializeJson(doc, client);
    DeserializationError err = deserializeJson(_httpJson, client, DeserializationOption::Filter(_filter));
    _globalDepth = 1;
    if (err == DeserializationError::Ok) {
      //updateOrAddLight(atoi(idStr), doc);
      updateOrAddLight(atoi(idStr), _httpJson);
    } else {
      Serial.print(F("JSON Error for ID "));
      Serial.print(idStr);
      Serial.print(F(": "));
      Serial.println(err.c_str());
      
      // Falls ein Parsing-Fehler auftritt, müssen wir zum nächsten 
      // Lampen-Anfang synchronisieren. findNextKey macht das automatisch.
    }
    // Den Such-Puffer in findNextKey kann man nicht von hier löschen, 
    // aber findNextKey fängt beim nächsten Aufruf eh frisch an zu sammeln.
  }
  
  client.stop();
  return !_lights.empty();
}


void HueBridge::updateOrAddLight(int id, JsonVariant doc) {
  const char* name = doc["name"] | "";
  bool on = doc["state"]["on"] | false;
  uint8_t bri = doc["state"]["bri"] | 0;
  bool hasBri = doc["state"].containsKey("bri");

  bool hasXY = false;
  float x = 0, y = 0;
  if (doc["state"].containsKey("xy") && doc["state"]["xy"].size() == 2) {
    x = doc["state"]["xy"][0].as<float>();
    y = doc["state"]["xy"][1].as<float>();
    hasXY = true;
  }

  bool hasCT = doc["state"].containsKey("ct");
  uint16_t ct = hasCT ? doc["state"]["ct"].as<uint16_t>() : 0;
  uint16_t minct = hasCT ? doc["capabilities"]["control"]["ct"]["min"].as<uint16_t>()|0 : 0;
  uint16_t maxct = hasCT ? doc["capabilities"]["control"]["ct"]["max"].as<uint16_t>()|0 : 0;

  HueLight* existing = getLightById(id);
  if (existing) {
    existing->updateValues(name, on, hasBri, bri, hasXY, x, y, hasCT, ct, minct, maxct);
    if (existing->isDirty()) setGroupsDirtyFlags(id);
  } else {
    // Sicherheitshalber prüfen wir, ob 'new' geklappt hat (Heap-Check)
    HueLight* newL = new HueLight(id, name, on, hasBri, bri, hasXY, x, y, hasCT, ct, minct, maxct);
    if (newL) {
      _lights.push_back(newL);
    } else {
      Serial.println(F("Critical: Out of Memory creating HueLight"));
    }
  }
}

HueLight* HueBridge::getLightById(uint8_t id) {
  for (auto* l : _lights) {
    if (l->getId() == id)
      return l;
  }
  return nullptr;
}

HueLight* HueBridge::getLightByName(const char* name) {
  for (auto* l : _lights) {
    //if (l->getName() == name) return l;
    if (strcmp(l->getName(), name)==0) return l;
  }
  return nullptr;
}

const std::vector<HueLight*>& HueBridge::getLights() const {
  return _lights;
}

HueGroup* HueBridge::getGroupById(uint8_t gid) {
  for (auto* g : _groups) {
    if (g->getId() == gid) return g;
  }
  return nullptr;
}

HueGroup* HueBridge::getGroupByName(const char* name) {
  for (auto* g : _groups) {
    //if (g->getName() == name) return g;
    if (strcmp(g->getName(), name)==0) return g;
  }
  return nullptr;
}

const std::vector<HueGroup*>& HueBridge::getGroups() const {
  return _groups;
}

/* readGroups() V1
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
*/

/* readGroups() V2 Strings entfernt
bool HueBridge::readGroups() {
    char path[64];
    snprintf(path, sizeof(path), "/api/%s/groups", _user.c_str());

    if (!httpGET(path)) return httpError("httpGet groups failed");
    if (!NetworkHelper::skipHeader(_client)) return httpError("skipHeader failed");

    // WICHTIG: Da wir hier neu einlesen, löschen wir die alten Gruppen
    for (auto* g : _groups) delete g;
    _groups.clear();

    char buffer[32]; 
    size_t bufIdx = 0;
    memset(buffer, 0, sizeof(buffer));
    
    _client.setTimeout(3000); 

    while (_client.connected() || _client.available()) {
        if (!_client.available()) { yield(); continue; }

        char c = _client.read();
        buffer[bufIdx++] = c;
        if (bufIdx >= sizeof(buffer) - 1) {
            memmove(buffer, buffer + 1, sizeof(buffer) - 2);
            bufIdx = sizeof(buffer) - 2;
        }
        buffer[bufIdx] = '\0';

        if (c == '{') {
            char* lastQuote = strrchr(buffer, '"');
            if (lastQuote) {
                char* firstQuote = nullptr;
                for (char* p = lastQuote - 1; p >= buffer; p--) {
                    if (*p == '"') { firstQuote = p; break; }
                }

                if (firstQuote) {
                    *lastQuote = '\0';
                    int id = atoi(firstQuote + 1);
                    
                    if (id >= 0) { // Gruppe 0 ist oft "All Lights"
                        DynamicJsonDocument doc(2048); // Genug Platz für Name + ID-Array
                        DeserializationError error = deserializeJson(doc, _client);
                        
                        if (!error) {
                            const char* name = doc["name"] | "";
                            std::vector<uint8_t> lightIds;

                            // Lampen-IDs effizient aus dem Array lesen
                            JsonArray lightsArr = doc["lights"].as<JsonArray>();
                            for (JsonVariant v : lightsArr) {
                                // Direkt in Zahl wandeln ohne String-Konstruktor
                                const char* lIdStr = v.as<const char*>();
                                if (lIdStr) lightIds.push_back((uint8_t)atoi(lIdStr));
                            }
                            
                            HueGroup* newG = new HueGroup(id, name, *this, lightIds);
                            if (newG) {
                                _groups.push_back(newG);
                            }
                        }
                        bufIdx = 0;
                        memset(buffer, 0, sizeof(buffer));
                    }
                }
            }
        }
    }
    _client.stop();
    return !_groups.empty();
}
*/

/* readGroups() V3 Helperfunktion addGroup() eingeführt
bool HueBridge::readGroups() {
  char path[64];
  snprintf(path, sizeof(path), "/api/%s/groups", _user.c_str());
  if (!httpGET(path) || !NetworkHelper::skipHeader(_client)) return false;

  for (auto* g : _groups) delete g;
  _groups.clear();

  char idStr[32];
  while (findNextKey(idStr, sizeof(idStr))) {
    DynamicJsonDocument doc(2048);
    if (deserializeJson(doc, _client) == DeserializationError::Ok) {
      // Logik zum Extrahieren der Light-IDs bleibt in updateOrAddGroup
      addGroup(atoi(idStr), doc);
    }
  }
  _client.stop();
  return !_groups.empty();
}
*/

/* readGroups() V4 lokalen WiFiClient eingeführt
bool HueBridge::readGroups() {
  WiFiClient client;
  char path[64];
  snprintf(path, sizeof(path), "/api/%s/groups", _user.c_str());
  if (!httpGET(client, path) || !NetworkHelper::skipHeader(client)) return false;

  for (auto* g : _groups) delete g;
  _groups.clear();

  char idStr[32];
  while (findNextKey(idStr, sizeof(idStr))) {
    DynamicJsonDocument doc(2048);
    if (deserializeJson(doc, client) == DeserializationError::Ok) {
      // Logik zum Extrahieren der Light-IDs bleibt in updateOrAddGroup
      addGroup(atoi(idStr), doc);
    }
  }
  _client.stop();
  return !_groups.empty();
}
*/

/* readGroups() V5 DynamicJsonDocument wiederverwendet (while-Schleife) */
bool HueBridge::readGroups() {
  _globalDepth = 0;
  WiFiClient client;
  char path[64];
  snprintf(path, sizeof(path), "/api/%s/groups", _user);
  if (!httpGET(client, path) || !NetworkHelper::skipHeader(client)) {
    Serial.println(F("could not connect to bridge for readGroups"));
    return false;
  }

  /*for (auto* g : _groups) delete g;
  _groups.clear();*/

  char idStr[32];
  // 2026-02-02 doc ersetzt durch _httpJson, _filter eingeführt
  //DynamicJsonDocument doc(2048);
  _filter.clear();
  _filter["name"]=true; _filter["lights"]=true;
  
  while (findNextKey(client, idStr, sizeof(idStr),true)) {
    //doc.clear();
    _httpJson.clear();
    if (deserializeJson(_httpJson, client, DeserializationOption::Filter(_filter)) == DeserializationError::Ok) {
      _globalDepth = 1;
      // Logik zum Extrahieren der Light-IDs bleibt in addGroup
      //addGroup(atoi(idStr), doc);
      updateOrAddGroup(atoi(idStr), _httpJson);
    } else {
      Serial.print(F("Deserialization failed for group "));
      Serial.println(idStr);
    }
  }
  return !_groups.empty();
}

void HueBridge::updateOrAddGroup(int id, JsonVariant doc) {
  //serializeJson(doc, Serial); 
  //Serial.println();
  const char* name = doc["name"] | "";
  std::vector<uint8_t> lightIds;

  JsonArray lightsArr = doc["lights"].as<JsonArray>();
  for (JsonVariant v : lightsArr) {
    const char* lIdStr = v.as<const char*>();
    if (lIdStr) lightIds.push_back((uint8_t)atoi(lIdStr));
  }
  HueGroup* existing = getGroupById(id);
  if (!existing) {
    HueGroup* newG = new HueGroup(id, name, *this, lightIds);
    if (newG) _groups.push_back(newG);
  } else {
    existing->updateValues(name, lightIds);
  }
}

/* readScenes() V1
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
*/

/* readScenes() V2 Strings entfernt
bool HueBridge::readScenes() {
    char path[128];
    snprintf(path, sizeof(path), "/api/%s/scenes", _user.c_str());

    if (!httpGET(path)) return httpError("httpGet scenes failed");
    if (!NetworkHelper::skipHeader(_client)) return httpError("skipHeader failed");

    // Speicher aufräumen
    for (auto* s : _scenes) delete s;
    _scenes.clear();

    char buffer[64]; // Kleiner Puffer für die Key-Suche
    size_t bufIdx = 0;
    memset(buffer, 0, sizeof(buffer));
    
    _client.setTimeout(4000); // Szenen-Listen können sehr lang sein

    while (_client.connected() || _client.available()) {
        if (!_client.available()) { yield(); continue; }

        char c = _client.read();
        buffer[bufIdx++] = c;
        
        // Puffer rotieren
        if (bufIdx >= sizeof(buffer) - 1) {
            memmove(buffer, buffer + 1, sizeof(buffer) - 2);
            bufIdx = sizeof(buffer) - 2;
        }
        buffer[bufIdx] = '\0';

        // Suche nach "ID":{
        if (c == '{') {
            char* lastQuote = strrchr(buffer, '"');
            if (lastQuote) {
                char* firstQuote = nullptr;
                for (char* p = lastQuote - 1; p >= buffer; p--) {
                    if (*p == '"') { firstQuote = p; break; }
                }

                if (firstQuote) {
                    *lastQuote = '\0'; // Key (Scene-ID) isolieren
                    const char* currentId = firstQuote + 1;

                    // Stream-Parsing startet hier
                    DynamicJsonDocument doc(2048);
                    DeserializationError error = deserializeJson(doc, _client);
                    
                    if (!error) {
                        const char* type = doc["type"] | "";
                        // Nur herkömmliche Lichtszenen speichern
                        if (strcmp(type, "LightScene") == 0) {
                            const char* name = doc["name"] | "";
                            
                            std::vector<uint8_t> lightIds;
                            JsonArray lightsArr = doc["lights"].as<JsonArray>();
                            for (JsonVariant v : lightsArr) {
                                const char* lIdStr = v.as<const char*>();
                                if (lIdStr) lightIds.push_back((uint8_t)atoi(lIdStr));
                            }
                            
                            // WICHTIG: currentId muss hier kopiert werden (HueScene Konstruktor)
                            HueScene* newS = new HueScene(currentId, name, lightIds);
                            if (newS) {
                                _scenes.push_back(newS);
                            }
                        }
                    }
                    bufIdx = 0;
                    memset(buffer, 0, sizeof(buffer));
                }
            }
        }
    }
    _client.stop();
    return !_scenes.empty();
}
*/

/* readScenes() V3 Helperfunktion addScene() eingeführt
bool HueBridge::readScenes() {
    char path[64];
    snprintf(path, sizeof(path), "/api/%s/scenes", _user.c_str());
    if (!httpGET(path) || !NetworkHelper::skipHeader(_client)) return false;

    for (auto* s : _scenes) delete s;
    _scenes.clear();

    char idStr[32]; // Szenen-IDs sind Strings!
    while (findNextKey(idStr, sizeof(idStr))) {
        DynamicJsonDocument doc(2048);
        if (deserializeJson(doc, _client) == DeserializationError::Ok) {
            // Hier übergeben wir idStr direkt als const char*
            addScene(idStr, doc); 
        }
    }
    _client.stop();
    return !_scenes.empty();
}
*/

/* readScenes() V4 lokalen WiFiClient eingeführt
bool HueBridge::readScenes() {
  WiFiClient client;
  char path[64];
  snprintf(path, sizeof(path), "/api/%s/scenes", _user.c_str());
  if (!httpGET(client, path) || !NetworkHelper::skipHeader(client)) return false;

  for (auto* s : _scenes) delete s;
  _scenes.clear();

  char idStr[32]; // Szenen-IDs sind Strings!
  while (findNextKey(idStr, sizeof(idStr))) {
    DynamicJsonDocument doc(2048);
    if (deserializeJson(doc, client) == DeserializationError::Ok) {
      // Hier übergeben wir idStr direkt als const char*
      addScene(idStr, doc); 
    }
  }
  return !_scenes.empty();
}
*/

/* readScenes() V5 JsonDocument wird wiederverwendet */
bool HueBridge::readScenes() {
  _globalDepth = 0;
  WiFiClient client;
  char path[64];
  snprintf(path, sizeof(path), "/api/%s/scenes", _user);
  if (!httpGET(client, path) || !NetworkHelper::skipHeader(client)) return false;
  
  for (auto* s : _scenes) delete s;
  _scenes.clear();

  char idStr[32]; // Szenen-IDs sind Strings!
  // 2026-02-02 doc ersetzt durch _httpJson, _filter eingeführt
  //DynamicJsonDocument doc(2048);
  _filter.clear();
  _filter["type"]=true; _filter["name"]=true; _filter["lights"]=true;
  
  while (findNextKey(client, idStr, sizeof(idStr),false)) {
    //doc.clear();
    _httpJson.clear();
    if (deserializeJson(_httpJson, client, DeserializationOption::Filter(_filter)) == DeserializationError::Ok) {
      // Hier übergeben wir idStr direkt als const char*
      addScene(idStr, _httpJson); 
    }
    _globalDepth = 1;
  }
  return !_scenes.empty();
}

void HueBridge::addScene(const char* idStr, JsonVariant doc) {
  //serializeJson(doc, Serial); 
  //Serial.println();
  const char* type = doc["type"] | "";
  if (strcmp(type, "LightScene") == 0) {
    const char* name = doc["name"] | "";
    
    std::vector<uint8_t> lightIds;
    JsonArray lightsArr = doc["lights"].as<JsonArray>();
    for (JsonVariant v : lightsArr) {
      const char* lIdStr = v.as<const char*>();
      if (lIdStr) lightIds.push_back((uint8_t)atoi(lIdStr));
    }
    
    HueScene* newS = new HueScene(idStr, name, lightIds);
    if (newS) _scenes.push_back(newS);
  }
}

/* getSceneByName() V1
HueScene* HueBridge::getSceneByName(const String& name) {
    for (auto* s : _scenes) {
        if (s->getName() == name)
            return s;
    }
    return nullptr;
}
*/

/* getSceneByName() V2 Strings entfernt */
HueScene* HueBridge::getSceneByName(const char* name) {
  for (auto* s : _scenes) {
    //if (strcmp(tmpname, name)==0) return s;
    if (strcmp(s->getName(), name)==0) return s;
  }
  return nullptr;
}

/*
const std::vector<HueScene*>& HueBridge::getScenes() const {
    return _scenes;
}*/

/* setScene() V1
bool HueBridge::setScene(const String& sceneName) {
  HueScene* s = getSceneByName(sceneName.c_str());
  if (s) return s->setActive(this);
  return false;
}
*/

/* setScene() V2 Strings entfernt */
bool HueBridge::setScene(const char* sceneName) {
  HueScene* s = getSceneByName(sceneName);
  if (s) return s->setActive(this);
  return false;
}

/* getScenePowerStates() V1
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
}*/

std::map<uint8_t, bool> HueBridge::getScenePowerStates(const char* sceneId) {
  std::map<uint8_t, bool> results;
  WiFiClient client;
  char path[128];
  snprintf(path, sizeof(path), "/api/%s/scenes/%s", _user, sceneId);

  if (httpGET(client, path) && NetworkHelper::skipHeader(client)) {
    // Filter erstellen: Wir wollen nur "lightstates" -> "alle IDs" -> "on"
    StaticJsonDocument<128> filter;
    filter["lightstates"][true]["on"] = true;

    // Dank Filter reicht jetzt ein deutlich kleineres Dokument!
    // 1024-2048 Bytes sollten selbst für viele Lampen dicke reichen.
    DynamicJsonDocument doc(2048); 
    DeserializationError error = deserializeJson(doc, client, DeserializationOption::Filter(filter));

    if (!error) {
      JsonObject lightstates = doc["lightstates"];
      for (JsonPair p : lightstates) {
        results[atoi(p.key().c_str())] = p.value()["on"] | false;
      }
    }
  }
  return results;
}


/* getSceneLightStates() V1
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
*/

/* getSceneLightStates() V2: Strings entfernt, direktes JSON parsing mit Filter, lokaler WiFiClient */ 
SceneLightStates HueBridge::getSceneLightStates(const char* sceneId) {
  SceneLightStates result;
  WiFiClient client; // Lokaler Client zum Schutz vor Exception 29
  
  char path[128];
  snprintf(path, sizeof(path), "/api/%s/scenes/%s", _user, sceneId);

  if (!httpGET(client, path) || !NetworkHelper::skipHeader(client)) {
      return result; 
  }

  // Filter erstellen: Wir wollen nur den "lightstates"-Zweig
  // Innerhalb der IDs interessieren uns nur on, bri und ct
  StaticJsonDocument<192> filter;
  JsonObject lightstatesFilter = filter.createNestedObject("lightstates");
  // [true] ist ein Platzhalter für "alle Keys in diesem Objekt"
  JsonObject idPattern = lightstatesFilter.createNestedObject("*"); 
  idPattern["on"] = true;
  idPattern["bri"] = true;
  idPattern["ct"] = true;

  // Dank Filter reicht ein moderates DynamicJsonDocument
  DynamicJsonDocument doc(3072); 
  DeserializationError error = deserializeJson(doc, client, DeserializationOption::Filter(filter));

  if (!error) {
    JsonObject lightstates = doc["lightstates"];
    for (JsonPair kv : lightstates) {
      uint8_t lightId = (uint8_t)atoi(kv.key().c_str());
      JsonObject obj = kv.value().as<JsonObject>();

      SceneLightState state;
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
  } else {
    Serial.print(F("Hue Scene Parsing Error: "));
    Serial.println(error.c_str());
  }

  // client.stop() erfolgt automatisch durch Destruktor beim Verlassen
  return result;
}




/* readSensors() V1
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
*/

/* readSensors() V2 Strings entfernt
bool HueBridge::readSensors() {
    // 1. Pfad bauen ohne String-Objekte
    char path[64];
    snprintf(path, sizeof(path), "/api/%s/sensors", _user.c_str());

    // 2. HTTP Request absetzen
    if (!httpGET(path)) return httpError("httpGet sensors failed");
    
    // 3. Header überspringen
    if (!NetworkHelper::skipHeader(_client)) return httpError("skipHeader failed");

    char buffer[32]; // Such-Puffer für "ID":{
    size_t bufIdx = 0;
    memset(buffer, 0, sizeof(buffer));
    
    _client.setTimeout(2000); 

    while (_client.connected() || _client.available()) {
        if (!_client.available()) { 
            yield(); 
            continue; 
        }

        char c = _client.read();
        
        buffer[bufIdx++] = c;
        if (bufIdx >= sizeof(buffer) - 1) {
            memmove(buffer, buffer + 1, sizeof(buffer) - 2);
            bufIdx = sizeof(buffer) - 2;
        }
        buffer[bufIdx] = '\0';

        if (c == '{') {
            char* lastQuote = strrchr(buffer, '"');
            if (lastQuote) {
                char* firstQuote = nullptr;
                for (char* p = lastQuote - 1; p >= buffer; p--) {
                    if (*p == '"') { firstQuote = p; break; }
                }

                if (firstQuote) {
                    *lastQuote = '\0';
                    int id = atoi(firstQuote + 1);
                    
                    if (id > 0) {
                        // Stream-Parsing: Liest direkt bis zum Ende des Sensor-Objekts
                        DynamicJsonDocument doc(2048);
                        DeserializationError error = deserializeJson(doc, _client);
                        
                        if (!error) {
                            updateOrAddSensor(id, doc);
                        }
                        bufIdx = 0;
                        memset(buffer, 0, sizeof(buffer));
                    }
                }
            }
        }
    }
    _client.stop();
    return !_sensors.empty();
}*/

/* readSensors() V3 Helperfunktion updateOrAddSensor() eingeführt
bool HueBridge::readSensors() {
    char path[64];
    snprintf(path, sizeof(path), "/api/%s/sensors", _user.c_str());
    if (!httpGET(path) || !NetworkHelper::skipHeader(_client)) return false;

    char idStr[32];
    while (findNextKey(idStr, sizeof(idStr))) {
        DynamicJsonDocument doc(2048);
        if (deserializeJson(doc, _client) == DeserializationError::Ok) {
            updateOrAddSensor(atoi(idStr), doc);
        }
    }
    _client.stop();
    return !_sensors.empty();
}
*/

/* readSensors() V4 lokalen WiFiClient eingeführt 
bool HueBridge::readSensors() {
  _globalDepth = 0;
  WiFiClient client;
  char path[64];
  snprintf(path, sizeof(path), "/api/%s/sensors", _user.c_str());
  if (!httpGET(client, path) || !NetworkHelper::skipHeader(client)) return false;

  char idStr[32];
  while (findNextKey(client, idStr, sizeof(idStr),true)) {
    DynamicJsonDocument doc(2048);
    DeserializationError err = deserializeJson(doc, client);
    if (err == DeserializationError::Ok) {
      updateOrAddSensor(atoi(idStr), doc);
    }else {
      Serial.print(F("JSON Error for ID "));
      Serial.print(idStr);
      Serial.print(F(": "));
      Serial.println(err.c_str());
    }
    _globalDepth = 1;
  }
  return !_sensors.empty();
} */

/* readSensors() V5 Json-Filter eingeführt wegen zu grosser Antworten von der Bridge */
bool HueBridge::readSensors() {
    _globalDepth = 0;
    WiFiClient client;
    char path[64];
    snprintf(path, sizeof(path), "/api/%s/sensors", _user);

    if (!httpGET(client, path) || !NetworkHelper::skipHeader(client)) return false;

    char idStr[16];
    // 2026-02-02 filter ersetzt durch _filter
    
    // Filter definieren, um den RAM-Bedarf pro Sensor zu drücken
    //StaticJsonDocument<96> filter;
    //filter["name"] = true;
    //filter["type"] = true;
    //filter["state"] = true; // Wir nehmen das ganze state-Objekt
    _filter.clear();
    _filter["name"] = true;
    _filter["type"] = true;
    _filter["state"] = true; // Wir nehmen das ganze state-Objekt
    // 2026-02-02 doc ersetzt durch _httpJson
    //DynamicJsonDocument doc(2048); 

    while (findNextKey(client, idStr, sizeof(idStr), true)) {
        //doc.clear();
        _httpJson.clear();
        // Parsing mit Filter!
        DeserializationError err = deserializeJson(_httpJson, client, DeserializationOption::Filter(_filter));
        
        _globalDepth = 1; // "Back to track" Synchronisation

        if (err == DeserializationError::Ok) {
            updateOrAddSensor(atoi(idStr), _httpJson);
        } else {
            Serial.print(F("Sensor JSON Error ID "));
            Serial.print(idStr);
            Serial.print(F(": ")); 
            Serial.println(err.c_str());
            
            // WICHTIG: Wenn der Parser abbricht, müssen wir den Rest des 
            // aktuellen Objekts im Stream überspringen, sonst findet 
            // findNextKey nur Müll.
            client.find((char*)"},"); // Versuche zum nächsten Geschwister-Element zu springen
        }
    }
    client.stop();
    return !_sensors.empty();
}

void HueBridge::updateOrAddSensor(int id, JsonVariant doc) {
  //serializeJson(doc, Serial); 
  //Serial.println();
  HueSensor* s = getSensorById(id);
  
  if (!s) {
    // Bei neuen Sensoren müssen wir die Strings (Name/Type) extrahieren
    const char* name = doc["name"] | "";
    const char* type = doc["type"] | "";

    s = new HueSensor(id, name, type);
    if (s) {
      _sensors.push_back(s);
    } else {
      Serial.println(F("Critical: Out of Memory creating HueSensor"));
      return;
    }
  }

  // State updaten, falls vorhanden
  if (doc.containsKey("state")) {
    // Wir übergeben das JsonObject direkt an die update-Logik
    s->updateState(doc["state"].as<JsonObject>());
  }
}


HueSensor* HueBridge::getSensorById(uint16_t sensorId) {
  for (auto* s : _sensors) {
    if (s->getId() == sensorId) return s;
  }
  return nullptr;
}

HueSensor* HueBridge::getSensorByName(const char* name) {
  for (auto* s : _sensors) {
    //if (s->getName() == name) return s;
    if (strcmp(s->getName(), name)==0) return s;
  }
  return nullptr;
}

/* setSensorState() V1
bool HueBridge::setSensorState(uint16_t id, const JsonObject& state) {
    String path =
        "/api/" + _user + "/sensors/" +
        String(id) + "/state";

    String payload;
    serializeJson(state, payload);

    return sendState(path, payload);
}
*/

/* setSensorState() V2 Strings entfernt */
bool HueBridge::setSensorState(uint16_t id, const JsonObject& state) {
    char path[128];
    snprintf(path, sizeof(path), "/api/%s/sensors/%d/state", _user, id);
    char payload[256];
    serializeJson(state, payload);

    return sendState(path, payload);
}


// ===== HTTP =====

/* findNextKey() V1
// Gibt true zurück, wenn ein Key gefunden wurde. 
// Schreibt die ID in 'out' und lässt den Stream exakt vor der '{' stehen.
bool HueBridge::findNextKey(char* out, size_t outSize) {
    char buffer[48]; // Reicht für Hue-IDs dicke
    size_t bufIdx = 0;
    memset(buffer, 0, sizeof(buffer));

    while (_client.connected() || _client.available()) {
        if (!_client.available()) { yield(); continue; }
        
        char c = _client.read();
        if (c == '{') {
            char* lastQuote = strrchr(buffer, '"');
            if (lastQuote) {
                char* firstQuote = nullptr;
                for (char* p = lastQuote - 1; p >= buffer; p--) {
                    if (*p == '"') { firstQuote = p; break; }
                }
                if (firstQuote) {
                    *lastQuote = '\0';
                    strlcpy(out, firstQuote + 1, outSize);
                    return true;
                }
            }
        }

        // Puffer-Rotation
        buffer[bufIdx++] = c;
        if (bufIdx >= sizeof(buffer) - 1) {
            memmove(buffer, buffer + 1, sizeof(buffer) - 2);
            bufIdx = sizeof(buffer) - 2;
        }
        buffer[bufIdx] = '\0';
    }
    return false;
}
*/

/* findNextKey() V2 lokalen WiFiClient als Parameter eingeführt 
bool HueBridge::findNextKey(WiFiClient& client, char* out, size_t outSize) {
  char buffer[32]; // Kleiner Such-Puffer
  size_t bufIdx = 0;
  memset(buffer, 0, sizeof(buffer));

  while (client.connected() || client.available()) {
    if (!client.available()) { yield(); continue; }
    char c = client.read();
    if (c == '{') {
      char* lastQuote = strrchr(buffer, '"');
      if (lastQuote) {
        char* firstQuote = nullptr;
        for (char* p = lastQuote - 1; p >= buffer; p--) {
          if (*p == '"') { firstQuote = p; break; }
        }
        if (firstQuote) {
          *lastQuote = '\0';
          strlcpy(out, firstQuote + 1, outSize);
          return true;
        }
      }
    }
    buffer[bufIdx++] = c;
    if (bufIdx >= sizeof(buffer) - 1) {
      memmove(buffer, buffer + 1, sizeof(buffer) - 2);
      bufIdx = sizeof(buffer) - 2;
    }
    buffer[bufIdx] = '\0';
  }
  return false;
}
*/

/* findNextKey() V3 neuer Check für numerische Keys 
bool HueBridge::findNextKey(WiFiClient& client, char* out, size_t outSize, bool numericOnly) {
  char buffer[48]; // Etwas größer für längere Szenen-IDs
  size_t bufIdx = 0;
  memset(buffer, 0, sizeof(buffer)); // <--- DER RESET: Jedes Mal bei Aufruf frisch

  while (client.connected() || client.available()) {
    if (!client.available()) { 
      yield(); 
      continue; 
    }
    
    char c = client.read();
    
    if (c == '{') {
      char* lastQuote = strrchr(buffer, '"');
      if (lastQuote) {
        char* firstQuote = nullptr;
        for (char* p = lastQuote - 1; p >= buffer; p--) {
          if (*p == '"') { firstQuote = p; break; }
        }

        if (firstQuote) {
          *lastQuote = '\0'; // Key isolieren
          const char* foundKey = firstQuote + 1;
          
          // Validierung
          bool isValid = true;
          if (numericOnly) {
            // Prüfen, ob es wirklich eine Zahl ist (Hue Lampen/Gruppen/Sensoren)
            if (strlen(foundKey) == 0) isValid = false;
            for (size_t i = 0; i < strlen(foundKey); i++) {
              if (!isdigit(foundKey[i])) { isValid = false; break; }
            }
          }

          if (isValid) {
            strlcpy(out, foundKey, outSize);
            return true; // Wir stehen im Stream direkt NACH der '{'
          }
        }
      }
    }
    
    // Zeichen in Puffer schieben
    buffer[bufIdx++] = c;
    if (bufIdx >= sizeof(buffer) - 1) {
      memmove(buffer, buffer + 1, sizeof(buffer) - 2);
      bufIdx = sizeof(buffer) - 2;
    }
    buffer[bufIdx] = '\0';
  }
  return false;
}*/

/* findNextKey() V4 Schutz durch Mitzählen der Klammertiefe 
bool HueBridge::findNextKey(WiFiClient& client, char* out, size_t outSize, bool numericOnly) {
  char buffer[48]; 
  size_t bufIdx = 0;
  memset(buffer, 0, sizeof(buffer));

  // Wir starten auf Ebene 0 (außerhalb des Hauptobjekts)
  // Oder Ebene 1, wenn wir schon im Haupt-JSON sind.
  //static int globalDepth = 0; 

  while (client.connected() || client.available()) {
    if (!client.available()) { yield(); continue; }
    char c = client.read();

    if (c == '{') {
      _globalDepth++;
      // Ein Key ist NUR dann eine ID (Lampe/Szene), wenn wir uns 
      // direkt auf der ersten Ebene unter dem Hauptobjekt befinden (Ebene 2)
      if (_globalDepth == 2) {
        char* lastQuote = strrchr(buffer, '"');
        if (lastQuote) {
          char* firstQuote = nullptr;
          for (char* p = lastQuote - 1; p >= buffer; p--) {
            if (*p == '"') { firstQuote = p; break; }
          }
          if (firstQuote) {
            *lastQuote = '\0';
            const char* foundKey = firstQuote + 1;

            bool isValid = true;
            if (numericOnly) {
              if (strlen(foundKey) == 0) isValid = false;
              for (size_t i = 0; i < strlen(foundKey); i++) {
                if (!isdigit(foundKey[i])) { isValid = false; break; }
              }
            }

            if (isValid) {
              strlcpy(out, foundKey, outSize);
              // Wir geben zurück, bleiben aber auf Ebene 2 stehen
              return true; 
            }
          }
        }
      }
    } else if (c == '}') {
      _globalDepth--;
    }

    // Puffer füllen (nur für Keys interessant)
    if (c != '{' && c != '}' && c != '[' && c != ']') {
        buffer[bufIdx++] = c;
        if (bufIdx >= sizeof(buffer) - 1) {
          memmove(buffer, buffer + 1, sizeof(buffer) - 2);
          bufIdx = sizeof(buffer) - 2;
        }
        buffer[bufIdx] = '\0';
    } else {
        // Bei jeder Klammer Puffer löschen, da ein Key nie über Klammern geht
        bufIdx = 0;
        buffer[0] = '\0';
    }

    // Wenn wir wieder auf Ebene 0 sind, ist das gesamte JSON-Dokument zu Ende
    if (_globalDepth <= 0 && bufIdx > 0) {
        // Sicherheitshalber Reset für den nächsten Request
        _globalDepth = 0; 
    }
  }
  //_globalDepth = 0; // Reset für den nächsten Aufruf (neue HTTP Query)
  return false;
}*/

/* findNextKey() V5 Schutz vor Zerstörung des Json durch Verwendung von peek() statt read() */
bool HueBridge::findNextKey(WiFiClient& client, char* out, size_t outSize, bool numericOnly) {
  char buffer[64]; 
  size_t bufIdx = 0;
  memset(buffer, 0, sizeof(buffer));

  while (client.connected() || client.available()) {
    if (!client.available()) { yield(); continue; }
    
    // peek() schaut nur das nächste Zeichen an, ohne es aus dem Stream zu löschen
    char c = client.peek(); 

    if (c == '{') {
      // NUR wenn wir auf der richtigen Ebene sind, prüfen wir den Key
      if (_globalDepth == 1) { // Wir sind im Hauptobjekt, suchen Kinder auf Ebene 1
        char* lastQuote = strrchr(buffer, '"');
        if (lastQuote) {
          char* firstQuote = nullptr;
          for (char* p = lastQuote - 1; p >= buffer; p--) {
            if (*p == '"') { firstQuote = p; break; }
          }
          if (firstQuote) {
            *lastQuote = '\0';
            const char* foundKey = firstQuote + 1;
            bool isValid = true;
            if (numericOnly) {
              for (size_t i = 0; i < strlen(foundKey); i++) {
                if (!isdigit(foundKey[i])) { isValid = false; break; }
              }
            }
            if (isValid) {
              strlcpy(out, foundKey, outSize);
              // WICHTIG: Wir lassen die '{' im Stream! deserializeJson wird sie lesen.
              // Aber wir müssen unsere interne Tiefe manuell erhöhen, 
              // da wir gleich 'return' machen und die '{' im Stream bleibt.
              _globalDepth++; 
              return true; 
            }
          }
        }
      }
      _globalDepth++; // Normales Hochzählen für Unterobjekte
    } else if (c == '}') {
      _globalDepth--;
    }

    // Jetzt das Zeichen wirklich aus dem Stream entfernen
    client.read(); 

    if (c != '{' && c != '}' && c != '[' && c != ']') {
        buffer[bufIdx++] = c;
        if (bufIdx >= sizeof(buffer) - 1) {
          memmove(buffer, buffer + 1, sizeof(buffer) - 2);
          bufIdx = sizeof(buffer) - 2;
        }
        buffer[bufIdx] = '\0';
    } else {
        bufIdx = 0;
        buffer[0] = '\0';
    }
  }
  return false;
}

bool HueBridge::httpGET(WiFiClient& client, const char* path) {
    EnsureTimeoutBeforeRequest(100);
    client.stop(); // Bestehende Verbindung dieses lokalen Clients kappen
    if (!client.connect(_ip, 80)) return false;

    client.print(F("GET ")); client.print(path); client.println(F(" HTTP/1.1"));
    client.print(F("Host: ")); client.println(_ip);
    client.println(F("Connection: close\r\n"));

    return waitForData(client);
}

/* alte httpGet(const String& path)
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
*/

bool HueBridge::sendLightState(uint8_t lightId, const char* jsonPayload) {
  //String path = "/api/" + _user + "/lights/" + String(lightId) + "/state";
  char path[128];
  snprintf(path, sizeof(path), "/api/%s/lights/%d/state", _user, lightId);
  return sendState(path, jsonPayload);
}

/* sendGroupState() V1
bool HueBridge::sendGroupState(uint16_t groupId, const String& jsonPayload) {
  String path = "/api/" + _user + "/groups/" + String(groupId) + "/action";
  return sendState(path, jsonPayload);
}
*/

/* sendGroupState V2: Strings entfernt */
bool HueBridge::sendGroupState(uint16_t groupId, const char* jsonPayload) {
  char path[64];
  snprintf(path, sizeof(path), "/api/%s/groups/%d/action", _user, groupId);
  return sendState(path, jsonPayload);
}

bool HueBridge::saveScene(const char* sceneId, const char* jsonPayload) {
    //String path = "/api/" + _user + "/scenes/" + sceneId;
    char path[128];
    snprintf(path, sizeof(path), "/api/%s/scenes/%s", _user, jsonPayload);
    return sendState(path, jsonPayload);
}

/* sendState() V1
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
*/

/* sendState() V2: Strings entfernt, lokalen WiFiClient eingeführt */
bool HueBridge::sendState(const char* path, const char* jsonPayload) {
  EnsureTimeoutBeforeRequest(100);
  WiFiClient client;
  if (!client.connect(_ip, 80)) return false;

  client.print(F("PUT ")); client.print(path); client.println(F(" HTTP/1.1"));
  client.print(F("Host: ")); client.println(_ip);
  client.print(F("Content-Length: ")); client.println(strlen(jsonPayload));
  client.println(F("Content-Type: application/json"));
  client.println(F("Connection: close\r\n"));
  client.print(jsonPayload);

  client.flush();
  if (waitForData(client, 1000)) {
    while (client.available() > 0) {
      client.read(); // Liest ein Byte und verwirft es sofort
      yield();       // Verhindert Watchdog-Probleme bei großen Antworten
    }
    client.stop();
    return true;
  }
  client.stop();
  return false;
}



/* waitForData() V1
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
*/

/* waitForData() V2 lokalen WiFiClient als Parameter eingeführt */
bool HueBridge::waitForData(WiFiClient& client, uint32_t timeout) {
  uint32_t start = millis();
  while (client.connected() && !client.available()) {
    if (millis() - start > timeout) {
      client.stop();
      return false;
    }
    yield();
  }
  return client.available() > 0;
}


/* deprecated skipHttpHeader()  ersetzt durch NetworkHelper::skipHttpHeader() 
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
*/

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


/* alte skipHttpHeader()

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
