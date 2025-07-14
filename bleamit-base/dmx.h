// dmx.h
#ifndef DMX_INPUT_H
#define DMX_INPUT_H

#include <Arduino.h>

namespace DMXInput {
  void begin();
  bool read(uint8_t &r, uint8_t &g, uint8_t &b);
}

#endif