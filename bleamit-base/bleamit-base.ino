#include <WiFi.h>
#include <WiFiManager.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_mac.h>
#include <WiFiUdp.h>
#include <map>

#define TOKEN 0xAB
#define ARTNET_PORT 6454
#define DMX_UNIVERSE 0
#define DMX_CHANNEL_RED 0   // Art-Net starts at channel 0
#define DMX_CHANNEL_GREEN 1
#define DMX_CHANNEL_BLUE 2

WiFiUDP udp;
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

bool parseArtNetPacket() {
  if (udp.parsePacket()) {
    uint8_t packet[530];
    int len = udp.read(packet, sizeof(packet));
    if (len < 18) return false;

    // Check for "Art-Net" header
    if (memcmp(packet, "Art-Net", 7) != 0 || packet[8] != 0x00 || packet[9] != 0x50) {
      return false; // Not ArtDMX
    }

    uint16_t universe = packet[14] | (packet[15] << 8);
    uint16_t dmxLen = packet[16] << 8 | packet[17];
    if (universe != DMX_UNIVERSE || dmxLen < 3) return false;

    red   = packet[18 + DMX_CHANNEL_RED];
    green = packet[18 + DMX_CHANNEL_GREEN];
    blue  = packet[18 + DMX_CHANNEL_BLUE];

    Serial.printf("🎨 ArtNet: R=%d G=%d B=%d\n", red, green, blue);
    return true;
  }
  return false;
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

  udp.begin(ARTNET_PORT);
  Serial.printf("🎧 Listening for Art-Net on UDP port %d\n", ARTNET_PORT);
  Serial.println("🚀 Base setup complete!");
}

void loop() {
  static unsigned long lastSend = 0;

  if (parseArtNetPacket()) {
    sendColor();
    lastSend = millis();
  }

  // Optionally resend last color every 2s to keep devices updated
  if (millis() - lastSend > 2000) {
    sendColor();
    lastSend = millis();
  }

  if (millis() - lastLog > 10000) {
    Serial.printf("🔍 Known nodes: %d\n", (int)nodeLastSeen.size());
    for (auto &entry : nodeLastSeen) {
      unsigned long age = (millis() - entry.second) / 1000;
      Serial.printf("   🟢 %s (last seen %lus ago)\n", entry.first.c_str(), age);
    }
    lastLog = millis();
  }
}
