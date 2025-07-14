// base.ino (updated)
// This version supports both Art-Net and DMX input, and ESP-NOW or BLE output
// See prior updates for dashboard_html.h and config.h

#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFiUdp.h>
#include <NimBLEDevice.h>
#include <map>
#include <set>
#include "dashboard_html.h"
#include "config.h"
#include "dmx.h"

#define TOKEN 0xAB
#define ART_NET_PORT 6454

WebServer server(80);
WiFiUDP udp;
ConfigManager config;

struct DeviceInfo {
  unsigned long lastSeen;
  uint8_t role;
};

std::map<String, DeviceInfo> nodeLastSeen;
std::set<String> approvedDevices;
std::set<String> pendingDevices;

uint8_t broadcastPeer[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
uint8_t red = 0, green = 0, blue = 0;
unsigned long lastSend = 0;

String formatMAC(const uint8_t* mac) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

void onDataReceived(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len >= 2 && data[0] == TOKEN) {
    String macStr = formatMAC(info->src_addr);
    uint8_t role = data[1];
    if (approvedDevices.count(macStr)) {
      nodeLastSeen[macStr] = { millis(), role };
    } else {
      pendingDevices.insert(macStr);
    }
  }
}

void onDataSent(const uint8_t *mac, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) {
    Serial.println("❌ ESP-NOW send failed");
  }
}

void setupESPNow() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ ESP-NOW init failed");
    return;
  }
  esp_now_register_recv_cb(onDataReceived);
  esp_now_register_send_cb(onDataSent);
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastPeer, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) == ESP_OK) {
    Serial.println("✅ Broadcast peer added");
  }
}

void sendColorViaESPNow() {
  uint8_t payload[4] = {TOKEN, red, green, blue};
  esp_now_send(broadcastPeer, payload, sizeof(payload));
}

void updateBLEAdvertisement() {
  uint8_t payload[6] = { 0xFF, 0xFF, red, green, blue, TOKEN };
  NimBLEAdvertisementData advData;
  advData.setName("bleamit-base");
  advData.setManufacturerData(payload, sizeof(payload));
  NimBLEDevice::getAdvertising()->stop();
  delay(50);
  NimBLEDevice::getAdvertising()->setAdvertisementData(advData);
  NimBLEDevice::getAdvertising()->start();
}

void checkArtNet() {
  int packetSize = udp.parsePacket();
  if (packetSize > 0) {
    uint8_t buffer[530];
    udp.read(buffer, sizeof(buffer));
    if (memcmp(buffer, "Art-Net", 7) == 0 && buffer[8] == 0x00 && buffer[9] == 0x50) {
      if ((buffer[16] << 8 | buffer[17]) >= 3) {
        red = buffer[18];
        green = buffer[19];
        blue = buffer[20];
      }
    }
  }
}

void handleModeRoutes() {
  server.on("/mode", []() {
    server.send(200, "text/plain", String(config.getInputMode()));
  });
  server.on("/setmode", []() {
    if (server.hasArg("val")) config.setInputMode((ConfigManager::InputMode)server.arg("val").toInt());
    server.send(200, "text/plain", "ok");
  });
  server.on("/outputmode", []() {
    server.send(200, "text/plain", String(config.getOutputMode()));
  });
  server.on("/setoutputmode", []() {
    if (server.hasArg("val")) config.setOutputMode((ConfigManager::OutputMode)server.arg("val").toInt());
    server.send(200, "text/plain", "ok");
  });
}

void setupWebServer() {
  server.on("/", []() { server.sendHeader("Location", "/dashboard", true); server.send(302, "text/plain", ""); });
  server.on("/dashboard", []() { server.send_P(200, "text/html", DASHBOARD_HTML); });
  handleModeRoutes();
  server.begin();
  Serial.println("🌐 Web server started");
}

void setup() {
  Serial.begin(115200);
  config.begin();
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  wm.autoConnect("bleamit-setup");
  Serial.println(WiFi.localIP());
  setupWebServer();
  udp.begin(ART_NET_PORT);
  if (config.getInputMode() == ConfigManager::MODE_DMX) {
    DMXInput::begin();
  }
  if (config.getOutputMode() == ConfigManager::MODE_ESP_NOW) {
    setupESPNow();
  } else {
    NimBLEDevice::init("bleamit-base");
    NimBLEDevice::createServer()->getAdvertising()->setMinInterval(100);
    NimBLEDevice::getAdvertising()->setMaxInterval(200);
    updateBLEAdvertisement();
  }
}

void loop() {
  server.handleClient();
  if (config.getInputMode() == ConfigManager::MODE_ARTNET) {
    checkArtNet();
  } else {
    DMXInput::read(red, green, blue);
  }
  if (millis() - lastSend > 1000) {
    if (config.getOutputMode() == ConfigManager::MODE_ESP_NOW) {
      sendColorViaESPNow();
    } else {
      updateBLEAdvertisement();
    }
    lastSend = millis();
  }
}
