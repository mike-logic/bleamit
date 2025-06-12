# bleamit

**bleamit** is a synchronized lighting system using ESP32 devices with ESP-NOW and BLE to broadcast color states to mobile devices in large environments. It supports Art-Net input, heartbeat tracking, and modular roles (base, hub, node).

## 🧠 Architecture

- **Base**: Receives Art-Net DMX packets over Wi-Fi and sends RGB values via ESP-NOW.
- **Hub** *(optional)*: Relays ESP-NOW color data from base to nodes.
- **Node**: Receives ESP-NOW and advertises RGB color over BLE (for mobile app sync).

## 🚀 Getting Started

### 1. Flash the Devices

- **base.ino** — Upload to the base ESP32 (M5Atom or equivalent)
- **hub.ino** — Optional relay device (only needed for large areas)
- **node.ino** — Upload to nodes that will BLE advertise

### 2. Connect Base to Wi-Fi

- The base uses **WiFiManager** — connect to the `bleamit-setup` AP on first boot and select your Wi-Fi network.

---

## 🎨 Art-Net Input (Base)

The base device listens for **Art-Net** packets on:

- **UDP Port**: `6454`
- **Universe**: `0`
- **DMX Channels**:
  - Channel 1 → **Red**
  - Channel 2 → **Green**
  - Channel 3 → **Blue**

### Test with Python

Use the included script to simulate Art-Net packets:

```bash
python test_art_net.py <ESP32_BASE_IP>
```

It will cycle through common RGB values and transmit them to the base over Art-Net.

---

## 💡 BLE Advertising

Only **nodes** perform BLE advertising, broadcasting RGB color in the **manufacturer data** format. The mobile app scans for these and updates its display.

---

## 📡 Heartbeat + Sync

- Nodes send heartbeat packets to hubs
- Hubs send heartbeats to base
- The base tracks `nodeLastSeen` to monitor active nodes

---

## 📁 Files

- `base.ino`: ESP-NOW + Art-Net receiver
- `hub.ino`: ESP-NOW relay (no BLE)
- `node.ino`: ESP-NOW receiver + BLE broadcaster
- `test_art_net.py`: Art-Net test script for local testing

---

## 📜 License

MIT License. Use freely, adapt as needed.