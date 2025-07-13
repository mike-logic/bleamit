import CoreBluetooth
import UIKit

public protocol BleamitDelegate: AnyObject {
    func bleamitDidUpdateColor(_ color: UIColor)
}

public class BleamitScanner: NSObject, CBCentralManagerDelegate {

    private var central: CBCentralManager!
    public weak var delegate: BleamitDelegate?

    public override init() {
        super.init()
        central = CBCentralManager(delegate: self, queue: nil)
    }

    public func centralManagerDidUpdateState(_ central: CBCentralManager) {
        if central.state == .poweredOn {
            central.scanForPeripherals(withServices: nil, options: [
                CBCentralManagerScanOptionAllowDuplicatesKey: true
            ])
        }
    }

    public func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral,
                               advertisementData: [String: Any], rssi RSSI: NSNumber) {
        guard let mfgData = advertisementData[CBAdvertisementDataManufacturerDataKey] as? Data else { return }
        let bytes = [UInt8](mfgData)
        guard bytes.count >= 4, bytes[0] == 0xAB else { return }

        let r = Int(bytes[1])
        let g = Int(bytes[2])
        let b = Int(bytes[3])
        DispatchQueue.main.async {
            self.delegate?.bleamitDidUpdateColor(UIColor(red: CGFloat(r)/255.0,
                                                         green: CGFloat(g)/255.0,
                                                         blue: CGFloat(b)/255.0,
                                                         alpha: 1.0))
        }
    }
}

