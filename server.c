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

int main(int argc, char **argv) {
    if (argc < 3) {
        logexit("numero de parametros insuficiente");
    }

    sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)) {
        logexit("erro no parse");
    }

    int s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("erro no socket");
    }

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("sockopt");
    }

    sockaddr *addr = (sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage))) {
        logexit("erro no bind");
    }

    if (0 != listen(s, MAX_CONNECTIONS)) {
        logexit("listen");
    }

    char addrstr[MSGSIZE];
    addrtostr(addr, addrstr, MSGSIZE);

    printf("bound to %s, waiting connection\n", addrstr);

    while (1) {
        sockaddr_storage client_storage;
        sockaddr *client_addr = (sockaddr *)(&client_storage);
        socklen_t storagelen = sizeof(client_storage);

        int client_sock = accept(s, client_addr, &storagelen);
        if (client_sock == -1) {
            logexit("accept");
        }

        char client_addrstr[MSGSIZE];
        addrtostr(client_addr, client_addrstr, MSGSIZE);

        printf("[log] connection from %s\n", client_addrstr);

        char buffer[MSGSIZE];
        memset(buffer, 0, MSGSIZE);

        size_t count = recv(client_sock, buffer, MSGSIZE, 0);
        printf("[msg] %s %d bytes: %s\n", client_addrstr, (int)count, buffer);

        sprintf(buffer, "remote endpoint: %.450s\n", client_addrstr);
        count = send(client_sock, buffer, strlen(buffer) + 1, 0);
        if (count != strlen(buffer) + 1) {
            logexit("send");
        }
        close(client_sock);
    }

    exit(EXIT_SUCCESS);
}