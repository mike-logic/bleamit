#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <NimBLEDevice.h>

#define TOKEN 0xAB
#define HEARTBEAT_INTERVAL 5000

uint8_t red = 0, green = 0, blue = 0;
unsigned long lastStatus = 0;
unsigned long lastHeartbeat = 0;
bool colorReceived = false;

uint8_t hubMAC[6] = {0};  // learned dynamically on receive

void updateBLEAdvertisement() {
  uint8_t payload[6] = { 0xFF, 0xFF, red, green, blue, TOKEN };

  NimBLEAdvertisementData advData;
  advData.setName("bleamit-node");
  advData.setManufacturerData(payload, sizeof(payload));

  NimBLEDevice::getAdvertising()->stop();
  delay(50);
  NimBLEDevice::getAdvertising()->setAdvertisementData(advData);
  NimBLEDevice::getAdvertising()->start();

  Serial.printf("📱 BLE Updated: R=%d G=%d B=%d (Token=%02X)\n", red, green, blue, TOKEN);
}

void addHubPeerIfNeeded(const uint8_t *mac) {
  if (esp_now_is_peer_exist(mac)) return;

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) == ESP_OK) {
    Serial.printf("✅ Added hub peer: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  } else {
    Serial.println("❌ Failed to add hub as peer");
  }
}

void sendHeartbeat() {
  if (hubMAC[0] == 0 && hubMAC[1] == 0) return;

  addHubPeerIfNeeded(hubMAC);

  uint8_t payload[3] = { TOKEN, 0x01, 0x01 };  // 💡 0x01 = Heartbeat, 0x01 = Node type
  esp_err_t result = esp_now_send(hubMAC, payload, sizeof(payload));
  if (result == ESP_OK) {
    Serial.println("💓 Heartbeat sent to Hub");
  } else {
    Serial.printf("❌ Heartbeat send failed (err=%d)\n", result);
  }
}

void onReceive(const esp_now_recv_info_t *recvInfo, const uint8_t *incomingData, int len) {
  Serial.printf("📥 Packet received: len=%d, from [%02X:%02X:%02X:%02X:%02X:%02X] data=[",
                len,
                recvInfo->src_addr[0], recvInfo->src_addr[1], recvInfo->src_addr[2],
                recvInfo->src_addr[3], recvInfo->src_addr[4], recvInfo->src_addr[5]);
  for (int i = 0; i < len; i++) Serial.printf("%02X ", incomingData[i]);
  Serial.println("]");

  if (len != 4 || incomingData[0] != TOKEN) return;

  memcpy(hubMAC, recvInfo->src_addr, 6);  // learn hub MAC
  red   = incomingData[1];
  green = incomingData[2];
  blue  = incomingData[3];
  colorReceived = true;

  Serial.printf("🔵 ESP-NOW Received from Hub: R=%d G=%d B=%d\n", red, green, blue);
  updateBLEAdvertisement();
}

void setupESPNow() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ ESP-NOW init failed");
    return;
  }
  esp_now_register_recv_cb(onReceive);
  Serial.println("✅ ESP-NOW initialized and callback registered");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("📶 Scanning for hub channel...");
  WiFi.mode(WIFI_STA);
  int found = WiFi.scanNetworks();
  int hubChannel = -1;

  for (int i = 0; i < found; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.startsWith("bleamit-hub-")) {
      hubChannel = ssid.substring(12).toInt();
      Serial.printf("📡 Found hub SSID '%s' → Channel %d\n", ssid.c_str(), hubChannel);
      break;
    }
  }

  if (hubChannel < 1 || hubChannel > 14) {
    Serial.println("❌ Hub SSID not found — halting.");
    while (true) delay(1000);
  }

  Serial.println("🔌 Disconnecting from WiFi...");
  WiFi.disconnect(true, true);
  delay(300);

  WiFi.mode(WIFI_AP_STA);  // 💡 critical for channel locking
  WiFi.softAP(("bleamit-node-" + String(hubChannel)).c_str(), nullptr, hubChannel);
  delay(200);

  Serial.printf("📡 Set ESP-NOW channel to %d (via softAP)\n", hubChannel);

  setupESPNow();

  NimBLEDevice::init("bleamit-node");
  NimBLEServer* pServer = NimBLEDevice::createServer();
  NimBLEAdvertising* adv = pServer->getAdvertising();
  adv->setMinInterval(100);
  adv->setMaxInterval(200);
  updateBLEAdvertisement();

  Serial.println("🚀 Node setup complete!");
}

void loop() {
  unsigned long now = millis();

  if (now - lastStatus > 5000) {
    if (colorReceived) {
      Serial.printf("💚 Node alive - Color: R=%d G=%d B=%d\n", red, green, blue);
    } else {
      Serial.printf("⚠️ Node alive - No color received yet\n");
    }
    lastStatus = now;
  }

  if (now - lastHeartbeat > HEARTBEAT_INTERVAL) {
    sendHeartbeat();
    lastHeartbeat = now;
  }
}
