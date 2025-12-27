#include "config.h"
#include <ESP8266WiFi.h>
#include "YamahaReceiver.h"
#include "OptomaBeamer.h"
#include "WLEDDevice.h"
#include "HyperionDevice.h"
#include "HueBridge.h"
#include "KinoAPI.h"
#include "KinoMacroEngine.h"
#include "SerialCommandDispatcher.h"
#include "KinoDeviceFactory.h"


const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
/*YamahaReceiver yamaha(YAMAHA_IP);
OptomaBeamer beamer(BEAMER_IP,BEAMER_ID);
WLEDDevice canvas(CANVAS_IP);
WLEDDevice sound(SOUND_IP);
HyperionDevice hyperion(HYPERION_IP);
HueBridge hue(HUE_BRIDGE_IP,HUE_TOKEN);*/
#include "YamahaReceiver.h"
#include "WLEDDevice.h"
#include "HueBridge.h"

YamahaReceiver* _yamaha = nullptr;
WLEDDevice* _canvas = nullptr;
WLEDDevice* _sound  = nullptr;
HueBridge* _hue     = nullptr;
OptomaBeamer* _beamer = nullptr;
HyperionDevice* _hyperion = nullptr;

#include "tests.h"

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    
    _yamaha   = KinoDeviceFactory::createYamaha({ YAMAHA_IP });
    _beamer   = KinoDeviceFactory::createOptoma({ BEAMER_IP, BEAMER_ID});
    _canvas   = KinoDeviceFactory::createWLED({ CANVAS_IP });
    _sound    = KinoDeviceFactory::createWLED({ SOUND_IP });
    _hue      = KinoDeviceFactory::createHue({ HUE_BRIDGE_IP, HUE_TOKEN });
    _hyperion = KinoDeviceFactory::createHyperion({HYPERION_IP});

    if (!_yamaha || !_beamer || !_canvas || !_sound || !_hue || !_hyperion) {
      Serial.println("FATAL: Device creation failed");
      Serial.println("Reboot to try again");
      while (true) yield();
    }

    Serial.println("Setze nun Tick-Intervalle");
    
    _yamaha->setTickInterval(0);
    _beamer->setTickInterval(0);
    _canvas->setTickInterval(0);
    _sound->setTickInterval(0);
    _hyperion->setTickInterval(0);
    _hue->setTickInterval(0);

    Serial.println("Initialisiere Geräte:");
    Serial.print("\tWLEDDevice canvas      : "); Serial.println(_canvas->begin()  ? F("✅ OK") : F("❌ Fehler"));
    Serial.print("\tWLEDDevice sound       : "); Serial.println(_sound->begin()   ? F("✅ OK") : F("❌ Fehler"));
    Serial.print("\tHueBridge  hue         : "); Serial.println(_hue->begin()     ? F("✅ OK") : F("❌ Fehler"));
    Serial.print("\tYamahaReceiver yamaha  : "); Serial.println(_yamaha->begin()  ? F("✅ OK") : F("❌ Fehler"));
    Serial.print("\tOptomaBeamer beamer    : "); Serial.println(_beamer->begin()  ? F("✅ OK") : F("❌ Fehler"));
    Serial.print("\tHyperionDevice hyperion: "); Serial.println(_hyperion->begin()? F("✅ OK") : F("❌ Fehler"));
    
    Serial.println("\nSerial Command Dispatcher V1.0 ready");

    if (!KinoAPI::startMacroEngine()) {
      Serial.println("MacroEngine konnte nicht gestartet werden!");
    } else {
      Serial.println("MacroEngine bereit");
    }

    
}



void loop() {
  if (_yamaha->tick()) showYamahaTicker();
  if (_beamer->tick()) showBeamerTicker();
  if (_canvas->tick()) showCanvasTicker();
  if (_sound->tick())  showSoundTicker();
  if (_hyperion->tick())showHyperionTicker();
  if (_hue->tick())    showHueTicker();
  handleSerialCommands();
  KinoAPI::handleMacroTicks();
  yield();
}
