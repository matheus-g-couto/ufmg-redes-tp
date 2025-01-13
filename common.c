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

void addrtostr(const sockaddr* addr, char* str, size_t strsize) {
    int version;
    char addrstr[INET_ADDRSTRLEN];
    uint16_t port;

    if (addr->sa_family == AF_INET) {
        version = 4;
        sockaddr_in* addr4 = (sockaddr_in*)addr;

        if (!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr, INET_ADDRSTRLEN + 1)) {
            logexit("ntop");
        }

        port = ntohs(addr4->sin_port);
    } else if (addr->sa_family == AF_INET6) {
        version = 6;
        sockaddr_in6* addr6 = (sockaddr_in6*)addr;

        if (!inet_ntop(AF_INET6, &(addr6->sin6_addr), addrstr, INET6_ADDRSTRLEN + 1)) {
            logexit("ntop");
        }

        port = ntohs(addr6->sin6_port);
    } else {
        logexit("unknown protocol");
    }

    if (str) {
        sprintf(str, "IPv%d %s %hu", version, addrstr, port);
    }
}

Client get_client(Client* clist, int n, int sock) {
    for (int i = 0; i < n; i++) {
        if (clist[i].sock == sock) return clist[i];
    }

    return (Client){-1, -1, 0};
}