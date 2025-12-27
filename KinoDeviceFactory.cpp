#include "KinoDeviceFactory.h"

// Yamaha
YamahaReceiver* KinoDeviceFactory::createYamaha(const YamahaConfig& cfg) {
    return new YamahaReceiver(cfg.ip);
}

// WLED
WLEDDevice* KinoDeviceFactory::createWLED(const WLEDConfig& cfg) {
    return new WLEDDevice(cfg.ip);
}

// Hue
HueBridge* KinoDeviceFactory::createHue(const HueConfig& cfg) {
    return new HueBridge(cfg.ip, cfg.username);
}

// Optoma
OptomaBeamer* KinoDeviceFactory::createOptoma(const OptomaConfig& cfg) {
  return new OptomaBeamer(cfg.ip, cfg.beamerId);
}

// Hyperion
HyperionDevice* KinoDeviceFactory::createHyperion(const HyperionConfig& cfg) {
  return new HyperionDevice(cfg.ip);
}
