# bleamit

**bleamit** is a synchronized lighting and color broadcast system designed for live events, concerts, and interactive installations.

It uses ESP32 microcontrollers with BLE advertisements and ESP-NOW communication to broadcast color data to mobile phones and LED nodes in real time.

## Roles

### 🟦 Base
- Connects to Art-Net or proxy server
- Broadcasts RGB color data to hubs and nodes
- Hosts the local dashboard for monitoring and approval

### 🟩 Hub
- Relays ESP-NOW color data from base to nodes
- Handles seat/section-specific packet filtering
- Sends heartbeat packets back to base

### 🟨 Node
- Receives color data from base or hub
- Advertises the current RGB color over BLE to nearby smartphones
- Sends heartbeat packets for presence tracking

---

## License
MIT License — 2025

### 🟥 Standalone

- Does **not** use ESP-NOW — no peer discovery or heartbeat
- Listens directly for Art-Net DMX packets on UDP port 6454
- Broadcasts BLE advertisements with RGB values exactly like a node
- Ideal for **small venues** or **standalone installations**
- Runs WiFiManager for easy setup
- Includes a built-in web dashboard with live logs at `/`