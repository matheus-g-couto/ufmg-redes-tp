#include <inttypes.h>
#include <pthread.h>
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

typedef struct client_data {
    int csock;
    struct sockaddr_storage storage;
} client_data;

typedef struct p2p_data {
    int sock;
    uint8_t alr_used_port;
} p2p_data;

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

    printf("[log] connection from %s\n", client_addrstr);

    handle_client(cdata->csock, client_addrstr);

    pthread_exit(EXIT_SUCCESS);
}

void p2p_thread(void *data) {
    p2p_data *pdata = (struct p2p_data *)data;

    sockaddr_storage client_storage;
    sockaddr *client_addr = (sockaddr *)(&client_storage);
    socklen_t storagelen = sizeof(client_storage);

    int csock = accept(pdata->sock, client_addr, &storagelen);
    pid_t cpid;

    char rec_buffer[MSGSIZE], send_buffer[MSGSIZE];

    cpid = fork();

    if (cpid == 0) {
        while (1) {
            memset(rec_buffer, 0, MSGSIZE);
            /*Receiving the request from client*/
            recv(csock, rec_buffer, sizeof(rec_buffer), 0);
            printf("\nCLIENT : %s\n", rec_buffer);
        }
    } else {
        while (1) {
            memset(send_buffer, 0, MSGSIZE);
            printf("\nType a message here ...  ");
            /*Read the message from client*/
            fgets(send_buffer, MSGSIZE - 1, stdin);
            /*Sends the message to client*/
            send(csock, send_buffer, strlen(send_buffer) + 1, 0);
            printf("\nMessage sent !\n");
        }
    }

    pthread_exit(EXIT_SUCCESS);
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

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argv);
    }

    uint16_t p2p_port, client_port;
    p2p_port = format_port(argv[1]);
    client_port = format_port(argv[2]);

    int ssock = socket(AF_INET6, SOCK_STREAM, 0), p2psock = socket(AF_INET6, SOCK_STREAM, 0);
    if (ssock == -1) {
        logexit("erro no socket");
    }
    if (p2psock == -1) {
        logexit("erro no socket");
    }

    // handle p2p connection
    config_sock(p2psock);

    uint8_t alr_used_port = 0;

    sockaddr_in6 p2p_addr;
    if (0 != server_sockaddr_init(p2p_port, &p2p_addr)) {
        logexit("init p2p sockaddr");
    }

    sockaddr *paddr = (sockaddr *)(&p2p_addr);
    if (0 != bind(p2psock, paddr, sizeof(p2p_addr))) {
        alr_used_port = 1;  // se o bind der errado, significa que a porta esta em uso

        if (0 != server_sockaddr_init(p2p_port + alr_used_port, &p2p_addr)) {
            logexit("init p2p sockaddr");
        }

        if (0 != bind(p2psock, paddr, sizeof(p2p_addr))) {
            logexit("bind");
        }
    }

    if (0 != listen(p2psock, 1)) {
        logexit("Peer limit exceeded");
    }

    printf("listening p2p on port %d\n", htons(p2p_port + alr_used_port));

    p2p_data *pdata = malloc(sizeof(*pdata));
    pdata->sock = p2psock;
    pdata->alr_used_port = alr_used_port;

    pthread_t ptid;
    pthread_create(&ptid, NULL, p2p_thread, pdata);

    // handle client connection
    config_sock(ssock);
    sockaddr_in6 server_addr;
    if (0 != server_sockaddr_init(client_port, &server_addr)) {
        logexit("init client sockaddr");
    }

    sockaddr *saddr = (sockaddr *)(&server_addr);
    if (0 != bind(ssock, saddr, sizeof(server_addr))) {
        logexit("erro no bind");
    }

    if (0 != listen(ssock, MAX_CONNECTIONS)) {
        logexit("listen");
    }

    char addrstr[MSGSIZE];
    addrtostr(saddr, addrstr, MSGSIZE);

    printf("listening client on %s, waiting connection\n", addrstr);

    while (1) {
        sockaddr_storage client_storage;
        sockaddr *client_addr = (sockaddr *)(&client_storage);
        socklen_t storagelen = sizeof(client_storage);

        int csock = accept(ssock, client_addr, &storagelen);
        if (csock == -1) {
            logexit("accept");
        }

        client_data *cdata = malloc(sizeof(*cdata));
        if (!cdata) {
            logexit("malloc");
        }

        cdata->csock = csock;
        memcpy(&(cdata->storage), &client_storage, sizeof(client_storage));

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, cdata);
    }
    close(ssock);

    exit(EXIT_SUCCESS);
    return 0;
}