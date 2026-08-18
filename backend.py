import socket

PORT = 9000

def start_server():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('0.0.0.0', PORT))
    print(f"[*] DB active on port: {PORT}")

    while True:
        data, addr = sock.recvfrom(1024)

        try:
            payload = data.decode('utf-8', errors='ignore'  ).strip('\x00 \n\r\t')

            if payload.upper().startswith("PING"):
                # --- NEW: Print visual proof of the return fire ---
                print(f"[HEALTH CHECK] Received from {addr[0]} PORT{addr[1]}| Firing PONG")
                sock.sendto(b"PONG", addr)
            else:
                print(f"[SECURE AUDIT LOG SAVED] {payload}")

        except Exception as e:
            print(f"[ERROR] Failed to process packet: {e}")

if __name__ == "__main__":
    start_server()