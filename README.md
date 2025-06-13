# bleamit

**bleamit** is a synchronized lighting system using ESP32 devices with ESP-NOW and BLE to broadcast color states to mobile devices in large environments. It supports Art-Net input, heartbeat tracking, and modular roles (base, hub, node).

## 🧠 Architecture

- **Base**: Receives Art-Net DMX packets over Wi-Fi and sends RGB values via ESP-NOW.
- **Hub** *(optional)*: Relays ESP-NOW color data from base to nodes.
- **Node**: Receives ESP-NOW and advertises RGB color over BLE (for mobile app sync).
- **Standalone**: Self-contained Art-Net listener that advertises RGB over BLE with no ESP-NOW — ideal for small venues or solo setups.

## 🚀 Getting Started

### 1. Flash the Devices

- **base.ino** — Upload to the base ESP32 (M5Atom or equivalent)
- **hub.ino** — Optional relay device (only needed for large areas)
- **node.ino** — Upload to nodes that will BLE advertise
- **standalone.ino** — For small installations that only need one broadcaster

### 2. Connect Base or Standalone to Wi-Fi

- Both use **WiFiManager** — connect to the `bleamit-setup` AP on first boot and select your Wi-Fi network.

---

## 🎨 Art-Net Input (Base + Standalone)

The base and standalone devices listen for **Art-Net** packets on:

- **UDP Port**: `6454`
- **Universe**: `0`
- **DMX Channels**:
  - Channel 1 → **Red**
  - Channel 2 → **Green**
  - Channel 3 → **Blue**

### Test with Python

Use the included script to simulate Art-Net packets:

```bash
python test_art_net.py <ESP32_IP>
```

It will cycle through common RGB values and transmit them to the device over Art-Net.

---

## 💡 BLE Advertising

**Nodes and Standalone devices** perform BLE advertising, broadcasting RGB color in the **manufacturer data** format. The mobile app scans for these and updates its display.

**BLE Format:**

| Byte Index | Value                |
|------------|----------------------|
| 0–1        | `0xFF 0xFF` (header) |
| 2–4        | `R G B` color        |
| 5          | `0xAB` (token)       |

---

## 📡 Heartbeat + Sync

- Nodes send heartbeat packets to hubs
- Hubs send heartbeats to base
- The base tracks `nodeLastSeen` to monitor active nodes
- **Standalone** devices do not participate in ESP-NOW or heartbeat logic

---

## 🖥 Web Dashboard (Base + Standalone)

The base and standalone devices serve a web dashboard at their IP address:

- View current connection info
- Review live RGB values
- Scroll real-time serial logs in browser (`/log`, `/info`)

---

## 📁 Files

- `base.ino`: ESP-NOW + Art-Net receiver + dashboard
- `hub.ino`: ESP-NOW relay (no BLE)
- `node.ino`: ESP-NOW receiver + BLE broadcaster
- `standalone.ino`: Art-Net only BLE broadcaster with web dashboard
- `test_art_net.py`: Art-Net test script for local testing

---

## 📜 License

MIT License. Use freely, adapt as needed.