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

bool isReconnecting = false;
bool isConnected = false;

void connectWiFi() {
  IPAddress ip(192, 168, 0, 227);   // IP fest einstellen
  IPAddress gateway(192, 168, 0, 1); // Gateway-Adresse definieren
  IPAddress subnet(255, 255, 255, 0); // Subnetmask
  IPAddress dns(192, 168, 0, 1); // DNS-Server
  WiFi.config(ip, dns, gateway, subnet);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print(F("Connecting to WiFi "));
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 20000) {
      Serial.print(".");
      delay(500);
  }
}


void disconnectWiFi() {
  static unsigned long lastCall = 0;
  unsigned long now = millis();
  // Sperre: Wenn der letzte Aufruf weniger als 10 Sekunden her ist, mach gar nichts
  if (now - lastCall < 10000) return; 
  isConnected = false;
  Serial.println(F("Disconnecting WiFi"));
  WiFi.persistent(false); // Verhindert unnötiges Schreiben in den Flash
  WiFi.disconnect(true);  // 'true' löscht die im SDK gespeicherten WiFi-Daten temporär
  WiFi.mode(WIFI_OFF);    // Schaltet das Funkmodul komplett ab
  delay(100);
  lastCall = now;
}

static WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;

void setup() {
    gotIpEventHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event) {
      Serial.print(F("WiFi Connected! IP: "));
      Serial.println(WiFi.localIP());
      isConnected = true;
    });
    
    disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event) {
      Serial.print(F("WiFi Lost. Reason: "));
      Serial.println(event.reason); 
      // Gründe: 8 = Verlassen (normal), 202 = Auth Fail, 201 = AP nicht gefunden...
    });
  
    Serial.begin(115200);
    
    connectWiFi();
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F(" Initial connection failed, but continuing..."));
        isConnected = false;
    } else {
        Serial.println(F(" done"));
        Serial.println(WiFi.localIP());
        isConnected = true;
    }

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
      delay(1000);
      connectWiFi();
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
      disconnectWiFi();
      tickFailures = 0;
      return;
    }
  } else {
    // WiFi ist weg, aber isReconnecting war false (unvorhergesehener Abbruch)
    // Hier rufen wir s.loop() auch NICHT auf.
    static unsigned long lastAttempt = 0;
    if (millis() - lastAttempt > 60000) {
      disconnectWiFi();
      isReconnecting = true;
      lastAttempt = millis();
    }
  }
  yield();
}
