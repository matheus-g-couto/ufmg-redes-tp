#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

#define MAX_CONNECTIONS 10

typedef struct sockaddr_storage sockaddr_storage;
typedef struct sockaddr sockaddr;
typedef struct sockaddr_in6 sockaddr_in6;

void usage(char **argv) {
    printf("Exemplo de uso:\n");
    printf("./server <p2p port> <client port>\n");
    exit(EXIT_FAILURE);
}

int server_sockaddr_init(uint16_t port, sockaddr_in6 *addr) {
    if (port == 0) {
        return -1;
    }

    memset(addr, 0, sizeof(&addr));
    addr->sin6_family = AF_INET6;
    addr->sin6_addr = in6addr_any;
    addr->sin6_port = port;

    return 0;
}

void handle_client(int csock, const char *client_addrstr) {
    char buffer[MSGSIZE];
    while (1) {
        memset(buffer, 0, MSGSIZE);

        size_t count = recv(csock, buffer, MSGSIZE, 0);
        if (count == 0) {
            printf("[log] conexão encerrada pelo cliente %s\n", client_addrstr);
            break;
        }

        printf("[msg] %s %d bytes: %s\n", client_addrstr, (int)count, buffer);

        if (0 == strncmp(buffer, "kill", 4)) {
            printf("[log] conexão encerrada pelo cliente %s\n", client_addrstr);
            count = send(csock, buffer, strlen(buffer) + 1, 0);
            break;
        } else {
            sprintf(buffer, "msg recebida por: %.450s\n", client_addrstr);
            count = send(csock, buffer, strlen(buffer) + 1, 0);
        }

        if (count != strlen(buffer) + 1) {
            logexit("send");
        }
    }

    close(csock);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argv);
    }

    uint16_t p2p_port, client_port;
    p2p_port = format_port(argv[1]);
    client_port = format_port(argv[2]);

    int ssock = socket(AF_INET6, SOCK_STREAM, 0);
    if (ssock == -1) {
        logexit("erro no socket");
    }

    // permite o socket receber conexoes ipv4 e ipv6
    int ipv6_only = 0;
    if (0 != setsockopt(ssock, IPPROTO_IPV6, IPV6_V6ONLY, &ipv6_only, sizeof(ipv6_only))) {
        logexit("config socket");
    }

    // reuso de socket
    int enable = 1;
    if (0 != setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("sockopt");
    }

    sockaddr_in6 server_addr;
    if (0 != server_sockaddr_init(client_port, &server_addr)) {
        logexit("init sockaddr");
    }

    sockaddr *addr = (sockaddr *)(&server_addr);
    if (0 != bind(ssock, addr, sizeof(server_addr))) {
        logexit("erro no bind");
    }

    if (0 != listen(ssock, MAX_CONNECTIONS)) {
        logexit("listen");
    }

    char addrstr[MSGSIZE];
    addrtostr(addr, addrstr, MSGSIZE);

    printf("bound to %s, waiting connection\n", addrstr);

    while (1) {
        sockaddr_storage client_storage;
        sockaddr *client_addr = (sockaddr *)(&client_storage);
        socklen_t storagelen = sizeof(client_storage);

        int csock = accept(ssock, client_addr, &storagelen);
        if (csock == -1) {
            logexit("accept");
        }

        char client_addrstr[MSGSIZE];
        addrtostr(client_addr, client_addrstr, MSGSIZE);

        printf("[log] connection from %s\n", client_addrstr);

        handle_client(csock, client_addrstr);
    }
    close(ssock);

    exit(EXIT_SUCCESS);
    return 0;
}