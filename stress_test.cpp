#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <atomic>


std::atomic<int> packets_sent(0);

void fire_payloads(int num_packets) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    for (int i = 0; i < num_packets; ++i) {

        std::string payload = "{\"user_id\": 994, \"action\": \"READ_CASE_FILE\", \"doc_id\": \"CONFIDENTIAL_" + std::to_string(rand() % 9000 + 1000) + "\"}";

        sendto(sock, payload.c_str(), payload.length(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        packets_sent++;


        usleep(100);
    }
    close(sock);
}
//ass blaster 9000
int main() {
    int num_threads = 10;
    int packets_per_thread = 1000;

    std::cout << "[STRESS TEST] Spawning " << num_threads << " threads..." << std::endl;
    std::cout << "[STRESS TEST] Firing " << num_threads * packets_per_thread << " UDP payloads at the proxy..." << std::endl;

    std::vector<std::thread> threads;


    for (int i = 0; i < num_threads; ++i) {
        threads.push_back(std::thread(fire_payloads, packets_per_thread));
    }


    for (auto& t : threads) {
        t.join();
    }

    std::cout << "[STRESS TEST] Successfully blasted " << packets_sent << " packets!" << std::endl;
    return 0;
}