#include <WiFi.h>
#include <WiFiManager.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_mac.h>
#include <map>

#define TOKEN 0xAB
uint8_t broadcastPeer[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

uint8_t red = 0, green = 0, blue = 0;
unsigned long lastLog = 0;

std::map<String, unsigned long> nodeLastSeen;

uint8_t getWiFiChannel() {
  uint8_t primary;
  wifi_second_chan_t second;
  esp_wifi_get_channel(&primary, &second);
  return primary;
}

void onDataSent(const uint8_t *mac, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) {
    Serial.printf("❌ Failed to send ESP-NOW packet (err=%d)\n", status);
  }
}

void onDataReceived(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len >= 2 && data[0] == TOKEN && data[1] == 0x01) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             info->src_addr[0], info->src_addr[1], info->src_addr[2],
             info->src_addr[3], info->src_addr[4], info->src_addr[5]);

    nodeLastSeen[macStr] = millis();
    Serial.printf("💓 Heartbeat from %s\n", macStr);
  }
}

void setupESPNow() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataReceived);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastPeer, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) == ESP_OK) {
    Serial.println("✅ Broadcast peer added");
  } else {
    Serial.println("❌ Failed to add broadcast peer");
  }
}

void sendColor() {
  uint8_t payload[4] = { TOKEN, red, green, blue };
  esp_err_t result = esp_now_send(broadcastPeer, payload, sizeof(payload));
  if (result == ESP_OK) {
    Serial.printf("📤 Sent ESP-NOW: [%02X %02X %02X %02X]\n", TOKEN, red, green, blue);
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  WiFiManager wm;
  if (!wm.autoConnect("bleamit-setup")) {
    Serial.println("❌ Failed to connect to WiFi");
    return;
  }

  Serial.print("✅ WiFi Connected. IP: ");
  Serial.println(WiFi.localIP());

  uint8_t ch = getWiFiChannel();
  Serial.printf("📡 WiFi Channel: %d\n", ch);

  setupESPNow();
  WiFi.mode(WIFI_AP_STA);
  delay(100);
  String ssid = "bleamit-ch" + String(ch);
  WiFi.softAP(ssid.c_str(), nullptr);
  Serial.printf("📡 Broadcasting SSID: %s on channel %d\n", ssid.c_str(), ch);
  delay(500);
  Serial.println("🚀 Base setup complete!");
}

void loop() {
  static uint8_t state = 0;
  switch (state++ % 5) {
    case 0: red = 0; green = 0; blue = 255; break;
    case 1: red = 255; green = 0; blue = 0; break;
    case 2: red = 0; green = 255; blue = 0; break;
    case 3: red = 255; green = 0; blue = 255; break;
    case 4: red = 0; green = 255; blue = 255; break;
  }

  sendColor();
  delay(2000);

  if (millis() - lastLog > 10000) {
    Serial.printf("🔍 Known nodes: %d\n", (int)nodeLastSeen.size());
    for (auto &entry : nodeLastSeen) {
      unsigned long age = (millis() - entry.second) / 1000;
      Serial.printf("   🟢 %s (last seen %lus ago)\n", entry.first.c_str(), age);
    }
    lastLog = millis();
  }
}
