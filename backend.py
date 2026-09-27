import socket
import psycopg2
import json
import requests
import time

# Internal Docker DNS for containerized Llama 3
OLLAMA_URL = "http://ollama:11434/api/generate"

def init_db():
    conn = psycopg2.connect(host="postgres_db", user="postgres", password="firm_password", dbname="auditdb")
    cursor = conn.cursor()
    cursor.execute("""
                   CREATE TABLE IF NOT EXISTS legal_audits (
                                                               id SERIAL PRIMARY KEY,
                                                               case_id INT,
                                                               raw_signature VARCHAR(255),
                       sanitized_text TEXT,
                       timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                       )
                   """)
    conn.commit()
    return conn

def sanitize_document(payload):
    prompt = f"You are a strict legal redaction engine. Read this raw case file: '{payload}'. Replace all names, phone numbers, and SSNs with [REDACTED_NAME], [REDACTED_PHONE], or [REDACTED_SSN]. Return ONLY the sanitized text, nothing else."
    try:
        response = requests.post(OLLAMA_URL, json={
            "model": "llama3",
            "prompt": prompt,
            "stream": False
        }, timeout=120)
        return response.json().get("response", "").strip()
    except requests.exceptions.Timeout:
        print("[LLM TIMEOUT] Skipping AI check.")
        return "[SANITIZATION_FAILED_TIMEOUT]"
    except Exception as e:
        return f"[SANITIZATION_FAILED: {str(e)}]"

conn = init_db()
print("[*] AI-Ready PostgreSQL Database Initialized")

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("0.0.0.0", 9000))
print("[*] AI DB Worker active on port: 9000")

while True:
    data, addr = sock.recvfrom(4096)
    msg = data.decode('utf-8')

    # Immediately answer C++ health checks
    if msg == "PING":
        sock.sendto(b"PONG", addr)
        continue

    # Split the raw payload from the C++ OpenSSL signature
    parts = msg.split("|||")
    if len(parts) == 2:
        raw_json, signature = parts
        try:
            parsed = json.loads(raw_json)
            case_id = parsed.get("case_id", 0)
            transcript = parsed.get("transcript", "")

            sanitized_text = sanitize_document(transcript)
            print(f"[LLM ANALYSIS] Redacted result: {sanitized_text}")

            cursor = conn.cursor()
            cursor.execute(
                "INSERT INTO legal_audits (case_id, raw_signature, sanitized_text) VALUES (%s, %s, %s)",
                (case_id, signature, sanitized_text)
            )
            conn.commit()
        except Exception as e:
            print(f"[DB ERROR] {e}")