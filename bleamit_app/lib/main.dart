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
  Color currentColor = Colors.black;
  String debugText = "Waiting for BLE data...";
  final String targetName = "bleamit-node";
  int packetCount = 0;
  int lastPacketTime = DateTime.now().millisecondsSinceEpoch;
  Timer? scanRestartTimer;
  StreamSubscription<List<ScanResult>>? scanSub;

  @override
  void initState() {
    super.initState();
    initPermissionsAndStart();
  }

  Future<void> initPermissionsAndStart() async {
    await [
      Permission.bluetoothScan,
      Permission.bluetoothConnect,
      Permission.location,
    ].request();

    scanSub = FlutterBluePlus.scanResults.listen(_onScanResult);
    _startScanLoop();
    _startScanRestartTimer();
  }

  void _startScanLoop() async {
    while (mounted) {
      try {
        debugPrint("📡 Starting scan...");
        await FlutterBluePlus.startScan(
          timeout: const Duration(seconds: 4),
          androidUsesFineLocation: true,
        );
        await Future.delayed(const Duration(seconds: 4));
      } catch (e) {
        debugPrint("❌ Scan error: $e");
        await Future.delayed(const Duration(seconds: 2));
      }
    }
  }

  void _startScanRestartTimer() {
    scanRestartTimer = Timer.periodic(const Duration(seconds: 6), (_) async {
      int now = DateTime.now().millisecondsSinceEpoch;
      if (now - lastPacketTime > 6000) {
        debugPrint("🔄 Force-restarting scan every 6s...");
        try {
          await FlutterBluePlus.stopScan();
          await Future.delayed(const Duration(milliseconds: 200));
          await FlutterBluePlus.startScan(
            timeout: const Duration(seconds: 4),
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

      if ((name == targetName || localName == targetName) && mfg.containsKey(0xFFFF)) {
        final data = Uint8List.fromList(mfg[0xFFFF]!);
        _processColorData(data);
        break;
      }
    }
  }

  void _processColorData(Uint8List data) {
    lastPacketTime = DateTime.now().millisecondsSinceEpoch;

    if (data.length >= 6 && data[0] == 0xFF && data[1] == 0xFF) {
      final r = data[2];
      final g = data[3];
      final b = data[4];
      final token = data[5];

      packetCount++;
      debugPrint("🟢 Token=$token | Color: R=$r G=$g B=$b");

      setState(() {
        currentColor = Color.fromARGB(255, r, g, b);
        debugText = "R=$r G=$g B=$b\nToken=$token\nPackets: $packetCount";
      });
    } else if (data.length == 4) {
      final r = data[0];
      final g = data[1];
      final b = data[2];
      final token = data[3];

      packetCount++;
      debugPrint("🟡 Legacy Token=$token | Color: R=$r G=$g B=$b");

      setState(() {
        currentColor = Color.fromARGB(255, r, g, b);
        debugText = "R=$r G=$g B=$b\nToken=$token\nPackets: $packetCount";
      });
    } else {
      debugPrint("❌ Invalid data: ${data.toList()}");
      setState(() {
        debugText = "Invalid data: ${data.toList()}";
      });
    }
  }

  @override
  void dispose() {
    scanRestartTimer?.cancel();
    scanSub?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: currentColor,
      body: SafeArea(
        child: Center(
          child: Container(
            margin: const EdgeInsets.all(20),
            padding: const EdgeInsets.all(20),
            decoration: BoxDecoration(
              color: Colors.black.withOpacity(0.7),
              borderRadius: BorderRadius.circular(15),
              border: Border.all(color: Colors.white.withOpacity(0.3)),
            ),
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                const Text(
                  "🎨 BLE Color Receiver",
                  style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold, color: Colors.white),
                  textAlign: TextAlign.center,
                ),
                const SizedBox(height: 20),
                Container(
                  padding: const EdgeInsets.all(15),
                  decoration: BoxDecoration(
                    color: Colors.black.withOpacity(0.5),
                    borderRadius: BorderRadius.circular(10),
                  ),
                  child: Text(
                    debugText,
                    style: const TextStyle(fontSize: 16, fontFamily: 'monospace', color: Colors.white),
                    textAlign: TextAlign.center,
                  ),
                ),
                const SizedBox(height: 15),
                Text(
                  "Listening for '$targetName'",
                  style: TextStyle(fontSize: 14, color: Colors.white.withOpacity(0.8)),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
