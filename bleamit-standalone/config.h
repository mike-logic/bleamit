#ifndef BLEAMIT_CONFIG_H
#define BLEAMIT_CONFIG_H

#include <Preferences.h>

class ConfigManager {
public:
  void begin() {
    prefs.begin("bleamit", false);
  }

  enum InputMode {
    MODE_ARTNET = 0,
    MODE_DMX = 1
  };

  enum OutputMode {
    MODE_ESP_NOW = 0,
    MODE_BLE = 1
  };

  InputMode getInputMode() {
    return static_cast<InputMode>(prefs.getUChar("input_mode", MODE_ARTNET));
  }

  void setInputMode(InputMode mode) {
    prefs.putUChar("input_mode", static_cast<uint8_t>(mode));
  }

  OutputMode getOutputMode() {
    return static_cast<OutputMode>(prefs.getUChar("output_mode", MODE_ESP_NOW));
  }

  void setOutputMode(OutputMode mode) {
    prefs.putUChar("output_mode", static_cast<uint8_t>(mode));
  }

private:
  Preferences prefs;
};

#endif
