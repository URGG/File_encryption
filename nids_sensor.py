import socket
import json
import time
from scapy.all import sniff, IP, TCP, UDP


PROXY_IP = "127.0.0.1"
PROXY_PORT = 8080
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def process_packet(packet):
    if IP in packet:
        src_ip = packet[IP].src
        dst_ip = packet[IP].dst
        protocol = "UNKNOWN"
        port = "N/A"


        if TCP in packet:
            protocol = "TCP_TRAFFIC"
            port = str(packet[TCP].dport)
        elif UDP in packet:
            protocol = "UDP_TRAFFIC"
            port = str(packet[UDP].dport)


        payload = {
            "user_id": src_ip,
            "action": protocol,
            "doc_id": f"{dst_ip}:{port}"
        }

        print(f"[SNIFFED] {src_ip} -> {dst_ip}:{port}")


        sock.sendto(json.dumps(payload).encode('utf-8'), (PROXY_IP, PROXY_PORT))


        time.sleep(1)

print("[*] NIDS Sensor Active. Tapping into live Mac network traffic...")

sniff(prn=process_packet, filter="ip", count=30)