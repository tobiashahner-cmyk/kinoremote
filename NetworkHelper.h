#ifndef NETWORK_HELPER_H
#define NETWORK_HELPER_H

#include <ESP8266WiFi.h>

namespace NetworkHelper {
    // erzwingt einen sauberen Grundzustand für den übergebenen WifiClient
    inline bool resetClient(WiFiClient& c) {
      c.stop();
      while(c.available()>0) { c.read(); yield();} // Leere den Puffer
      return true;
    }
  
    // Überspringt den HTTP-Header, damit wir direkt beim XML/JSON landen
    inline bool skipHeader(WiFiClient& client) {
        // find() ist effizient, da es nicht den ganzen Text puffert
        return client.find((char*)"\r\n\r\n");
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
