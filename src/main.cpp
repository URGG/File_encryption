#include <iostream>
#include "../UDPSocket.h"
#include "../LoadBalancer.h"
#include "../AuditSigner.h"

#define PROXY_PORT 8080
#define TARGET_PORT 9000
#define MAXLINE 1024

int main() {
    try {
        // 1. Initialize Objects
        UDPSocket proxySocket(PROXY_PORT);
        LoadBalancer balancer(TARGET_PORT);

        AuditSigner signer("SECRET_KEY");

        // 2. Configure Backend Targets
        balancer.addBackend("172.20.0.5");
        balancer.addBackend("172.20.0.6");
        balancer.addBackend("172.20.0.7");

        std::cout << "[PROXY] Active on port " << PROXY_PORT
                  << ". Balancing across " << balancer.getTargetCount() << " targets..." << std::endl;

        char buffer[MAXLINE];
        struct sockaddr_in client_addr;


        while (true) {
            ssize_t n = proxySocket.receive(buffer, MAXLINE - 1, client_addr);
            if (n < 0) continue;
            buffer[n] = '\0';

            std::string target_ip;
            struct sockaddr_in target_addr;

            if (balancer.getNextTarget(target_addr, target_ip)) {


                std::string secure_payload = signer.signPayload(std::string(buffer));


                proxySocket.sendTo(secure_payload.c_str(), secure_payload.length(), target_addr);

                std::cout << "[AUDIT LOG SECURED] Routed to: " << target_ip << std::endl;
            } else {
                std::cerr << "[ALERT] All logging databases down. Packet dropped." << std::endl;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[FATAL EXCEPTION] " << e.what() << std::endl;
        return 1;
    }
    return 0;
}