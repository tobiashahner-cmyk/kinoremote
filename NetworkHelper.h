#ifndef NETWORK_HELPER_H
#define NETWORK_HELPER_H

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

namespace NetworkHelper {
    // erzwingt einen sauberen Grundzustand für den übergebenen WifiClient
    inline bool resetClient(WiFiClient& c) {
      while(c.available()>0) { c.read(); yield();} // Leere den Puffer
      c.stop();
      delay(10);
      return true;
    }

    inline bool resetHttpClient(HTTPClient& http) {
      if (http.connected()) {
        WiFiClient* stream = http.getStreamPtr();
        if (stream != nullptr) {
          int i = 0;
          // Puffer leeren, um einen sauberen TCP-Close zu ermöglichen
          while (stream->available() && i < 512) {
            stream->read();
            i++;
            yield();
          }
        }
      }
      http.end(); // Schließt die Verbindung und gibt interne Puffer frei
      return true;
    }

    inline bool resetWiFiClient(WiFiClient& wifi) {
      if (wifi.connected()) {
        wifi.flush(); // ausgehende Daten definitiv raus
      }
      while(wifi.available()>0) { wifi.read(); yield();} // Leere den Puffer
      wifi.stop();
      // yield-Schleife für das LwIP-Backend
      for (int i = 0; i < 5; i++) {
        yield();
        delay(10); 
      }
      return true;
    }

    inline bool resetClients(WiFiClient& wifi, HTTPClient& http, bool success) {
      resetHttpClient(http);
      resetWiFiClient(wifi);
      if (!success) delay(1000);  // zusätzliche Aufräumzeit nach Fehlern
      return success;
    }
  
    // Überspringt den HTTP-Header, damit wir direkt beim XML/JSON landen
    inline bool skipHeader(WiFiClient& client) {
      // find() ist effizient, da es nicht den ganzen Text puffert
      client.setTimeout(1000);
      bool ok = client.find((char*)"\r\n\r\n");
      if (!ok) {
        Serial.println(F("NetworkHelper::skipHeader failed"));
      }
      return ok;
    }
    

    // Liest einen Wert zwischen zwei Tags, ohne das XML zu speichern
    // Beispiel: <Val>-350</Val> -> liefert "-350"
    inline String readTagValue(WiFiClient& client, const char* tag) {
        String startTag = "<";
        startTag += tag;
        startTag += ">";
        
        if (client.find((char*)startTag.c_str())) {
            return client.readStringUntil('<'); 
        }
        return "";
    }


    inline bool findFlash(WiFiClient& client, const __FlashStringHelper* helper) {
        if (!helper) return false;
        
        // Wir kopieren den Flash-String in einen temporären Puffer im RAM
        // String(helper) erledigt das Kopieren sicher für uns.
        String searchStr(helper); 
        
        // Jetzt kann find() sicher im RAM suchen
        return client.find((char*)searchStr.c_str());
    }
}

#endif
