#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <NimBLEDevice.h>

#define TOKEN 0xAB
#define ART_NET_PORT 6454

WiFiUDP udp;
WebServer server(80);

uint8_t red = 0, green = 0, blue = 0;
unsigned long lastStatus = 0;
String logBuffer;

void logLine(const String& msg) {
  Serial.println(msg);
  logBuffer += msg + "\n";
  if (logBuffer.length() > 4096) {
    logBuffer = logBuffer.substring(logBuffer.length() - 4096);
  }
}

void updateBLEAdvertisement() {
  uint8_t payload[6] = { 0xFF, 0xFF, red, green, blue, TOKEN };

  NimBLEAdvertisementData advData;
  advData.setName("bleamit-node");
  advData.setManufacturerData(payload, sizeof(payload));

  NimBLEDevice::getAdvertising()->stop();
  delay(50);
  NimBLEDevice::getAdvertising()->setAdvertisementData(advData);
  NimBLEDevice::getAdvertising()->start();

  logLine("📱 BLE Updated: R=" + String(red) + " G=" + String(green) + " B=" + String(blue));
}

void handleArtNet() {
  uint8_t packet[530];
  int len = udp.parsePacket();
  if (len > 0) {
    int bytesRead = udp.read(packet, sizeof(packet));
    if (bytesRead < 18 || memcmp(packet, "Art-Net", 7) != 0) return;

    uint16_t opcode = packet[8] | (packet[9] << 8);
    if (opcode != 0x5000) return;  // ArtDMX

    uint16_t length = packet[17] | (packet[16] << 8);
    if (length < 3) return;

    red   = packet[18];
    green = packet[19];
    blue  = packet[20];

    updateBLEAdvertisement();
  }
}

void setupWebServer() {
  server.on("/", []() {
    server.send(200, "text/html", R"rawlite(
      <!DOCTYPE html>
      <html>
      <head>
        <meta charset="UTF-8">
        <title>BLEAMIT Console</title>
        <style>
          body { background: #111; color: #eee; font-family: monospace; padding: 1em; }
          h1 { color: #0ff; }
          pre { background: #222; padding: 1em; height: 300px; overflow-y: scroll; border: 1px solid #333; }
        </style>
      </head>
      <body>
        <h1>bleamit Standalone Console</h1>
        <pre id="log">Loading...</pre>
        <script>
          function updateLog() {
            fetch('/log').then(r => r.text()).then(t => {
              let log = document.getElementById('log');
              log.textContent = t;
              log.scrollTop = log.scrollHeight;
            });
          }
          setInterval(updateLog, 2000);
          updateLog();
        </script>
      </body>
      </html>
    )rawlite");
  });

  server.on("/log", []() {
    server.send(200, "text/plain", logBuffer);
  });

  server.begin();
  logLine("🌐 Web dashboard started");
}

void setup() {
  Serial.begin(115200);
  delay(500);

  logLine("📶 Starting WiFiManager...");
  WiFiManager wm;
  wm.setTimeout(180);
  bool res = wm.autoConnect("bleamit-setup");

  if (!res) {
    logLine("❌ Failed to connect — rebooting");
    delay(3000);
    ESP.restart();
  }

  logLine("✅ Connected to WiFi: " + WiFi.SSID() + " | IP: " + WiFi.localIP().toString());

  udp.begin(ART_NET_PORT);
  logLine("🎨 Listening for Art-Net on port " + String(ART_NET_PORT));

  NimBLEDevice::init("bleamit-node");
  NimBLEServer* pServer = NimBLEDevice::createServer();
  NimBLEAdvertising* adv = pServer->getAdvertising();
  adv->setMinInterval(100);
  adv->setMaxInterval(200);
  updateBLEAdvertisement();

  setupWebServer();

  logLine("🚀 Standalone BLE Node ready!");
}

void loop() {
  handleArtNet();
  server.handleClient();

  unsigned long now = millis();
  if (now - lastStatus > 5000) {
    logLine("💚 Node alive - R=" + String(red) + " G=" + String(green) + " B=" + String(blue));
    lastStatus = now;
  }
}
