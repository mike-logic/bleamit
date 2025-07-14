// dmx.cpp
#include <Arduino.h>
#include "config.h"

#define DMX_RX_PIN 16  // GPIO pin connected to MAX485 RO
HardwareSerial DMXSerial(2); // UART2

namespace DMXInput {
  void begin() {
    DMXSerial.begin(250000, SERIAL_8N2, DMX_RX_PIN, -1);
    Serial.println("🎚️ DMX listening on GPIO16 (Serial2 @ 250k)");
  }

  bool read(uint8_t &r, uint8_t &g, uint8_t &b) {
    static uint8_t dmxBuffer[512];
    static int dmxIndex = 0;
    bool updated = false;

    while (DMXSerial.available()) {
      uint8_t byte = DMXSerial.read();

      if (dmxIndex == 0 && byte == 0) {
        dmxIndex = 1; // Start of frame
      } else if (dmxIndex < 512) {
        dmxBuffer[dmxIndex++] = byte;
      } else {
        dmxIndex = 0; // Overflow
      }

      if (dmxIndex >= 4) {
        r = dmxBuffer[1];
        g = dmxBuffer[2];
        b = dmxBuffer[3];
        dmxIndex = 0;
        updated = true;
        break;
      }
    }
    return updated;
  }
}
