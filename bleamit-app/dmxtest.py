import socket
import time
import sys
import subprocess
import threading
import re
from datetime import datetime

ARTNET_PORT = 6454
UNIVERSE = 0
CHANNELS = 512
RUNNING = True
send_times = {}

def timestamp():
    return datetime.now().strftime("%H:%M:%S.%f")[:-3]

def make_artdmx_packet(r, g, b):
    packet = bytearray()
    packet += b'Art-Net\x00'
    packet += bytes([0x00, 0x50])
    packet += bytes([0x00, 0x0e])
    packet += bytes([0x00, 0x00])
    packet += UNIVERSE.to_bytes(2, 'little')
    packet += (CHANNELS).to_bytes(2, 'big')

    dmx_data = [0] * CHANNELS
    dmx_data[0] = r
    dmx_data[1] = g
    dmx_data[2] = b

    packet += bytes(dmx_data)
    return packet

def interpolate_color(start, end, steps):
    for i in range(steps):
        r = int(start[0] + (end[0] - start[0]) * i / steps)
        g = int(start[1] + (end[1] - start[1]) * i / steps)
        b = int(start[2] + (end[2] - start[2]) * i / steps)
        yield (r, g, b)

def send_artnet_loop(sock, ip):
    print("🌈 Starting DMX fade show...")

    base_colors = [
        (255, 0, 0),     # Red
        (255, 255, 0),   # Yellow
        (0, 255, 0),     # Green
        (0, 255, 255),   # Cyan
        (0, 0, 255),     # Blue
        (255, 0, 255),   # Magenta
        (255, 255, 255), # White flash
        (0, 0, 0),       # Blackout
    ]

    fade_steps = 8
    delay = 0.08  # 80ms per frame

    i = 0
    while RUNNING:
        start = base_colors[i % len(base_colors)]
        end = base_colors[(i + 1) % len(base_colors)]

        for rgb in interpolate_color(start, end, fade_steps):
            if not RUNNING:
                break
            packet = make_artdmx_packet(*rgb)
            sock.sendto(packet, (ip, ARTNET_PORT))
            send_times[rgb] = time.time()
            print(f"{timestamp()} Sent: R={rgb[0]} G={rgb[1]} B={rgb[2]}")
            time.sleep(delay)

        i += 1

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
        print("Usage: python testapp_fade.py <ESP32_IP>")
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

