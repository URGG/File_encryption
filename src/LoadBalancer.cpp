#include "../LoadBalancer.h"

#include <arpa/inet.h>
#include <iostream>
#include <chrono>
#include <cstring>

LoadBalancer::LoadBalancer(uint16_t port) : current_index(0), target_port(port), keep_running(true) {
    // Launch the background thread immediately upon object creation
    health_thread = std::thread(&LoadBalancer::healthCheckLoop, this);
}

LoadBalancer::~LoadBalancer() {
    keep_running = false; // Signal the background thread to stop
    if (health_thread.joinable()) {
        health_thread.join(); // Wait for the thread to safely finish before destroying the object
    }
}

void LoadBalancer::addBackend(const std::string& ip) {
    std::lock_guard<std::mutex> lock(mtx); // Lock the list while adding

    Backend backend;
    backend.ip = ip;
    backend.is_healthy = true; // Assume healthy on startup
    backend.address.sin_family = AF_INET;
    backend.address.sin_addr.s_addr = inet_addr(ip.c_str());
    backend.address.sin_port = htons(target_port);

    backends.push_back(backend);
}

bool LoadBalancer::getNextTarget(struct sockaddr_in& out_addr, std::string& out_ip) {
    std::lock_guard<std::mutex> lock(mtx); // Lock the list before reading it

    if (backends.empty()) return false;

    // Loop through backends until we find a healthy one
    size_t start_index = current_index;
    do {
        if (backends[current_index].is_healthy) {
            out_addr = backends[current_index].address;
            out_ip = backends[current_index].ip;

            // Increment for the next packet (Round-Robin)
            current_index = (current_index + 1) % backends.size();
            return true;
        }
        current_index = (current_index + 1) % backends.size();
    } while (current_index != start_index);

    // If we loop all the way around and find nothing, all servers are offline
    return false;
}

size_t LoadBalancer::getTargetCount() {
    std::lock_guard<std::mutex> lock(mtx);
    return backends.size();
}


void LoadBalancer::healthCheckLoop() {
    while (keep_running) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << "[SYSTEM] Running background health checks..." << std::endl;

        std::vector<Backend> backends_copy;
        {
            std::lock_guard<std::mutex> lock(mtx);
            backends_copy = backends;
        }

        for (auto& backend : backends_copy) {
            int ping_sock = socket(AF_INET, SOCK_DGRAM, 0);

            struct timeval tv;
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            setsockopt(ping_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

            const char* ping_msg = "PING";
            sendto(ping_sock, ping_msg, strlen(ping_msg), 0, (struct sockaddr*)&backend.address, sizeof(backend.address));

            char buffer[32];
            memset(buffer, 0, sizeof(buffer));

            // We use nullptr here so C++ accepts the PONG even if Docker's NAT slightly altered the return IP
            ssize_t n = recvfrom(ping_sock, buffer, sizeof(buffer) - 1, 0, nullptr, nullptr);

            if (n > 0) {
                backend.is_healthy = true;
                std::cout << "  -> Backend " << backend.ip << " is ONLINE" << std::endl;
            } else {
                backend.is_healthy = false;

                std::cerr << "[DEBUG] Backend " << backend.ip << " drop reason: " << strerror(errno) << std::endl;
            }

            close(ping_sock);
        }

        {
            std::lock_guard<std::mutex> lock(mtx);
            for (size_t i = 0; i < backends.size(); ++i) {
                backends[i].is_healthy = backends_copy[i].is_healthy;
            }
        }
    }
}