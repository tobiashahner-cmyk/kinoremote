#pragma once

#include "KinoAPI.h"
#include "KinoVariant.h"
#include "KinoError.h"

#define OLED_SCL D1
#define OLED_SDA D2


namespace oled {
  void begin();
  void showRotary(const char* devName, const char* propKey, int value, bool refresh=false);
  void showDevices(int selectedIndex);
}
