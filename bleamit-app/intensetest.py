import socket
import time
import sys
import subprocess
import threading
import re
from datetime import datetime
import random

ARTNET_PORT = 6454
UNIVERSE = 0
CHANNELS = 512
RUNNING = True
send_times = {}

def timestamp():
    return datetime.now().strftime("%H:%M:%S.%f")[:-3]

def make_artdmx_packet(r, g, b):
    packet = bytearray()
    packet += b'Art-Net\x00'              # ID
    packet += bytes([0x00, 0x50])         # OpCode: ArtDMX
    packet += bytes([0x00, 0x0e])         # Protocol version
    packet += bytes([0x00, 0x00])         # Sequence, Physical
    packet += UNIVERSE.to_bytes(2, 'little')
    packet += (CHANNELS).to_bytes(2, 'big')

    dmx_data = [0] * CHANNELS
    dmx_data[0] = r
    dmx_data[1] = g
    dmx_data[2] = b

    packet += bytes(dmx_data)
    return packet

def send_artnet_loop(sock, ip):
    global RUNNING
    print("🎆 Starting intense light show...")
    while RUNNING:
        r = random.randint(0, 255)
        g = random.randint(0, 255)
        b = random.randint(0, 255)

        packet = make_artdmx_packet(r, g, b)
        sock.sendto(packet, (ip, ARTNET_PORT))
        send_times[(r, g, b)] = time.time()

        print(f"{timestamp()} Sent: R={r} G={g} B={b}")
        time.sleep(0.1)  # 100ms between frames — simulate strobe/chase

def parse_flutter_logs():
    pattern = re.compile(r"Color update: R=(\d+) G=(\d+) B=(\d+)")
    process = subprocess.Popen(["flutter", "logs"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    print("📱 Launching Flutter logs (press Ctrl+C to stop)...\n")

    try:
        for line in process.stdout:
            line = line.strip()
            match = pattern.search(line)
            if match:
                r, g, b = map(int, match.groups())
                rgb = (r, g, b)
                now = time.time()
                latency = now - send_times.get(rgb, now)
                print(f"{timestamp()} Received: R={r} G={g} B={b} | Latency: {latency:.3f}s")
            else:
                print(line)
    except KeyboardInterrupt:
        pass
    finally:
        process.terminate()

def main():
    global RUNNING

    if len(sys.argv) < 2:
        print("Usage: python testapp.py <ESP32_IP>")
        sys.exit(1)

    target_ip = sys.argv[1]
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    try:
        sender_thread = threading.Thread(target=send_artnet_loop, args=(sock, target_ip))
        sender_thread.start()
        parse_flutter_logs()
    except KeyboardInterrupt:
        print("\n🛑 Interrupted by user.")
    finally:
        RUNNING = False
        sender_thread.join()
        sock.close()
        print("🛑 Cleanup complete.")

if __name__ == "__main__":
    main()

