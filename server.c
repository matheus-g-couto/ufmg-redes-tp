#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

#define MAX_CLIENT_CONNECTIONS 10
#define MAX_PEERS 2

typedef struct sockaddr_storage sockaddr_storage;
typedef struct sockaddr sockaddr;
typedef struct sockaddr_in6 sockaddr_in6;

typedef struct client_data {
    int sock;
} client_data;

typedef struct p2p_data {
    int port;
} p2p_data;

typedef struct user_info {
    char id[10];
    int loc_or_auth;
} user_info;

int peer_id;

int active_con;
user_info *users[10];

void usage() {
    printf("Exemplo de uso:\n");
    printf("./server <p2p port> <client port>\n");
    exit(EXIT_FAILURE);
}

void config_sock(int sock) {
    // permite o socket receber conexoes ipv4 e ipv6
    int ipv6_only = 0;
    if (0 != setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &ipv6_only, sizeof(ipv6_only))) {
        logexit("config socket");
    }

    // reuso de socket
    int enable = 1;
    if (0 != setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("sockopt");
    }
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

int start_client_sock(uint16_t port) {
    int sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock == -1) {
        logexit("erro no socket");
    }

    config_sock(sock);

    sockaddr_in6 addr;
    if (0 != server_sockaddr_init(port, &addr)) {
        logexit("init client sockaddr");
    }

    sockaddr *saddr = (sockaddr *)(&addr);
    if (0 != bind(sock, saddr, sizeof(addr))) {
        logexit("erro no bind");
    }

    if (0 != listen(sock, MAX_CLIENT_CONNECTIONS)) {
        logexit("listen");
    }

    return sock;
}

int start_p2p_sock(uint16_t port) {
    int sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock == -1) {
        logexit("erro no socket");
    }

    config_sock(sock);

    sockaddr_in6 addr;
    if (0 != server_sockaddr_init(port, &addr)) {
        logexit("init p2p sockaddr");
    }

    if (0 != connect(sock, (sockaddr *)&addr, sizeof(addr))) {
        printf("No peer found, starting to listen...\n");

        if (0 != bind(sock, (sockaddr *)&addr, sizeof(addr))) {
            logexit("Peer bind error");
        }

        if (0 != listen(sock, MAX_PEERS)) {
            logexit("Peer listen error");
        }
    } else {
        // handle new peer
    }

    return sock;
}

int handle_client(int sock) {
    printf("[log] connection\n");

    char buffer[MSGSIZE];
    memset(buffer, 0, MSGSIZE);

    size_t count = recv(sock, buffer, MSGSIZE, 0);
    if (count <= 0) {
        printf("[log] conexão encerrada\n");
        close(sock);
        active_con--;
        return 1;
    }

    printf("[msg] %d bytes: %s\n", (int)count, buffer);

    if (0 == strncmp(buffer, "kill", 4)) {
        printf("[log] conexão encerrada pelo cliente\n");
        count = send(sock, buffer, strlen(buffer) + 1, 0);

        close(sock);
        active_con--;
        return 1;
    }

    sprintf(buffer, "msg recebida\n");
    count = send(sock, buffer, strlen(buffer) + 1, 0);

    if (count != strlen(buffer) + 1) {
        logexit("send");
    }

    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage();
    }

    // get ports
    uint16_t p2p_port, client_port;
    p2p_port = format_port(argv[1]);
    client_port = format_port(argv[2]);

    // start sockets
    int clientsock = start_client_sock(client_port);
    int p2psock = start_p2p_sock(p2p_port);

    // *users = malloc(10 * sizeof(user_info));
    // active_con = 0;

    // adiciona os sockets ao set
    fd_set current_socks, ready_socks;
    FD_ZERO(&current_socks);
    FD_SET(clientsock, &current_socks);
    FD_SET(p2psock, &current_socks);

    int has_peer = 0;

    char buffer[MSGSIZE];

    while (1) {
        ready_socks = current_socks;
        memset(buffer, 0, MSGSIZE);

        if (0 > select(FD_SETSIZE, &ready_socks, NULL, NULL, NULL)) {
            logexit("erro no select");
        }

        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &ready_socks)) {
                printf("aaaa\n");
                if (i == clientsock) {
                    int newclientsock = accept(clientsock, NULL, NULL);

                    if (newclientsock >= 0) {
                        FD_SET(newclientsock, &current_socks);
                        printf("New client connected.\n");
                    }
                } else if (i == p2psock) {
                    int newpeersock = accept(p2psock, NULL, NULL);

                    if (newpeersock >= 0) {
                        if (has_peer >= MAX_PEERS) {
                            printf("Peer limit exceeded\n");
                            sprintf(buffer, "Peer limit exceeded\n");
                            send(newpeersock, buffer, strlen(buffer) + 1, 0);
                            close(newpeersock);
                        } else {
                            printf("Peer connected.\n");
                            FD_SET(newpeersock, &current_socks);
                            has_peer++;
                        }
                    }
                } else {
                    if (has_peer && i == p2psock) {
                        // handlePeerCommunication(i, &has_peer);
                    } else {
                        int end_connection = handle_client(i);
                        if (end_connection) {
                            FD_CLR(i, &current_socks);
                        }
                    }
                }
            }
        }
    }
    close(clientsock);
    close(p2psock);

    exit(EXIT_SUCCESS);
    return 0;
}