#include "oled.h"
#include <U8x8lib.h>
#include <SPI.h>

namespace oled {
  enum PageShowing : uint8_t {
    NONE = 0,
    ROTARY,
    MENU,
  };

  struct menuState {
    int devIndex = 0;
    int macroIndex = 0;
    int selectedItem = 0;
    
  } menu;
  
  U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);
  PageShowing actPage = NONE;
  
  void begin() {
    u8x8.begin();
    u8x8.setPowerSave(0); 
    u8x8.setFont(u8x8_font_victoriamedium8_r);
    u8x8.clear();
    u8x8.setCursor(3,10);
    u8x8.print("ready");
    actPage = NONE;
  }
  
  void showRotary(const char* devName, const char* propKey, int value, bool refresh) {
    char line[17];
    if (actPage == ROTARY && !refresh) {
      snprintf(line, sizeof(line),"%d", value);
      line[sizeof(line)-1] = '\0';
      int len = 2*strlen(line);
      int offset = 8 - (len/2);    
      u8x8.draw2x2String(offset,4,line);
      return;
    } 
    u8x8.clear();
    u8x8.setFont(u8x8_font_victoriamedium8_r);
    // Überschrift in Zeile 1: "devName.propKey"
    snprintf(line, sizeof(line), "%s.%s", devName, propKey);
    line[sizeof(line)-1] = '\0';
    int len = strlen(line);
    int offset = (16-len)/2;
    u8x8.setCursor(offset, 0);
    u8x8.print(line);
    if (strlen(devName)+strlen(propKey)>sizeof(line)) {
      // Anzeige hat nicht gepasst, schreibe den Rest in die nächste Zeile
      int startpos = sizeof(line)-strlen(devName)-2;
      strlcpy(line, &propKey[startpos], sizeof(line));
      offset = (16-strlen(line))/2;
      u8x8.setCursor(offset,1);
      u8x8.print(line);
    } else {
      u8x8.setCursor(0,1);
      u8x8.print(F("            "));
    }

    snprintf(line, sizeof(line),"%d", value);
    line[sizeof(line)-1] = '\0';
    len = 2*strlen(line);
    offset = 8 - (len/2);    
    u8x8.draw2x2String(offset,4,line);
    actPage = ROTARY;
  }

  void showDevices(int selectedDeviceIndex) {
    if (actPage != MENU) {
      u8x8.setFont(u8x8_font_victoriamedium8_r);
      actPage = MENU;
      int lineIndex = 0;
      char line[17];
      u8x8.setCursor(0,0);
      u8x8.print(F("< back"));
      size_t devCount;
      KinoVariant devName;
      KinoError err = KinoAPI::getDeviceCount(devCount);
      for (int i=0; i<devCount; i++) {
        lineIndex++;
        err = KinoAPI::getDeviceName(i, devName);
        if (err != KinoError::OK) continue;
        strlcpy(line, devName.c_str(), sizeof(line));
        u8x8.setCursor(0, lineIndex);
        u8x8.setInverseFont(i==selectedDeviceIndex ? 1 : 0);
        u8x8.print(line);
      }
    }
  }
}
