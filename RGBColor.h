#pragma once
#include <Arduino.h>

struct RGBColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  
  // Überladener Operator für direkten Vergleich
  bool operator!=(const RGBColor& other) const {
    return r != other.r || g != other.g || b != other.b;
  }
};
