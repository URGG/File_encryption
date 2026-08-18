#include "../UDPSocket.h"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <stdexcept>

UDPSocket::UDPSocket(uint16_t port) {
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        throw std::runtime_error("Socket creation failed: " + std::string(strerror(errno)));
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(sockfd, (const struct sockaddr *)&address, sizeof(address)) < 0) {
        close(sockfd);
        throw std::runtime_error("Bind failed: " + std::string(strerror(errno)));
    }
}

UDPSocket::~UDPSocket() {
    if (sockfd >= 0) {
        close(sockfd);
    }
}

ssize_t UDPSocket::receive(char* buffer, size_t max_len, struct sockaddr_in& client_addr) {
    socklen_t len = sizeof(client_addr);
    return recvfrom(sockfd, buffer, max_len, 0, (struct sockaddr *)&client_addr, &len);
}

ssize_t UDPSocket::sendTo(const char* buffer, size_t len, const struct sockaddr_in& target_addr) {
    return sendto(sockfd, buffer, len, 0, (const struct sockaddr *)&target_addr, sizeof(target_addr));
}