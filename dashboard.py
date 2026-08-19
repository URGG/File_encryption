from flask import Flask, render_template_string, jsonify
import psycopg2
import json

app = Flask(__name__)

def get_db_connection():
    return psycopg2.connect(
        host="172.20.0.10",
        database="auditdb",
        user="postgres",
        password="firm_password"
    )

HTML_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <title>AI Security | Live Audit</title>
    <style>
        body { font-family: 'Courier New', Courier, monospace; background: #0a0a0a; color: #00ff00; padding: 30px; margin: 0;}
        h1 { border-bottom: 1px solid #333; padding-bottom: 10px; font-size: 1.5em; }
        table { width: 100%; border-collapse: collapse; margin-top: 20px; }
        th, td { border: 1px solid #222; padding: 12px; text-align: left; }
        th { background: #111; color: #fff; }
        tr:nth-child(even) { background-color: #151515; }
        .signature { color: #555; font-size: 0.85em; word-break: break-all; }
        .timestamp { color: #888; }
        .threat-row { background-color: #4a0000 !important; color: #ff6b6b; border: 1px solid #ff0000; }
        .pulse { display: inline-block; width: 10px; height: 10px; background: red; border-radius: 50%; animation: blink 1s infinite; margin-right: 10px;}
        @keyframes blink { 50% { opacity: 0; } }
    </style>
    <script>
        setInterval(() => {
            fetch('/api/logs')
                .then(response => response.json())
                .then(data => {
                    let tableContent = "<tr><th>Timestamp</th><th>User Action</th><th>Document ID</th><th>SHA-256 Signature</th><th>AI Status</th></tr>";
                    data.logs.forEach(log => {
                        let action = "UNKNOWN";
                        let doc_id = "UNKNOWN";
                        try {
                            let obj = JSON.parse(log.payload);
                            action = `${obj.action} (UID: ${obj.user_id})`;
                            doc_id = obj.doc_id;
                        } catch (e) {}
                        
                        let rowClass = log.is_threat ? "threat-row" : "";
                        let aiStatus = log.is_threat ? "⚠️ ANOMALY DETECTED" : "✅ CLEAR";

                        tableContent += `<tr class="${rowClass}">
                            <td class="timestamp">${log.time}</td>
                            <td>${action}</td>
                            <td>${doc_id}</td>
                            <td class="signature">${log.signature}</td>
                            <td><strong>${aiStatus}</strong></td>
                        </tr>`;
                    });
                    document.getElementById("log-table").innerHTML = tableContent;
                });
        }, 2000);
    </script>
</head>
<body>
    <h1><span class="pulse"></span> Live Llama 3 Threat Analysis Stream</h1>
    <table id="log-table">
        <tr><th>Timestamp</th><th>User Action</th><th>Document ID</th><th>SHA-256 Signature</th><th>AI Status</th></tr>
    </table>
</body>
</html>
"""

@app.route('/')
def index():
    return render_template_string(HTML_TEMPLATE)

@app.route('/api/logs')
def get_logs():
    conn = get_db_connection()
    cur = conn.cursor()
    cur.execute("SELECT raw_payload, signature, created_at, is_threat FROM audit_logs ORDER BY id DESC LIMIT 20")
    rows = cur.fetchall()
    cur.close()
    conn.close()

    logs = [
        {"payload": row[0], "signature": row[1], "time": row[2].strftime("%Y-%m-%d %H:%M:%S"), "is_threat": row[3]}
        for row in rows
    ]
    return jsonify({"logs": logs})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=3000)