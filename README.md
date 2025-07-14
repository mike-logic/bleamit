# bleamit

**bleamit** is a synchronized lighting system using ESP32 devices with ESP-NOW and BLE to broadcast color states to mobile devices in large environments. It supports Art-Net and DMX input, heartbeat tracking, and modular roles (base, hub, node).

## 🧠 Architecture

- **Base**: Receives Art-Net (or DMX) input and sends RGB values via ESP-NOW or BLE.
- **Hub** *(optional)*: Relays ESP-NOW color data from base to nodes.
- **Node**: Receives ESP-NOW and advertises RGB color over BLE (for mobile app sync).
- **Standalone**: Self-contained Art-Net or DMX listener that advertises RGB over BLE — ideal for small venues or solo setups.

## 🚀 Getting Started

### 1. Flash the Devices

- **bleamit-base.ino** — Upload to the base ESP32 (M5Atom or equivalent)
- **hub.ino** — Optional relay device (only needed for large areas)
- **node.ino** — Upload to nodes that will BLE advertise
- **bleamit-standalone.ino** — For small installations that only need one broadcaster

### 🧪 Try It Instantly in Your Browser

You can flash firmware to any ESP32 directly from your browser using **Chrome** or **Edge** — no IDE required.

👉 **[Open BLEAMIT Web Flasher](https://mike-logic.github.io/bleamit/)**

Select one of the following roles and click "Install":

- 🟦 **Base** — receives Art-Net or DMX and sends color via ESP-NOW or BLE
- 🟩 **Hub** — relays ESP-NOW from base to nodes
- 🟨 **Node** — receives color via ESP-NOW and advertises via BLE
- 🟧 **Standalone** — directly listens for Art-Net or DMX and advertises via BLE

This requires a compatible ESP32 Dev Module connected via USB.

### 2. Connect Base or Standalone to Wi-Fi

- Both use **WiFiManager** — connect to the `bleamit-setup` AP on first boot and select your Wi-Fi network.

---

## 🎛 Input Modes: Art-Net or DMX

You can choose how each base or standalone device receives color input:

- **Art-Net** (default): Listens for UDP Art-Net DMX packets on Wi-Fi
- **DMX**: Receives wired DMX512 signal via XLR connector using a MAX485 module

### DMX Wiring (via MAX485)

- MAX485 `RO` → ESP32 `GPIO16`
- MAX485 `RE`, `DE` → GND (always receive mode)
- MAX485 `A`/`B` → XLR Pin 3 (Data+) / Pin 2 (Data-)
- MAX485 `GND` → XLR Pin 1

The device reads **DMX channels 1–3** as:

- Channel 1 → **Red**
- Channel 2 → **Green**
- Channel 3 → **Blue**

You can toggle input mode from the web dashboard.

---

## 🖥 Web Dashboard (Base + Standalone)

Access the IP address shown after WiFi setup to open the local dashboard:

- 🔧 Toggle input mode: Art-Net or DMX
- 🔧 Toggle output mode: ESP-NOW (Base) or BLE (Standalone)
- 📶 View approved / pending nodes
- 🧠 Review live RGB values
- 📜 Scroll real-time serial logs in browser

---

## 🎨 Art-Net Input

Devices listening in Art-Net mode respond to:

- **UDP Port**: `6454`
- **Universe**: `0`
- **Channels**:
  - Channel 1 → Red
  - Channel 2 → Green
  - Channel 3 → Blue

### Test with Python

Use the included script:

```bash
python test_art_net.py <ESP32_IP>
```

---

## 💡 BLE Advertising

**Nodes and Standalone devices** advertise BLE packets that include RGB values. Mobile apps can scan for these and update the display.

**BLE Payload Format:**

| Byte Index | Value                |
| ---------- | -------------------- |
| 0–1        | `0xFF 0xFF` (header) |
| 2–4        | `R G B` color        |
| 5          | `0xAB` (token)       |

---

## 📡 Heartbeat + Sync

- Nodes → hubs → base relay presence via ESP-NOW
- Base tracks last seen timestamps and displays in the dashboard
- Standalone devices do not participate in heartbeat sync

---

## 📁 Files

- `bleamit-base.ino`: Configurable Art-Net/DMX input + ESP-NOW/BLE output + dashboard
- `bleamit-standalone.ino`: Self-contained broadcaster with Art-Net or DMX input + BLE
- `hub.ino`: ESP-NOW relay (optional)
- `node.ino`: ESP-NOW receiver and BLE advertiser
- `test_art_net.py`: Art-Net color testing script

---

## 📜 License

MIT License. Use freely, adapt as needed.
