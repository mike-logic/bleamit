# BLEamit Native SDK

BLEamit is a lightweight, zero-pairing RGB lighting sync SDK that listens for BLE advertisements from ESP32-based lighting nodes and updates your app's UI in real time.

## Features

- 🔍 Passive BLE scan for `bleamit-node` devices
- 🎨 RGB color data extraction from manufacturer data
- 🧠 Simple integration with your existing Android or iOS app
- 🚫 No device pairing or connection required

---

## 📱 Android Integration

### 1. Add SDK Source

Include the `BleamitScanner.kt` file in your Android app or module.

### 2. Usage

```kotlin
val scanner = BleamitScanner(this)
scanner.setListener(object : BleamitListener {
    override fun onColorUpdate(r: Int, g: Int, b: Int) {
        runOnUiThread {
            view.setBackgroundColor(Color.rgb(r, g, b))
        }
    }
})
scanner.start()
```

### 3. Permissions

Add the following to your `AndroidManifest.xml`:

```xml
<uses-permission android:name="android.permission.BLUETOOTH"/>
<uses-permission android:name="android.permission.BLUETOOTH_ADMIN"/>
<uses-permission android:name="android.permission.ACCESS_FINE_LOCATION"/>
```

---

## 🍎 iOS Integration

### 1. Add SDK Source

Include the `BleamitScanner.swift` file in your Xcode project.

### 2. Usage

```swift
class ViewController: UIViewController, BleamitDelegate {
    let bleamit = BleamitScanner()

    override func viewDidLoad() {
        super.viewDidLoad()
        bleamit.delegate = self
    }

    func bleamitDidUpdateColor(_ color: UIColor) {
        self.view.backgroundColor = color
    }
}
```

### 3. Info.plist

Add the following keys:

```xml
<key>NSBluetoothAlwaysUsageDescription</key>
<string>BLEamit requires Bluetooth access</string>
<key>NSLocationWhenInUseUsageDescription</key>
<string>BLEamit requires location access for BLE scanning</string>
```

---

## 🧪 BLE Format

Manufacturer-specific data (0xFFFF):

```
Byte 0: 0xAB (token)
Byte 1: Red (0–255)
Byte 2: Green (0–255)
Byte 3: Blue (0–255)
```

---

MIT Licensed — Mike Logic, 2025.
