import socket
import time
import sys

ARTNET_PORT = 6454
UNIVERSE = 0
CHANNELS = 512

def make_artdmx_packet(r, g, b):
    packet = bytearray()
    packet += b'Art-Net\x00'                  # ID
    packet += bytes([0x00, 0x50])             # OpCode = OpDmx (0x5000 LE)
    packet += bytes([0x00, 0x0e])             # Protocol version
    packet += bytes([0x00, 0x00])             # Sequence, Physical
    packet += UNIVERSE.to_bytes(2, 'little')  # Universe
    packet += (CHANNELS).to_bytes(2, 'big')   # Data length

    dmx_data = [0] * CHANNELS
    dmx_data[0] = r
    dmx_data[1] = g
    dmx_data[2] = b

    packet += bytes(dmx_data)
    return packet

def send_color(sock, ip, r, g, b):
    packet = make_artdmx_packet(r, g, b)
    sock.sendto(packet, (ip, ARTNET_PORT))
    print(f"🎨 Sent to {ip}: R={r} G={g} B={b}")

def main():
    if len(sys.argv) < 2:
        print("Usage: python send_artnet_rgb.py <ESP32_IP>")
        sys.exit(1)

    target_ip = sys.argv[1]
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    try:
        while True:
            for r, g, b in [(255, 0, 0), (0, 255, 0), (0, 0, 255), (255, 
255, 0), (0, 255, 255)]:
                send_color(sock, target_ip, r, g, b)
                time.sleep(2)
    except KeyboardInterrupt:
        print("\n🛑 Stopped by user.")
    finally:
        sock.close()

if __name__ == "__main__":
    main()

