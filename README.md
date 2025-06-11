# bleamit v1.0

A lightweight ESP-NOW + BLE broadcast system designed for low-latency RGB lighting sync across distributed nodes — now supporting **Hub architecture**.

---

## 🧠 Architecture

```text
[ BASE ] → [ HUB ] → [ NODE ]
```

- **Base**: Generates RGB values and transmits them over ESP-NOW to the Hub.
- **Hub**: Relays RGB values via ESP-NOW to Nodes and advertises them over BLE.
- **Node**: Receives RGB via ESP-NOW, advertises the color over BLE, and sends heartbeats upstream.

This hub-based design supports large-scale event lighting where Base is Wi-Fi connected, Hubs relay to different regions, and Nodes remain lightweight.

---

## 📡 Channel Synchronization

Each device discovers its ESP-NOW channel by scanning SSIDs:

- **Base**: Sets softAP as `bleamit-chX`
- **Hub**: Scans for `bleamit-chX` → sets channel → starts `bleamit-hub-X`
- **Node**: Scans for `bleamit-hub-X` → sets channel → starts `bleamit-node-X`

This ensures a clean, scalable chain without hardcoded channels or BLE configuration.

---

## 🔄 Heartbeat Flow

- **Node → Hub**: Every 5 seconds, the Node sends a heartbeat to the Hub.
- **Hub → Base**: Hub sends heartbeats and reports active Node MACs.
- All devices log alive status to Serial for visibility.

---

## 🚀 Version 1.0 Setup

### `base.ino`
- Uses WiFiManager for user setup.
- Broadcasts RGB payload to Hub over ESP-NOW.
- Advertises ESP-NOW channel via SSID: `bleamit-chX`.

### `hub.ino`
- Scans for `bleamit-chX` SSID from Base.
- Sets ESP-NOW channel to match Base.
- Relays RGB data over ESP-NOW broadcast to all listening Nodes.
- Sends heartbeat to Base and tracks Node check-ins.
- Advertises current RGB value over BLE.

### `node.ino`
- Scans for `bleamit-hub-X` SSID from Hub.
- Sets ESP-NOW channel and listens for RGB broadcast.
- Sends periodic heartbeat to Hub.
- Updates local BLE advertisement with current color.

---

## 🧪 Usage

1. Flash and boot the **Base**, connect to Wi-Fi when prompted.
2. Power on the **Hub** — it scans for Base SSID, syncs channel, and begins relaying.
3. Boot one or more **Nodes** — they scan the Hub SSID, receive RGB values, and advertise over BLE.

---

## 🏷 Version

- `v1.0` – Initial stable release with full Base → Hub → Node flow and BLE broadcasting.
