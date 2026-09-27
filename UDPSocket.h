//
// Created by George Urgiles on 8/11/26.
//

#ifndef GAMESERVER_UDPSOCKET_H
#define GAMESERVER_UDPSOCKET_H


#include <string>
#include <netinet/in.h>
#include <sys/socket.h>

class UDPSocket {
private:
    int sockfd;
    struct sockaddr_in address;

public:

    UDPSocket(uint16_t port);


    ~UDPSocket();


    ssize_t receive(char* buffer, size_t max_len, struct sockaddr_in& client_addr);
    ssize_t sendTo(const char* buffer, size_t len, const struct sockaddr_in& target_addr);
};

#endif