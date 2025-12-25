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


const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
YamahaReceiver yamaha(YAMAHA_IP);
OptomaBeamer beamer(BEAMER_IP,BEAMER_ID);
WLEDDevice canvas(CANVAS_IP);
WLEDDevice sound(SOUND_IP);
HyperionDevice hyperion(HYPERION_IP);
HueBridge hue(HUE_BRIDGE_IP,HUE_TOKEN);

#include "tests.h"

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    
    
    yamaha.setTickInterval(0);
    beamer.setTickInterval(0);
    canvas.setTickInterval(0);
    sound.setTickInterval(0);
    hyperion.setTickInterval(0);
    hue.setTickInterval(0);

    Serial.println("Initialisiere Geräte:");
    Serial.print("\tWLEDDevice canvas      : "); Serial.println(canvas.begin()  ? F("✅ OK\n") : F("❌ Fehler\n"));
    Serial.print("\tWLEDDevice sound       : "); Serial.println(sound.begin()   ? F("✅ OK\n") : F("❌ Fehler\n"));
    Serial.print("\tHueBridge  hue         : "); Serial.println(hue.begin()     ? F("✅ OK\n") : F("❌ Fehler\n"));
    Serial.print("\tYamahaReceiver yamaha  : "); Serial.println(yamaha.begin()  ? F("✅ OK\n") : F("❌ Fehler\n"));
    Serial.print("\tOptomaBeamer beamer    : "); Serial.println(beamer.begin()  ? F("✅ OK\n") : F("❌ Fehler\n"));
    Serial.print("\tHyperionDevice hyperion: "); Serial.println(hyperion.begin()? F("✅ OK\n") : F("❌ Fehler\n"));
    
    Serial.println("\nSerial Command Dispatcher V1.0 ready");

    if (!KinoAPI::startMacroEngine()) {
      Serial.println("MacroEngine konnte nicht gestartet werden!");
    } else {
      Serial.println("MacroEngine bereit");
    }
}



void loop() {
  if (yamaha.tick()) showYamahaTicker();
  if (beamer.tick()) showBeamerTicker();
  if (canvas.tick()) showCanvasTicker();
  if (sound.tick())  showSoundTicker();
  if (hyperion.tick())showHyperionTicker();
  if (hue.tick())    showHueTicker();
  handleSerialCommands();
  KinoAPI::handleMacroTicks();
  yield();
}
