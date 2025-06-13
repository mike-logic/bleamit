# Bleamit

**Bleamit** is a lightweight, real-time ESP32 lighting and display sync system designed for stadiums, live events, or distributed LED installations.

This repository includes firmware for:
- 🧠 `base` — broadcasts RGB data over ESP-NOW and BLE, receives Art-Net input, hosts a dashboard for device approval and monitoring.
- 📡 `hub` — receives color from base, relays to nodes, and sends its own heartbeat to the base.
- 💡 `node` — receives RGB values from base or hub, displays color via BLE advertising, and sends heartbeat to upstream.

---

## 🚀 Features

- ✅ Real-time color sync using ESP-NOW
- ✅ Art-Net input support (from lighting consoles or PC software)
- ✅ BLE advertising for nearby mobile apps (e.g. via Flutter)
- ✅ WiFiManager config for easy setup
- ✅ Built-in web dashboard to approve/reject devices and view status
- ✅ Heartbeat-based device check-ins for monitoring
- ✅ Device role detection: hub vs node
- ✅ Channel auto-discovery via SSID scanning

---

## 🗂 Directory Structure

```
/bleamit-base/     # Firmware for the base device
/bleamit-hub/      # Firmware for relaying hub device
/bleamit-node/     # Firmware for endpoint nodes
/dashboard_html.h  # Shared HTML UI for the dashboard
```

---

## 🌐 Web Dashboard

Once connected, navigate to `http://<base-ip>/dashboard` to:

- View connected nodes and hubs
- Approve or reject pending devices
- View last seen time and role (hub or node)
- Monitor in real time

---

## 🛠 Getting Started

### Requirements
- Platform: ESP32 boards (e.g., M5Stack AtomS3, generic ESP32 dev boards)
- Arduino IDE or PlatformIO
- Libraries:
  - `esp_now`, `esp_wifi`, `WiFiManager`, `WebServer`, `WiFiUDP`
  - NimBLE for BLE functionality (node only)

### Flashing
- Each directory is a separate Arduino sketch.
- Flash the correct one to your device:
  - `base` to the controller
  - `hub` to intermediary repeaters
  - `node` to end units

---

## 🔧 Configuration Notes

- **Art-Net** (port 6454) is only handled on the base.
- **SoftAP SSIDs** encode channel numbers for auto channel sync:
  - Base: `bleamit-ch#`
  - Hub: `bleamit-hub-ch#`

---

## 📡 Communication Protocol

| Type   | Direction        | Payload Format                  |
|--------|------------------|----------------------------------|
| Color  | Base → All       | `[0xAB, R, G, B]`                |
| Heartbeat | Node/Hub → Base | `[0xAB, role]` (0x01=node, 0x02=hub) |

---

## 🧪 Debugging

Use the serial monitor to track:
- Heartbeats from devices
- ESP-NOW send/receive events
- Art-Net RGB values
- BLE advertising state (on node)

---

## 🔐 Security

- No authentication yet. Future updates will support:
  - Tokenized heartbeat verification
  - OTA firmware updates over ESP-NOW
  - Per-device pairing

---

## 📦 License

MIT