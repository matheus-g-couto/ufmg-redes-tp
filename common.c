#include <arpa/inet.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

typedef struct sockaddr_storage sockaddr_storage;
typedef struct sockaddr sockaddr;

typedef struct in_addr in_addr;
typedef struct in6_addr in6_addr;
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr_in6 sockaddr_in6;

typedef struct {
    int id;
    int sock;
    int loc;
} Client;

typedef struct {
    char id[11];
    int special;
} User;

void logexit(const char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

uint16_t format_port(const char* portstr) {
    uint16_t port = (uint16_t)atoi(portstr);

    return htons(port);
}

int addrparse(const char* addrstr, uint16_t port, sockaddr_storage* storage) {
    if (addrstr == NULL || port == 0) {
        return -1;
    }

    in_addr inaddr4;
    if (inet_pton(AF_INET, addrstr, &inaddr4)) {
        sockaddr_in* addr4 = (sockaddr_in*)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }

    struct in6_addr inaddr6;
    if (inet_pton(AF_INET6, addrstr, &inaddr6)) {
        sockaddr_in6* addr6 = (sockaddr_in6*)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
        return 0;
    }
    return -1;
}

Client get_client(Client* clist, int n, int sock) {
    for (int i = 0; i < n; i++) {
        if (clist[i].sock == sock) return clist[i];
    }

    return (Client){-1, -1, 0};
}