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

uint8_t baseMAC[6] = {0};  // dynamically filled from broadcast sender

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
  Serial.printf("📱 BLE Payload: [%02X %02X %02X %02X %02X %02X]\n",
                payload[0], payload[1], payload[2], payload[3], payload[4], payload[5]);
}

void addBasePeerIfNeeded(const uint8_t *mac) {
  if (esp_now_is_peer_exist(mac)) return;

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) == ESP_OK) {
    Serial.printf("✅ Added base peer: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  } else {
    Serial.println("❌ Failed to add base as peer");
  }
}

void sendHeartbeat() {
  if (baseMAC[0] == 0 && baseMAC[1] == 0 && baseMAC[2] == 0) return;  // not set yet

  addBasePeerIfNeeded(baseMAC);

  uint8_t payload[2] = { TOKEN, 0x01 };
  esp_err_t result = esp_now_send(baseMAC, payload, sizeof(payload));
  if (result == ESP_OK) {
    Serial.println("💓 Heartbeat sent to base");
  } else {
    Serial.printf("❌ Heartbeat send failed (err=%d)\n", result);
  }
}

void onReceive(const esp_now_recv_info_t *recvInfo, const uint8_t *incomingData, int len) {
  Serial.printf("📥 Received packet: len=%d, data=[", len);
  for (int i = 0; i < len; i++) Serial.printf("%02X ", incomingData[i]);
  Serial.println("]");

  if (len != 4 || incomingData[0] != TOKEN) return;

  memcpy(baseMAC, recvInfo->src_addr, 6);  // save for heartbeat
  red   = incomingData[1];
  green = incomingData[2];
  blue  = incomingData[3];
  colorReceived = true;

  Serial.printf("🔵 ESP-NOW Received: R=%d G=%d B=%d\n", red, green, blue);
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

  WiFi.mode(WIFI_STA);
  Serial.println("📶 Scanning for base channel...");
  int found = WiFi.scanNetworks();
  int baseChannel = -1;
  for (int i = 0; i < found; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.startsWith("bleamit-ch")) {
      baseChannel = ssid.substring(10).toInt();
      Serial.printf("📡 Found base SSID '%s' → Channel %d\n", ssid.c_str(), baseChannel);
      break;
    }
  }

  if (baseChannel < 1 || baseChannel > 14) {
    Serial.println("❌ Base SSID not found — halting.");
    while (true) delay(1000);
  }

  WiFi.disconnect(true, true);
  delay(500);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(("bleamit-node-" + String(baseChannel)).c_str(), nullptr, baseChannel);
  delay(200);

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
