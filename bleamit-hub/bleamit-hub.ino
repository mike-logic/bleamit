#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <map>

#define TOKEN 0xAB
#define ROLE_HUB 0x02
#define HEARTBEAT_INTERVAL 5000

uint8_t red = 0, green = 0, blue = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastLog = 0;
bool colorReceived = false;

uint8_t baseMAC[6] = {0};  // dynamically updated
std::map<String, unsigned long> nodeLastSeen;

void sendToNodes() {
  uint8_t payload[4] = { TOKEN, red, green, blue };
  esp_err_t result = esp_now_send(NULL, payload, sizeof(payload));  // NULL = broadcast
  if (result == ESP_OK) {
    Serial.printf("📤 Relayed to Nodes: [%02X %02X %02X %02X]\n", TOKEN, red, green, blue);
  } else {
    Serial.printf("❌ Failed to relay to nodes: err=%d\n", result);
  }
}

void sendHeartbeatToBase() {
  if (baseMAC[0] == 0 && baseMAC[1] == 0) return;

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, baseMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (!esp_now_is_peer_exist(baseMAC)) {
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("❌ Failed to add Base as peer");
      return;
    }
    Serial.println("✅ Base peer added");
  }

  uint8_t payload[2] = { TOKEN, ROLE_HUB };  // ✅ Send correct role
  esp_err_t result = esp_now_send(baseMAC, payload, sizeof(payload));
  if (result == ESP_OK) {
    Serial.println("💓 Heartbeat sent to Base");
  } else {
    Serial.printf("❌ Heartbeat to Base failed: err=%d\n", result);
  }
}

void onDataSent(const uint8_t *mac, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) {
    Serial.println("❌ ESP-NOW send failed");
  }
}

void onDataReceived(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len == 4 && data[0] == TOKEN) {
    memcpy(baseMAC, info->src_addr, 6);
    red = data[1];
    green = data[2];
    blue = data[3];
    colorReceived = true;

    Serial.printf("🔵 From Base: R=%d G=%d B=%d\n", red, green, blue);
    sendToNodes();
    return;
  }

  if (len >= 2 && data[0] == TOKEN && data[1] == 0x01) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             info->src_addr[0], info->src_addr[1], info->src_addr[2],
             info->src_addr[3], info->src_addr[4], info->src_addr[5]);

    nodeLastSeen[macStr] = millis();
    Serial.printf("💓 Heartbeat from Node: %s\n", macStr);
  }
}

void addBroadcastPeer() {
  uint8_t broadcastMAC[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  if (esp_now_is_peer_exist(broadcastMAC)) {
    Serial.println("ℹ️ Broadcast peer already exists");
    return;
  }

  esp_now_peer_info_t broadcastPeer = {};
  memset(broadcastPeer.peer_addr, 0xFF, 6);
  broadcastPeer.channel = 0;
  broadcastPeer.encrypt = false;

  esp_err_t result = esp_now_add_peer(&broadcastPeer);
  if (result == ESP_OK) {
    Serial.println("✅ Broadcast peer added (for Node relay)");
  } else {
    Serial.printf("❌ Failed to add broadcast peer: err=%d\n", result);
  }
}

void setupESPNow() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(onDataReceived);
  esp_now_register_send_cb(onDataSent);

  addBroadcastPeer();
  Serial.println("✅ ESP-NOW initialized");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);
  Serial.println("📶 Scanning for Base SSID...");
  int found = WiFi.scanNetworks();
  int baseChannel = -1;

  for (int i = 0; i < found; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.startsWith("bleamit-ch")) {
      baseChannel = ssid.substring(10).toInt();
      Serial.printf("📡 Found Base SSID '%s' → Channel %d\n", ssid.c_str(), baseChannel);
      break;
    }
  }

  if (baseChannel < 1 || baseChannel > 14) {
    Serial.println("❌ Base SSID not found — halting.");
    while (true) delay(1000);
  }

  WiFi.disconnect(true, true);
  delay(200);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(("bleamit-hub-" + String(baseChannel)).c_str(), nullptr, baseChannel);

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(baseChannel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  Serial.printf("📡 Set ESP-NOW channel to %d\n", baseChannel);

  setupESPNow();
  Serial.println("🚀 Hub setup complete");
}

void loop() {
  unsigned long now = millis();

  if (now - lastHeartbeat > HEARTBEAT_INTERVAL) {
    sendHeartbeatToBase();
    lastHeartbeat = now;
  }

  if (now - lastLog > 10000) {
    Serial.printf("🔍 Known Nodes: %d\n", (int)nodeLastSeen.size());
    for (auto &entry : nodeLastSeen) {
      unsigned long age = (millis() - entry.second) / 1000;
      Serial.printf("   🟢 %s (last seen %lus ago)\n", entry.first.c_str(), age);
    }
    lastLog = now;
  }
}
