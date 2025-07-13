package com.bleamit.sdk

import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothManager
import android.bluetooth.le.*
import android.content.Context
import android.util.Log

interface BleamitListener {
    fun onColorUpdate(r: Int, g: Int, b: Int)
}

class BleamitScanner(private val context: Context) {

    private val scanner: BluetoothLeScanner by lazy {
        val bluetoothManager = context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        bluetoothManager.adapter.bluetoothLeScanner
    }

    private var listener: BleamitListener? = null

    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            result.scanRecord?.manufacturerSpecificData?.let { data ->
                val bleamitData = data.get(0xFFFF)
                if (bleamitData != null && bleamitData.size >= 4 && bleamitData[0].toInt() == 0xAB) {
                    val r = bleamitData[1].toInt() and 0xFF
                    val g = bleamitData[2].toInt() and 0xFF
                    val b = bleamitData[3].toInt() and 0xFF
                    listener?.onColorUpdate(r, g, b)
                }
            }
        }
    }

    fun start() {
        val settings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build()
        val filters = listOf(ScanFilter.Builder().build())
        scanner.startScan(filters, settings, scanCallback)
    }

    fun stop() {
        scanner.stopScan(scanCallback)
    }

    fun setListener(l: BleamitListener) {
        listener = l
    }
}

