#ifndef LOADBALANCER_H
#define LOADBALANCER_H

#include <vector>
#include <string>
#include <netinet/in.h>
#include <thread>
#include<mutex>
#include <atomic>

struct Backend {
    std::string ip;
    struct sockaddr_in address;
    bool is_healthy;

};

class LoadBalancer {
private:
    std::vector<Backend> backends;
    size_t current_index;
    uint16_t target_port;

    std::mutex mtx;
    std::atomic<bool> keep_running;
    std::thread health_thread;

    void healthCheckLoop();
public:
    LoadBalancer(uint16_t port);
    ~LoadBalancer();

    void addBackend(const std::string& ip);

    bool getNextTarget(struct sockaddr_in& out_addr, std::string& out_ip);

    size_t getTargetCount();
};

#endif