# bleamit

**BLE + ESP-NOW RGB Lighting Control Framework**

Bleamit is a modular lighting control system built on ESP32 devices, 
combining ESP-NOW mesh networking with BLE broadcasting and Art-Net/DMX 
input support. It’s designed for scalable venue-wide lighting effects — 
from small interactive installations to stadium-level audience 
experiences.

---

## 🔧 Project Structure

bleamit/
├── base/ # Main DMX receiver and ESP-NOW broadcaster
├── node/ # BLE advertiser nodes receiving color payloads
├── hub/ # (Planned) ESP-NOW mesh relay layer for large venues
├── flutter_app/ # (Planned) BLE color receiver mobile app
├── README.md

---

## 🚀 Current Status

✅ Working:
- **ESP-NOW base → node communication**
- **BLE advertising from nodes to mobile devices**
- **Channel synchronization using SSID scanning**
- **Heartbeat system for base to track node check-ins**

🔧 In Progress:
- Multi-base + mesh-aware hub system
- Scalable seat-based payload filtering
- Flutter app v2 with larger payload parsing
- Web-based dashboard with pixel mapping and analytics

---

## 📡 System Overview

### Base Device
- Connects to Wi-Fi
- Receives DMX/Art-Net input (future)
- Broadcasts RGB+Token payload via ESP-NOW
- Receives heartbeat replies from nodes
- Advertises SSID to help nodes sync to Wi-Fi channel

### Node Device
- Scans for base SSID and syncs channel
- Initializes ESP-NOW + BLE
- Receives RGB color payloads from base
- Advertises updated color data via BLE to nearby mobile apps
- Sends heartbeat every few seconds with its MAC address

### Planned Hub Device
- Relays ESP-NOW packets from base to further-away nodes
- Supports mesh networking for larger venues
- Filters payloads for seat-specific mapping

---

## 📱 Flutter App

Current functionality:
- Scans for BLE packets from `bleamit-node`
- Updates screen color from `[R,G,B,TOKEN]` data

Planned:
- Accepts larger BLE payloads
- Includes seat number filtering
- Sends debug logs or analytics

---

## ⚡ Example BLE Payload Format

For mobile devices:
[Device_ID (2B), Seat_Row (1B), Seat_Col (1B), R (1B), G (1B), B (1B), 
TOKEN (1B)]

> Keep packets < 31 bytes for BLE advertising compatibility

---

## 🗺️ Scalable Mesh Design

- **1 Base → many Hubs → many Nodes**
- Supports redundancy and long-range mesh layout
- Each device forwards only what's relevant to its region
- Payloads can be filtered by seat map on Hubs

---

## 💡 Goals

- Ultra-low latency color sync (<200ms total chain)
- Fully offline-capable deployments
- Mobile app works without pairing or connection
- Modular system that scales from small installs to full arenas

---

## 🛠️ Setup

You can flash each firmware folder (`base/`, `node/`) using Arduino IDE or 
PlatformIO. All code uses ESP32 (Dev Module or S3 recommended).

---

## 📦 Future Additions

- `hub/` firmware
- `flutter_app/` update
- Web-based seat mapper + Art-Net proxy
- OTA update system for all devices

---

## 📄 License

MIT License — Open source and hackable. Build your own lightshows.

---

