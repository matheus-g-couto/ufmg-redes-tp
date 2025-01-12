#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    int connsock;
    struct sockaddr_storage storage;
} client_data;

typedef struct p2p_data {
    int port;
} p2p_data;

typedef struct user_info {
    char id[10];
    int loc_or_auth;
} user_info;

uint8_t active_con;
user_info *users[10];

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

void client_thread(void *data) {
    client_data *cdata = (struct client_data *)data;
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);

    char client_addrstr[MSGSIZE];
    addrtostr(caddr, client_addrstr, MSGSIZE);

    if (active_con > MAX_CLIENT_CONNECTIONS) {
        printf("Client limit exceeded\n");
        close(cdata->connsock);
        pthread_exit(EXIT_FAILURE);
        return;
    }

    handle_client(cdata->connsock, client_addrstr);

    pthread_exit(EXIT_SUCCESS);
}

void handle_client(int connsock, const char *client_addrstr) {
    printf("[log] connection from %s\n", client_addrstr);
    active_con++;

    char buffer[MSGSIZE];
    while (1) {
        memset(buffer, 0, MSGSIZE);

        size_t count = recv(connsock, buffer, MSGSIZE, 0);
        if (count == 0) {
            printf("[log] conexão encerrada pelo cliente %s\n", client_addrstr);
            break;
        }

        printf("[msg] %s %d bytes: %s\n", client_addrstr, (int)count, buffer);

        if (0 == strncmp(buffer, "kill", 4)) {
            printf("[log] conexão encerrada pelo cliente %s\n", client_addrstr);
            count = send(connsock, buffer, strlen(buffer) + 1, 0);
            break;
        } else {
            sprintf(buffer, "msg recebida por: %.450s\n", client_addrstr);
            count = send(connsock, buffer, strlen(buffer) + 1, 0);
        }

        if (count != strlen(buffer) + 1) {
            logexit("send");
        }
    }

    close(connsock);
    active_con--;
}

// void handle_client_limit(int connsock, const char *client_addrstr) {
//     char buffer[MSGSIZE];

//     close(connsock);
// }

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
        printf("No peer found, starting to listen...");

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

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argv);
    }

    uint16_t p2p_port, client_port;
    p2p_port = format_port(argv[1]);
    client_port = format_port(argv[2]);

    // start sockets
    int clientsock = start_client_sock(client_port);
    int p2psock = start_p2p_sock(p2p_port);

    // char addrstr[MSGSIZE];
    // addrtostr(saddr, addrstr, MSGSIZE);

    // printf("listening client on %s, waiting connection\n", addrstr);

    *users = malloc(10 * sizeof(user_info));
    active_con = 0;

    while (1) {
        sockaddr_storage client_storage;
        sockaddr *client_addr = (sockaddr *)(&client_storage);
        socklen_t storagelen = sizeof(client_storage);

        int connsock = accept(clientsock, client_addr, &storagelen);
        if (connsock == -1) {
            logexit("accept");
        }

        client_data *cdata = malloc(sizeof(*cdata));
        if (!cdata) {
            logexit("malloc");
        }

        cdata->connsock = connsock;
        memcpy(&(cdata->storage), &client_storage, sizeof(client_storage));

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, cdata);
    }
    close(clientsock);
    close(p2psock);

    exit(EXIT_SUCCESS);
    return 0;
}