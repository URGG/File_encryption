#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <mutex>
#include <thread>
#include <chrono>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <iomanip>
#include <sstream>

// --- Thread-Safe Bloom Filter ---
class BloomFilter {
private:
    std::vector<bool> bit_array;
    size_t m_bits;
    size_t k_hashes;
    mutable std::mutex filter_mutex;

    size_t hash2(const std::string& key) const {
        size_t hash = 14695981039346656037ULL;
        for (char c : key) {
            hash ^= static_cast<size_t>(c);
            hash *= 1099511628211ULL;
        }
        return hash;
    }

public:
    BloomFilter(size_t expected_elements, double false_positive_rate) {
        m_bits = std::ceil(-(expected_elements * std::log(false_positive_rate)) / (std::log(2) * std::log(2)));
        k_hashes = std::ceil((m_bits / static_cast<double>(expected_elements)) * std::log(2));
        bit_array.resize(m_bits, false);
    }

    void add(const std::string& key) {
        size_t h1 = std::hash<std::string>{}(key);
        size_t h2 = hash2(key);
        std::lock_guard<std::mutex> lock(filter_mutex);
        for (size_t i = 0; i < k_hashes; ++i) {
            bit_array[(h1 + i * h2) % m_bits] = true;
        }
    }

    bool possibly_contains(const std::string& key) const {
        size_t h1 = std::hash<std::string>{}(key);
        size_t h2 = hash2(key);
        std::lock_guard<std::mutex> lock(filter_mutex);
        for (size_t i = 0; i < k_hashes; ++i) {
            if (!bit_array[(h1 + i * h2) % m_bits]) return false;
        }
        return true;
    }
};

// --- OpenSSL EVP SHA-256 Signer ---
std::string generate_signature(const std::string& data) {
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_sha256();
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int lengthOfHash = 0;

    EVP_DigestInit_ex(context, md, nullptr);
    EVP_DigestUpdate(context, data.c_str(), data.length());
    EVP_DigestFinal_ex(context, hash, &lengthOfHash);
    EVP_MD_CTX_free(context);

    std::stringstream ss;
    for (unsigned int i = 0; i < lengthOfHash; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

// Global Filter
BloomFilter doc_filter(100000, 0.01);

// Global Worker Pool
std::vector<std::string> workers = {"172.20.0.5", "172.20.0.6", "172.20.0.7"};
std::vector<bool> worker_status = {false, false, false};
std::mutex pool_mutex;
int current_worker = 0;

// --- Background Health Checker ---
void health_check_loop() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct timeval tv;
    tv.tv_sec = 1; tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::lock_guard<std::mutex> lock(pool_mutex);
        std::cout << "[SYSTEM] Running background health checks..." << std::endl;

        for (size_t i = 0; i < workers.size(); ++i) {
            struct sockaddr_in worker_addr{};
            worker_addr.sin_family = AF_INET;
            worker_addr.sin_port = htons(9000);
            inet_pton(AF_INET, workers[i].c_str(), &worker_addr.sin_addr);

            std::string ping = "PING";
            sendto(sock, ping.c_str(), ping.length(), 0, (struct sockaddr*)&worker_addr, sizeof(worker_addr));

            char buffer[1024];
            int n = recvfrom(sock, buffer, sizeof(buffer), 0, nullptr, nullptr);
            if (n > 0) {
                worker_status[i] = true;
                std::cout << "   -> Backend " << workers[i] << " is ONLINE" << std::endl;
            } else {
                worker_status[i] = false;
                std::cout << "[DEBUG] Backend " << workers[i] << " drop reason: Resource temporarily unavailable" << std::endl;
            }
        }
    }
}

// --- Main UDP Proxy Loop ---
int main() {
    int server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));

    std::cout << "[*] C++ Routing Engine & Bloom Filter Active on UDP 8080" << std::endl;
    std::thread hc_thread(health_check_loop);
    hc_thread.detach();

    while (true) {
        char buffer[2048];
        struct sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int bytes = recvfrom(server_fd, buffer, sizeof(buffer), 0, (struct sockaddr*)&client_addr, &client_len);
        
        if (bytes <= 0) continue;
        std::string payload(buffer, bytes);

        // Extract case_id using fast string parsing
        std::string case_id = "";
        size_t key_pos = payload.find("\"case_id\":");
        if (key_pos != std::string::npos) {
            size_t start_pos = payload.find_first_of("0123456789", key_pos);
            size_t end_pos = payload.find_first_not_of("0123456789", start_pos);
            if (start_pos != std::string::npos && end_pos != std::string::npos) {
                case_id = payload.substr(start_pos, end_pos - start_pos);
            }
        }

        // Bloom Filter Protection
        if (!case_id.empty()) {
            if (doc_filter.possibly_contains(case_id)) {
                std::cout << "[BLOOM FILTER] Duplicate detected: Case " << case_id << " | Dropping packet." << std::endl;
                continue;
            } else {
                doc_filter.add(case_id);
            }
        }

        // Append OpenSSL Signature
        std::string signature = generate_signature(payload);
        std::string final_payload = payload + "|||" + signature;

        // Round-Robin Routing
        std::lock_guard<std::mutex> lock(pool_mutex);
        int starting_worker = current_worker;
        bool sent = false;

        do {
            if (worker_status[current_worker]) {
                struct sockaddr_in dest_addr{};
                dest_addr.sin_family = AF_INET;
                dest_addr.sin_port = htons(9000);
                inet_pton(AF_INET, workers[current_worker].c_str(), &dest_addr.sin_addr);

                sendto(server_fd, final_payload.c_str(), final_payload.length(), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
                std::cout << "[AUDIT LOG SECURED] Routed to: " << workers[current_worker] << std::endl;
                
                current_worker = (current_worker + 1) % workers.size();
                sent = true;
                break;
            }
            current_worker = (current_worker + 1) % workers.size();
        } while (current_worker != starting_worker);

        if (!sent) std::cout << "[WARNING] ALL BACKEND WORKERS OFFLINE. Packet dropped." << std::endl;
    }
    return 0;
}