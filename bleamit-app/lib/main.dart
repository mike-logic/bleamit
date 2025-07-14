import 'dart:async';
import 'dart:typed_data';
import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';

void main() => runApp(const MyApp());

class MyApp extends StatelessWidget {
  const MyApp({super.key});
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'BLE Color Viewer',
      theme: ThemeData.dark(),
      home: const ColorReceiver(),
    );
  }
}

class ColorReceiver extends StatefulWidget {
  const ColorReceiver({super.key});
  @override
  State<ColorReceiver> createState() => _ColorReceiverState();
}

class _ColorReceiverState extends State<ColorReceiver> {
  Color _currentColor = Colors.black;
  Color _previousColor = Colors.black;
  String _targetName = "bleamit-node";

  int _packetCount = 0;
  int _lastPacketTime = DateTime.now().millisecondsSinceEpoch;
  int _lastRssi = -100;

  bool _connected = false;
  bool _showDebug = false;

  Timer? _scanMonitorTimer;
  StreamSubscription<List<ScanResult>>? _scanSub;

  @override
  void initState() {
    super.initState();
    _initPermissionsAndStart();
  }

  Future<void> _initPermissionsAndStart() async {
    await [
      Permission.bluetoothScan,
      Permission.bluetoothConnect,
      Permission.location,
    ].request();

    _scanSub = FlutterBluePlus.scanResults.listen(_onScanResult);

    if (!FlutterBluePlus.isScanningNow) {
      await FlutterBluePlus.startScan(
        timeout: const Duration(seconds: 60),
        androidUsesFineLocation: true,
      );
    }

    _startScanMonitor();
  }

  void _startScanMonitor() {
    int lastRestartTime = DateTime.now().millisecondsSinceEpoch;

    _scanMonitorTimer = Timer.periodic(const Duration(seconds: 10), (_) async {
      final now = DateTime.now().millisecondsSinceEpoch;
      final secondsSinceLast = (now - _lastPacketTime) / 1000;
      final secondsSinceLastRestart = (now - lastRestartTime) / 1000;

      if (secondsSinceLast > 15 && secondsSinceLastRestart > 30) {
        debugPrint("🔄 Restarting scan (stale data)...");
        try {
          lastRestartTime = now;
          await FlutterBluePlus.stopScan();
          await Future.delayed(const Duration(milliseconds: 200));
          await FlutterBluePlus.startScan(
            timeout: const Duration(seconds: 60),
            androidUsesFineLocation: true,
          );
        } catch (e) {
          debugPrint("❌ Scan restart error: $e");
        }
      }
    });
  }

  void _onScanResult(List<ScanResult> results) {
    for (final result in results) {
      final name = result.device.name;
      final localName = result.advertisementData.localName;
      final mfg = result.advertisementData.manufacturerData;

      if ((name == _targetName || localName == _targetName) && mfg.containsKey(0xFFFF)) {
        final data = Uint8List.fromList(mfg[0xFFFF]!);
        _lastRssi = result.rssi;
        _processColorData(data);
        break;
      }
    }
  }

  void _processColorData(Uint8List data) {
    _lastPacketTime = DateTime.now().millisecondsSinceEpoch;
    _connected = true;

    int r = 0, g = 0, b = 0, token = 0;

    if (data.length >= 6 && data[0] == 0xFF && data[1] == 0xFF) {
      r = data[2];
      g = data[3];
      b = data[4];
      token = data[5];
    } else if (data.length == 4) {
      r = data[0];
      g = data[1];
      b = data[2];
      token = data[3];
    } else {
      debugPrint("❌ Invalid data: ${data.toList()}");
      return;
    }

    _packetCount++;
    debugPrint("🎨 Color update: R=$r G=$g B=$b | Token=$token");

    setState(() {
      _previousColor = _currentColor;
      _currentColor = Color.fromARGB(255, r, g, b);
    });
  }

  Widget _buildStatusIcons() {
    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceBetween,
      children: [
        Icon(
          _connected ? Icons.check_circle : Icons.cancel,
          color: _connected ? Colors.green : Colors.red,
        ),
        Row(
          children: [
            Icon(Icons.signal_cellular_alt, color: Colors.white),
            const SizedBox(width: 4),
            Text("$_lastRssi dBm", style: const TextStyle(color: Colors.white)),
          ],
        ),
        IconButton(
          icon: const Icon(Icons.bug_report, color: Colors.white),
          onPressed: () => setState(() => _showDebug = !_showDebug),
        )
      ],
    );
  }

  Widget _buildDebugPanel() {
    if (!_showDebug) return const SizedBox.shrink();

    final timeSince = ((DateTime.now().millisecondsSinceEpoch - _lastPacketTime) / 1000).toStringAsFixed(1);
    return Positioned(
      top: 50,
      left: 20,
      right: 20,
      child: Container(
        padding: const EdgeInsets.all(15),
        decoration: BoxDecoration(
          color: Colors.black.withOpacity(0.8),
          borderRadius: BorderRadius.circular(12),
          border: Border.all(color: Colors.white30),
        ),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            const Text("🐞 Debug Info", style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold, color: Colors.white)),
            const SizedBox(height: 8),
            Text("Packets: $_packetCount", style: const TextStyle(color: Colors.white)),
            Text("RSSI: $_lastRssi dBm", style: const TextStyle(color: Colors.white)),
            Text("Last packet: $timeSince s ago", style: const TextStyle(color: Colors.white)),
          ],
        ),
      ),
    );
  }

  @override
  void dispose() {
    _scanMonitorTimer?.cancel();
    _scanSub?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: AnimatedContainer(
        duration: const Duration(milliseconds: 400),
        color: _currentColor,
        child: SafeArea(
          child: Stack(
            children: [
              Center(
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    const Text(
                      "🎨 BLE Color Viewer",
                      style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold, color: Colors.white),
                    ),
                    const SizedBox(height: 10),
                    Text(
                      _connected ? "Receiving from $_targetName" : "No active show detected",
                      style: const TextStyle(color: Colors.white70),
                    ),
                  ],
                ),
              ),
              Positioned(
                top: 10,
                left: 15,
                right: 15,
                child: _buildStatusIcons(),
              ),
              _buildDebugPanel(),
            ],
          ),
        ),
      ),
    );
  }
}
