#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFiUdp.h>
#include <map>
#include <set>
#include "dashboard_html.h"

#define TOKEN 0xAB
#define ART_NET_PORT 6454

WebServer server(80);
WiFiUDP udp;

struct DeviceInfo {
  unsigned long lastSeen;
  uint8_t role;  // 0x01 = node, 0x02 = hub
};

std::map<String, DeviceInfo> nodeLastSeen;
std::set<String> approvedDevices;
std::set<String> pendingDevices;

uint8_t broadcastPeer[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t red = 0, green = 0, blue = 0;
unsigned long lastLog = 0;
bool artnetOverride = false;

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

void sendColor() {
  uint8_t payload[4] = {TOKEN, red, green, blue};
  esp_err_t result = esp_now_send(broadcastPeer, payload, sizeof(payload));
  if (result == ESP_OK) {
    Serial.printf("📤 Sent ESP-NOW: [%02X %02X %02X %02X]\n", TOKEN, red, green, blue);
  }
}

void handleDashboard() {
  server.send_P(200, "text/html", DASHBOARD_HTML);
}

void handleStatusJson() {
  String json = "{ \"approved\": [";
  bool first = true;
  for (auto &pair : nodeLastSeen) {
    if (!first) json += ",";
    json += "{\"mac\":\"" + pair.first + "\",\"role\":" + String(pair.second.role) +
            ",\"lastSeen\":" + String((millis() - pair.second.lastSeen) / 1000) + "}";
    first = false;
  }
  json += "], \"pending\": [";
  first = true;
  for (auto &mac : pendingDevices) {
    if (!first) json += ",";
    json += "\"" + mac + "\"";
    first = false;
  }
  json += "] }";

  server.send(200, "application/json", json);
}

void handleApprove() {
  if (server.hasArg("mac")) {
    String mac = server.arg("mac");
    approvedDevices.insert(mac);
    pendingDevices.erase(mac);
    Serial.println("✅ Approved: " + mac);
    server.send(200, "text/plain", "Approved " + mac);
  } else {
    server.send(400, "text/plain", "Missing MAC");
  }
}

void handleReject() {
  if (server.hasArg("mac")) {
    String mac = server.arg("mac");
    approvedDevices.erase(mac);
    pendingDevices.erase(mac);
    Serial.println("⛔ Rejected: " + mac);
    server.send(200, "text/plain", "Rejected " + mac);
  } else {
    server.send(400, "text/plain", "Missing MAC");
  }
}

void setupWebServer() {
  server.on("/", []() {
    server.sendHeader("Location", "/dashboard", true);
    server.send(302, "text/plain", "");
  });

  server.on("/dashboard", handleDashboard);
  server.on("/status.json", handleStatusJson);
  server.on("/approve", handleApprove);
  server.on("/reject", handleReject);
  server.begin();
  Serial.println("🌐 Web server started");
}

void checkArtNet() {
  int packetSize = udp.parsePacket();
  if (packetSize > 0) {
    uint8_t buffer[530];
    udp.read(buffer, sizeof(buffer));

    if (memcmp(buffer, "Art-Net", 7) == 0 && buffer[8] == 0x00 && buffer[9] == 0x50) {
      uint16_t universe = buffer[14] | (buffer[15] << 8);
      uint16_t length = buffer[16] << 8 | buffer[17];

      if (length >= 3 && universe == 0) {
        red   = buffer[18];
        green = buffer[19];
        blue  = buffer[20];
        artnetOverride = true;
        Serial.printf("🎨 Art-Net RGB: %d %d %d\n", red, green, blue);
        sendColor();
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  if (!wm.autoConnect("bleamit-setup")) {
    Serial.println("❌ WiFi failed");
    return;
  }

  Serial.print("✅ IP: ");
  Serial.println(WiFi.localIP());

  setupESPNow();
  setupWebServer();

  WiFi.mode(WIFI_AP_STA);
  delay(100);
  String ssid = "bleamit-ch" + String(WiFi.channel());
  WiFi.softAP(ssid.c_str(), nullptr);
  Serial.println("🚀 Base ready");

  udp.begin(ART_NET_PORT);
}

void loop() {
  static uint8_t state = 0;
  static unsigned long lastSend = 0;

  server.handleClient();
  checkArtNet();

  if (!artnetOverride && millis() - lastSend > 2000) {
    switch (state++ % 5) {
      case 0: red = 0; green = 0; blue = 255; break;
      case 1: red = 255; green = 0; blue = 0; break;
      case 2: red = 0; green = 255; blue = 0; break;
      case 3: red = 255; green = 0; blue = 255; break;
      case 4: red = 0; green = 255; blue = 255; break;
    }
    sendColor();
    lastSend = millis();
  }
}
