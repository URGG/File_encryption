import socket
import psycopg2
import time
import requests
import json

PORT = 9000
# host.docker.internal routes the container traffic securely to your Mac's localhost
OLLAMA_URL = "http://host.docker.internal:11434/api/generate"

def get_db_connection():
    return psycopg2.connect(
        host="172.20.0.10",
        database="auditdb",
        user="postgres",
        password="firm_password"
    )

def init_db():
    while True:
        try:
            conn = get_db_connection()
            cur = conn.cursor()
            # Drop the old table so we can rebuild it with the new is_threat column
            cur.execute('DROP TABLE IF EXISTS audit_logs')
            cur.execute('''
                        CREATE TABLE audit_logs (
                                                    id SERIAL PRIMARY KEY,
                                                    raw_payload TEXT,
                                                    signature TEXT,
                                                    is_threat BOOLEAN DEFAULT FALSE,
                                                    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                        )
                        ''')
            conn.commit()
            cur.close()
            conn.close()
            print("[*] AI-Ready PostgreSQL Database Initialized")
            break
        except Exception:
            time.sleep(2)

def analyze_threat(payload):
    # Prompt Llama 3 to analyze the behavior
    prompt = f"You are a cybersecurity SIEM system. Evaluate this database access event: {payload}. If the user ID is 994 and they are accessing CONFIDENTIAL case files, flag this as a potential data exfiltration anomaly. Reply strictly with the word YES or NO."

    try:
        response = requests.post(OLLAMA_URL, json={
            "model": "llama3",
            "prompt": prompt,
            "stream": False
        }, timeout=15)

        result = response.json().get("response", "").strip().upper()
        print(f"[LLM ANALYSIS] Result: {result}")
        return "YES" in result
    except Exception as e:
        print(f"[LLM TIMEOUT] Skipping AI check.")
        return False

def start_server():
    init_db()
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('0.0.0.0', PORT))
    print(f"[*] AI DB Worker active on port: {PORT}")

    while True:
        data, addr = sock.recvfrom(1024)

        try:
            payload = data.decode('utf-8', errors='ignore').strip('\x00 \n\r\t')

            if payload.upper().startswith("PING"):
                sock.sendto(b"PONG", addr)
            else:
                parts = payload.split(" | SHA256_SIG: ")
                json_part = parts[0]
                sig_part = parts[1] if len(parts) > 1 else "N/A"

                # --- NEW: Run the payload through Llama 3 ---
                is_anomalous = analyze_threat(json_part)

                conn = get_db_connection()
                cur = conn.cursor()
                cur.execute(
                    "INSERT INTO audit_logs (raw_payload, signature, is_threat) VALUES (%s, %s, %s)",
                    (json_part, sig_part, is_anomalous)
                )
                conn.commit()
                cur.close()
                conn.close()

        except Exception as e:
            pass

if __name__ == "__main__":
    start_server()