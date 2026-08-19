# Secure Audit Router (SIEM Pipeline)

![C++](https://img.shields.io/badge/C++-17-blue.svg)
![Python](https://img.shields.io/badge/Python-3.9-yellow.svg)
![PostgreSQL](https://img.shields.io/badge/PostgreSQL-15-336791.svg)
![Docker](https://img.shields.io/badge/Docker-Compose-2496ED.svg)
![OpenSSL](https://img.shields.io/badge/OpenSSL-Cryptography-red.svg)

A high-throughput, polyglot UDP load balancer and cryptographic auditing pipeline designed for secure, isolated container networks.

This architecture bridges a hyper-optimized C++ routing engine with Python data workers to process, cryptographically sign, and permanently store UDP network events in a PostgreSQL relational database. It features a live-updating web dashboard for real-time security monitoring (SIEM).

##  Core Architecture & Technical Milestones

* **Bare-Metal UDP Connection Tracking:** Defeats stateless Docker NAT limitations by actively tracking ephemeral UDP sockets via the Linux kernel (`connect()`), establishing a secure return path for real-time `PONG` health checks.
* **OpenSSL Cryptographic Signing:** Integrates OpenSSL `EVP` protocols within the main network loop to dynamically hash incoming data payloads via SHA-256, generating immutable compliance logs without bottlenecking throughput.
* **Active Failover & Health Monitoring:** A multi-threaded background process continuously probes backend logging nodes, seamlessly rerouting traffic the second a microservice drops offline.
* **Polyglot Microservices:** Routes strictly formatted C++ network byte streams into high-level Python workers (`psycopg2`), permanently indexing the JSON payloads and signatures into a PostgreSQL volume.
* **Live Telemetry Dashboard:** A Flask-based frontend continuously queries the database to serve a real-time, asynchronous HTML monitoring interface.

##  The Routing Flow
1. `Client` blasts an unencrypted UDP JSON payload to the Proxy.
2. `C++ Engine` intercepts the datagram, signs it with a secret key via OpenSSL, and routes it to an `ONLINE` backend.
3. `Python Worker` catches the routed packet and parses the signature.
4. `PostgreSQL` permanently commits the payload and signature to disk.
5. `Web Dashboard` renders the newly secured audit log in real-time.

---

##  Quick Start

### 1. Boot the Cluster
Ensure Docker Desktop is running
```bash
docker-compose up --build