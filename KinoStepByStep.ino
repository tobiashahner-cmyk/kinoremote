#include "config.h"
#include <ESP8266WiFi.h>
#include "KinoAPI.h"
#include "KinoMacroEngine.h"
#include "SerialCommandDispatcher.h"
#include "KinoDeviceFactory.h"
#include "webserver.h"
//#include "tests.h"

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

WiFiClient globalWifiClient;
bool isReconnecting = false;
bool isConnected = false;

void reconnectWiFi() {
  static unsigned long lastCall = 0;
  unsigned long now = millis();
  // Sperre: Wenn der letzte Aufruf weniger als 10 Sekunden her ist, mach gar nichts
  if (now - lastCall < 10000) return; 
  isReconnecting = true;
  isConnected = false;
  Serial.println(F("Reconnecting WiFi"));
  globalWifiClient.stop();
  WiFi.disconnect(false);
  lastCall = now;
}


void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    Serial.print("connecting to WiFi ");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println(" done");
    Serial.println(WiFi.localIP());
    isConnected = true;

    Serial.println("Lasse Devices initialisieren...");
    bool initOk = KinoDeviceFactory::initDevices();
    if (!initOk) {
      Serial.println("\t ... es gab Fehler");
    } else {
      Serial.println("\t ... OK");
    }
    
    Serial.println("\nSerial Command Dispatcher V1.0 ready");

    if (!KinoAPI::startMacroEngine()) {
      Serial.println("MacroEngine konnte nicht gestartet werden!");
    } else {
      Serial.println("MacroEngine bereit");
    }

    webserver::begin();
    randomSeed(analogRead(A0)); 
}

int tickFailures = 0;


void loop() {
  handleSerialCommands();
  
  // Wenn wir im Reconnect-Modus sind: ALLES andere pausieren!
  if (isReconnecting) {
    if (WiFi.status() != WL_CONNECTED) {
      isReconnecting = false;
      Serial.println(F("WiFi disconnected, waiting for new connection"));
      isConnected = false;
      delay(1000);
      WiFi.begin(ssid, password);
    }
    yield();
    return; // Raus hier! Kein s.loop(), keine Ticks.
  }

  // Ab hier: Nur wenn WiFi wirklich da ist
  if (WiFi.status() == WL_CONNECTED) {
    if (!isConnected) {
      Serial.println(F("WiFi is back now"));
      isConnected = true;
    }
    
    webserver::loop(); // Webserver nur bei stabilem WiFi!
    
    KinoAPI::handleMacroTicks();
    KinoError tickRes = KinoAPI::handleDeviceTicks(kino_showTicker);
    if (tickRes == KinoError::DeviceNotReady) tickFailures++;
    if (tickRes == KinoError::OK) tickFailures = 0;
    yield();
    int devCount = KinoDeviceFactory::getDeviceCount();
    if (tickFailures == devCount) {
      reconnectWiFi();
      tickFailures = 0;
      return;
    }
  } else {
    // WiFi ist weg, aber isReconnecting war false (unvorhergesehener Abbruch)
    // Hier rufen wir s.loop() auch NICHT auf.
    static unsigned long lastAttempt = 0;
    if (millis() - lastAttempt > 30000) {
      reconnectWiFi();
      lastAttempt = millis();
    }
  }
  yield();
}
