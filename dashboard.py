from flask import Flask, jsonify
import psycopg2

app = Flask(__name__)

def get_db_connection():
    return psycopg2.connect(host="postgres_db", user="postgres", password="firm_password", dbname="auditdb")

@app.route('/')
def index():
    return """
    <html>
        <head>
            <title>Legal Cloud Compliance SOC</title>
            <style>
                body { font-family: -apple-system, sans-serif; background: #0d1117; color: #c9d1d9; padding: 2rem; }
                table { width: 100%; border-collapse: collapse; margin-top: 20px; }
                th, td { border: 1px solid #30363d; padding: 12px; text-align: left; }
                th { background: #161b22; }
                .hash { color: #58a6ff; font-family: monospace; }
                .redacted { color: #3fb950; font-weight: bold; }
            </style>
        </head>
        <body>
            <h1>Enterprise Document Sanitizer</h1>
            <table>
                <thead>
                    <tr>
                        <th>Ingestion Time</th>
                        <th>Case ID</th>
                        <th>Chain of Custody (SHA-256)</th>
                        <th>Sanitized Case Transcript</th>
                        <th>Status</th>
                    </tr>
                </thead>
                <tbody id="logs"></tbody>
            </table>
            <script>
                function fetchLogs() {
                    fetch('/api/logs').then(res => res.json()).then(data => {
                        const tbody = document.getElementById('logs');
                        tbody.innerHTML = '';
                        data.forEach(row => {
                            tbody.innerHTML += `<tr>
                                <td>${row[4]}</td>
                                <td>${row[1]}</td>
                                <td class="hash">${row[2]}</td>
                                <td>${row[3]}</td>
                                <td class="redacted">✅ Redacted</td>
                            </tr>`;
                        });
                    });
                }
                setInterval(fetchLogs, 2000);
                fetchLogs();
            </script>
        </body>
    </html>
    """

@app.route('/api/logs')
def logs():
    try:
        conn = get_db_connection()
        cur = conn.cursor()
        cur.execute("SELECT * FROM legal_audits ORDER BY timestamp DESC LIMIT 50;")
        rows = cur.fetchall()
        cur.close()
        conn.close()
        return jsonify(rows)
    except Exception as e:
        return jsonify({"error": str(e)})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=3000)