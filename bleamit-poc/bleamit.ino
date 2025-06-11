#include <NimBLEDevice.h>
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "YOUR SSID";
const char* password = "YOUR SSID PASSWORD";

WebServer server(80);

uint8_t red = 0, green = 0, blue = 0;
uint8_t advToken = 0;

NimBLEAdvertising* pAdvertising = nullptr;

void updateAdvertisement() {
  NimBLEAdvertisementData advData;

  // 6-byte payload: [0xFF, 0xFF, R, G, B, token]
  uint8_t payload[6] = { 0xFF, 0xFF, red, green, blue, advToken++ };

  advData.setName("bleamit-node");
  advData.setManufacturerData(payload, sizeof(payload));

  if (pAdvertising) {
    pAdvertising->stop();
    delay(50);  // NimBLE requires a short delay between stop/start
    pAdvertising->setAdvertisementData(advData);
    pAdvertising->start();
  }

  Serial.printf("Updated BLE adv: R=%d G=%d B=%d Token=%d | Payload: [%d,%d,%d,%d,%d,%d]\n",
                red, green, blue, advToken - 1,
                payload[0], payload[1], payload[2],
                payload[3], payload[4], payload[5]);
}

void handleColor() {
  if (server.hasArg("r")) red = constrain(server.arg("r").toInt(), 0, 255);
  if (server.hasArg("g")) green = constrain(server.arg("g").toInt(), 0, 255);
  if (server.hasArg("b")) blue = constrain(server.arg("b").toInt(), 0, 255);

  updateAdvertisement();
  server.send(200, "text/plain", "Color updated.");
}

void handleRoot() {
  String html = R"rawliteral(
    <html>
      <head>
        <title>BLE Color Picker</title>
        <script>
          function sendColor(r, g, b) {
            fetch(`/color?r=${r}&g=${g}&b=${b}`)
              .then(() => location.reload());
          }
        </script>
      </head>
      <body style="font-family: Arial, sans-serif; padding: 20px;">
        <h1>BLE Color Picker</h1>
        <div style="display: grid; gap: 10px; max-width: 200px;">
          <button onclick="sendColor(255,0,0)" style="background: red;">Red</button>
          <button onclick="sendColor(0,255,0)" style="background: green;">Green</button>
          <button onclick="sendColor(0,0,255)" style="background: blue;">Blue</button>
          <button onclick="sendColor(255,255,0)" style="background: yellow;">Yellow</button>
          <button onclick="sendColor(255,0,255)" style="background: magenta;">Magenta</button>
          <button onclick="sendColor(0,255,255)" style="background: cyan;">Cyan</button>
          <button onclick="sendColor(255,255,255)" style="background: white;">White</button>
          <button onclick="sendColor(0,0,0)" style="background: black; color:white;">Off</button>
        </div>
      </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected! IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/color", handleColor);
  server.begin();

  NimBLEDevice::init("bleamit-node");
  NimBLEServer* pServer = NimBLEDevice::createServer();
  pAdvertising = pServer->getAdvertising();
  pAdvertising->setMinInterval(100);
  pAdvertising->setMaxInterval(200);

  updateAdvertisement();
}

void loop() {
  server.handleClient();
}
